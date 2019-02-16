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

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAboutData>
#include <KActionCollection>
#include <KPluginFactory>
#include <KPluginLoader>

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

WakaTimePlugin::WakaTimePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent) {
    Q_UNUSED(args);
}

WakaTimePlugin::~WakaTimePlugin() {
}

void WakaTimeView::viewCreated(KTextEditor::View *view) {
    this->connectDocumentSignals(view->document());
}

void WakaTimeView::viewDestroyed(QObject *view) {
    this->disconnectDocumentSignals(
        static_cast<KTextEditor::View *>(view)->document());
}

WakaTimeView::WakaTimeView(KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow), hasSent(false),
      lastTimeSent(QDateTime::currentDateTime()),
      nam(new QNetworkAccessManager(this)),
      binPathCache(QMap<QString, QString>()) {
    this->apiKey = QString::fromLocal8Bit("", 0);
    this->lastFileSent = QString::fromLocal8Bit("", 0);

    this->readConfig();
    this->userAgent = this->getUserAgent();

    // Connect the request handling slot method
    connect(nam,
            SIGNAL(finished(QNetworkReply *)),
            this,
            SLOT(slotNetworkReplyFinshed(QNetworkReply *)));

    connect(m_mainWindow,
            &KTextEditor::MainWindow::viewCreated,
            this,
            &WakaTimeView::viewCreated);

    foreach (KTextEditor::View *view, m_mainWindow->views()) {
        this->connectDocumentSignals(view->document());
    }
}

WakaTimeView::~WakaTimeView() {
    delete nam;
}

QObject *WakaTimePlugin::createView(KTextEditor::MainWindow *mainWindow) {
    return new WakaTimeView(mainWindow);
}

QString WakaTimeView::getBinPath(QString binName) {
#ifdef Q_OS_WIN
    return QString();
#endif

    if (binPathCache.contains(binName)) {
        return binPathCache.value(binName);
    }

    static const QString slash = QString::fromLocal8Bit("/");
    static const QString colon = QString::fromLocal8Bit(":");

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
    return QString::fromLocal8Bit("(KDE %1) Katepart/%1 kate-wakatime/%2")
        .arg(QString::fromLocal8Bit("5"))
        .arg(QString::fromLocal8Bit(kWakaTimePluginVersion))
        .toLocal8Bit();
}

void WakaTimeView::sendAction(KTextEditor::Document *doc, bool isWrite) {
    QString filePath = doc->url().toLocalFile();

    // Could be untitled, or a URI (including HTTP); only local files are
    // handled for now
    if (!filePath.length()) {
#ifndef NDEBUG
        qCDebug(gLogWakaTime) << "Nothing to send about";
#endif
        return;
    }

    QFileInfo fileInfo(filePath);

    // They have it sending the real file path, maybe not respecting symlinks,
    // etc
    filePath = fileInfo.canonicalFilePath();
#ifndef NDEBUG
    qCDebug(gLogWakaTime) << "File path:" << filePath;
#endif

    // Compare date and make sure it has been at least 15 minutes
    const qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 deltaMs = currentMs - this->lastTimeSent.toMSecsSinceEpoch();
    QString lastFileSent = this->lastFileSent;
    static const qint64 intervalMs = 120000; // ms

    // If the current file has not changed and it has not been 2 minutes since
    // the last heartbeat was sent, do NOT send this heartbeat. This does not
    // apply to write events as they are always sent.
    if (!isWrite) {
        if (this->hasSent && deltaMs <= intervalMs &&
            lastFileSent == filePath) {
#ifndef NDEBUG
            qCDebug(gLogWakaTime)
                << "Not enough time has passed since last send";
            qCDebug(gLogWakaTime)
                << "Delta:" << deltaMs / 1000 / 60 << "/ 2 minutes";
#endif
            return;
        }
    }

    // Get the project name, by traversing up until .git or .svn is found
    QString projectName;
    QDir currentDirectory = QDir(fileInfo.canonicalPath());
    QDir projectDirectory;
    bool vcDirFound = false;
    static QStringList filters;
    static const QString gitStr = QString::fromLocal8Bit(".git");
    static const QString svnStr = QString::fromLocal8Bit(".svn");
    static const QString rootSlash = QString::fromLocal8Bit("/");
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

    static QUrl url(QString::fromLocal8Bit(kWakaTimeViewActionUrl));
    QNetworkRequest request(url);
    static const QByteArray apiKeyBytes = this->apiKey.toLocal8Bit();
    static const QString authString =
        QString::fromLocal8Bit("Basic %1")
            .arg(QString::fromLocal8Bit(apiKeyBytes.toBase64()));
    QJsonDocument object;

    QVariantMap data;
    static const QString keyFile = QString::fromLocal8Bit("file");
    static const QString keyTime = QString::fromLocal8Bit("time");

    data.insert(keyFile, filePath);
    data.insert(keyTime, currentMs / 1000);
    if (projectName.length()) {
        static const QString keyProject = QString::fromLocal8Bit("project");
        data.insert(keyProject, projectName);
    } else {
        qCDebug(gLogWakaTime) << "Warning: No project name found";
    }
    static const QString git = QString::fromLocal8Bit("git");
    static QString gitPath = getBinPath(git);
    if (!gitPath.isNull()) {
        static const QString cmd = gitPath.append(
            QString::fromLocal8Bit(" symbolic-ref --short HEAD"));
        QProcess proc;
        proc.setWorkingDirectory(projectDirectory.canonicalPath());
#ifndef NDEBUG
        qCDebug(gLogWakaTime)
            << "Running " << cmd << " in " << projectDirectory.canonicalPath();
#endif
        proc.start(cmd);
        if (proc.waitForFinished()) {
            QByteArray out = proc.readAllStandardOutput();
            QString branch =
                QString::fromStdString(out.toStdString()).trimmed();
            if (!branch.isNull() && branch.size() > 0) {
                static const QString keyBranch =
                    QString::fromLocal8Bit("branch");
                data.insert(keyBranch, branch);
            }
        } else {
            qCDebug(gLogWakaTime) << "Failed to get branch (git)";
#ifndef NDEBUG
            qCDebug(gLogWakaTime)
                << "stderr: "
                << QString::fromStdString(
                       proc.readAllStandardError().toStdString())
                       .trimmed();
            qCDebug(gLogWakaTime)
                << "stdout: "
                << QString::fromStdString(
                       proc.readAllStandardOutput().toStdString())
                       .trimmed();
#endif
            qCDebug(gLogWakaTime)
                << "If this is not expected, please file a bug report.";
        }
    }
#ifndef NDEBUG
    if (gitPath.isNull()) {
        qCDebug(gLogWakaTime) << "\"git\" not found in PATH";
    }
#endif

    if (isWrite) {
        static const QString keyIsWrite = QString::fromLocal8Bit("is_write");
        data.insert(keyIsWrite, isWrite);
    }

    // This is good enough for the language most of the time
    QString mode = doc->mode();
    static const QString keyLanguage = QString::fromLocal8Bit("language");
    if (mode.length()) {
        data.insert(keyLanguage, mode);
    } else {
        mode = doc->highlightingMode();
        if (mode.length()) {
            data.insert(keyLanguage, mode);
        }
    }

    static const QString keyEntity = QString::fromLocal8Bit("entity");
    static const QString keyType = QString::fromLocal8Bit("type");
    static const QString valueFile = QString::fromLocal8Bit("file");
    static const QString keyCategory = QString::fromLocal8Bit("category");
    static const QString keyLines = QString::fromLocal8Bit("lines");
    static const QString keyLineNo = QString::fromLocal8Bit("lineno");
    static const QString keyCursorPos = QString::fromLocal8Bit("cursorpos");
    static const QString valueCoding = QString::fromLocal8Bit("coding");
    data.insert(keyEntity, filePath);
    data.insert(keyType, valueFile);
    data.insert(keyLines, doc->lines());
    data.insert(keyCategory, valueCoding);
    foreach (KTextEditor::View *view, m_mainWindow->views()) {
        if (view->document() == doc) {
            data.insert(keyLineNo, view->cursorPosition().line() + 1);
            data.insert(keyCursorPos, view->cursorPosition().column() + 1);
            break;
        }
    }

    object = QJsonDocument::fromVariant(data);
    QByteArray requestContent = object.toJson();
    static const QString contentType =
        QString::fromLocal8Bit("application/json");

    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("User-Agent", this->userAgent);
    request.setRawHeader("Authorization", authString.toLocal8Bit());

    const QDateTime dt = QDateTime::currentDateTime();
    const QString timeZone = QTimeZone::systemTimeZone().displayName(dt);
    request.setRawHeader("TimeZone", timeZone.toLocal8Bit());

#ifndef NDEBUG
    qCDebug(gLogWakaTime) << object;
    request.setRawHeader("X-Ignore",
                         QByteArray("If this request is bad, please ignore it "
                                    "while this plugin is being developed."));
#endif

    nam->post(request, requestContent);

    this->lastTimeSent = QDateTime::currentDateTime();
    this->lastFileSent = filePath;
}

void WakaTimeView::readConfig(void) {
    QString configFilePath = QDir::homePath() + QDir::separator() +
                             QString::fromLocal8Bit(".wakatime.cfg");
    if (!QFile::exists(configFilePath)) {
        qCDebug(gLogWakaTime)
            << QString::fromUtf8("%1 does not exist").arg(configFilePath);
        return;
    }

    QSettings config(configFilePath, QSettings::IniFormat);
    if (!config.contains(QString::fromLocal8Bit("settings/api_key"))) {
        qCDebug(gLogWakaTime) << "No API key set in ~/.wakatime.cfg";
        return;
    }

    QString key =
        config.value(QString::fromLocal8Bit("settings/api_key")).toString();
    if (!key.trimmed().length()) {
        qCDebug(gLogWakaTime) << "API Key is blank";
        return;
    }

    // Assume valid at this point
    this->apiKey = key;
    this->hideFilenames = config.value(QString::fromLocal8Bit("settings/hidefilenames")).toBool();
}

bool WakaTimeView::documentIsConnected(KTextEditor::Document *document) {
    foreach (KTextEditor::Document *doc, this->connectedDocuments) {
        if (doc == document) {
            return true;
        }
    }
    return false;
}

void WakaTimeView::connectDocumentSignals(KTextEditor::Document *document) {
    if (!document || this->documentIsConnected(document)) {
        return;
    }

    // When document goes from saved state to changed state (not yet saved on
    // disk)
    connect(document,
            SIGNAL(modifiedChanged(KTextEditor::Document *)),
            this,
            SLOT(slotDocumentModifiedChanged(KTextEditor::Document *)));

    // Written to disk
    connect(document,
            SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)),
            this,
            SLOT(slotDocumentWrittenToDisk(KTextEditor::Document *)));

    // Text changes (might be heavy)
    // This event unfortunately is emitted twice in separate threads for every
    // key stroke (maybe key up and down is the reason)
    connect(document,
            SIGNAL(textChanged(KTextEditor::Document *)),
            this,
            SLOT(slotDocumentModifiedChanged(KTextEditor::Document *)));

    this->connectedDocuments << document;
}

void WakaTimeView::disconnectDocumentSignals(KTextEditor::Document *document) {
    if (!this->documentIsConnected(document)) {
        return;
    }

    disconnect(document, SIGNAL(modifiedChanged(KTextEditor::Document *)));
    disconnect(document,
               SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)));
    disconnect(document,
               SIGNAL(documentSavedOrUploaded(KTextEditor::Document *, bool)));
    disconnect(document, SIGNAL(textChanged(KTextEditor::Document *)));

    this->connectedDocuments.removeOne(document);
}

// Slots
void WakaTimeView::slotDocumentModifiedChanged(KTextEditor::Document *doc) {
    this->sendAction(doc, false);
}

void WakaTimeView::slotDocumentWrittenToDisk(KTextEditor::Document *doc) {
    this->sendAction(doc, true);
}

void WakaTimeView::slotNetworkReplyFinshed(QNetworkReply *reply) {
    const QVariant statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

#ifndef NDEBUG
    qCDebug(gLogWakaTime) << "Status code:" << statusCode.toInt();
#endif

    const QByteArray contents = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(contents);
    if (doc.isNull()) {
        qCDebug(gLogWakaTime) << "Could not parse response. All responses are "
                                 "expected to be JSON serialised";
        return;
    }

    const QVariantMap received = doc.toVariant().toMap();

    if (reply->error() == QNetworkReply::NoError && statusCode == 201) {
        qCDebug(gLogWakaTime) << "Sent data successfully";

        this->hasSent = true;
    } else {
        qCDebug(gLogWakaTime)
            << "Request did not succeed, status code:" << statusCode.toInt();
        static const QString errorsKeyStr = QString::fromLocal8Bit("errors");

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
