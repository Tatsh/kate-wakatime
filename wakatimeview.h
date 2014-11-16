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

#ifndef WAKATIMEVIEW_H
#define WAKATIMEVIEW_H

#include <QObject>
#include <KXMLGUIClient>

namespace KTextEditor
{
    class Document;
}

class QNetworkAccessManager;
class QNetworkReply;

#define kWakaTimeViewActionUrl "https://wakatime.com/api/v1/actions"

class WakaTimeView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    public:
        explicit WakaTimeView(KTextEditor::View *view = 0);
        ~WakaTimeView();

    private slots:
        void slotDocumentModifiedChanged(KTextEditor::Document *);
        void slotDocumentWrittenToDisk(KTextEditor::Document *);
        void slotNetworkReplyFinshed(QNetworkReply *);

    private:
        void readConfig();
        void sendAction(KTextEditor::Document *doc, bool isWrite);
        QByteArray getUserAgent();
        void connectDocumentSignals(KTextEditor::Document *);

    private:
        KTextEditor::View *m_view;
        QByteArray userAgent;

        // Initialised in constructor definition
        QString apiKey;
        bool hasSent;
        QDateTime lastPoll;
        QNetworkAccessManager *nam;
};

#endif
