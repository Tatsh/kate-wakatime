#ifndef WAKATIMEVIEW_H
#define WAKATIMEVIEW_H

#include <QObject>
#include <KXMLGUIClient>

class WakaTimeView : public QObject, public KXMLGUIClient
{
	Q_OBJECT
	public:
		explicit WakaTimeView(KTextEditor::View *view = 0);
		~WakaTimeView();
	private slots:
		void insertWakaTime();
	private:
		KTextEditor::View *m_view;
};

#endif
