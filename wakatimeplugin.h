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

#ifndef WAKATIMEPLUGIN_H
#define WAKATIMEPLUGIN_H

#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>

#include "ui_configdialog.h"

#define kWakaTimePluginVersion "1.3.9"

Q_DECLARE_LOGGING_CATEGORY(gLogWakaTime)

namespace KTextEditor {
class Document;
class MainWindow;
class View;
} // namespace KTextEditor

class QDateTime;
class QFile;
class QNetworkAccessManager;
class QNetworkReply;

class OfflineQueue;
class WakaTimeView;

class WakaTimePlugin : public KTextEditor::Plugin {
public:
    explicit WakaTimePlugin(QObject *parent = 0,
                            const QList<QVariant> & = QList<QVariant>());
    virtual ~WakaTimePlugin();
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    QList<WakaTimeView *> m_views;
};

class WakaTimeView : public QObject, public KXMLGUIClient {
    Q_OBJECT

public:
    enum WakaTimeApiHttpHeaders {
        AuthorizationHeader,
        TimeZoneHeader,
        XIgnoreHeader,
        XMachineName,
    };

    WakaTimeView(KTextEditor::MainWindow *);
    ~WakaTimeView();

private Q_SLOTS:
    void slotConfigureWakaTime();
    void slotDocumentModifiedChanged(KTextEditor::Document *);
    void slotDocumentWrittenToDisk(KTextEditor::Document *);
    void slotNetworkReplyFinished(QNetworkReply *);
    void viewCreated(KTextEditor::View *);
    void viewDestroyed(QObject *);

private:
    QByteArray apiAuthBytes();
    bool documentIsConnected(KTextEditor::Document *);
    void connectDocumentSignals(KTextEditor::Document *);
    void disconnectDocumentSignals(KTextEditor::Document *);
    QString getBinPath(QString);
    void readConfig();
    void sendAction(KTextEditor::Document *, bool);
    void sendHeartbeat(const QVariantMap &, bool, bool saveToQueue = true);
    void sendQueuedHeartbeats();
    void writeConfig();

private:
    KTextEditor::MainWindow *m_mainWindow;
    QString apiKey;
    QString apiUrl;
    QMap<QString, QString> binPathCache;
    bool hasSent;
    bool hideFilenames;

    // Initialised in constructor definition
    QList<KTextEditor::Document *> connectedDocuments;
    QSettings *config;
    QString lastFileSent;
    QDateTime lastTimeSent;
    QNetworkAccessManager *nam;
    OfflineQueue *queue;
    QByteArray userAgent;
};

#endif
