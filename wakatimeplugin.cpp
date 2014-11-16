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
#include "wakatimeview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KAboutData>
#include <KDateTime>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

// QJson
#include <qjson/parser.h>
#include <qjson/serializer.h>

K_PLUGIN_FACTORY(WakaTimePluginFactory, registerPlugin<WakaTimePlugin>("ktexteditor_wakatime");)
K_EXPORT_PLUGIN(WakaTimePluginFactory(KAboutData(
    "ktexteditor_kdatatool",
    "ktexteditor_plugins",
    ki18n("WakaTime"),
    "0.2",
    ki18n("Plugin for WakaTime integration"),
    KAboutData::License_LGPL_V3
)))

int debugArea() {
    static int sArea = KDebug::registerArea("wakatime");
    return sArea;
}

WakaTimePlugin::WakaTimePlugin(QObject *parent, const QVariantList &args) :
    KTextEditor::Plugin(parent)
{
    Q_UNUSED(args);
}

WakaTimePlugin::~WakaTimePlugin()
{
}

void WakaTimePlugin::addView(KTextEditor::View *view)
{
    WakaTimeView *nview = new WakaTimeView(view);
    m_views.append(nview);
}

void WakaTimePlugin::removeView(KTextEditor::View *view)
{
    for (int z = 0; z < m_views.size(); z++) {
        if (m_views.at(z)->parentClient() == view) {
            WakaTimeView *nview = m_views.at(z);
            m_views.removeAll(nview);
            delete nview;
        }
    }
}

/**
 * @todo Offline support (save a persistent queue until network connectivity is regained).
 */
WakaTimeView::WakaTimeView(KTextEditor::View *view) :
    QObject(view),
    KXMLGUIClient(view),
    m_view(view),
    apiKey(""),
    hasSent(false),
    lastPoll(QDateTime::currentDateTime()),
    nam(new QNetworkAccessManager(this))
{
    setComponentData(WakaTimePluginFactory::componentData());

    this->readConfig();
    this->userAgent = this->getUserAgent();

    // Connect the request handling slot method
    connect(
        nam, SIGNAL(finished(QNetworkReply *)),
        this, SLOT(slotNetworkReplyFinshed(QNetworkReply *))
    );

    this->connectDocumentSignals(view->document());
}

WakaTimeView::~WakaTimeView()
{
    delete nam;
}

/**
 * @todo Correctly set Kate version.
 */
QByteArray WakaTimeView::getUserAgent()
{
    return QString("kate-wakatime/%1 (KDE %2) Katepart/3.1x.x").arg(WAKATIME_PLUGIN_VERSION).arg(KDE::versionString()).toLocal8Bit();
}

/**
 * @todo Respect 'hidefilenames' option.
 * @todo Handle number of lines changed?
 * @todo Get branch name of project (Git).
 * @todo Get branch name of project (Subversion).
 * @todo Better way to get project name?
 */
void WakaTimeView::sendAction(KTextEditor::Document *doc, bool isWrite)
{
    QString filePath = doc->url().toLocalFile();

    // Could be untitled, or a URI (including HTTP); only local files are handled for now
    if (!filePath.length()) {
        //kDebug(debugArea()) << "Nothing to send about";
        return;
    }

    // TODO Compare date and make sure it has been at least 15 minutes
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;
    static int interval = (60 * 15) * 1000;
    if (this->hasSent && (current - this->lastPoll.currentMSecsSinceEpoch()) <= interval) {
        //kDebug(debugArea()) << "Not enough time has passed since last send";
        return;
    }

    QFileInfo fileInfo(filePath);

    // They have it sending the real file path, maybe not respecting symlinks, etc
    filePath = fileInfo.canonicalFilePath();

    // Get the project name, by traversing up until .git or .svn is found
    QString projectName;
    QDir currentDirectory = QDir(QFileInfo(fileInfo.canonicalPath()).canonicalPath());
    QDir projectDirectory;
    bool vcDirFound = false;
    QStringList filters;
    filters << ".git" << ".svn";
    QString typeOfVcs;

    //kDebug(debugArea()) << currentDirectory.canonicalPath();
    while (!vcDirFound) {
        if (currentDirectory.canonicalPath() == "/") {
            break;
        }

        QFileInfoList entries = currentDirectory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);

        foreach(QFileInfo entry, entries) {
            QString name = entry.fileName();

            if ((name == ".git" || name == ".svn") && entry.isDir()) {
                vcDirFound = true;
                projectName = currentDirectory.dirName();
                projectDirectory = QDir(currentDirectory);
                typeOfVcs = name;
                break;
            }
        }

        currentDirectory.cdUp();
    }

    QUrl url(kWakaTimeViewActionUrl);
    QNetworkRequest request(url);
    QByteArray apiKeyBytes = this->apiKey.toLocal8Bit();
    QString authString = QString("Basic %1").arg(QString(apiKeyBytes.toBase64()));
    QJson::Serializer serializer;

    QVariantMap data;
    data.insert("file", filePath);
    data.insert("time", current);
    if (projectName.length()) {
        data.insert("project", projectName);
    }
//     if (typeOfVcs == ".git") {
//         // git branch -a | fgrep '*' | awk '{ print $2 }', etc
//     }
    //data.insert("lines");
    if (isWrite) {
        data.insert("is_write", isWrite);
    }

    // This is good enough for the language most of the time
    QString mode = doc->mode();
    if (mode.length()) {
        data.insert("language", mode);
    }

    bool serializedOk;
    QByteArray requestContent = serializer.serialize(data, &serializedOk);

    if (!serializedOk) {
        kError(debugArea()) << "QJson could not serialise the data";
        kError(debugArea()) << serializer.errorMessage();
        return;
    }

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", this->userAgent);
    request.setRawHeader("Authorization", authString.toLocal8Bit());

    QString timeZone = KDateTime::currentLocalDateTime().timeZone().name();
    request.setRawHeader("TimeZone", timeZone.toLocal8Bit());

    // For now
    request.setRawHeader("X-Ignore", QByteArray("If this request is bad, please ignore it while this plugin is being developed."));

    nam->post(request, requestContent);
}

/**
 * @todo Alert user when an error occurs here.
 * @todo Make a dialog for configuration that edits ~/.wakatime.cfg
 */
void WakaTimeView::readConfig()
{
    QString configFilePath = QDir::homePath() + QDir::separator() + ".wakatime.cfg";
    if (!QFile::exists(configFilePath)) {
        kError(debugArea()) << QString("%1 does not exist").arg(configFilePath);
        return;
    }

    QSettings config(configFilePath, QSettings::IniFormat);
    if (!config.contains("settings/api_key")) {
        kError(debugArea()) << "No API key set in ~/.wakatime.cfg";
        return;
    }

    QString key = config.value("settings/api_key").toString();
    if (key.length() < 36) {
        kError(debugArea()) << "API key exists but is not correct length";
        return;
    }

    // Assume valid at this point
    this->apiKey = key;
    //kDebug(debugArea()) << QString("API key: %1").arg(this->apiKey);
}

void WakaTimeView::connectDocumentSignals(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }

    // When document goes from saved state to changed state (not yet saved on disk)
    connect(
        document, SIGNAL(modifiedChanged(KTextEditor::Document *)),
        this, SLOT(slotDocumentModifiedChanged(KTextEditor::Document *))
    );

    // When document is first saved?
    connect(
        document, SIGNAL(documentNameChanged(KTextEditor::Document*)),
        this, SLOT(slotDocumentWrittenToDisk(KTextEditor::Document *))
    );

    // Written to disk
    connect(
        document, SIGNAL(documentSavedOrUploaded(KTextEditor::Document*,bool)),
        this, SLOT(slotDocumentWrittenToDisk(KTextEditor::Document*))
    );

    // Text changes (might be heavy)
    // This event unfortunately is emitted twice in separate threads for every key stroke (maybe key up and down is the reason)
    connect(
        document, SIGNAL(textChanged(KTextEditor::Document *)),
        this, SLOT(slotDocumentModifiedChanged(KTextEditor::Document*))
    );
}

// Slots
void WakaTimeView::slotDocumentModifiedChanged(KTextEditor::Document *doc)
{
    this->sendAction(doc, false);
}

void WakaTimeView::slotDocumentWrittenToDisk(KTextEditor::Document *doc)
{
    this->sendAction(doc, true);
}

void WakaTimeView::slotNetworkReplyFinshed(QNetworkReply *reply)
{
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QJson::Parser parser;
    bool parsedOk;
    QVariantMap received;

    //kDebug(debugArea()) << "network reply finished slot handler";
    //kDebug(debugArea()) << "Status code:" << statusCode.toInt();

    received = parser.parse(reply->readAll(), &parsedOk).toMap();
    if (!parsedOk) {
        kDebug(debugArea()) << "QJson could not parse response. All responses are expected to be JSON serialised";
        return;
    }

    if (reply->error() == QNetworkReply::NoError && statusCode == 201) {
//         kDebug(debugArea()) << "Sent data successfully";
//         kDebug(debugArea()) << "ID received:" << received["data"].toMap()["id"].toString();

        this->hasSent = true;
        this->lastPoll = QDateTime::currentDateTime(); // Reset
    }
    else {
        kError(debugArea()) << "Request did not succeed, status code:" << statusCode.toInt();

        if (statusCode == 401) {
            kError(debugArea()) << "Check authentication details in ~/.wakatime.cfg";
        }

        foreach (QVariant error, received["errors"].toList()) {
            kError(debugArea()) << error.toByteArray();
        }
    }

    // Documentation says the QNetworkReply object is owned here but this
    // delete causes a segfault with Kate
    //delete reply;
}

#include "wakatimeview.moc"
