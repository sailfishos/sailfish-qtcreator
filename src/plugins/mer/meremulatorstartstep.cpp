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

#include "meremulatorstartstep.h"
#include "merconnectionmanager.h"
#include "merconstants.h"

#include <remotelinux/abstractremotelinuxdeployservice.h>
#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Mer {
namespace Internal {

class MerEmulatorStartService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    MerEmulatorStartService(QObject *parent = 0) : AbstractRemoteLinuxDeployService(parent) {}

private:
    bool isDeploymentNecessary() const { return true; }

    void doDeviceSetup()
    {
        emit progressMessage(tr("Checking whether to start Emulator..."));
        IDevice::ConstPtr device = deviceConfiguration();
        if (device->machineType() == IDevice::Hardware) {
            emit progressMessage(tr("Target device is not an emulator. Nothing to do."));
            handleDeviceSetupDone(true);
            return;
        }

        const QString vmName = device->id().toString();
        MerConnectionManager *em = MerConnectionManager::instance();
        if (em->isConnected(device->sshParameters())) {
            emit progressMessage(tr("Emulator is already running. Nothing to do."));
            handleDeviceSetupDone(true);
            return;
        } else {
            emit errorMessage(tr("Virtual Machine '%1' is not running!").arg(vmName));
            handleDeviceSetupDone(false);
            return;
        }
    }

    void stopDeviceSetup() { handleDeviceSetupDone(false); }
    void doDeploy() { handleDeploymentDone(); }
    void stopDeployment() { handleDeploymentDone(); }
};

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
    setDefaultDisplayName(stepDisplayName());
}

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl, MerEmulatorStartStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

AbstractRemoteLinuxDeployService *MerEmulatorStartStep::deployService() const
{
    return m_service;
}

bool MerEmulatorStartStep::initInternal(QString *error)
{
    return deployService()->isDeploymentPossible(error);
}

void MerEmulatorStartStep::ctor()
{
    m_service = new MerEmulatorStartService(this);
}

Core::Id MerEmulatorStartStep::stepId()
{
    return Core::Id("Mer.MerEmulatorCheckStep");
}

QString MerEmulatorStartStep::stepDisplayName()
{
    return tr("Start Emulator, if necessary");
}

} // Internal
} // Mer

#include "meremulatorstartstep.moc"
