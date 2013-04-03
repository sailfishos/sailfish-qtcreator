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

#include "mervirtualmachinebutton.h"
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/modemanager.h>
#include <QAction>

namespace Mer {
namespace Internal {

const QSize iconSize = QSize(24, 20);

MerVirtualMachineButton::MerVirtualMachineButton(QObject *parent)
    : QObject(parent)
    , m_action(new QAction(this))
    , m_initalized(false)
    , m_running(false)
    , m_visible(false)
    , m_enabled(false)
{
}

void MerVirtualMachineButton::initialize()
{
    if (m_initalized)
        return;

    m_action->setText(m_name);
    m_action->setIcon(m_icon.pixmap(iconSize));
    m_action->setToolTip(m_startTip);
    connect(m_action, SIGNAL(triggered()), SLOT(handleTriggered()));

    Core::Command *command =
            Core::ActionManager::registerAction(m_action, Core::Id(m_name),
                                                Core::Context(Core::Constants::C_GLOBAL));
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setAttribute(Core::Command::CA_UpdateIcon);

    Core::ModeManager::addAction(command->action(), 1);
    m_action->setEnabled(m_enabled);
    m_action->setVisible(m_visible);
    m_initalized = true;
}

void MerVirtualMachineButton::update()
{
    QIcon::State state;
    QString toolTip;
    if (m_running) {
        state = QIcon::On;
        toolTip = m_stopTip;
    } else {
        state = QIcon::Off;
        toolTip = m_startTip;
    }

    m_action->setEnabled(m_enabled);
    m_action->setVisible(m_visible);
    m_action->setToolTip(toolTip);
    m_action->setIcon(m_icon.pixmap(iconSize, QIcon::Normal, state));
}

void MerVirtualMachineButton::handleTriggered()
{
    if (m_running)
        emit stopRequest();
    else
        emit startRequest();
}

} // Internal
} // Mer
