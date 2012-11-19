/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#ifndef JOLLAPLUGIN_H
#define JOLLAPLUGIN_H

#include <iwidgetplugin.h>

namespace QmlDesigner {

class JollaPlugin : public QObject, QmlDesigner::IWidgetPlugin
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmlDesignerPlugin" FILE "jollaplugin.json")
#endif
    Q_INTERFACES(QmlDesigner::IWidgetPlugin)

public:
    JollaPlugin();

    QString metaInfo() const;
    QString pluginName() const;
};

} // namespace QmlDesigner

#endif // JOLLAPLUGIN_H
