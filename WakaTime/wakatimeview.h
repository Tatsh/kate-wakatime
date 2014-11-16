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
        void connectSignalsToSlots();

    private:
        KTextEditor::View *m_view;
        QByteArray userAgent;
        QDateTime lastPoll;

        // Initialised in constructor definition
        QString apiKey;
        bool hasSent;
        QNetworkAccessManager *nam;
};

#endif
