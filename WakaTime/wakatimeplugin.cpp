#include "wakatimeplugin.h"
#include "wakatimeview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>

K_PLUGIN_FACTORY(WakaTimePluginFactory, registerPlugin<WakaTimePlugin>("ktexteditor_wakatime");)
K_EXPORT_PLUGIN(WakaTimePluginFactory("ktexteditor_wakatime", "ktexteditor_plugins"))

WakaTimePlugin::WakaTimePlugin(QObject *parent, const QVariantList &args)
: KTextEditor::Plugin(parent)
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
	for(int z = 0; z < m_views.size(); z++)
	{
		if(m_views.at(z)->parentClient() == view)
		{
			WakaTimeView *nview = m_views.at(z);
			m_views.removeAll(nview);
			delete nview;
		}
	}
}

void WakaTimePlugin::readConfig()
{
}

void WakaTimePlugin::writeConfig()
{
}

WakaTimeView::WakaTimeView(KTextEditor::View *view)
: QObject(view)
, KXMLGUIClient(view)
, m_view(view)
{
	setComponentData(WakaTimePluginFactory::componentData());
	
	KAction *action = new KAction(i18n("KTextEditor - WakaTime"), this);
	actionCollection()->addAction("tools_wakatime", action);
	//action->setShortcut(Qt::CTRL + Qt::Key_XYZ);
	connect(action, SIGNAL(triggered()), this, SLOT(insertWakaTime()));
	
	setXMLFile("wakatimeui.rc");
}

WakaTimeView::~WakaTimeView()
{
}

void WakaTimeView::insertWakaTime()
{
	m_view->document()->insertText(m_view->cursorPosition(), i18n("Hello, World!"));
}

#include "wakatimeview.moc"
