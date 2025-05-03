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
#include "offlinequeue.h"

#include <sys/utsname.h>

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAboutData>
#include <KPluginFactory>
#include <KF6/KTextEditor/ktexteditor_version.h>
#include <KLocalizedString>
#include <KMessageBox>
#include <KActionCollection>
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

const QString kUrlHeartsBulk =
    QStringLiteral("%1users/current/heartbeats.bulk");

const QString kSettingsKeyApiKey = QStringLiteral("settings/api_key");
const QString kSettingsKeyApiUrl = QStringLiteral("settings/api_url");
const QString kSettingsKeyHideFilenames =
    QStringLiteral("settings/hidefilenames");

const QString kContentTypeJson = QStringLiteral("application/json");

const QString kJsonKeyBranch = QStringLiteral("branch");
const QString kJsonKeyCategory = QStringLiteral("category");
const QString kJsonKeyEntity = QStringLiteral("entity");
const QString kJsonKeyProject = QStringLiteral("project");
const QString kJsonKeyTime = QStringLiteral("time");
const QString kJsonKeyType = QStringLiteral("type");
const QString kJsonValueCoding = QStringLiteral("coding");
const QString kJsonValueFile = QStringLiteral("file");

const QString kStringLiteralSlash = QStringLiteral("/");
const QString kStringLiteralUnknown = QStringLiteral("Unknown");

const QByteArray timeZoneBytes() {
    return QTimeZone::systemTimeZone()
        .displayName(QDateTime::currentDateTime())
        .toLocal8Bit();
}

static QByteArray headerName(WakaTimeView::WakaTimeApiHttpHeaders header) {
    switch (header) {
    case WakaTimeView::AuthorizationHeader:
        return QByteArrayLiteral("Authorization");
    case WakaTimeView::TimeZoneHeader:
        return QByteArrayLiteral("TimeZone");
    case WakaTimeView::XIgnoreHeader:
        return QByteArrayLiteral("X-Ignore");
    case WakaTimeView::XMachineName:
        return QByteArrayLiteral("X-Machine-Name");
    }

    return QByteArray();
}

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
    disconnectDocumentSignals(
        static_cast<KTextEditor::View *>(view)->document());
}

WakaTimeView::WakaTimeView(KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow), apiKey(QString()),
      binPathCache(QMap<QString, QString>()), hasSent(false),
      lastFileSent(QString()), lastTimeSent(QDateTime::currentDateTime()),
      nam(new QNetworkAccessManager(this)), queue(new OfflineQueue()) {
    KXMLGUIClient::setComponentName(QStringLiteral("katewakatime"),
                                    i18n("WakaTime"));
    setXMLFile(QStringLiteral("ui.rc"));
    QAction *const a =
        actionCollection()->addAction(QStringLiteral("configure_wakatime"));
    a->setText(i18n("Configure WakaTime..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("wakatime")));
    connect(
        a, &QAction::triggered, this, &WakaTimeView::slotConfigureWakaTime);
    mainWindow->guiFactory()->addClient(this);

    const QString configFilePath =
        QDir::homePath() + QDir::separator() + QStringLiteral(".wakatime.cfg");
    struct utsname buf;
    config = new QSettings(configFilePath, QSettings::IniFormat, this);
    readConfig();
    int unameRes = uname(&buf);
    userAgent =
        QString(
            QStringLiteral(
                "wakatime/%1 (%2-%3-%4-%5) KTextEditor/%6 kate-wakatime/%7"))
            .arg(config->value(QStringLiteral("internal/cli_version"))
                     .toString()
                     .trimmed())
            .arg(unameRes == 0 ? QString::fromUtf8(buf.sysname) :
                                 kStringLiteralUnknown)
            .arg(unameRes == 0 ? QString::fromUtf8(buf.release) :
                                 kStringLiteralUnknown)
            .arg(unameRes == 0 ? QString::fromUtf8(buf.nodename) :
                                 kStringLiteralUnknown)
            .arg(unameRes == 0 ? QString::fromUtf8(buf.machine) :
                                 kStringLiteralUnknown)
            .arg(QStringLiteral(KTEXTEDITOR_VERSION_STRING))
            .arg(QStringLiteral(kWakaTimePluginVersion))
            .toUtf8();

    // Connect the request handling slot method
    connect(nam,
            &QNetworkAccessManager::finished,
            this,
            &WakaTimeView::slotNetworkReplyFinished);

    connect(m_mainWindow,
            &KTextEditor::MainWindow::viewCreated,
            this,
            &WakaTimeView::viewCreated);

    for (KTextEditor::View *const view : m_mainWindow->views()) {
        connectDocumentSignals(view->document());
    }

    QNetworkAccessManager *m = new QNetworkAccessManager(this);
    connect(m, &QNetworkAccessManager::finished, [this](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            this->sendQueuedHeartbeats();
        }
    });
    const QUrl u(this->apiUrl);
    const QUrl requestUrl(
        u.scheme().append(QStringLiteral("://")).append(u.host()));
    const QNetworkRequest req(requestUrl);
    m->get(req);
}

WakaTimeView::~WakaTimeView() {
    delete nam;
    delete queue;
    delete config;
    m_mainWindow->guiFactory()->removeClient(this);
}

QObject *WakaTimePlugin::createView(KTextEditor::MainWindow *mainWindow) {
    return new WakaTimeView(mainWindow);
}

void WakaTimeView::slotConfigureWakaTime() {
    if (!KTextEditor::Editor::instance()->application()->activeMainWindow()) {
        return;
    }

    const KTextEditor::View *const kv(KTextEditor::Editor::instance()
                                          ->application()
                                          ->activeMainWindow()
                                          ->activeView());
    if (!kv) {
        return;
    }

    QDialog dialog(KTextEditor::Editor::instance()
                       ->application()
                       ->activeMainWindow()
                       ->window());
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

QString WakaTimeView::getBinPath(QString binName) {
#ifdef Q_OS_WIN
    return QString();
#endif

    if (binPathCache.contains(binName)) {
        return binPathCache.value(binName);
    }

    static const char *const kDefaultPath =
        "/usr/bin:/usr/local/bin:/opt/bin:/opt/local/bin";
    static const QString colon = QStringLiteral(":");

    const char *const path = getenv("PATH");
    QStringList paths = QString::fromUtf8(path ? path : kDefaultPath)
                            .split(colon, Qt::SkipEmptyParts);

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

void WakaTimeView::sendAction(KTextEditor::Document *doc, bool isWrite) {
    QString filePath = doc->url().toLocalFile();

    // Could be untitled, or a URI (including HTTP); only local files are
    // handled for now
    if (!filePath.length()) {
        qCDebug(gLogWakaTime) << "Nothing to send about";
        return;
    }

    const QFileInfo fileInfo(filePath);

    // They have it sending the real file path, maybe not respecting symlinks,
    // etc
    filePath = fileInfo.canonicalFilePath();
    qCDebug(gLogWakaTime) << "File path:" << filePath;

    // Compare date and make sure it has been at least 15 minutes
    const qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 deltaMs = currentMs - lastTimeSent.toMSecsSinceEpoch();
    static const qint64 intervalMs = 120000; // ms

    // If the current file has not changed and it has not been 2 minutes since
    // the last heartbeat was sent, do NOT send this heartbeat. This does not
    // apply to write events as they are always sent.
    if (!isWrite) {
        if (hasSent && deltaMs <= intervalMs && lastFileSent == filePath) {
            qCDebug(gLogWakaTime)
                << "Not enough time has passed since last send";
            qCDebug(gLogWakaTime)
                << "Delta:" << deltaMs / 1000 / 60 << "/ 2 minutes";
            return;
        }
    }

    // Get the project name, by traversing up until .git or .svn is found
    QString projectName;
    QDir currentDirectory = QDir(fileInfo.canonicalPath());
    QDir projectDirectory;
    bool vcDirFound = false;
    static QStringList filters;
    static const QString gitStr = QStringLiteral(".git");
    static const QString svnStr = QStringLiteral(".svn");
    filters << gitStr << svnStr;
    QString typeOfVcs;

    while (!vcDirFound) {
        if (!currentDirectory.canonicalPath().compare(kStringLiteralSlash)) {
            break;
        }

        QFileInfoList entries = currentDirectory.entryInfoList(
            QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);

        for (QFileInfo entry : entries) {
            QString name = entry.fileName();

            if ((name.compare(gitStr) || name.compare(svnStr)) &&
                entry.isDir()) {
                vcDirFound = true;
                projectName = currentDirectory.dirName();
                projectDirectory = QDir(currentDirectory);
                typeOfVcs = name;
                break;
            }
        }

        currentDirectory.cdUp();
    }

    QVariantMap data;
    data.insert(kJsonKeyTime, currentMs / 1000);
    if (projectName.length()) {
        data.insert(kJsonKeyProject, projectName);
    } else {
        qCDebug(gLogWakaTime) << "Warning: No project name found";
    }
    static const QString git = QStringLiteral("git");
    static QString gitPath = getBinPath(git);
    if (!gitPath.isNull() && !hideFilenames) {
        QProcess proc;
        QStringList arguments;
        arguments << QStringLiteral("symbolic-ref")
                  << QStringLiteral("--short") << QStringLiteral("HEAD");
        proc.setWorkingDirectory(projectDirectory.canonicalPath());
        qCDebug(gLogWakaTime) << "Running" << gitPath << arguments << "in"
                              << projectDirectory.canonicalPath();
        proc.start(gitPath, arguments, QIODevice::ReadWrite | QIODevice::Text);
        if (proc.waitForFinished()) {
            const QByteArray out = proc.readAllStandardOutput();
            const QString branch =
                QString::fromUtf8(out.constData()).trimmed();
            if (!branch.isNull() && branch.size() > 0) {
                data.insert(kJsonKeyBranch, branch);
            }
        } else {
            qCDebug(gLogWakaTime) << "Failed to get branch (git)";
            qCDebug(gLogWakaTime)
                << "stderr:"
                << QString::fromUtf8(proc.readAllStandardError().constData())
                       .trimmed();
            qCDebug(gLogWakaTime)
                << "stdout:"
                << QString::fromUtf8(proc.readAllStandardOutput().constData())
                       .trimmed();
            qCInfo(gLogWakaTime)
                << "If this is not expected, please file a bug report.";
        }
    }
    if (gitPath.isNull()) {
        qCInfo(gLogWakaTime) << "\"git\" not found in PATH";
    }

    if (isWrite) {
        static const QString keyIsWrite = QStringLiteral("is_write");
        data.insert(keyIsWrite, isWrite);
    }

    // This is good enough for the language most of the time
    QString mode = doc->mode();
    static const QString keyLanguage = QStringLiteral("language");
    if (mode.length()) {
        data.insert(keyLanguage, mode);
    } else {
        mode = doc->highlightingMode();
        if (mode.length()) {
            data.insert(keyLanguage, mode);
        }
    }

    data.insert(kJsonKeyType, kJsonValueFile);
    data.insert(kJsonKeyCategory, kJsonValueCoding);

    if (!hideFilenames) {
        data.insert(kJsonKeyEntity, filePath);
        data.insert(QStringLiteral("lines"), doc->lines());
        for (KTextEditor::View *view : m_mainWindow->views()) {
            if (view->document() == doc) {
                data.insert(QStringLiteral("lineno"),
                            view->cursorPosition().line() + 1);
                data.insert(QStringLiteral("cursorpos"),
                            view->cursorPosition().column() + 1);
                break;
            }
        }
    } else {
        data.insert(
            kJsonKeyEntity,
            QStringLiteral("HIDDEN.%1").arg(fileInfo.completeSuffix()));
    }

    sendHeartbeat(data, isWrite);

    lastTimeSent = QDateTime::currentDateTime();
    lastFileSent = filePath;
}

void WakaTimeView::sendQueuedHeartbeats() {
    QString heartbeats = QStringLiteral("[");
    int i = 0;
    const int limit = 25;
    const QList<QStringList> list = queue->popMany(limit);
    const int size = list.size();
    if (!size) {
        return;
    }
    for (QStringList heartbeat : list) {
        QString jsonBody = heartbeat.at(1);
        if (i < (size - 1)) {
            jsonBody.append(QStringLiteral(","));
        }
        heartbeats.append(jsonBody);
        i++;
    }
    heartbeats.append(QStringLiteral("]"));
    static QUrl url(QString(kUrlHeartsBulk).arg(apiUrl));
    QNetworkRequest request(url);
    const QByteArray requestContent = heartbeats.toUtf8();

    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeJson);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setRawHeader(headerName(WakaTimeView::AuthorizationHeader),
                         WakaTimeView::apiAuthBytes());
    request.setRawHeader(headerName(WakaTimeView::TimeZoneHeader),
                         timeZoneBytes());

#ifdef QT_DEBUG
    request.setRawHeader(headerName(WakaTimeView::XIgnoreHeader),
                         QByteArray("If this request is bad, please ignore it "
                                    "while this plugin is being developed."));
#endif

    nam->post(request, requestContent);
}

QByteArray WakaTimeView::apiAuthBytes() {
    return QStringLiteral("Basic %1")
        .arg(QString::fromLocal8Bit(apiKey.toLocal8Bit().toBase64()))
        .toLocal8Bit();
}

void WakaTimeView::sendHeartbeat(const QVariantMap &data,
                                 bool isWrite,
                                 bool saveToQueue) {
    const QJsonDocument object = QJsonDocument::fromVariant(data);
    const QByteArray requestContent =
        QByteArrayLiteral("[") + object.toJson() + QByteArrayLiteral("]");
    const QUrl url(QString(kUrlHeartsBulk).arg(apiUrl));
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeJson);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
    request.setRawHeader(headerName(WakaTimeView::AuthorizationHeader),
                         WakaTimeView::apiAuthBytes());
    request.setRawHeader(headerName(WakaTimeView::TimeZoneHeader),
                         timeZoneBytes());

    qCDebug(gLogWakaTime) << "Single heartbeat in array:" << object;
#ifdef QT_DEBUG
    request.setRawHeader(headerName(WakaTimeView::XIgnoreHeader),
                         QByteArray("If this request is bad, please ignore it "
                                    "while this plugin is being developed."));
#endif

    if (saveToQueue) {
        // time-type-category-project-branch-entity-is_write
        const QString rowId =
            QStringLiteral("%1-%2-%3-%4-%5-%6-%7")
                .arg(data[kJsonKeyTime].toString(),
                     data[kJsonKeyType].toString(),
                     data[kJsonKeyCategory].toString(),
                     data[kJsonKeyProject].toString(),
                     data[kJsonKeyBranch].toString(),
                     data[kJsonKeyEntity].toString(),
                     isWrite ? QStringLiteral("1") : QStringLiteral("0"));
        const QString rowData = QString::fromUtf8(requestContent.constData());
        QStringList row;
        row << rowId << rowData;
        queue->push(row);
    }

    nam->post(request, requestContent);
}

void WakaTimeView::writeConfig(void) {
    config->setValue(kSettingsKeyApiKey, apiKey);
    config->setValue(kSettingsKeyApiUrl, apiUrl);
    config->setValue(kSettingsKeyHideFilenames, hideFilenames);
    config->sync();
    if (config->status() != QSettings::NoError) {
        qCDebug(gLogWakaTime)
            << "Failed to save WakaTime settings:" << config->status();
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
        QString(config->value(kSettingsKeyApiUrl).toString())
            .trimmed()
            .length()) {
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
    disconnect(document,
               SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)));
    disconnect(document,
               SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)));
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

void WakaTimeView::slotNetworkReplyFinished(QNetworkReply *reply) {
    const QVariant statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qCDebug(gLogWakaTime) << "Status code:" << statusCode.toInt();

    const QByteArray contents = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(contents);
    if (doc.isNull()) {
        qCDebug(gLogWakaTime) << "Could not parse response. All responses are "
                                 "expected to be JSON serialised";
        return;
    }

    const QVariantMap received = doc.toVariant().toMap();

    if (reply->error() == QNetworkReply::NoError &&
        (statusCode == 201 || statusCode == 202)) {
        qCDebug(gLogWakaTime) << "Sent data successfully";
        queue->pop();
        hasSent = true;
    } else {
        qCDebug(gLogWakaTime) << "URL:" << reply->url().toString();
        qCDebug(gLogWakaTime)
            << "Request did not succeed, status code:" << statusCode.toInt();
        static const QString errorsKeyStr = QStringLiteral("errors");

        if (statusCode == 401) {
            KMessageBox::error(nullptr,
                               i18n("WakaTime could not authenticate the last "
                                    "request. Verify your API key setting."));
        }

        for (QVariant error : received[errorsKeyStr].toList()) {
            qCDebug(gLogWakaTime) << error.toByteArray();
        }
    }
}

#include "wakatimeplugin.moc"
