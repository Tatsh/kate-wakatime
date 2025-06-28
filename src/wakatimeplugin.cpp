/**
 * This file is part of kate-wakatime.
 * Copyright 2014 Andrew Udvare <audvare@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version
 *   3, or (at your option) any later version, as published by the Free
 *   Software Foundation.
 *
 *   This library is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with the kdelibs library; see the file COPYING.LIB. If
 *   not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 *   Fifth Floor, Boston, MA 02110-1301, USA. or see
 *   <http://www.gnu.org/licenses/>.
 */

#include "wakatimeplugin.h"

#include <sys/utsname.h>

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAboutData>
#include <KActionCollection>
#include <KF6/KTextEditor/ktexteditor_version.h>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KXMLGUIFactory>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcess>
#include <QtCore/QTimeZone>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtWidgets/QDialog>

Q_LOGGING_CATEGORY(gLogWakaTime, "wakatime")

K_PLUGIN_FACTORY_WITH_JSON(WakaTimePluginFactory,
                           "ktexteditor_wakatime.json",
                           registerPlugin<WakaTimePlugin>();)

const QString kSettingsKeyApiKey = QStringLiteral("settings/api_key");
const QString kSettingsKeyApiUrl = QStringLiteral("settings/api_url");
const QString kSettingsKeyHideFilenames = QStringLiteral("settings/hidefilenames");

const QString kStringLiteralSlash = QStringLiteral("/");
const QString kStringLiteralUnknown = QStringLiteral("Unknown");

const QString kWakaTimeCli = QStringLiteral("wakatime-cli");

WakaTimePlugin::WakaTimePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent) {
    Q_UNUSED(args);
}

WakaTimePlugin::~WakaTimePlugin() {
}

void WakaTimeView::viewCreated(KTextEditor::View *view) {
    connectDocumentSignals(view->document());
}

void WakaTimeView::viewDestroyed(QObject *view) {
    disconnectDocumentSignals(static_cast<KTextEditor::View *>(view)->document());
}

WakaTimeView::WakaTimeView(KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow), apiKey(QString()),
      binPathCache(QMap<QString, QString>()), hasSent(false), lastFileSent(QString()),
      lastTimeSent(QDateTime::currentDateTime()) {
    KXMLGUIClient::setComponentName(QStringLiteral("katewakatime"), i18n("WakaTime"));
    setXMLFile(QStringLiteral("ui.rc"));
    QAction *const a = actionCollection()->addAction(QStringLiteral("configure_wakatime"));
    a->setText(i18n("Configure WakaTime..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("wakatime")));
    connect(a, &QAction::triggered, this, &WakaTimeView::slotConfigureWakaTime);
    mainWindow->guiFactory()->addClient(this);
    // Configuration
    const QString configFilePath =
        QDir::homePath() + QDir::separator() + QStringLiteral(".wakatime.cfg");
    config = new QSettings(configFilePath, QSettings::IniFormat, this);
    readConfig();
    // Connections
    connect(m_mainWindow, &KTextEditor::MainWindow::viewCreated, this, &WakaTimeView::viewCreated);
    for (KTextEditor::View *const view : m_mainWindow->views()) {
        connectDocumentSignals(view->document());
    }
}

WakaTimeView::~WakaTimeView() {
    delete config;
    m_mainWindow->guiFactory()->removeClient(this);
}

QObject *WakaTimePlugin::createView(KTextEditor::MainWindow *mainWindow) {
    return new WakaTimeView(mainWindow);
}

void WakaTimeView::slotConfigureWakaTime() {
    QDialog dialog(KTextEditor::Editor::instance()->application()->activeMainWindow()->window());
    Ui::ConfigureWakaTimeDialog ui;
    ui.setupUi(&dialog);
    ui.lineEdit_apiKey->setText(apiKey);
    if (apiKey.isNull() || !apiKey.size()) {
        ui.lineEdit_apiKey->setFocus();
    }
    ui.lineEdit_apiUrl->setText(apiUrl);
    ui.checkBox_hideFilenames->setChecked(hideFilenames);
    dialog.setWindowTitle(i18n("Configure WakaTime"));
    if (dialog.exec() == QDialog::Accepted) {
        QString newApiKey = ui.lineEdit_apiKey->text();
        if (newApiKey.size() >= 36 && newApiKey.size() <= 41) {
            apiKey = newApiKey;
        }
        hideFilenames = ui.checkBox_hideFilenames->isChecked();
        writeConfig();
    }
}

QString WakaTimeView::getBinPath(const QString &binName) {
#ifdef Q_OS_WIN
    return QString();
#endif
    if (binPathCache.contains(binName)) {
        return binPathCache.value(binName);
    }
    QString dotWakaTime = QStringLiteral("%1/.wakatime").arg(QDir::homePath());
    static const char *const kDefaultPath = "/usr/bin:/usr/local/bin:/opt/bin:/opt/local/bin";
    static const QString colon = QStringLiteral(":");

    const char *const path = getenv("PATH");
    QStringList paths =
        QString::fromUtf8(path ? path : kDefaultPath).split(colon, Qt::SkipEmptyParts);
    paths.insert(0, dotWakaTime);
    for (QString path : paths) {
        QStringList dirListing = QDir(path).entryList();
        for (QString entry : dirListing) {
            if (entry == binName) {
                entry = path.append(kStringLiteralSlash).append(entry);
                binPathCache[binName] = entry;
                return entry;
            }
        }
    }
    return QString();
}

QString WakaTimeView::getProjectDirectory(const QFileInfo &fileInfo) {
    QDir currentDirectory = QDir(fileInfo.canonicalPath());
    static QStringList filters;
    static const QString gitStr = QStringLiteral(".git");
    static const QString svnStr = QStringLiteral(".svn");
    filters << gitStr << svnStr;
    bool vcDirFound = false;
    while (!vcDirFound) {
        if (!currentDirectory.canonicalPath().compare(kStringLiteralSlash)) {
            break;
        }
        QFileInfoList entries =
            currentDirectory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
        for (QFileInfo entry : entries) {
            QString name = entry.fileName();
            if ((name.compare(gitStr) || name.compare(svnStr)) && entry.isDir()) {
                vcDirFound = true;
                return currentDirectory.dirName();
            }
        }
        currentDirectory.cdUp();
    }
    return QString();
}

void WakaTimeView::sendAction(KTextEditor::Document *doc, bool isWrite) {
    auto wakatimeCliPath = getBinPath(kWakaTimeCli);
    if (wakatimeCliPath.isEmpty()) {
        wakatimeCliPath = getBinPath(QStringLiteral("wakatime"));
        if (!wakatimeCliPath.isEmpty()) {
            qCWarning(gLogWakaTime) << "wakatime-cli not found in PATH.";
            return;
        }
    }
    auto filePath = doc->url().toLocalFile();
    // Could be untitled, or a URI (including HTTP). Only local files are
    // handled for now.
    if (filePath.isEmpty()) {
        qCDebug(gLogWakaTime) << "Nothing to send about";
        return;
    }
    QStringList arguments;
    const QFileInfo fileInfo(filePath);
    // They have it sending the real file path, maybe not respecting symlinks,
    // etc.
    filePath = fileInfo.canonicalFilePath();
    qCDebug(gLogWakaTime) << "File path:" << filePath;
    // Compare date and make sure it has been at least 15 minutes.
    const auto currentMs = QDateTime::currentMSecsSinceEpoch();
    const auto deltaMs = currentMs - lastTimeSent.toMSecsSinceEpoch();
    static const auto intervalMs = 120000; // ms
    // If the current file has not changed and it has not been 2 minutes since the last heartbeat
    // was sent, do NOT send this heartbeat. This does not apply to write events as they are
    // always sent.
    if (!isWrite) {
        if (hasSent && deltaMs <= intervalMs && lastFileSent == filePath) {
            qCDebug(gLogWakaTime) << "Not enough time has passed since last send";
            qCDebug(gLogWakaTime) << "Delta:" << deltaMs / 1000 / 60 << "/ 2 minutes";
            return;
        }
    }
    arguments << QStringLiteral("--entity") << filePath;
    arguments << QStringLiteral("--plugin")
              << QStringLiteral("ktexteditor-wakatime/%1").arg(VERSION);
    arguments << QStringLiteral("--key") << apiKey;
    arguments << QStringLiteral("--api-url") << apiUrl;
    if (hideFilenames) {
        arguments << QStringLiteral("--hide-filenames");
    }
    // Get the project name by traversing up until .git or .svn is found.
    auto projectName = getProjectDirectory(fileInfo);
    if (!projectName.isEmpty()) {
        arguments << QStringLiteral("--alternate-project") << projectName;
    } else {
        qCDebug(gLogWakaTime) << "Warning: No project name found";
    }
    if (isWrite) {
        arguments << QStringLiteral("--write");
    }
    // This is good enough for the language most of the time.
    QString mode = doc->mode();
    static const QString keyLanguage = QStringLiteral("language");
    if (!mode.isEmpty()) {
        arguments << QStringLiteral("--language") << mode;
    } else {
        mode = doc->highlightingMode();
        if (!mode.isEmpty()) {
            arguments << QStringLiteral("--language") << mode;
        }
    }
    for (KTextEditor::View *view : m_mainWindow->views()) {
        if (view->document() == doc) {
            arguments << QStringLiteral("--lineno")
                      << QString::number(view->cursorPosition().line() + 1);
            arguments << QStringLiteral("--cursorpos")
                      << QString::number(view->cursorPosition().column() + 1);
            arguments << QStringLiteral("--lines-in-file")
                      << QString::number(view->document()->lines());
            break;
        }
    }
    qCDebug(gLogWakaTime) << "Running:" << wakatimeCliPath << arguments.join(QStringLiteral(" "));
    auto ret = QProcess::execute(wakatimeCliPath, arguments);
    if (ret != 0) {
        qCWarning(gLogWakaTime) << "wakatime-cli returned error code" << ret;
        return;
    }
    lastTimeSent = QDateTime::currentDateTime();
    lastFileSent = filePath;
    hasSent = true;
}

QByteArray WakaTimeView::apiAuthBytes() {
    return QStringLiteral("Basic %1")
        .arg(QString::fromLocal8Bit(apiKey.toLocal8Bit().toBase64()))
        .toLocal8Bit();
}

void WakaTimeView::writeConfig(void) {
    config->setValue(kSettingsKeyApiKey, apiKey);
    config->setValue(kSettingsKeyApiUrl, apiUrl);
    config->setValue(kSettingsKeyHideFilenames, hideFilenames);
    config->sync();
    if (config->status() != QSettings::NoError) {
        qCDebug(gLogWakaTime) << "Failed to save WakaTime settings:" << config->status();
    }
}

void WakaTimeView::readConfig(void) {
    const QString apiKeyPath = kSettingsKeyApiKey;
    const QString apiUrlPath = kSettingsKeyApiUrl;

    if (!config->contains(kSettingsKeyApiKey)) {
        qCDebug(gLogWakaTime) << "No API key set in ~/.wakatime.cfg";
        return;
    }

    const QString key = config->value(kSettingsKeyApiKey).toString().trimmed();
    if (!key.length()) {
        qCDebug(gLogWakaTime) << "API Key is blank";
        return;
    }

    QString url = QStringLiteral("https://api.wakatime.com/api/v1/");
    if (config->contains(kSettingsKeyApiUrl) &&
        QString(config->value(kSettingsKeyApiUrl).toString()).trimmed().length()) {
        url = QString(config->value(kSettingsKeyApiUrl).toString()).trimmed();
    }

    // Assume valid at this point
    apiKey = key;
    apiUrl = url;
    hideFilenames = config->value(kSettingsKeyHideFilenames).toBool();
}

bool WakaTimeView::documentIsConnected(KTextEditor::Document *document) {
    for (const KTextEditor::Document *const doc : connectedDocuments) {
        if (doc == document) {
            return true;
        }
    }
    return false;
}

void WakaTimeView::connectDocumentSignals(KTextEditor::Document *document) {
    if (!document || documentIsConnected(document)) {
        return;
    }

    // When document goes from saved state to changed state (not yet saved on
    // disk)
    connect(document,
            &KTextEditor::Document::modifiedChanged,
            this,
            &WakaTimeView::slotDocumentModifiedChanged);

    // Written to disk
    connect(document,
            &KTextEditor::Document::documentSavedOrUploaded,
            this,
            &WakaTimeView::slotDocumentWrittenToDisk);

    // Text changes (might be heavy)
    // This event unfortunately is emitted twice in separate threads for every
    // key stroke (maybe key up and down is the reason)
    connect(document,
            &KTextEditor::Document::textChanged,
            this,
            &WakaTimeView::slotDocumentModifiedChanged);

    connectedDocuments << document;
}

void WakaTimeView::disconnectDocumentSignals(KTextEditor::Document *document) {
    if (!documentIsConnected(document)) {
        return;
    }
    disconnect(document, SIGNAL(modifiedChanged(KTextEditor::Document *)));
    disconnect(document, SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)));
    disconnect(document, SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)));
    disconnect(document, SIGNAL(textChanged(KTextEditor::Document *)));
    connectedDocuments.removeOne(document);
}

// Slots
void WakaTimeView::slotDocumentModifiedChanged(KTextEditor::Document *doc) {
    sendAction(doc, false);
}

void WakaTimeView::slotDocumentWrittenToDisk(KTextEditor::Document *doc) {
    sendAction(doc, true);
}

#include "wakatimeplugin.moc"
