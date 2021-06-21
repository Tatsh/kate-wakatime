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

#include <KCoreAddons/KAboutData>
#include <KCoreAddons/KPluginFactory>
#include <KCoreAddons/KPluginLoader>
#include <KI18n/KLocalizedString>
#include <KWidgetsAddons/KMessageBox>
#include <KXmlGui/KActionCollection>
#include <KXmlGui/KXMLGUIFactory>

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

const QByteArray timeZoneBytes() {
    const QDateTime dt = QDateTime::currentDateTime();
    const QString timeZone = QTimeZone::systemTimeZone().displayName(dt);
    return timeZone.toLocal8Bit();
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
    KXMLGUIClient::setComponentName(QLatin1String("katewakatime"),
                                    i18n("WakaTime"));
    setXMLFile(QLatin1String("ui.rc"));
    QAction *a =
        actionCollection()->addAction(QLatin1String("configure_wakatime"));
    a->setText(i18n("Configure WakaTime..."));
    a->setIcon(QIcon::fromTheme(QLatin1String("wakatime")));
    connect(
        a, &QAction::triggered, this, &WakaTimeView::slotConfigureWakaTime);
    mainWindow->guiFactory()->addClient(this);

    QString configFilePath =
        QDir::homePath() + QDir::separator() + QLatin1String(".wakatime.cfg");
    struct utsname buf;
    config = new QSettings(configFilePath, QSettings::IniFormat, this);
    readConfig();
    int unameRes = uname(&buf);
    static const QString unk = QStringLiteral("Unknown");
    userAgent =
        QString(QStringLiteral("wakatime/%1 (%2-%3-%4-%5) KTextEditor/%6 kate-wakatime/%7"))
            .arg(QString(config->value(QLatin1String("internal/cli_version"))
                     .toString())
                     .trimmed())
            .arg(unameRes == 0 ? QString::fromUtf8(buf.sysname) : unk)
            .arg(unameRes == 0 ? QString::fromUtf8(buf.release) : unk)
            .arg(unameRes == 0 ? QString::fromUtf8(buf.nodename) : unk)
            .arg(unameRes == 0 ? QString::fromUtf8(buf.machine) : unk)
            .arg(unk)
            .arg(QStringLiteral(kWakaTimePluginVersion)).toUtf8();

    // Connect the request handling slot method
    connect(nam,
            &QNetworkAccessManager::finished,
            this,
            &WakaTimeView::slotNetworkReplyFinished);

    connect(m_mainWindow,
            &KTextEditor::MainWindow::viewCreated,
            this,
            &WakaTimeView::viewCreated);

    for (KTextEditor::View *view : m_mainWindow->views()) {
        connectDocumentSignals(view->document());
    }

    sendQueuedHeartbeats();
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
    ui.lineEdit_apiUrl->setText(apiUrl);
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

    static const char *const kDefaultPath =
        "/usr/bin:/usr/local/bin:/opt/bin:/opt/local/bin";
    static const QString slash = QLatin1String("/");
    static const QString colon = QLatin1String(":");

    const char *const path = getenv("PATH");
    QStringList paths = QString::fromUtf8(path ? path : kDefaultPath)
                            .split(colon, Qt::SkipEmptyParts);

    for (QString path : paths) {
        QStringList dirListing = QDir(path).entryList();
        for (QString entry : dirListing) {
            if (entry == binName) {
                entry = path.append(slash).append(entry);
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

    QFileInfo fileInfo(filePath);

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
    static const QString gitStr = QLatin1String(".git");
    static const QString svnStr = QLatin1String(".svn");
    static const QString rootSlash = QLatin1String("/");
    filters << gitStr << svnStr;
    QString typeOfVcs;

    while (!vcDirFound) {
        if (!currentDirectory.canonicalPath().compare(rootSlash)) {
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
    static const QString keyTime = QLatin1String("time");
    static const QString keyProject = QLatin1String("project");
    static const QString keyBranch = QLatin1String("branch");

    data.insert(keyTime, currentMs / 1000);
    if (projectName.length()) {
        data.insert(keyProject, projectName);
    } else {
        qCDebug(gLogWakaTime) << "Warning: No project name found";
    }
    static const QString git = QLatin1String("git");
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
        static const QString keyIsWrite = QLatin1String("is_write");
        data.insert(keyIsWrite, isWrite);
    }

    // This is good enough for the language most of the time
    QString mode = doc->mode();
    static const QString keyLanguage = QLatin1String("language");
    if (mode.length()) {
        data.insert(keyLanguage, mode);
    } else {
        mode = doc->highlightingMode();
        if (mode.length()) {
            data.insert(keyLanguage, mode);
        }
    }

    static const QString keyType = QLatin1String("type");
    static const QString keyCategory = QLatin1String("category");
    static const QString valueFile = QLatin1String("file");
    static const QString valueCoding = QLatin1String("coding");
    static const QString keyEntity = QLatin1String("entity");

    data.insert(keyType, valueFile);
    data.insert(keyCategory, valueCoding);

    if (!hideFilenames) {
        static const QString keyLines = QLatin1String("lines");
        static const QString keyLineNo = QLatin1String("lineno");
        static const QString keyCursorPos = QLatin1String("cursorpos");
        data.insert(keyEntity, filePath);
        data.insert(keyLines, doc->lines());
        for (KTextEditor::View *view : m_mainWindow->views()) {
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
    QString heartbeats = QLatin1String("[");
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
            jsonBody.append(QLatin1String(","));
        }
        heartbeats.append(jsonBody);
        i++;
    }
    heartbeats.append(QLatin1String("]"));
    static QUrl url(
        QString(QStringLiteral("%1/v1/users/current/heartbeats.bulk"))
            .arg(apiUrl));
    static const QString contentType = QLatin1String("application/json");
    QNetworkRequest request(url);
    QByteArray requestContent = heartbeats.toUtf8();

    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
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
    QJsonDocument object = QJsonDocument::fromVariant(data);
    QByteArray requestContent =
        QByteArrayLiteral("[") + object.toJson() + QByteArrayLiteral("]");
    static const QString contentType = QLatin1String("application/json");

    static QUrl url(
        QString(QStringLiteral("%1/v1/users/current/heartbeats.bulk"))
            .arg(apiUrl));
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
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
        static const QString keyType = QLatin1String("type");
        static const QString keyCategory = QLatin1String("category");
        static const QString valueFile = QLatin1String("file");
        static const QString valueCoding = QLatin1String("coding");
        static const QString keyEntity = QLatin1String("entity");
        static const QString keyTime = QLatin1String("time");
        static const QString keyProject = QLatin1String("project");
        static const QString keyBranch = QLatin1String("branch");

        // time-type-category-project-branch-entity-is_write
        const QString rowId =
            QStringLiteral("%1-%2-%3-%4-%5-%6-%7")
                .arg(data[keyTime].toString(),
                     data[keyType].toString(),
                     data[keyCategory].toString(),
                     data[keyProject].toString(),
                     data[keyBranch].toString(),
                     data[keyEntity].toString(),
                     isWrite ? QLatin1String("1") : QLatin1String("0"));
        const QString rowData = QString::fromUtf8(requestContent.constData());
        QStringList row;
        row << rowId << rowData;
        queue->push(row);
    }

    nam->post(request, requestContent);
}

void WakaTimeView::writeConfig(void) {
    config->setValue(QLatin1String("settings/api_key"), apiKey);
    config->setValue(QLatin1String("settings/api_url"), apiUrl);
    config->setValue(QLatin1String("settings/hidefilenames"), hideFilenames);
    config->sync();
    QSettings::Status status = config->status();
    if (status != QSettings::NoError) {
        qCDebug(gLogWakaTime) << "Failed to save WakaTime settings:" << status;
    }
}

void WakaTimeView::readConfig(void) {
    const QString apiKeyPath = QLatin1String("settings/api_key");
    const QString apiUrlPath = QLatin1String("settings/api_url");

    if (!config->contains(apiKeyPath)) {
        qCDebug(gLogWakaTime) << "No API key set in ~/.wakatime.cfg";
        return;
    }

    QString key = config->value(apiKeyPath).toString().trimmed();
    if (!key.length()) {
        qCDebug(gLogWakaTime) << "API Key is blank";
        return;
    }

    QString url = QStringLiteral("https://wakatime.com/api");
    if (config->contains(apiUrlPath) &&
        QString(config->value(apiUrlPath).toString()).trimmed().length()) {
        url = QString(config->value(apiUrlPath).toString()).trimmed();
    }

    // Assume valid at this point
    apiKey = key;
    apiUrl = url;
    hideFilenames =
        config->value(QLatin1String("settings/hidefilenames")).toBool();
}

bool WakaTimeView::documentIsConnected(KTextEditor::Document *document) {
    for (KTextEditor::Document *doc : connectedDocuments) {
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

        if (statusCode == 201) { // 202 only happens from the bulk request
            queue->pop();
        }
        hasSent = true;
    } else {
        qCDebug(gLogWakaTime) << "URL:" << reply->url().toString();
        qCDebug(gLogWakaTime)
            << "Request did not succeed, status code:" << statusCode.toInt();
        static const QString errorsKeyStr = QLatin1String("errors");

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
