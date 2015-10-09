/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "merplugin.h"

#include "meraddvmstartbuildstepprojectlistener.h"
#include "merbuildstepfactory.h"
#include "merconnection.h"
#include "merconnectionmanager.h"
#include "merconstants.h"
#include "merdeployconfigurationfactory.h"
#include "merdeploystepfactory.h"
#include "merdevicefactory.h"
#include "meremulatormodedialog.h"
#include "mergeneraloptionspage.h"
#include "mermode.h"
#include "meroptionspage.h"
#include "merqtversionfactory.h"
#include "merrunconfigurationfactory.h"
#include "merruncontrolfactory.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "mertoolchainfactory.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QtPlugin>

using namespace Core;
using namespace ExtensionSystem;

namespace Mer {
namespace Internal {

namespace {
const char *VM_NAME_PROPERTY = "merVmName";
}

MerPlugin::MerPlugin()
{
}

MerPlugin::~MerPlugin()
{
}

bool MerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(errorString)

    MerSdkManager::verbose = arguments.count(QLatin1String("-mer-verbose"));

    new MerSettings(this);

    addAutoReleasedObject(new MerSdkManager);
    addAutoReleasedObject(new MerVirtualBoxManager);
    addAutoReleasedObject(new MerConnectionManager);
    addAutoReleasedObject(new MerOptionsPage);
    addAutoReleasedObject(new MerGeneralOptionsPage);
    addAutoReleasedObject(new MerDeviceFactory);
    addAutoReleasedObject(new MerQtVersionFactory);
    addAutoReleasedObject(new MerToolChainFactory);
    addAutoReleasedObject(new MerAddVmStartBuildStepProjectListener);
    addAutoReleasedObject(new MerDeployConfigurationFactory);
    addAutoReleasedObject(new MerRunConfigurationFactory);
    addAutoReleasedObject(new MerRunControlFactory);
    addAutoReleasedObject(new MerBuildStepFactory);
    addAutoReleasedObject(new MerDeployStepFactory);

    addAutoReleasedObject(new MerMode);

    Command *emulatorConnectionCommand =
        ActionManager::command(Constants::MER_EMULATOR_CONNECTON_ACTION_ID);
    ModeManager::addAction(emulatorConnectionCommand->action(), 1);

    Command *sdkConnectionCommand =
        ActionManager::command(Constants::MER_SDK_CONNECTON_ACTION_ID);
    ModeManager::addAction(sdkConnectionCommand->action(), 1);

    ActionContainer *toolsMenu =
        ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *menu = ActionManager::createMenu(Constants::MER_TOOLS_MENU);
    menu->menu()->setTitle(tr("Me&r"));
    toolsMenu->addMenu(menu);

    MerEmulatorModeDialog *emulatorModeDialog = new MerEmulatorModeDialog(this);
    Command *emulatorModeCommand =
        ActionManager::registerAction(emulatorModeDialog->action(),
                                            Constants::MER_EMULATOR_MODE_ACTION_ID,
                                            Context(Core::Constants::C_GLOBAL));
    menu->addAction(emulatorModeCommand);

    return true;
}

void MerPlugin::extensionsInitialized()
{
}

IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    m_stopList.clear();
    QList<MerSdk*> sdks = MerSdkManager::sdks();
    foreach(const MerSdk* sdk, sdks) {
        MerConnection *connection = sdk->connection();
        bool headless = false;
        if (!connection->isVirtualMachineOff(&headless) && headless) {
            QMessageBox *prompt = new QMessageBox(
                    QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("The headless virtual machine \"%1\" is still running.\n\n"
                        "Close the virtual machine now?").arg(connection->virtualMachine()),
                    QMessageBox::Yes | QMessageBox::No,
                    ICore::mainWindow());
            prompt->setProperty(VM_NAME_PROPERTY, connection->virtualMachine());
            connect(prompt, &QMessageBox::finished,
                    this, &MerPlugin::handlePromptClosed);
            m_stopList.insert(connection->virtualMachine(), connection);
            prompt->open();
        }
    }
    if(m_stopList.isEmpty())
        return SynchronousShutdown;
    else
        return AsynchronousShutdown;
}

void MerPlugin::handlePromptClosed(int result)
{
    QMessageBox *prompt = qobject_cast<QMessageBox *>(sender());
    prompt->deleteLater();

    QString vm = prompt->property(VM_NAME_PROPERTY).toString();

    if (result == QMessageBox::Yes) {
        MerConnection *connection = m_stopList.value(vm);
        connect(connection, &MerConnection::stateChanged,
                this, &MerPlugin::handleConnectionStateChanged);
        connect(connection, &MerConnection::lockDownFailed,
                this, &MerPlugin::handleLockDownFailed);
        connection->lockDown(true);
    } else {
        m_stopList.remove(vm);
    }

    if(m_stopList.isEmpty()) {
        emit asynchronousShutdownFinished();
    }
}

void MerPlugin::handleConnectionStateChanged()
{
    MerConnection *connection = qobject_cast<MerConnection *>(sender());

    if (connection->state() == MerConnection::Disconnected) {
        m_stopList.remove(connection->virtualMachine());

        if (m_stopList.isEmpty()) {
            emit asynchronousShutdownFinished();
        }
    }
}

void MerPlugin::handleLockDownFailed()
{
    MerConnection *connection = qobject_cast<MerConnection *>(sender());

    m_stopList.remove(connection->virtualMachine());

    if (m_stopList.isEmpty()) {
        emit asynchronousShutdownFinished();
    }
}

} // Internal
} // Mer
