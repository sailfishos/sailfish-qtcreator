/****************************************************************************
**
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

#include "virtualmachine_p.h"

#include "vmconnection_p.h"

namespace Sfdk {

/*!
 * \class VirtualMachine
 */

VirtualMachine::ConnectionUiCreator VirtualMachine::s_connectionUiCreator;

VirtualMachine::VirtualMachine(std::unique_ptr<VirtualMachinePrivate> &&dd, QObject *parent)
    : QObject(parent)
    , d_ptr(std::move(dd))
{
    Q_D(VirtualMachine);
    QTC_ASSERT(s_connectionUiCreator, return);
    d->connectionUi_ = s_connectionUiCreator(this);
    d->connection = std::make_unique<VmConnection>(this);
    connect(d->connection.get(), &VmConnection::stateChanged, this, &VirtualMachine::stateChanged);
    connect(d->connection.get(), &VmConnection::virtualMachineOffChanged,
            this, &VirtualMachine::virtualMachineOffChanged);
    connect(d->connection.get(), &VmConnection::lockDownFailed, this, &VirtualMachine::lockDownFailed);
}

VirtualMachine::~VirtualMachine()
{
}

QString VirtualMachine::name() const
{
    return d_func()->name;
}

void VirtualMachine::setName(const QString &name)
{
    Q_D(VirtualMachine);

    if (d->name == name)
        return;

    d->prepareForNameChange();

    d->name = name;

    emit nameChanged(name);
}

VirtualMachine::State VirtualMachine::state() const
{
    return d_func()->connection->state();
}

QString VirtualMachine::errorString() const
{
    return d_func()->connection->errorString();
}

QSsh::SshConnectionParameters VirtualMachine::sshParameters() const
{
    return d_func()->sshParameters;
}

void VirtualMachine::setSshParameters(const QSsh::SshConnectionParameters &sshParameters)
{
    Q_D(VirtualMachine);
    if (d->sshParameters == sshParameters)
        return;
    d->sshParameters = sshParameters;
    emit sshParametersChanged();
}

bool VirtualMachine::isHeadless() const
{
    return d_func()->headless;
}

void VirtualMachine::setHeadless(bool headless)
{
    Q_D(VirtualMachine);
    if (d->headless == headless)
        return;
    d->headless = headless;
    emit headlessChanged(headless);
}

bool VirtualMachine::isAutoConnectEnabled() const
{
    return d_func()->autoConnectEnabled;
}

void VirtualMachine::setAutoConnectEnabled(bool autoConnectEnabled)
{
    Q_D(VirtualMachine);
    if (d->autoConnectEnabled == autoConnectEnabled)
        return;
    d->autoConnectEnabled = autoConnectEnabled;
    emit autoConnectEnabledChanged(autoConnectEnabled);
}

bool VirtualMachine::isOff(bool *runningHeadless, bool *startedOutside) const
{
    return d_func()->connection->isVirtualMachineOff(runningHeadless, startedOutside);
}

bool VirtualMachine::lockDown(bool lockDown)
{
    return d_func()->connection->lockDown(lockDown);
}

void VirtualMachine::refresh(Sfdk::VirtualMachine::Synchronization synchronization)
{
    d_func()->connection->refresh(synchronization);
}

bool VirtualMachine::connectTo(Sfdk::VirtualMachine::ConnectOptions options)
{
    return d_func()->connection->connectTo(options);
}

void VirtualMachine::disconnectFrom()
{
    d_func()->connection->disconnectFrom();
}

/*!
 * \class VirtualMachinePrivate
 * \internal
 */

} // namespace Sfdk
