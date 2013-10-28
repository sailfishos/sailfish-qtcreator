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

#include "merhardwaredevice.h"
#include "merhardwaredevicewidget.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"
#include "merdevicefactory.h"

#include <remotelinux/remotelinux_constants.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <ssh/sshconnection.h>
#include <projectexplorer/devicesupport/devicemanager.h>

#include <QDir>
#include <QTimer>
#include <QProgressDialog>

namespace Mer {
namespace Internal {

MerHardwareDevice::Ptr MerHardwareDevice::create()
{
    return Ptr(new MerHardwareDevice);
}

MerHardwareDevice::Ptr MerHardwareDevice::create(const QString &name,
                                 Origin origin,
                                 Core::Id id)
{
    return Ptr(new MerHardwareDevice(name, Constants::MER_DEVICE_TYPE_ARM, IDevice::Hardware, origin, id));
}

QString MerHardwareDevice::displayType() const
{
    return QLatin1String("Mer ARM");
}

MerHardwareDevice::MerHardwareDevice(const QString &name,
                     Core::Id type,
                     MachineType machineType,
                     Origin origin,
                     Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

MerHardwareDevice::MerHardwareDevice(const MerHardwareDevice &other)
    : RemoteLinux::LinuxDevice(other)
{
}

ProjectExplorer::IDevice::Ptr MerHardwareDevice::clone() const
{
    return Ptr(new MerHardwareDevice(*this));
}

MerHardwareDevice::MerHardwareDevice()
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

} // Internal
} // Mer
