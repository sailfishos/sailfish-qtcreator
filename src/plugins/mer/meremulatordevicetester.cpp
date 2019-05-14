/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
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

#include "meremulatordevicetester.h"

#include "merconnection.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerEmulatorDeviceTester::MerEmulatorDeviceTester(QObject *parent)
    : GenericLinuxDeviceTester(parent),
      m_pastVmStart(false)
{
}

MerEmulatorDeviceTester::~MerEmulatorDeviceTester()
{
}

void MerEmulatorDeviceTester::testDevice(const IDevice::Ptr &deviceConfiguration)
{
    m_device = deviceConfiguration.dynamicCast<MerEmulatorDevice>();
    QTC_ASSERT(m_device, { emit finished(TestFailure); return; });

    if (m_device->connection()->state() == MerConnection::Connected) {
        m_pastVmStart = true;
        GenericLinuxDeviceTester::testDevice(m_device.staticCast<IDevice>());
    } else {
        connect(m_device->connection(), &MerConnection::stateChanged,
                this, &MerEmulatorDeviceTester::onConnectionStateChanged);
        m_device->connection()->connectTo(MerConnection::AskStartVm);
    }
}

void MerEmulatorDeviceTester::stopTest()
{
    QTC_ASSERT(m_device, return);

    if (m_pastVmStart) {
        GenericLinuxDeviceTester::stopTest();
    } else {
        m_device->connection()->disconnect(this);
        emit finished(TestFailure);
    }
}

void MerEmulatorDeviceTester::onConnectionStateChanged()
{
    bool ok = false;

    switch (m_device->connection()->state()) {
    case MerConnection::Disconnected:
        emit errorMessage(tr("Virtual machine could not be started: User aborted"));
        break;

    case MerConnection::Error:
        emit errorMessage(m_device->connection()->errorString());
        break;

    case MerConnection::Connected:
        ok = true;
        break;

    case MerConnection::StartingVm:
        emit progressMessage(tr("Starting virtual machine..."));
        return;

    default:
        return;
    }

    m_device->connection()->disconnect(this);

    if (!ok) {
        emit finished(TestFailure);
    } else {
        m_pastVmStart = true;
        GenericLinuxDeviceTester::testDevice(m_device.staticCast<IDevice>());
    }
}

}
}
