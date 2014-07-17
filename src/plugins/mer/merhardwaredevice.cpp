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
#include "merhardwaredevicewizardpages.h"

#include <remotelinux/remotelinux_constants.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <ssh/sshconnection.h>
#include <projectexplorer/devicesupport/devicemanager.h>

#include <QDir>
#include <QTimer>
#include <QProgressDialog>
#include <QWizard>

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

ProjectExplorer::IDevice::Ptr MerHardwareDevice::clone() const
{
    return Ptr(new MerHardwareDevice(*this));
}

MerHardwareDevice::MerHardwareDevice()
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

void MerHardwareDevice::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerHardwareDevice::sharedSshPath() const
{
    return m_sharedSshPath;
}

void MerHardwareDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_sharedSshPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_SSH)).toString();
}

QVariantMap MerHardwareDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_SSH), m_sharedSshPath);
    return map;
}

ProjectExplorer::IDeviceWidget *MerHardwareDevice::createWidget()
{
    return new MerHardwareDeviceWidget(sharedFromThis());
}


QList<Core::Id> MerHardwareDevice::actionIds() const
{
    QList<Core::Id> ids;
    //ids << Core::Id(Constants::MER_HARDWARE_DEPLOYKEY_ACTION_ID);
    return ids;
}

QString MerHardwareDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Constants::MER_HARDWARE_DEPLOYKEY_ACTION_ID)
        return tr("Redeploy SSH Keys");
    return QString();
}


void MerHardwareDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    Q_UNUSED(parent);
    QTC_ASSERT(actionIds().contains(actionId), return);

    if (actionId ==  Constants::MER_HARDWARE_DEPLOYKEY_ACTION_ID) {

        //TODO:
        return;
    }
}

} // Internal
} // Mer
