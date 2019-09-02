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

#include <utils/qtcassert.h>

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

int VBoxVirtualMachine::memorySizeMb() const
{
    Q_D(const VBoxVirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.memorySizeMb;
}

void VBoxVirtualMachine::setMemorySizeMb(int memorySizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VBoxVirtualMachine);

    const QPointer<const QObject> context_{context};
    VirtualBoxManager::setMemorySizeMb(name(), memorySizeMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.memorySizeMb != memorySizeMb) {
            d->virtualMachineInfo.memorySizeMb = memorySizeMb;
            emit memorySizeMbChanged(memorySizeMb);
        }
        if (context_)
            functor(ok);
    });
}

int VBoxVirtualMachine::cpuCount() const
{
    Q_D(const VBoxVirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.cpuCount;
}

void VBoxVirtualMachine::setCpuCount(int cpuCount, const QObject *context, const Functor<bool> &functor)
{
    Q_D(VBoxVirtualMachine);

    const QPointer<const QObject> context_{context};
    VirtualBoxManager::setCpuCount(name(), cpuCount, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.cpuCount != cpuCount) {
            d->virtualMachineInfo.cpuCount = cpuCount;
            emit cpuCountChanged(cpuCount);
        }
        if (context_)
            functor(ok);
    });
}

int VBoxVirtualMachine::vdiCapacityMb() const
{
    Q_D(const VBoxVirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.vdiCapacityMb;
}

void VBoxVirtualMachine::setVdiCapacityMb(int vdiCapacityMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VBoxVirtualMachine);

    const QPointer<const QObject> context_{context};
    VirtualBoxManager::setVdiCapacityMb(name(), vdiCapacityMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.vdiCapacityMb != vdiCapacityMb) {
            d->virtualMachineInfo.vdiCapacityMb = vdiCapacityMb;
            emit vdiCapacityMbChanged(vdiCapacityMb);
        }
        if (context_)
            functor(ok);
    });
}

bool VBoxVirtualMachine::hasPortForwarding(quint16 hostPort, QString *ruleName) const
{
    Q_D(const VBoxVirtualMachine);
    QTC_ASSERT(d->initialized(), return false);

    for (int i = 0; i < d->portForwardingRules.size(); i++) {
        if (d->portForwardingRules[i].values().contains(hostPort)) {
            if (ruleName)
                *ruleName = d->portForwardingRules[i].key(hostPort);
            return true;
        }
    }
    return false;
}

void VBoxVirtualMachine::addPortForwarding(const QString &ruleName, const QString &protocol,
        quint16 hostPort, quint16 emulatorVmPort,
        const QObject *context, const Functor<bool> &functor)
{
    Q_D(VBoxVirtualMachine);

    const QPointer<const QObject> context_{context};
    VirtualBoxManager::updatePortForwardingRule(name(), ruleName, protocol, hostPort,
            emulatorVmPort, this, [=](bool ok) {
        if (ok) {
            QTC_ASSERT(!d->portForwardingRules.isEmpty(), return);
            // FIXME magic number
            d->portForwardingRules[0].insert(ruleName, emulatorVmPort);
            emit portForwardingChanged();
        }
        if (context_)
            functor(ok);
    });
}

void VBoxVirtualMachine::removePortForwarding(const QString &ruleName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VBoxVirtualMachine);

    const QPointer<const QObject> context_{context};
    VirtualBoxManager::deletePortForwardingRule(name(), ruleName, this, [=](bool ok) {
        if (ok) {
            QTC_ASSERT(!d->portForwardingRules.isEmpty(), return);
            d->portForwardingRules[0].remove(ruleName);
            emit portForwardingChanged();
        }
        if (context_)
            functor(ok);
    });
}

QStringList VBoxVirtualMachine::snapshots() const
{
    Q_D(const VBoxVirtualMachine);
    QTC_ASSERT(d->initialized(), return {});
    return d->virtualMachineInfo.snapshots;
}

void VBoxVirtualMachine::restoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    const QPointer<const QObject> context_{context};
    VirtualBoxManager::restoreSnapshot(name(), snapshotName, this, [=](bool restoreOk) {
        refreshConfiguration(this, [=](bool refreshOk) {
            if (context_)
                functor(restoreOk && refreshOk);
        });
    });
}

void VBoxVirtualMachine::refreshConfiguration(const QObject *context, const Functor<bool> &functor)
{
    Q_D(VBoxVirtualMachine);

    const QPointer<const QObject> context_{context};
    auto allOk = std::make_shared<bool>(true);

    VirtualBoxManager::fetchVirtualMachineInfo(name(),
            VirtualBoxManager::VdiInfo | VirtualBoxManager::SnapshotInfo, this,
            [=](const VirtualMachineInfo &info, bool ok) {
        if (!ok) {
            *allOk = false;
            return;
        }

        VirtualMachineInfo oldInfo = d->virtualMachineInfo;
        d->virtualMachineInfo = info;

        if (oldInfo.memorySizeMb != info.memorySizeMb)
            emit memorySizeMbChanged(info.memorySizeMb);
        if (oldInfo.cpuCount != info.cpuCount)
            emit cpuCountChanged(info.cpuCount);
        if (oldInfo.vdiCapacityMb != info.vdiCapacityMb)
            emit vdiCapacityMbChanged(info.vdiCapacityMb);
        if (oldInfo.snapshots != info.snapshots)
            emit snapshotsChanged();
    });

    VirtualBoxManager::fetchPortForwardingRules(name(), this,
            [=](const QList<QMap<QString, quint16>> &rules, bool ok) {
        if (!ok) {
            *allOk = false;
            if (context_)
                functor(false);
            return;
        }

        const bool changed = d->portForwardingRules != rules;
        d->portForwardingRules = rules;
        if (changed)
            emit portForwardingChanged();

        d->setInitialized();

        if (context_)
            functor(*allOk);
    });
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

    // FIXME add an external entity responsible for this
    // FIXME when changing name is disallowed, this should be invoked immediately during class construction
    q->refreshConfiguration(q, [=](bool ok) {
        QTC_CHECK(ok);
    });
}

} // namespace Sfdk
