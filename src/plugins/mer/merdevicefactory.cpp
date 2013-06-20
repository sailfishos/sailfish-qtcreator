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
#include "meremulatordevicedialog.h"
#include "meremulatordevice.h"
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
    //if (type == Constants::MER_DEVICE_TYPE_ARM)
    //    return tr("Mer ARM Device");
    return QString();
}

bool MerDeviceFactory::canCreate(Core::Id type)
{
    return type == Core::Id(Constants::MER_DEVICE_TYPE_I486); //||
          //  type == Core::Id(Constants::MER_DEVICE_TYPE_ARM);
}


QList<Core::Id> MerDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::MER_DEVICE_TYPE_I486);
                           //  << Core::Id(Constants::MER_DEVICE_TYPE_ARM);
}

ProjectExplorer::IDevice::Ptr MerDeviceFactory::create(Core::Id id) const
{
    QTC_ASSERT(canCreate(id), return ProjectExplorer::IDevice::Ptr());
    MerEmulatorDeviceDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return ProjectExplorer::IDevice::Ptr();

    QSsh::SshConnectionParameters sshParams;
    sshParams.host = QLatin1String("localhost");
    sshParams.userName = dialog.userName();
    sshParams.port = dialog.sshPort();
    sshParams.timeout = dialog.timeout();
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;
    sshParams.privateKeyFile = dialog.privateKey();

    //hardcoded values requested by customer;
    QString mac = QString(QLatin1String("08:00:5A:11:00:0%1")).arg(dialog.index());
    MerEmulatorDevice::Ptr device = MerEmulatorDevice::create();
    device->setVirtualMachine(dialog.emulatorVm());
    device->setMac(mac);
    device->setSubnet(QLatin1String("10.220.220"));
    device->setIndex(dialog.index());
    device->setDisplayName(dialog.configName());
    device->setFreePorts(Utils::PortList::fromString(dialog.freePorts()));
    device->setSshParameters(sshParams);
    device->setSharedConfigPath(dialog.sharedConfigPath());
    device->setSharedSshPath(dialog.sharedSshPath());
    if(dialog.requireSshKeys()) {
        QStringList users;
        //TODO: reconsider hardcoded values
        users << QLatin1String("nemo");
        users << QLatin1String("root");
        if (!users.contains(dialog.userName()))
            users << dialog.userName();
        foreach(const QString& user, users)
        {
            device->generteSshKey(user);
        }
    }

    RemoteLinux::GenericLinuxDeviceTester* tester = new RemoteLinux::GenericLinuxDeviceTester();
    RemoteLinux::LinuxDeviceTestDialog dlg(device,tester);
    dlg.exec();
    return device;
}

bool MerDeviceFactory::canRestore(const QVariantMap &map) const
{
    return canCreate(ProjectExplorer::IDevice::typeFromMap(map));
}

ProjectExplorer::IDevice::Ptr MerDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return ProjectExplorer::IDevice::Ptr());
    const ProjectExplorer::IDevice::Ptr device = MerEmulatorDevice::create();
    device->fromMap(map);
    return device;
}

} // Internal
} // Mer
