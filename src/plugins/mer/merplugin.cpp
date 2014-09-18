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
#include "merdevicefactory.h"
#include "merqtversionfactory.h"
#include "mertoolchainfactory.h"
#include "meraddvmstartbuildstepprojectlistener.h"
#include "merdeployconfigurationfactory.h"
#include "merrunconfigurationfactory.h"
#include "merruncontrolfactory.h"
#include "meroptionspage.h"
#include "merbuildstepfactory.h"
#include "merdeploystepfactory.h"
#include "mersdkmanager.h"
#include "merconnection.h"
#include "merconnectionmanager.h"
#include "mervirtualboxmanager.h"
#include "mermode.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtPlugin>
#include <QMessageBox>
#include <QTimer>

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

    addAutoReleasedObject(new MerSdkManager);
    addAutoReleasedObject(new MerVirtualBoxManager);
    addAutoReleasedObject(new MerConnectionManager);
    addAutoReleasedObject(new MerOptionsPage);
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

    return true;
}

void MerPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    m_stopList.clear();
    QList<MerSdk*> sdks = MerSdkManager::instance()->sdks();
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
                    Core::ICore::mainWindow());
            prompt->setProperty(VM_NAME_PROPERTY, connection->virtualMachine());
            connect(prompt, SIGNAL(finished(int)), this, SLOT(handlePromptClosed(int)));
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
        connect(connection, SIGNAL(stateChanged()), this, SLOT(handleConnectionStateChanged()));
        connect(connection, SIGNAL(lockDownFailed()), this, SLOT(handleLockDownFailed()));
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

Q_EXPORT_PLUGIN2(Mer, Mer::Internal::MerPlugin)
