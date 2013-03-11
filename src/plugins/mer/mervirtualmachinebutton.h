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

#ifndef MERVIRTUALMACHINEBUTTON_H
#define MERVIRTUALMACHINEBUTTON_H

#include <qglobal.h>
#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

class MerVirtualMachineButton : public QObject
{
    Q_OBJECT
public:
    explicit MerVirtualMachineButton(QObject *parent = 0);

    void setName(const QString &name) { m_name = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setStartTip(const QString &tip) { m_startTip = tip; }
    void setStopTip(const QString &tip) { m_stopTip = tip; }
    void setRunning(bool running) { m_running = running; }
    bool isRunning() const { return m_running; }
    void setVisible(bool visible) { m_visible = visible; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    void initialize();
    void update();

signals:
    void startRequest();
    void stopRequest();

private slots:
    void handleTriggered();

private:
    QAction *m_action;
    QIcon m_icon;
    QString m_name;
    QString m_startTip;
    QString m_stopTip;
    bool m_initalized;
    bool m_running;
    bool m_visible;
    bool m_enabled;
};

}
}
#endif // MERVIRTUALMACHINEBUTTON_H
