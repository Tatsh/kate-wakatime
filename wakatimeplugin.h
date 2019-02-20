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

#include "ui_configdialog.h"

#define kWakaTimeViewActionUrl "https://wakatime.com/api/v1/actions"
#define kWakaTimePluginVersion "1.1.0"

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

class WakaTimeView;

class WakaTimePlugin : public KTextEditor::Plugin {
public:
    explicit WakaTimePlugin(QObject *parent = 0,
                            const QList<QVariant> & = QList<QVariant>());
    virtual ~WakaTimePlugin();

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    QList<class WakaTimeView *> m_views;
};

class WakaTimeView : public QObject, public KXMLGUIClient {
    Q_OBJECT

public:
    WakaTimeView(KTextEditor::MainWindow *);
    ~WakaTimeView();

private Q_SLOTS:
    void slotDocumentModifiedChanged(KTextEditor::Document *);
    void slotDocumentWrittenToDisk(KTextEditor::Document *);
    void slotNetworkReplyFinshed(QNetworkReply *);
    void slotConfigureWakaTime();
    void viewCreated(KTextEditor::View *);
    void viewDestroyed(QObject *);

private:
    void readConfig();
    void writeConfig();
    void sendAction(KTextEditor::Document *, bool);
    QByteArray getUserAgent();
    void connectDocumentSignals(KTextEditor::Document *);
    bool documentIsConnected(KTextEditor::Document *);
    void disconnectDocumentSignals(KTextEditor::Document *);
    QString getBinPath(QString);

private:
    KTextEditor::MainWindow *m_mainWindow;
    QByteArray userAgent;
    QString apiKey;
    bool hideFilenames;
    bool hasSent;
    QList<KTextEditor::Document *> connectedDocuments;

    // Initialised in constructor definition
    QDateTime lastTimeSent;
    QString lastFileSent;
    QNetworkAccessManager *nam;
    QMap<QString, QString> binPathCache;
};

#endif
