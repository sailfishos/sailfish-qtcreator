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

#include "vboxvirtualmachine_p.h"

#include "virtualboxmanager_p.h"

namespace Sfdk {

/*!
 * \class VBoxVirtualMachine
 */

VBoxVirtualMachine::VBoxVirtualMachine(QObject *parent)
    : VirtualMachine(std::make_unique<VBoxVirtualMachinePrivate>(this), parent)
{
    connect(this, &VirtualMachine::nameChanged, this, [this]() { d_func()->onNameChanged(); });
}

VBoxVirtualMachine::~VBoxVirtualMachine()
{
    d_func()->prepareForNameChange();
}

// Provides list of all used VMs, that is valid also during configuration of new build
// engines/emulators, before the changes are applied.
QStringList VBoxVirtualMachine::usedVirtualMachines()
{
    return VBoxVirtualMachinePrivate::s_usedVmNames.keys();
}

/*!
 * \class VBoxVirtualMachinePrivate
 * \internal
 */

QMap<QString, int> VBoxVirtualMachinePrivate::s_usedVmNames;

void VBoxVirtualMachinePrivate::start(const QObject *context, const Functor<bool> &functor)
{
    VirtualBoxManager::startVirtualMachine(q_func()->name(), q_func()->isHeadless(), context,
            functor);
}

void VBoxVirtualMachinePrivate::stop(const QObject *context, const Functor<bool> &functor)
{
    VirtualBoxManager::shutVirtualMachine(q_func()->name(), context, functor);
}

void VBoxVirtualMachinePrivate::probe(const QObject *context,
        const Functor<BasicState, bool> &functor) const
{
    VirtualBoxManager::probe(q_func()->name(), context, functor);
}

void VBoxVirtualMachinePrivate::prepareForNameChange()
{
    Q_Q(VBoxVirtualMachine);
    if (!q->name().isEmpty()) {
        if (--s_usedVmNames[q->name()] == 0)
            s_usedVmNames.remove(q->name());
    }
}

void VBoxVirtualMachinePrivate::onNameChanged()
{
    Q_Q(VBoxVirtualMachine);
    if (!q->name().isEmpty()) {
        if (++s_usedVmNames[q->name()] != 1)
            qCWarning(lib) << "VirtualMachine: Another instance for VM" << q->name() << "already exists";
    }
}

} // namespace Sfdk
