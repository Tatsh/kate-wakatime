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

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAboutData>
#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KXMLGUIFactory>

#include <QDialog>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QTimeZone>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

Q_LOGGING_CATEGORY(gLogWakaTime, "wakatime")

K_PLUGIN_FACTORY_WITH_JSON(WakaTimePluginFactory,
                           "ktexteditor_wakatime.json",
                           registerPlugin<WakaTimePlugin>();)

const QByteArray timeZoneBytes() {
    const QDateTime dt = QDateTime::currentDateTime();
    const QString timeZone = QTimeZone::systemTimeZone().displayName(dt);
    return timeZone.toLocal8Bit();
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
    : QObject(mainWindow), m_mainWindow(mainWindow), hasSent(false),
      lastTimeSent(QDateTime::currentDateTime()),
      nam(new QNetworkAccessManager(this)),
      binPathCache(QMap<QString, QString>()), queue(new OfflineQueue()) {
    KXMLGUIClient::setComponentName(QStringLiteral("katewakatime"),
                                    i18n("WakaTime"));
    setXMLFile(QStringLiteral("ui.rc"));
    QAction *a =
        actionCollection()->addAction(QStringLiteral("configure_wakatime"));
    a->setText(i18n("Configure WakaTime..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("wakatime")));
    connect(
        a, &QAction::triggered, this, &WakaTimeView::slotConfigureWakaTime);
    mainWindow->guiFactory()->addClient(this);

    apiKey = QStringLiteral("");
    lastFileSent = QStringLiteral("");

    readConfig();
    userAgent = getUserAgent();

    // Connect the request handling slot method
    connect(nam,
            &QNetworkAccessManager::finished,
            this,
            &WakaTimeView::slotNetworkReplyFinshed);

    connect(m_mainWindow,
            &KTextEditor::MainWindow::viewCreated,
            this,
            &WakaTimeView::viewCreated);

    foreach (KTextEditor::View *view, m_mainWindow->views()) {
        connectDocumentSignals(view->document());
    }

    sendQueuedHeartbeats();
}

WakaTimeView::~WakaTimeView() {
    delete nam;
    m_mainWindow->guiFactory()->removeClient(this);
}

QObject *WakaTimePlugin::createView(KTextEditor::MainWindow *mainWindow) {
    return new WakaTimeView(mainWindow);
}

void WakaTimeView::slotConfigureWakaTime() {
    if (!KTextEditor::Editor::instance()->application()->activeMainWindow()) {
        return;
    }

    KTextEditor::View *kv(KTextEditor::Editor::instance()
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
    ui.checkBox_hideFilenames->setChecked(hideFilenames);

    dialog.setWindowTitle(i18n("Configure WakaTime"));
    if (dialog.exec() == QDialog::Accepted) {
        QString newApiKey = ui.lineEdit_apiKey->text();
        if (newApiKey.size() == 36) {
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

    static const QString slash = QStringLiteral("/");
    static const QString colon = QStringLiteral(":");

    QStringList paths = QString::fromUtf8(getenv("PATH"))
                            .split(colon, QString::SkipEmptyParts);
    foreach (QString path, paths) {
        QStringList dirListing = QDir(path).entryList();
        foreach (QString entry, dirListing) {
            if (entry == binName) {
                entry = path.append(slash).append(entry);
                binPathCache[binName] = entry;
                return entry;
            }
        }
    }

    return QString();
}

QByteArray WakaTimeView::getUserAgent(void) {
    return QStringLiteral("(KDE %1) Katepart/%1 kate-wakatime/%2")
        .arg(QStringLiteral("5"), QStringLiteral(kWakaTimePluginVersion))
        .toLocal8Bit();
}

void WakaTimeView::sendAction(KTextEditor::Document *doc, bool isWrite) {
    QString filePath = doc->url().toLocalFile();

    // Could be untitled, or a URI (including HTTP); only local files are
    // handled for now
    if (!filePath.length()) {
        qCDebug(gLogWakaTime) << "Nothing to send about";
        return;
    }

    QFileInfo fileInfo(filePath);

    // They have it sending the real file path, maybe not respecting symlinks,
    // etc
    filePath = fileInfo.canonicalFilePath();
    qCDebug(gLogWakaTime) << "File path:" << filePath;

    // Compare date and make sure it has been at least 15 minutes
    const qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 deltaMs = currentMs - lastTimeSent.toMSecsSinceEpoch();
    QString lastFileSent = lastFileSent;
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
    static const QString rootSlash = QStringLiteral("/");
    filters << gitStr << svnStr;
    QString typeOfVcs;

    while (!vcDirFound) {
        if (!currentDirectory.canonicalPath().compare(rootSlash)) {
            break;
        }

        QFileInfoList entries = currentDirectory.entryInfoList(
            QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);

        foreach (QFileInfo entry, entries) {
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
    static const QString keyTime = QStringLiteral("time");
    static const QString keyProject = QStringLiteral("project");
    static const QString keyBranch = QStringLiteral("branch");

    data.insert(keyTime, currentMs / 1000);
    if (projectName.length()) {
        data.insert(keyProject, projectName);
    } else {
        qCDebug(gLogWakaTime) << "Warning: No project name found";
    }
    static const QString git = QStringLiteral("git");
    static QString gitPath = getBinPath(git);
    if (!gitPath.isNull() && !hideFilenames) {
        static const QString cmd =
            gitPath.append(QStringLiteral(" symbolic-ref --short HEAD"));
        QProcess proc;
        proc.setWorkingDirectory(projectDirectory.canonicalPath());
        qCDebug(gLogWakaTime)
            << "Running " << cmd << "in" << projectDirectory.canonicalPath();
        proc.start(cmd);
        if (proc.waitForFinished()) {
            QByteArray out = proc.readAllStandardOutput();
            QString branch = QString::fromUtf8(out.constData()).trimmed();
            if (!branch.isNull() && branch.size() > 0) {
                data.insert(keyBranch, branch);
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

    static const QString keyType = QStringLiteral("type");
    static const QString keyCategory = QStringLiteral("category");
    static const QString valueFile = QStringLiteral("file");
    static const QString valueCoding = QStringLiteral("coding");
    static const QString keyEntity = QStringLiteral("entity");

    data.insert(keyType, valueFile);
    data.insert(keyCategory, valueCoding);

    if (!hideFilenames) {
        static const QString keyLines = QStringLiteral("lines");
        static const QString keyLineNo = QStringLiteral("lineno");
        static const QString keyCursorPos = QStringLiteral("cursorpos");
        data.insert(keyEntity, filePath);
        data.insert(keyLines, doc->lines());
        foreach (KTextEditor::View *view, m_mainWindow->views()) {
            if (view->document() == doc) {
                data.insert(keyLineNo, view->cursorPosition().line() + 1);
                data.insert(keyCursorPos, view->cursorPosition().column() + 1);
                break;
            }
        }
    } else {
        data.insert(
            keyEntity,
            QStringLiteral("HIDDEN.%1").arg(fileInfo.completeSuffix()));
    }

    sendHeartbeat(data, isWrite);

    lastTimeSent = QDateTime::currentDateTime();
    lastFileSent = filePath;
}

void WakaTimeView::sendQueuedHeartbeats() {
    if (nam->networkAccessible() != QNetworkAccessManager::Accessible) {
        return;
    }
    QString heartbeats = QStringLiteral("[");
    int i = 0;
    const int limit = 25;
    const QList<QStringList> list = queue->popMany(limit);
    const int size = list.size();
    if (!size) {
        return;
    }
    foreach (QStringList heartbeat, list) {
        QString jsonBody = heartbeat.at(1);
        if (i < (size - 1)) {
            jsonBody.append(QStringLiteral(","));
        }
        heartbeats.append(jsonBody);
        i++;
    }
    heartbeats.append(QStringLiteral("]"));
    qCDebug(gLogWakaTime()) << "offline heartbeats" << heartbeats;
    static QUrl url(QStringLiteral(kWakaTimeViewHeartbeatsBulkUrl));
    static const QString contentType = QStringLiteral("application/json");
    QNetworkRequest request(url);
    QByteArray requestContent = heartbeats.toUtf8();

    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("User-Agent", userAgent);
    request.setRawHeader("Authorization", apiAuthBytes());
    request.setRawHeader("TimeZone", timeZoneBytes());
#ifndef NDEBUG
    request.setRawHeader("X-Ignore",
                         QByteArray("If this request is bad, please ignore it "
                                    "while this plugin is being developed."));
#endif

    nam->post(request, requestContent);
}

QByteArray WakaTimeView::apiAuthBytes() {
    static const QByteArray apiKeyBytes = apiKey.toLocal8Bit();
    return QStringLiteral("Basic %1")
        .arg(QString::fromLocal8Bit(apiKeyBytes.toBase64()))
        .toLocal8Bit();
}

void WakaTimeView::sendHeartbeat(const QVariantMap &data,
                                 bool isWrite,
                                 bool saveToQueue) {
    QJsonDocument object = QJsonDocument::fromVariant(data);
    QByteArray requestContent = object.toJson();
    static const QString contentType = QStringLiteral("application/json");

    static QUrl url(QStringLiteral(kWakaTimeViewActionUrl));
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("User-Agent", userAgent);
    request.setRawHeader("Authorization", apiAuthBytes());
    request.setRawHeader("TimeZone", timeZoneBytes());

    qCDebug(gLogWakaTime) << object;
#ifndef NDEBUG
    request.setRawHeader("X-Ignore",
                         QByteArray("If this request is bad, please ignore it "
                                    "while this plugin is being developed."));
#endif

    if (saveToQueue) {
        static const QString keyType = QStringLiteral("type");
        static const QString keyCategory = QStringLiteral("category");
        static const QString valueFile = QStringLiteral("file");
        static const QString valueCoding = QStringLiteral("coding");
        static const QString keyEntity = QStringLiteral("entity");
        static const QString keyTime = QStringLiteral("time");
        static const QString keyProject = QStringLiteral("project");
        static const QString keyBranch = QStringLiteral("branch");

        // time-type-category-project-branch-entity-is_write
        const QString rowId =
            QStringLiteral("%1-%2-%3-%4-%5-%6-%7")
                .arg(data[keyTime].toString(),
                     data[keyType].toString(),
                     data[keyCategory].toString(),
                     data[keyProject].toString(),
                     data[keyBranch].toString(),
                     data[keyEntity].toString(),
                     isWrite ? QStringLiteral("1") : QStringLiteral("0"));
        const QString rowData = QString::fromUtf8(requestContent.constData());
        QStringList row;
        row << rowId << rowData;
        queue->push(row);
    }

    nam->post(request, requestContent);
}

void WakaTimeView::writeConfig(void) {
    QString configFilePath =
        QDir::homePath() + QDir::separator() + QStringLiteral(".wakatime.cfg");
    if (!QFile::exists(configFilePath)) {
        qCDebug(gLogWakaTime)
            << QStringLiteral("%1 does not exist").arg(configFilePath);
        return;
    }
    QSettings config(configFilePath, QSettings::IniFormat);
    config.setValue(QStringLiteral("settings/api_key"), apiKey);
    config.setValue(QStringLiteral("settings/hidefilenames"), hideFilenames);
    config.sync();
    QSettings::Status status;
    if ((status = config.status()) != QSettings::NoError) {
        qCDebug(gLogWakaTime)
            << "Failed to save WakaTime settings: " << status;
    }
}

void WakaTimeView::readConfig(void) {
    QString configFilePath =
        QDir::homePath() + QDir::separator() + QStringLiteral(".wakatime.cfg");
    if (!QFile::exists(configFilePath)) {
        qCDebug(gLogWakaTime)
            << QStringLiteral("%1 does not exist").arg(configFilePath);
        return;
    }

    QSettings config(configFilePath, QSettings::IniFormat);
    if (!config.contains(QStringLiteral("settings/api_key"))) {
        qCDebug(gLogWakaTime) << "No API key set in ~/.wakatime.cfg";
        return;
    }

    QString key = config.value(QStringLiteral("settings/api_key")).toString();
    if (!key.trimmed().length()) {
        qCDebug(gLogWakaTime) << "API Key is blank";
        return;
    }

    // Assume valid at this point
    apiKey = key;
    hideFilenames =
        config.value(QStringLiteral("settings/hidefilenames")).toBool();
}

bool WakaTimeView::documentIsConnected(KTextEditor::Document *document) {
    foreach (KTextEditor::Document *doc, connectedDocuments) {
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

void WakaTimeView::slotNetworkReplyFinshed(QNetworkReply *reply) {
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
        qCDebug(gLogWakaTime) << "Received:" << doc;

        if (statusCode == 201) { // 202 only happens from the bulk request
            queue->pop();
        }
        hasSent = true;
    } else {
        qCDebug(gLogWakaTime)
            << "Request did not succeed, status code:" << statusCode.toInt();
        static const QString errorsKeyStr = QStringLiteral("errors");

        if (statusCode == 401) {
            // TODO A handler for an incorrect API key will be worked on
            // shortly.
            qCDebug(gLogWakaTime)
                << "Check authentication details in ~/.wakatime.cfg";
        }

        foreach (QVariant error, received[errorsKeyStr].toList()) {
            qCDebug(gLogWakaTime) << error.toByteArray();
        }
    }

    // Documentation says the QNetworkReply object is owned here but this
    // delete causes a segfault with Kate
    // delete reply;
}

#include "wakatimeplugin.moc"
