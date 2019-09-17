/****************************************************************************
**
** Copyright (C) 2015-2017,2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "merdevice.h"

#include <sfdk/sfdkconstants.h>

#include <utils/qtcassert.h>

#include "merconstants.h"

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Sfdk;

namespace Mer {

namespace {
    //! \todo Add IDevice::machineTypeFromMap and remove this workaround
    // See MachineTypeKey in projectexplorer/devicesupport/idevice.cpp
    const char workaround_MachineTypeKey[] = "Type";
}

QString MerDevice::displayType() const
{
    return tr("Sailfish OS Device");
}

void MerDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_sharedSshPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_SSH)).toString();
    m_qmlLivePorts = Utils::PortList::fromString(map.value(QLatin1String(Constants::MER_DEVICE_QML_LIVE_PORTS),
                                                           QString::number(Sfdk::Constants::DEFAULT_QML_LIVE_PORT))
                                                 .toString());
}

QVariantMap MerDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_SSH), m_sharedSshPath);
    map.insert(QLatin1String(Constants::MER_DEVICE_QML_LIVE_PORTS), m_qmlLivePorts.toString());
    return map;
}

IDevice::MachineType
    MerDevice::workaround_machineTypeFromMap(const QVariantMap &map)
{
    return static_cast<MachineType>(
            map.value(QLatin1String(workaround_MachineTypeKey), Hardware).toInt());
}

void MerDevice::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerDevice::sharedSshPath() const
{
    return m_sharedSshPath;
}

Utils::PortList MerDevice::qmlLivePorts() const
{
    return m_qmlLivePorts;
}

void MerDevice::setQmlLivePorts(const Utils::PortList &qmlLivePorts)
{
    m_qmlLivePorts = qmlLivePorts;
}

QList<Utils::Port> MerDevice::qmlLivePortsList() const
{
    Utils::PortList ports(m_qmlLivePorts);
    QList<Utils::Port> retv;
    while (ports.hasMore() && retv.count() < Sfdk::Constants::MAX_PORT_LIST_PORTS)
        retv.append(ports.getNext());
    QTC_CHECK(!ports.hasMore());
    return retv;
}

MerDevice::MerDevice()
{
    setType(Core::Id(Constants::MER_DEVICE_TYPE));
    setDeviceState(IDevice::DeviceStateUnknown);
    m_qmlLivePorts.addPort(Utils::Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT));
}

MerDevice::~MerDevice()
{
}

}
