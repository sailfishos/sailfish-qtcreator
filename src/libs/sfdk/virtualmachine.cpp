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

#include "asynchronous_p.h"
#include "sdk_p.h"
#include "sfdkconstants.h"
#include "vboxvirtualmachine_p.h"
#include "vmconnection_p.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QThread>

namespace Sfdk {

/*!
 * \class VirtualMachine
 */

VirtualMachine::ConnectionUiCreator VirtualMachine::s_connectionUiCreator;

VirtualMachine::VirtualMachine(std::unique_ptr<VirtualMachinePrivate> &&dd, const QString &type,
        const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(std::move(dd))
{
    Q_D(VirtualMachine);
    d->type = type;
    d->name = name;
    QTC_ASSERT(s_connectionUiCreator, return);
    d->connectionUi_ = s_connectionUiCreator(this);
    d->connection = std::make_unique<VmConnection>(this);
    connect(d->connection.get(), &VmConnection::stateChanged, this, &VirtualMachine::stateChanged);
    connect(d->connection.get(), &VmConnection::virtualMachineOffChanged,
            this, &VirtualMachine::virtualMachineOffChanged);
    connect(d->connection.get(), &VmConnection::lockDownFailed, this, &VirtualMachine::lockDownFailed);

    // FIXME add an external entity responsible for this initialization
    static bool once = true;
    if (once) {
        once = false;
        VBoxVirtualMachine::fetchHostTotalMemorySizeMb(QCoreApplication::instance(),
                [](int availableMemorySizeMb, bool ok) {
            QTC_ASSERT(ok, return);
            VirtualMachinePrivate::s_availableMemmorySizeMb = availableMemorySizeMb;
        });
    }
}

VirtualMachine::~VirtualMachine()
{
}

QUrl VirtualMachine::uri() const
{
    return VirtualMachineFactory::makeUri(d_func()->type, d_func()->name);
}

QString VirtualMachine::type() const
{
    return d_func()->type;
}

QString VirtualMachine::displayType() const
{
    return d_func()->displayType;
}

QString VirtualMachine::name() const
{
    return d_func()->name;
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

int VirtualMachine::memorySizeMb() const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.memorySizeMb;
}

void VirtualMachine::setMemorySizeMb(int memorySizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};
    d->doSetMemorySizeMb(memorySizeMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.memorySizeMb != memorySizeMb) {
            d->virtualMachineInfo.memorySizeMb = memorySizeMb;
            emit memorySizeMbChanged(memorySizeMb);
        }
        if (context_)
            functor(ok);
    });
}

int VirtualMachine::availableMemorySizeMb()
{
    QTC_CHECK(VirtualMachinePrivate::s_availableMemmorySizeMb > 0);
    return VirtualMachinePrivate::s_availableMemmorySizeMb;
}

int VirtualMachine::cpuCount() const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.cpuCount;
}

void VirtualMachine::setCpuCount(int cpuCount, const QObject *context, const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};
    d->doSetCpuCount(cpuCount, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.cpuCount != cpuCount) {
            d->virtualMachineInfo.cpuCount = cpuCount;
            emit cpuCountChanged(cpuCount);
        }
        if (context_)
            functor(ok);
    });
}

int VirtualMachine::availableCpuCount()
{
    return QThread::idealThreadCount();
}

int VirtualMachine::vdiCapacityMb() const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.vdiCapacityMb;
}

void VirtualMachine::setVdiCapacityMb(int vdiCapacityMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};
    d->doSetVdiCapacityMb(vdiCapacityMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.vdiCapacityMb != vdiCapacityMb) {
            d->virtualMachineInfo.vdiCapacityMb = vdiCapacityMb;
            emit vdiCapacityMbChanged(vdiCapacityMb);
        }
        if (context_)
            functor(ok);
    });
}

bool VirtualMachine::hasPortForwarding(quint16 hostPort, QString *ruleName) const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return false);

    const QList<const QMap<QString, quint16> *> ruleset{
        &d->virtualMachineInfo.otherPorts,
        &d->virtualMachineInfo.qmlLivePorts,
        &d->virtualMachineInfo.freePorts
    };
    for (auto *const rules : ruleset) {
        if (rules->values().contains(hostPort)) {
            if (ruleName)
                *ruleName = rules->key(hostPort);
            return true;
        }
    }
    return false;
}

void VirtualMachine::addPortForwarding(const QString &ruleName, const QString &protocol,
        quint16 hostPort, quint16 emulatorVmPort,
        const QObject *context, const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};
    d->doAddPortForwarding(ruleName, protocol, hostPort,
            emulatorVmPort, this, [=](bool ok) {
        if (ok) {
            d->virtualMachineInfo.otherPorts.insert(ruleName, emulatorVmPort);
            emit portForwardingChanged();
        }
        if (context_)
            functor(ok);
    });
}

void VirtualMachine::removePortForwarding(const QString &ruleName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};
    d->doRemovePortForwarding(ruleName, this, [=](bool ok) {
        if (ok) {
            d->virtualMachineInfo.otherPorts.remove(ruleName);
            emit portForwardingChanged();
        }
        if (context_)
            functor(ok);
    });
}

QStringList VirtualMachine::snapshots() const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return {});
    return d->virtualMachineInfo.snapshots;
}

void VirtualMachine::restoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};
    d->doRestoreSnapshot(snapshotName, this, [=](bool restoreOk) {
        refreshConfiguration(this, [=](bool refreshOk) {
            if (context_)
                functor(restoreOk && refreshOk);
        });
    });
}

void VirtualMachine::refreshConfiguration(const QObject *context, const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};

    d->fetchInfo(VirtualMachineInfo::VdiInfo | VirtualMachineInfo::SnapshotInfo, this,
            [=](const VirtualMachineInfo &info, bool ok) {
        if (!ok) {
            if (context_)
                functor(false);
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
        if (oldInfo.otherPorts != info.otherPorts
                || oldInfo.qmlLivePorts != info.qmlLivePorts
                || oldInfo.freePorts != info.freePorts) {
            emit portForwardingChanged();
        }

        d->initialized_ = true;

        if (context_)
            functor(true);
    });
}

void VirtualMachine::refreshState(Sfdk::VirtualMachine::Synchronization synchronization)
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

int VirtualMachinePrivate::s_availableMemmorySizeMb = 0;

void VirtualMachinePrivate::setSharedPath(SharedPath which, const Utils::FileName &path,
        const QObject *context, const Functor<bool> &functor)
{
    const QPointer<const QObject> context_{context};
    doSetSharedPath(which, path, q_func(), [=](bool ok) {
        if (!ok) {
            if (context_)
                functor(false);
        }
        // FIXME Currently there is no user of these cached values - the other classes do
        // fetchInfo unnecessary
        switch (which) {
        case SharedConfig:
            virtualMachineInfo.sharedConfig = path.toString();
            break;
        case SharedSsh:
            virtualMachineInfo.sharedSsh = path.toString();
            break;
        case SharedHome:
            virtualMachineInfo.sharedHome = path.toString();
            break;
        case SharedTargets:
            virtualMachineInfo.sharedTargets = path.toString();
            break;
        case SharedSrc:
            virtualMachineInfo.sharedSrc = path.toString();
            break;
        }
        if (context_)
            functor(true);
    });
}

void VirtualMachinePrivate::setReservedPortForwarding(ReservedPort which, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    const QPointer<const QObject> context_{context};
    doSetReservedPortForwarding(which, port, q_func(), [=](bool ok) {
        if (!ok) {
            if (context_)
                functor(false);
        }
        // FIXME Currently there is no user of these cached values - the other classes do
        // fetchInfo unnecessary
        switch (which) {
        case SshPort:
            virtualMachineInfo.sshPort = port;
            break;
        case WwwPort:
            virtualMachineInfo.wwwPort = port;
            break;
        }
        if (context_)
            functor(true);
    });
}

void VirtualMachinePrivate::setReservedPortListForwarding(ReservedPortList which,
        const QList<Utils::Port> &ports, const QObject *context,
        const Functor<const QMap<QString, quint16> &, bool> &functor)
{
    const QPointer<const QObject> context_{context};
    doSetReservedPortListForwarding(which, ports, q_func(),
            [=](const QMap<QString, quint16> &savedPorts, bool ok) {
        if (!ok) {
            if (context_)
                functor({}, false);
        }
        // FIXME Currently there is no user of these cached values - the other classes do
        // fetchInfo unnecessary
        switch (which) {
        case FreePortList:
            virtualMachineInfo.freePorts = savedPorts;
            break;
        case QmlLivePortList:
            virtualMachineInfo.qmlLivePorts = savedPorts;
            break;
        }
        if (context_)
            functor(savedPorts, true);
    });

}

/*!
 * \class VirtualMachineFactory
 * \internal
 */

VirtualMachineFactory *VirtualMachineFactory::s_instance = nullptr;

VirtualMachineFactory::VirtualMachineFactory(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

VirtualMachineFactory::~VirtualMachineFactory()
{
    s_instance = nullptr;
}

void VirtualMachineFactory::unusedVirtualMachines(const QObject *context,
        const Functor<const QList<VirtualMachineDescriptor> &, bool> &functor)
{
    const QPointer<const QObject> context_{context};
    auto descriptors = std::make_shared<QList<VirtualMachineDescriptor>>();
    auto allOk = std::make_shared<bool>(true);

    for (const Meta &meta : s_instance->m_metas) {
        meta.fetchRegisteredVirtualMachines(s_instance,
                [=](const QStringList &registeredVms, bool ok) {
            if (!ok) {
                *allOk = false;
                return;
            }
            for (const QString &name : registeredVms) {
                const QUrl uri = makeUri(meta.type, name);
                if (s_instance->m_used.contains(uri))
                    continue;
                VirtualMachineDescriptor descriptor;
                descriptor.uri = uri;
                descriptor.displayType = meta.displayType;
                descriptor.name = name;
                descriptors->append(descriptor);
            }
        });
    }

    SdkPrivate::commandQueue()->enqueueCheckPoint(s_instance, [=]() {
        if (!context_)
            return;
        if (allOk)
            functor(*descriptors, true);
        else
            functor({}, false);
    });
}

std::unique_ptr<VirtualMachine> VirtualMachineFactory::create(const QUrl &uri)
{
    qCDebug(vms) << "Creating VM" << uri.toString();

    const QString type = typeFromUri(uri);
    const QString name = nameFromUri(uri);

    const Meta meta = Utils::findOrDefault(s_instance->m_metas, Utils::equal(&Meta::type, type));
    if (meta.isNull()) {
        qCritical(lib) << "Unrecognized virtual machine type" << type;
        return {};
    }

    if (++s_instance->m_used[uri] != 1) {
        qCWarning(lib) << "VirtualMachine: Another instance for VM" << uri.toString()
            << "already exists";
    }

    std::unique_ptr<VirtualMachine> vm = meta.create(name);

    // FIXME add an external entity responsible for this
    vm->refreshConfiguration(vm.get(), [=](bool ok) {
        QTC_CHECK(ok);
    });

    connect(vm.get(), &QObject::destroyed, s_instance, [=]() {
        if (--s_instance->m_used[uri] == 0)
            s_instance->m_used.remove(uri);
    });

    return std::move(vm);
}

QUrl VirtualMachineFactory::makeUri(const QString &type, const QString &name)
{
    QUrl uri;
    uri.setScheme(Constants::VIRTUAL_MACHINE_URI_SCHEME);
    uri.setPath(type);
    uri.setFragment(name);
    return uri;
}

QString VirtualMachineFactory::typeFromUri(const QUrl &uri)
{
    QTC_ASSERT(uri.isValid(), return {});
    QTC_ASSERT(uri.scheme() == Constants::VIRTUAL_MACHINE_URI_SCHEME, return {});
    QTC_CHECK(!uri.path().isEmpty());
    return uri.path();
}

QString VirtualMachineFactory::nameFromUri(const QUrl &uri)
{
    QTC_ASSERT(uri.isValid(), return {});
    QTC_ASSERT(uri.scheme() == Constants::VIRTUAL_MACHINE_URI_SCHEME, return {});
    QTC_CHECK(!uri.fragment().isEmpty());
    return uri.fragment();
}

} // namespace Sfdk
