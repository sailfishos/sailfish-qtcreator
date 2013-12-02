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

#include "merdevicefactory.h"
#include "meremulatordevicewizard.h"
#include "meremulatordevice.h"
#include "merhardwaredevicewizard.h"
#include "merhardwaredevice.h"
#include "merconstants.h"
#include "mersdkmanager.h"

#include <utils/qtcassert.h>
#include <remotelinux/linuxdevicetestdialog.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>

namespace Mer {
namespace Internal {

MerDeviceFactory::MerDeviceFactory()
{
    setObjectName(QLatin1String("MerDeviceFactory"));
}

QString MerDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == Constants::MER_DEVICE_TYPE_I486)
        return tr("Mer Emulator Device");
    if (type == Constants::MER_DEVICE_TYPE_ARM)
        return tr("Mer ARM Device");
    return QString();
}

bool MerDeviceFactory::canCreate(Core::Id type)
{
    return type == Core::Id(Constants::MER_DEVICE_TYPE_I486) ||
           type == Core::Id(Constants::MER_DEVICE_TYPE_ARM);
}


QList<Core::Id> MerDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::MER_DEVICE_TYPE_I486)
                             << Core::Id(Constants::MER_DEVICE_TYPE_ARM);
}

ProjectExplorer::IDevice::Ptr MerDeviceFactory::create(Core::Id id) const
{
    QTC_ASSERT(canCreate(id), return ProjectExplorer::IDevice::Ptr());

    if(id == Constants::MER_DEVICE_TYPE_I486) {
        MerEmulatorDeviceWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return ProjectExplorer::IDevice::Ptr();

        QSsh::SshConnectionParameters sshParams;
        sshParams.host = QLatin1String("localhost");
        sshParams.userName = wizard.userName();
        sshParams.port = wizard.sshPort();
        sshParams.timeout = wizard.timeout();
        sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;
        sshParams.privateKeyFile = wizard.userPrivateKey();

        //hardcoded values requested by customer;
        MerEmulatorDevice::Ptr device = MerEmulatorDevice::create();
        device->setVirtualMachine(wizard.emulatorVm());
        device->setMac(wizard.mac());
        device->setSubnet(QLatin1String("10.220.220"));
        device->setDisplayName(wizard.configName());
        device->setFreePorts(Utils::PortList::fromString(wizard.freePorts()));
        device->setSshParameters(sshParams);
        device->setSharedConfigPath(wizard.sharedConfigPath());
        device->setSharedSshPath(wizard.sharedSshPath());

        if(wizard.isUserNewSshKeysRquired() && !wizard.userPrivateKey().isEmpty()) {
            device->generteSshKey(wizard.userName());
        }

        if(wizard.isRootNewSshKeysRquired() && !wizard.rootPrivateKey().isEmpty()) {
            device->generteSshKey(wizard.rootName());
        }

        RemoteLinux::GenericLinuxDeviceTester* tester = new RemoteLinux::GenericLinuxDeviceTester();
        RemoteLinux::LinuxDeviceTestDialog dlg(device,tester);
        dlg.exec();
        return device;
    }

    if(id == Constants::MER_DEVICE_TYPE_ARM) {
        MerHardwareDeviceWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return ProjectExplorer::IDevice::Ptr();

        QSsh::SshConnectionParameters sshParams;
        //sshParams.options &= ~SshConnectionOptions(SshEnableStrictConformanceChecks); // For older SSH servers.
        sshParams.host = wizard.hostName();
        sshParams.userName = wizard.userName();
        sshParams.port = 22;
        sshParams.timeout = 10;
        sshParams.authenticationType = wizard.authenticationType();
        if (sshParams.authenticationType == QSsh::SshConnectionParameters::AuthenticationByPassword)
            sshParams.password = wizard.password();
        else
            sshParams.privateKeyFile = wizard.privateKeyFilePath();
        MerHardwareDevice::Ptr device = MerHardwareDevice::create(wizard.configurationName());
        device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));
        device->setSshParameters(sshParams);

        RemoteLinux::GenericLinuxDeviceTester* tester = new RemoteLinux::GenericLinuxDeviceTester();
        RemoteLinux::LinuxDeviceTestDialog dlg(device,tester);
        dlg.exec();
        return device;
    }

    return ProjectExplorer::IDevice::Ptr();
}

bool MerDeviceFactory::canRestore(const QVariantMap &map) const
{
    return canCreate(ProjectExplorer::IDevice::typeFromMap(map));
}

ProjectExplorer::IDevice::Ptr MerDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return ProjectExplorer::IDevice::Ptr());
    if(ProjectExplorer::IDevice::typeFromMap(map) == Constants::MER_DEVICE_TYPE_I486) {
        const ProjectExplorer::IDevice::Ptr device = MerEmulatorDevice::create();
        device->fromMap(map);
        return device;
    }
    if(ProjectExplorer::IDevice::typeFromMap(map) == Constants::MER_DEVICE_TYPE_ARM) {
        const ProjectExplorer::IDevice::Ptr device = MerHardwareDevice::create();
        device->fromMap(map);
        return device;
    }
    return ProjectExplorer::IDevice::Ptr();
}

} // Internal
} // Mer
