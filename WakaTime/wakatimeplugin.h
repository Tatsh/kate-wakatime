#ifndef WAKATIMEPLUGIN_H
#define WAKATIMEPLUGIN_H

#include <KTextEditor/Plugin>

#include <QtCore/QDateTime>

#define WAKATIME_PLUGIN_VERSION "0.1"

namespace KTextEditor
{
    class Document;
    class View;
}

class QFile;
class WakaTimeView;

class WakaTimePlugin : public KTextEditor::Plugin
{
    public:
        explicit WakaTimePlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
        virtual ~WakaTimePlugin();

        void addView (KTextEditor::View *view);
        void removeView (KTextEditor::View *view);

    private:
        QList<class WakaTimeView*> m_views;
};

#endif
