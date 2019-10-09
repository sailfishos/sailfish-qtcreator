/****************************************************************************
**
** Copyright (C) 2012-2015,2019 Jolla Ltd.
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

#include "merhardwaredevice.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "merhardwaredevicewidget.h"
#include "merhardwaredevicewizardpages.h"
#include "mersdkmanager.h"

#include <sfdk/sfdkconstants.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <remotelinux/remotelinux_constants.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProgressDialog>
#include <QTimer>
#include <QWizard>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerHardwareDevice::MerHardwareDevice()
    : m_architecture(ProjectExplorer::Abi::UnknownArchitecture)
{
    setMachineType(IDevice::Hardware);
    m_qmlLivePorts.addPort(Utils::Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT));
}

IDevice::Ptr MerHardwareDevice::clone() const
{
    return Ptr(new MerHardwareDevice(*this));
}

void MerHardwareDevice::fromMap(const QVariantMap &map)
{
    MerDevice::fromMap(map);
    m_architecture = static_cast<Abi::Architecture>(
            map.value(QLatin1String(Constants::MER_DEVICE_ARCHITECTURE),
                Abi::UnknownArchitecture).toInt());
    m_qmlLivePorts = Utils::PortList::fromString(map.value(QLatin1String(Constants::MER_DEVICE_QML_LIVE_PORTS),
                                                           QString::number(Sfdk::Constants::DEFAULT_QML_LIVE_PORT))
                                                 .toString());
}

QVariantMap MerHardwareDevice::toMap() const
{
    QVariantMap map = MerDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_ARCHITECTURE), m_architecture);
    map.insert(QLatin1String(Constants::MER_DEVICE_QML_LIVE_PORTS), m_qmlLivePorts.toString());
    return map;
}

Abi::Architecture MerHardwareDevice::architecture() const
{
    return m_architecture;
}

void MerHardwareDevice::setArchitecture(const Abi::Architecture &architecture)
{
    m_architecture = architecture;
}

IDeviceWidget *MerHardwareDevice::createWidget()
{
    return new MerHardwareDeviceWidget(sharedFromThis());
}

Utils::PortList MerHardwareDevice::qmlLivePorts() const
{
    return m_qmlLivePorts;
}

void MerHardwareDevice::setQmlLivePorts(const Utils::PortList &qmlLivePorts)
{
    m_qmlLivePorts = qmlLivePorts;
}

} // Internal
} // Mer
