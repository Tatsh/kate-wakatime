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
