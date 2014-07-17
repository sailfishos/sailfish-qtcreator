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

#include "meractionmanager.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "meremulatordevice.h"
#include "merconnectionmanager.h"
#include "mervirtualboxmanager.h"
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Core;

namespace Mer {
namespace Internal {

MerActionManager *MerActionManager::m_instance = 0;

MerActionManager::MerActionManager():
    m_menu(0),
    m_startMenu(0),
    m_stopMenu(0)
{
    //connect(DeviceManager::instance(), SIGNAL(devicesLoaded()), SLOT(updateActions()));
    //connect(DeviceManager::instance(), SIGNAL(updated()), SLOT(updateActions()));
    //connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()),SLOT(updateActions()));
    //setupActions();
}

MerActionManager::~MerActionManager()
{
    m_instance = 0;
}

MerActionManager *MerActionManager::instance()
{
    QTC_CHECK(m_instance);
    return m_instance;
}

void MerActionManager::setupActions()
{
    m_menu = ActionManager::createMenu(Constants::MER_SAILFISH_MENU);
    m_menu->menu()->setTitle(tr("&SailfishSDK"));
    m_menu->menu()->setEnabled(true);

    ActionContainer *menubar = ActionManager::actionContainer(Core::Constants::MENU_BAR);
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_HELP);
    menubar->addMenu(mtools, m_menu);

    m_menu->appendGroup(Constants::MER_SAILFISH_VM_GROUP_MENU);
    m_menu->appendGroup(Constants::MER_SAILFISH_OTHER_GROUP_MENU);

    m_startMenu = ActionManager::createMenu(Constants::MER_SAILFISH_START_MENU);
    m_startMenu->menu()->setIcon(QIcon(QLatin1String(Constants::MER_SAILFISH_START_ICON)));
    m_startMenu->menu()->setTitle(tr("&Start Virtual Machine"));
    m_menu->addMenu(m_startMenu,Constants::MER_SAILFISH_VM_GROUP_MENU);

    m_stopMenu = ActionManager::createMenu(Constants::MER_SAILFISH_STOP_MENU);
    m_stopMenu->menu()->setTitle(tr("&Stop Virtual Machine"));
    m_stopMenu->menu()->setIcon(QIcon(QLatin1String(Constants::MER_SAILFISH_STOP_ICON)));
    m_menu->addMenu(m_stopMenu,Constants::MER_SAILFISH_VM_GROUP_MENU);
    QAction *optionsAction = new QAction(tr("Options..."),m_menu);
    connect(optionsAction,SIGNAL(triggered()),this,SLOT(handleOptionsTriggered()));

    m_menu->menu()->addAction(optionsAction);
}


void MerActionManager::updateActions()
{

    QMenu *startMenu = m_startMenu->menu();
    startMenu->clear();
    QMenu *stopMenu = m_stopMenu->menu();
    stopMenu->clear();

    const QList<MerSdk*> sdks = MerSdkManager::instance()->sdks();

    foreach(MerSdk* sdk, sdks) {
        QAction *startAction = new QAction(sdk->virtualMachineName(),m_startMenu);
        startMenu->addAction(startAction);
        connect(startAction,SIGNAL(triggered()),this, SLOT(handleStartTriggered()));
        QAction *stopAction = new QAction(sdk->virtualMachineName(),m_stopMenu);
        connect(stopAction,SIGNAL(triggered()),this, SLOT(handleStopTriggered()));
        stopMenu->addAction(stopAction);
    }

    int count = DeviceManager::instance()->deviceCount();
    for(int i = 0 ;  i < count; ++i) {
        IDevice::ConstPtr d = DeviceManager::instance()->deviceAt(i);
        if (d->type() == Constants::MER_DEVICE_TYPE_I486) {
            const MerEmulatorDevice* device = static_cast<const MerEmulatorDevice*>(d.data());
            QAction *startAction = new QAction(device->virtualMachine(),m_startMenu);
            connect(startAction,SIGNAL(triggered()),this, SLOT(handleStartTriggered()));
            startMenu->addAction(startAction);
            QAction *stopAction = new QAction(device->virtualMachine(),m_stopMenu);
            connect(stopAction,SIGNAL(triggered()),this, SLOT(handleStopTriggered()));
            stopMenu->addAction(stopAction);
        }
    }

    startMenu->menuAction()->setEnabled(!startMenu->actions().isEmpty());
    stopMenu->menuAction()->setEnabled(!startMenu->actions().isEmpty());
}

void MerActionManager::handleOptionsTriggered()
{
    Core::ICore::showOptionsDialog(Constants::MER_OPTIONS_CATEGORY,Constants::MER_OPTIONS_ID);
}

void MerActionManager::handleStartTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && !action->text().isEmpty())
    MerVirtualBoxManager::startVirtualMachine(action->text(),false);
}

void MerActionManager::handleStopTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && !action->text().isEmpty())
    MerVirtualBoxManager::shutVirtualMachine(action->text());
}

//This ungly code fixes the isse of showing the messagebox, while QProcess
//form build stps ends. Since messagebox reenters event loop
//QProcess emits finnished singal and cleans up. Meanwhile users closes box end continues
//with invalid m_process pointer.

Prompt* MerActionManager::createStartPrompt(const QString& virtualMachine)
{
   Prompt* prompt = new Prompt();
   prompt->m_title = tr("Start Virtual Machine");
   prompt->m_text = tr("Virtual Machine '%1' is not running! Please start the "
                       "Virtual Machine and retry after the Virtual Machine is "
                       "running.\n\n"
                       "Start Virtual Machine now?").arg(virtualMachine);
   prompt->m_vm = virtualMachine;
   return prompt;
}

Prompt* MerActionManager::createClosePrompt(const QString& virtualMachine)
{
    Prompt* prompt = new Prompt();
    prompt->m_title = tr("Stop Virtual Machine");
    prompt->m_text = tr("The headless virtual machine '%1' is still running!\n\n"
                        "Stop Virtual Machine now?").arg(virtualMachine);
    prompt->m_vm = virtualMachine;
    return prompt;
}

Prompt::Prompt()
{
    QTimer::singleShot(0,this,SLOT(show()));
}

void Prompt::show()
{
    const QMessageBox::StandardButton response =
        QMessageBox::question(0, m_title, m_text, QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (response == QMessageBox::Yes) {
        emit closed(m_vm,true);
    } else {
        emit closed(m_vm,false);
    }
}

}
}
