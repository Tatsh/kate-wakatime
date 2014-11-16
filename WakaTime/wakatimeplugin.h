#ifndef WAKATIMEPLUGIN_H
#define WAKATIMEPLUGIN_H

#include <KTextEditor/Plugin>

namespace KTextEditor
{
	class View;
}

class WakaTimeView;

class WakaTimePlugin
  : public KTextEditor::Plugin
{
  public:
    // Constructor
    explicit WakaTimePlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    // Destructor
    virtual ~WakaTimePlugin();

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);
 
    void readConfig();
    void writeConfig();
 
//     void readConfig (KConfig *);
//     void writeConfig (KConfig *);
 
  private:
    QList<class WakaTimeView*> m_views;
};

#endif
