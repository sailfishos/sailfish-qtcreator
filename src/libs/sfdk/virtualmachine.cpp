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
#include <utils/port.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace Utils;

namespace Sfdk {

namespace {
const char VM_INFO_CACHE_FILE[] = "vminfo.xml";
const char VM_INFO_CACHE_DOC_TYPE[] = "SfdkVmInfo";
const char VM_INFO_MAP[] = "VmInfos";

const char VM_INFO_SHARED_HOME[] = "SharedHome";
const char VM_INFO_SHARED_TARGETS[] = "SharedTargets";
const char VM_INFO_SHARED_CONFIG[] = "SharedConfig";
const char VM_INFO_SHARED_SRC[] = "SharedSrc";
const char VM_INFO_SHARED_SSH[] = "SharedSsh";
const char VM_INFO_SSH_PORT[] = "SshPort";
const char VM_INFO_WWW_PORT[] = "WwwPort";
const char VM_INFO_FREE_PORTS[] = "FreePorts";
const char VM_INFO_QML_LIVE_PORTS[] = "QmlLivePorts";
const char VM_INFO_OTHER_PORTS[] = "OtherPorts";
const char VM_INFO_MACS[] = "Macs";
const char VM_INFO_HEADLESS[] = "Headless";
const char VM_INFO_MEMORY_SIZE_MB[] = "MemorySizeMb";
const char VM_INFO_CPU_COUNT[] = "CpuCount";
const char VM_INFO_VDI_CAPACITY_MB[] = "VdiCapacityMb";
const char VM_INFO_SNAPSHOTS[] = "Snapshots";

class VirtualMachineInfoCache
{
public:
    static Utils::optional<VirtualMachineInfo> info(const QUrl &vmUri);
    static void insert(const QUrl &vmUri, const VirtualMachineInfo &info);

private:
    static Utils::FileName cacheFileName();
    static QString lockFileName(const Utils::FileName &file);
    static QString docType();
};

} // namespace anonymous

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

bool VirtualMachine::isLockedDown() const
{
    return d_func()->connection->isLockedDown();
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
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doSetMemorySizeMb(memorySizeMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.memorySizeMb != memorySizeMb) {
            d->virtualMachineInfo.memorySizeMb = memorySizeMb;
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
            emit memorySizeMbChanged(memorySizeMb);
        }
        if (context_)
            functor(ok);
    });
}

int VirtualMachine::availableMemorySizeMb()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return static_cast<int>(status.ullTotalPhys >> 20);
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return static_cast<int>((pages * page_size) >> 20);
#endif
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
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doSetCpuCount(cpuCount, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.cpuCount != cpuCount) {
            d->virtualMachineInfo.cpuCount = cpuCount;
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
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
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doSetVdiCapacityMb(vdiCapacityMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.vdiCapacityMb != vdiCapacityMb) {
            d->virtualMachineInfo.vdiCapacityMb = vdiCapacityMb;
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
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
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doAddPortForwarding(ruleName, protocol, hostPort,
            emulatorVmPort, this, [=](bool ok) {
        if (ok) {
            d->virtualMachineInfo.otherPorts.insert(ruleName, emulatorVmPort);
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
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
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doRemovePortForwarding(ruleName, this, [=](bool ok) {
        if (ok) {
            d->virtualMachineInfo.otherPorts.remove(ruleName);
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
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

void VirtualMachine::refreshConfiguration(const QObject *context, const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    const QPointer<const QObject> context_{context};

    auto refresh = [=](const VirtualMachineInfo &info, bool ok) {
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
    };

    if (SdkPrivate::useCachedVmInfo()) {
        Utils::optional<VirtualMachineInfo> info = VirtualMachineInfoCache::info(uri());
        if (info) {
            SdkPrivate::commandQueue()->enqueueCheckPoint(this, [=]() {
                refresh(*info, true);
            });
            return;
        }
    }

    d->fetchInfo(VirtualMachineInfo::VdiInfo | VirtualMachineInfo::SnapshotInfo, this,
            [=](const VirtualMachineInfo &info, bool ok) {
        if (ok)
            VirtualMachineInfoCache::insert(uri(), info);
        refresh(info, ok);
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

void VirtualMachinePrivate::setSharedPath(SharedPath which, const Utils::FileName &path,
        const QObject *context, const Functor<bool> &functor)
{
    const QPointer<const QObject> context_{context};
    doSetSharedPath(which, path, q_func(), [=](bool ok) {
        Q_Q(VirtualMachine);
        if (!ok) {
            if (context_)
                functor(false);
        }
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
        VirtualMachineInfoCache::insert(q->uri(), virtualMachineInfo);
        if (context_)
            functor(true);
    });
}

void VirtualMachinePrivate::setReservedPortForwarding(ReservedPort which, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    const QPointer<const QObject> context_{context};
    doSetReservedPortForwarding(which, port, q_func(), [=](bool ok) {
        Q_Q(VirtualMachine);
        if (!ok) {
            if (context_)
                functor(false);
        }
        switch (which) {
        case SshPort:
            virtualMachineInfo.sshPort = port;
            break;
        case WwwPort:
            virtualMachineInfo.wwwPort = port;
            break;
        }
        VirtualMachineInfoCache::insert(q->uri(), virtualMachineInfo);
        if (context_)
            functor(true);
    });
}

void VirtualMachinePrivate::setReservedPortListForwarding(ReservedPortList which,
        const QList<Utils::Port> &ports, const QObject *context,
        const Functor<const QList<Utils::Port> &, bool> &functor)
{
    const QPointer<const QObject> context_{context};
    doSetReservedPortListForwarding(which, ports, q_func(),
            [=](const QMap<QString, quint16> &savedPorts, bool ok) {
        Q_Q(VirtualMachine);
        if (!ok) {
            if (context_)
                functor({}, false);
        }
        switch (which) {
        case FreePortList:
            virtualMachineInfo.freePorts = savedPorts;
            break;
        case QmlLivePortList:
            virtualMachineInfo.qmlLivePorts = savedPorts;
            break;
        }
        VirtualMachineInfoCache::insert(q->uri(), virtualMachineInfo);
        auto toPorts = [](const QList<quint16> &numbers) {
            return Utils::transform(numbers, [](quint16 number) { return Port(number); });
        };
        if (context_)
            functor(toPorts(savedPorts.values()), true);
    });

}

void VirtualMachinePrivate::restoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VirtualMachine);
    QTC_CHECK(q->isLockedDown());

    const QPointer<const QObject> context_{context};
    doRestoreSnapshot(snapshotName, q, [=](bool restoreOk) {
        Q_Q(VirtualMachine);
        q->refreshConfiguration(q, [=](bool refreshOk) {
            if (context_)
                functor(restoreOk && refreshOk);
        });
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

    connect(vm.get(), &QObject::destroyed, s_instance, [=]() {
        if (--s_instance->m_used[uri] == 0)
            s_instance->m_used.remove(uri);
    });

#ifdef Q_OS_MACOS
    return std::move(vm);
#else
    return vm;
#endif
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

/*!
 * \class VirtualMachineInfoCache
 * \internal
 */

Utils::optional<VirtualMachineInfo> VirtualMachineInfoCache::info(const QUrl &vmUri)
{
    const FileName fileName = cacheFileName();
    if (!fileName.exists())
        return {};

    QLockFile lockFile(lockFileName(fileName));
    if (!lockFile.lock()) {
        qCWarning(vms) << "Failed to acquire access to VM info cache file:" << lockFile.error();
        return {};
    }

    if (!FileUtils::isFileNewerThan(fileName, SdkPrivate::lastMaintained())) {
        qCDebug(vms) << "Dropping possibly outdated VM info cache";
        QFile::remove(fileName.toString());
        return {};
    }

    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return {};
    QVariantMap data = reader.restoreValues();
    QVariantMap vmInfos = data.value(VM_INFO_MAP).toMap();

    QVariantMap vmData = vmInfos.value(vmUri.toString()).toMap();
    if (vmData.isEmpty())
        return {};

    Utils::optional<VirtualMachineInfo> info(Utils::in_place);
    info->fromMap(vmData);

    qCDebug(vms) << "Using cached VM info for" << vmUri.toString();
    return info;
}

void VirtualMachineInfoCache::insert(const QUrl &vmUri, const VirtualMachineInfo &info)
{
    const FileName fileName = cacheFileName();

    const bool mkpathOk = QDir(fileName.parentDir().toString()).mkpath(".");
    QTC_CHECK(mkpathOk);

    QLockFile lockFile(lockFileName(fileName));
    if (!lockFile.lock()) {
        qCWarning(vms) << "Failed to acquire access to VM info cache file:" << lockFile.error();
        return;
    }

    QVariantMap data;

    if (fileName.exists()) {
        PersistentSettingsReader reader;
        if (!reader.load(fileName))
            return;
        data = reader.restoreValues();
    }

    QVariantMap vmInfos = data.value(VM_INFO_MAP).toMap();

    const QVariantMap oldInfo = vmInfos.value(vmUri.toString()).toMap();
    const QVariantMap info_ = info.toMap();
    if (oldInfo == info_) {
        qCDebug(vms) << "Already caching the same VM info. Not updating VM info cache";
        return;
    } else {
        qCDebug(vms) << "Updating VM info cache";
    }

    vmInfos.insert(vmUri.toString(), info_);
    data.insert(VM_INFO_MAP, vmInfos);

    PersistentSettingsWriter writer(fileName, docType());
    QString errorString;
    if (!writer.save(data, &errorString))
        qCWarning(vms) << "Failed to write VM info cache:" << errorString;
}

Utils::FileName VirtualMachineInfoCache::cacheFileName()
{
    return SdkPrivate::cacheFile(VM_INFO_CACHE_FILE);
}

QString VirtualMachineInfoCache::lockFileName(const Utils::FileName &file)
{
    const QString dotFile = file.parentDir().appendPath("." + file.fileName()).toString();
    return dotFile + ".lock";
}

QString VirtualMachineInfoCache::docType()
{
    return {VM_INFO_CACHE_DOC_TYPE};
}

/*!
 * \class VirtualMachineInfo
 * \internal
 */

void VirtualMachineInfo::fromMap(const QVariantMap &data)
{
    sharedHome = data.value(VM_INFO_SHARED_HOME).toString();
    sharedTargets = data.value(VM_INFO_SHARED_TARGETS).toString();
    sharedConfig = data.value(VM_INFO_SHARED_CONFIG).toString();
    sharedSrc = data.value(VM_INFO_SHARED_SRC).toString();
    sharedSsh = data.value(VM_INFO_SHARED_SSH).toString();
    sshPort = data.value(VM_INFO_SSH_PORT).toUInt();
    wwwPort = data.value(VM_INFO_WWW_PORT).toUInt();

    auto portsFromMap = [](QMap<QString, quint16> *ports, const QVariantMap &portsData) {
        ports->clear();
        for (auto it = portsData.cbegin(); it != portsData.cend(); ++it)
            ports->insert(it.key(), it.value().toUInt());
    };
    portsFromMap(&freePorts, data.value(VM_INFO_FREE_PORTS).toMap());
    portsFromMap(&qmlLivePorts, data.value(VM_INFO_QML_LIVE_PORTS).toMap());
    portsFromMap(&otherPorts, data.value(VM_INFO_OTHER_PORTS).toMap());

    macs = data.value(VM_INFO_MACS).toStringList();
    headless = data.value(VM_INFO_HEADLESS).toBool();
    memorySizeMb = data.value(VM_INFO_MEMORY_SIZE_MB).toInt();
    cpuCount = data.value(VM_INFO_CPU_COUNT).toInt();
    vdiCapacityMb = data.value(VM_INFO_VDI_CAPACITY_MB).toInt();
    snapshots = data.value(VM_INFO_SNAPSHOTS).toStringList();
}

QVariantMap VirtualMachineInfo::toMap() const
{
    QVariantMap data;

    data.insert(VM_INFO_SHARED_HOME, sharedHome);
    data.insert(VM_INFO_SHARED_TARGETS, sharedTargets);
    data.insert(VM_INFO_SHARED_CONFIG, sharedConfig);
    data.insert(VM_INFO_SHARED_SRC, sharedSrc);
    data.insert(VM_INFO_SHARED_SSH, sharedSsh);
    data.insert(VM_INFO_SSH_PORT, sshPort);
    data.insert(VM_INFO_WWW_PORT, wwwPort);

    auto portsToMap = [](const QMap<QString, quint16> &ports) {
        QVariantMap data;
        for (auto it = ports.cbegin(); it != ports.cend(); ++it)
            data.insert(it.key(), it.value());
        return data;
    };
    data.insert(VM_INFO_FREE_PORTS, portsToMap(freePorts));
    data.insert(VM_INFO_QML_LIVE_PORTS, portsToMap(qmlLivePorts));
    data.insert(VM_INFO_OTHER_PORTS, portsToMap(otherPorts));

    data.insert(VM_INFO_MACS, macs);
    data.insert(VM_INFO_HEADLESS, headless);
    data.insert(VM_INFO_MEMORY_SIZE_MB, memorySizeMb);
    data.insert(VM_INFO_CPU_COUNT, cpuCount);
    data.insert(VM_INFO_VDI_CAPACITY_MB, vdiCapacityMb);
    data.insert(VM_INFO_SNAPSHOTS, snapshots);

    return data;
}

} // namespace Sfdk
