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
#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/port.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <QTimer>
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
const int  VM_INFO_CACHE_UPDATE_DELAY_MS = 2000;
const char VM_INFO_MAP[] = "VmInfos";

const char VM_INFO_SHARED_INSTALL[] = "SharedInstall";
const char VM_INFO_SHARED_HOME[] = "SharedHome";
const char VM_INFO_SHARED_TARGETS[] = "SharedTargets";
const char VM_INFO_SHARED_CONFIG[] = "SharedConfig";
const char VM_INFO_SHARED_MEDIA[] = "SharedMedia";
const char VM_INFO_SHARED_SRC[] = "SharedSrc";
const char VM_INFO_SHARED_SSH[] = "SharedSsh";
const char VM_INFO_SSH_PORT[] = "SshPort";
const char VM_INFO_WWW_PORT[] = "WwwPort";
const char VM_INFO_DBUS_PORT[] = "DBusPort";
const char VM_INFO_FREE_PORTS[] = "FreePorts";
const char VM_INFO_QML_LIVE_PORTS[] = "QmlLivePorts";
const char VM_INFO_OTHER_PORTS[] = "OtherPorts";
const char VM_INFO_MACS[] = "Macs";
const char VM_INFO_HEADLESS[] = "Headless";
const char VM_INFO_MEMORY_SIZE_MB[] = "MemorySizeMb";
const char VM_INFO_SWAP_SUPPORTED[] = "SwapSupported";
const char VM_INFO_SWAP_SIZE_MB[] = "SwapSizeMb";
const char VM_INFO_CPU_COUNT[] = "CpuCount";
const char VM_INFO_STORAGE_SIZE_MB[] = "StorageSizeMb";
const char VM_INFO_SNAPSHOTS[] = "Snapshots";

} // namespace anonymous

class VirtualMachineInfoCache : public QObject
{
    Q_OBJECT

public:
    VirtualMachineInfoCache(QObject *parent);
    ~VirtualMachineInfoCache() override;

    static VirtualMachineInfoCache *instance() { return s_instance; }

    static Utils::optional<VirtualMachineInfo> info(const QUrl &vmUri);
    static void insert(const QUrl &vmUri, const VirtualMachineInfo &info);

signals:
    void updated();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void enableUpdates();
    static Utils::FilePath cacheFilePath();
    static QString lockFilePath(const Utils::FilePath &file);
    static QString docType();

private:
    static VirtualMachineInfoCache *s_instance;
    std::unique_ptr<FileSystemWatcher> m_watcher;
    QBasicTimer m_updateTimer;
};

/*!
 * \class VirtualMachine
 */

VirtualMachine::ConnectionUiCreator VirtualMachine::s_connectionUiCreator;

VirtualMachine::VirtualMachine(std::unique_ptr<VirtualMachinePrivate> &&dd, const QString &type,
        const Features &features, const QString &name, QObject *parent)
    : QObject(parent)
    , d_ptr(std::move(dd))
{
    Q_D(VirtualMachine);
    d->setParent(this);
    d->type = type;
    d->features = features;
    d->name = name;
    QTC_ASSERT(s_connectionUiCreator, return);
    d->connectionUi_ = s_connectionUiCreator(this);
    d->connection = std::make_unique<VmConnection>(this);
    connect(d->connection.get(), &VmConnection::stateChanged, this, &VirtualMachine::stateChanged);
    connect(d->connection.get(), &VmConnection::stateChangePendingChanged,
            this, &VirtualMachine::stateChangePendingChanged);
    connect(d->connection.get(), &VmConnection::virtualMachineOffChanged,
            this, &VirtualMachine::virtualMachineOffChanged);
    connect(d->connection.get(), &VmConnection::lockDownFailed, this, &VirtualMachine::lockDownFailed);
    connect(d->connection.get(), &VmConnection::initGuest, d, &VirtualMachinePrivate::doInitGuest);

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        if (SdkPrivate::isUpdatesEnabled()) {
            d->enableUpdates();
        } else {
            connect(SdkPrivate::instance(), &SdkPrivate::enableUpdatesRequested,
                    this, [=]() { d->enableUpdates(); });
        }
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

VirtualMachine::Features VirtualMachine::features() const
{
    return d_func()->features;
}

QString VirtualMachine::name() const
{
    return d_func()->name;
}

VirtualMachine::State VirtualMachine::state() const
{
    return d_func()->connection->state();
}

bool VirtualMachine::isStateChangePending() const
{
    return d_func()->connection->isStateChangePending();
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
    QTC_ASSERT(d->features & OptionalHeadless, return);
    if (d->headless == headless)
        return;
    d->headless = headless;
    emit headlessChanged(headless);
}

bool VirtualMachine::isAutoConnectEnabled() const
{
    return d_func()->autoConnectEnabled;
}

bool VirtualMachine::setAutoConnectEnabled(bool autoConnectEnabled)
{
    Q_D(VirtualMachine);
    QTC_ASSERT(!autoConnectEnabled
            || !SdkPrivate::isVersionedSettingsEnabled()
            || SdkPrivate::isUpdatesEnabled(), return d->autoConnectEnabled);

    if (d->autoConnectEnabled == autoConnectEnabled)
        return d->autoConnectEnabled;

    const bool old = d->autoConnectEnabled;
    d->autoConnectEnabled = autoConnectEnabled;
    emit autoConnectEnabledChanged(autoConnectEnabled);
    return old;
}

bool VirtualMachine::isOff(bool *runningHeadless, bool *startedOutside) const
{
    return d_func()->connection->isVirtualMachineOff(runningHeadless, startedOutside);
}

void VirtualMachine::lockDown(bool lockDown, const QObject *context, const Functor<bool> &functor)
{
    if (lockDown)
        d_func()->connection->lockDown(context, functor);
    else
        d_func()->connection->release(context, functor);
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
    QTC_ASSERT(d->features & LimitMemorySize, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
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

int VirtualMachine::swapSizeMb() const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.swapSizeMb;
}

void VirtualMachine::setSwapSizeMb(int swapSizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);
    QTC_ASSERT(d->features & SwapMemory, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doSetSwapSizeMb(swapSizeMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.swapSizeMb != swapSizeMb) {
            d->virtualMachineInfo.swapSizeMb = swapSizeMb;
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
            emit swapSizeMbChanged(swapSizeMb);
        }
        if (context_)
            functor(ok);
    });
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
    QTC_ASSERT(d->features & LimitCpuCount, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
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

int VirtualMachine::storageSizeMb() const
{
    Q_D(const VirtualMachine);
    QTC_ASSERT(d->initialized(), return false);
    return d->virtualMachineInfo.storageSizeMb;
}

void VirtualMachine::setStorageSizeMb(int storageSizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);
    QTC_ASSERT(d->features & (GrowStorageSize | ShrinkStorageSize), {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_ASSERT(!(storageSizeMb > d->virtualMachineInfo.storageSizeMb) || d->features & GrowStorageSize, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_ASSERT(!(storageSizeMb < d->virtualMachineInfo.storageSizeMb) || d->features & ShrinkStorageSize, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doSetStorageSizeMb(storageSizeMb, this, [=](bool ok) {
        if (ok && d->virtualMachineInfo.storageSizeMb != storageSizeMb) {
            d->virtualMachineInfo.storageSizeMb = storageSizeMb;
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
            emit storageSizeMbChanged(storageSizeMb);
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
    QTC_ASSERT(d->features & Snapshots, return {});
    QTC_ASSERT(d->initialized(), return {});
    return d->virtualMachineInfo.snapshots;
}

void VirtualMachine::takeSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);
    QTC_ASSERT(d->features & Snapshots, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doTakeSnapshot(snapshotName, this, [=](bool ok) {
        if (ok) {
            d->virtualMachineInfo.snapshots.append(snapshotName);
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
            emit snapshotsChanged();
        }
        if (context_)
            functor(ok);
    });
}

void VirtualMachine::restoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);
    QTC_ASSERT(d->features & Snapshots, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_CHECK(isLockedDown());

    BatchComposer composer = BatchComposer::createBatch("VirtualMachine::restoreSnapshot");

    auto allOk = std::make_shared<bool>(true);

    d->doRestoreSnapshot(snapshotName, this, [=](bool restoreOk) {
        QTC_CHECK(restoreOk);
        *allOk &= restoreOk;
    });

    refreshConfiguration(this, [=](bool refreshOk) {
        QTC_CHECK(refreshOk);
        *allOk &= refreshOk;
    });

    emit d->aboutToRestoreSnapshot(allOk);

    BatchComposer::enqueueCheckPoint(context, [=]() { functor(*allOk); });
}

void VirtualMachine::removeSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_D(VirtualMachine);
    QTC_ASSERT(d->features & Snapshots, {
        BatchComposer::enqueueCheckPoint(context, std::bind(functor, false));
        return;
    });
    QTC_CHECK(isLockedDown());

    const QPointer<const QObject> context_{context};
    d->doRemoveSnapshot(snapshotName, this, [=](bool ok) {
        if (ok) {
            d->virtualMachineInfo.snapshots.removeAll(snapshotName);
            VirtualMachineInfoCache::insert(uri(), d->virtualMachineInfo);
            emit snapshotsChanged();
        }
        if (context_)
            functor(ok);
    });
}

void VirtualMachine::refreshConfiguration(const QObject *context, const Functor<bool> &functor)
{
    Q_D(VirtualMachine);

    BatchComposer composer = BatchComposer::createBatch("VirtualMachine::refreshConfiguration");

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
        if (oldInfo.swapSizeMb != info.swapSizeMb)
            emit swapSizeMbChanged(info.swapSizeMb);
        if (oldInfo.cpuCount != info.cpuCount)
            emit cpuCountChanged(info.cpuCount);
        if (oldInfo.storageSizeMb != info.storageSizeMb)
            emit storageSizeMbChanged(info.storageSizeMb);
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
            BatchComposer::enqueueCheckPoint(this, [=]() {
                refresh(*info, true);
            });
            return;
        }
    }

    d->fetchInfo(VirtualMachineInfo::StorageInfo | VirtualMachineInfo::SnapshotInfo, this,
            [=](const VirtualMachineInfo &info, bool ok) {
        if (ok)
            VirtualMachineInfoCache::insert(uri(), info);
        refresh(info, ok);
    });
}

void VirtualMachine::refreshState(const QObject *context, const Functor<bool> &functor)
{
    d_func()->connection->refresh(context, functor);
}

void VirtualMachine::connectTo(ConnectOptions options, const QObject *context,
        const Functor<bool> &functor)
{
    d_func()->connection->connectTo(options, context, functor);
}

void VirtualMachine::disconnectFrom(const QObject *context, const Functor<bool> &functor)
{
    d_func()->connection->disconnectFrom(context, functor);
}

/*!
 * \class VirtualMachinePrivate
 * \internal
 */

VirtualMachinePrivate::~VirtualMachinePrivate()
{
}

void VirtualMachinePrivate::setSharedPath(SharedPath which, const Utils::FilePath &path,
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
        case SharedInstall:
            virtualMachineInfo.sharedInstall = path.toString();
            break;
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
        case SharedMedia:
            virtualMachineInfo.sharedMedia = path.toString();
            break;
        }
        VirtualMachineInfoCache::insert(q->uri(), virtualMachineInfo);
        if (context_)
            functor(true);
    });
}

QString VirtualMachinePrivate::alignedMountPointFor(const QString &hostPath)
{
    QString path = QDir::fromNativeSeparators(hostPath);

    QTC_ASSERT(QFileInfo(path).isAbsolute(),
            path = QFileInfo(path).absoluteFilePath());

    // Here we are mostly concerned about possibly duplicated leading slashes
    QTC_ASSERT(path == QDir::cleanPath(path),
            path = QDir::cleanPath(path));

    // We do not expect paths on network shares not mapped to a drive letter
    QTC_ASSERT(!path.startsWith("//"), path = path.mid(1));

    // We cannot really do more than trying to avoid an immediate crash here
    QTC_ASSERT(path.length() >= 3, return path);

    if (HostOsInfo::isWindowsHost()) {
        // C:/Users/user -> /c/Users/user
        path[1] = path[0].toLower();
        path[0] = '/';
    }

    // Avoid name clashes
    if (path.startsWith("/home/mersdk") || path.startsWith("/home/deploy")) {
        QTC_CHECK(false);
        path[6] = path[6].toUpper();
    } else if (path == "/home") {
        QTC_CHECK(false);
        path = "/Home";
    } else if (path.startsWith("/home")) {
        ; // no clash
    } else if (path.startsWith("/users", Qt::CaseInsensitive)) {
        ; // no clash
    } else if (path[2] == '/') {
        ; // path starting with drive letter - no clash
    } else {
        path[1] = path[1].toUpper();
    }

    return path;
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
        case DBusPort:
            virtualMachineInfo.dBusPort = port;
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

void VirtualMachinePrivate::enableUpdates()
{
    Q_Q(VirtualMachine);
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);

    QObject::connect(VirtualMachineInfoCache::instance(), &VirtualMachineInfoCache::updated,
            q, [=]() {
        // Even if the cached info is not used, the cache serves as a mean to to
        // notify about changes to those VM properties that are not persisted by
        // BuildEngine or Emulator classes.
        qCDebug(vms) << "VM info cache updated. Refreshing configuration of" << q->uri().toString();

        // TODO Is that ideal?
        q->refreshConfiguration(q, [](bool ok) { QTC_CHECK(ok); });
    });
}

void VirtualMachinePrivate::doInitGuest()
{
    // Do not check for "running", it's likely it will end in "degraded" state
    const QString watcher(R"(
        while [[ $(systemctl is-system-running) == starting ]]; do
            sleep 1
        done
    )");

    BatchComposer::enqueue<RemoteProcessRunner>("wait-for-startup-completion",
            watcher, sshParameters);
}

/*!
 * \class VirtualMachineFactory
 * \internal
 */

VirtualMachineFactory *VirtualMachineFactory::s_instance = nullptr;

VirtualMachineFactory::VirtualMachineFactory(QObject *parent)
    : QObject(parent)
    , m_vmInfoCache(std::make_unique<VirtualMachineInfoCache>(this))
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
    BatchComposer composer = BatchComposer::createBatch("VirtualMachineFactory::unusedVirtualMachines");

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

    BatchComposer::enqueueCheckPoint(s_instance, [=]() {
        if (!context_)
            return;
        if (allOk)
            functor(*descriptors, true);
        else
            functor({}, false);
    });
}

std::unique_ptr<VirtualMachine> VirtualMachineFactory::create(const QUrl &uri,
        VirtualMachine::Features featureMask)
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

    std::unique_ptr<VirtualMachine> vm = meta.create(name, featureMask);

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

VirtualMachineInfoCache *VirtualMachineInfoCache::s_instance = nullptr;

VirtualMachineInfoCache::VirtualMachineInfoCache(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        connect(SdkPrivate::instance(), &SdkPrivate::enableUpdatesRequested,
                this, &VirtualMachineInfoCache::enableUpdates);
    }
}

VirtualMachineInfoCache::~VirtualMachineInfoCache()
{
    s_instance = nullptr;
}

Utils::optional<VirtualMachineInfo> VirtualMachineInfoCache::info(const QUrl &vmUri)
{
    const FilePath filePath = cacheFilePath();
    if (!filePath.exists())
        return {};

    QLockFile lockFile(lockFilePath(filePath));
    if (!lockFile.lock()) {
        qCWarning(vms) << "Failed to acquire access to VM info cache file:" << lockFile.error();
        return {};
    }

    if (!filePath.isNewerThan(SdkPrivate::lastMaintained())) {
        qCDebug(vms) << "Dropping possibly outdated VM info cache";
        QFile::remove(filePath.toString());
        return {};
    }

    PersistentSettingsReader reader;
    if (!reader.load(filePath))
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
    const FilePath filePath = cacheFilePath();

    const bool mkpathOk = QDir(filePath.parentDir().toString()).mkpath(".");
    QTC_CHECK(mkpathOk);

    QLockFile lockFile(lockFilePath(filePath));
    if (!lockFile.lock()) {
        qCWarning(vms) << "Failed to acquire access to VM info cache file:" << lockFile.error();
        return;
    }

    QVariantMap data;

    if (filePath.exists()) {
        PersistentSettingsReader reader;
        if (!reader.load(filePath))
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

    PersistentSettingsWriter writer(filePath, docType());
    QString errorString;
    if (!writer.save(data, &errorString))
        qCWarning(vms) << "Failed to write VM info cache:" << errorString;
}

void VirtualMachineInfoCache::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_updateTimer.timerId()) {
        m_updateTimer.stop();
        emit updated();
    } else  {
        QObject::timerEvent(event);
    }
}

void VirtualMachineInfoCache::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);

    qCDebug(vms) << "Enabling receiving updates to VM info cache";

    const FilePath cacheFilePath = this->cacheFilePath();

    // FileSystemWatcher needs them existing
    const bool mkpathOk = QDir(cacheFilePath.parentDir().toString()).mkpath(".");
    QTC_CHECK(mkpathOk);
    if (!cacheFilePath.exists()) {
        const bool createFileOk = QFile(cacheFilePath.toString()).open(QIODevice::WriteOnly);
        QTC_CHECK(createFileOk);
    }

    m_watcher = std::make_unique<FileSystemWatcher>(this);
    m_watcher->addFile(cacheFilePath.toString(), FileSystemWatcher::WatchModifiedDate);
    connect(Sdk::instance(), &Sdk::aboutToShutDown, this, [=]() {
        m_watcher.reset();
    });
    connect(m_watcher.get(), &FileSystemWatcher::fileChanged, this, [this]() {
        m_updateTimer.start(VM_INFO_CACHE_UPDATE_DELAY_MS, this);
    });

    // Do not emit updated() initially - there are other triggers for the
    // initial refresh of VM properties.
}

Utils::FilePath VirtualMachineInfoCache::cacheFilePath()
{
    return SdkPrivate::cacheFile(VM_INFO_CACHE_FILE);
}

QString VirtualMachineInfoCache::lockFilePath(const Utils::FilePath &file)
{
    const QString dotFile = file.parentDir().pathAppended("." + file.fileName()).toString();
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
    sharedInstall = data.value(VM_INFO_SHARED_INSTALL).toString();
    sharedHome = data.value(VM_INFO_SHARED_HOME).toString();
    sharedTargets = data.value(VM_INFO_SHARED_TARGETS).toString();
    sharedConfig = data.value(VM_INFO_SHARED_CONFIG).toString();
    sharedMedia = data.value(VM_INFO_SHARED_MEDIA).toString();
    sharedSrc = data.value(VM_INFO_SHARED_SRC).toString();
    sharedSsh = data.value(VM_INFO_SHARED_SSH).toString();
    sshPort = data.value(VM_INFO_SSH_PORT).toUInt();
    wwwPort = data.value(VM_INFO_WWW_PORT).toUInt();
    dBusPort = data.value(VM_INFO_DBUS_PORT).toUInt();

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
    swapSizeMb = data.value(VM_INFO_SWAP_SIZE_MB).toInt();
    cpuCount = data.value(VM_INFO_CPU_COUNT).toInt();
    storageSizeMb = data.value(VM_INFO_STORAGE_SIZE_MB).toInt();
    snapshots = data.value(VM_INFO_SNAPSHOTS).toStringList();
}

QVariantMap VirtualMachineInfo::toMap() const
{
    QVariantMap data;

    data.insert(VM_INFO_SHARED_INSTALL, sharedInstall);
    data.insert(VM_INFO_SHARED_HOME, sharedHome);
    data.insert(VM_INFO_SHARED_TARGETS, sharedTargets);
    data.insert(VM_INFO_SHARED_CONFIG, sharedConfig);
    data.insert(VM_INFO_SHARED_MEDIA, sharedMedia);
    data.insert(VM_INFO_SHARED_SRC, sharedSrc);
    data.insert(VM_INFO_SHARED_SSH, sharedSsh);
    data.insert(VM_INFO_SSH_PORT, sshPort);
    data.insert(VM_INFO_WWW_PORT, wwwPort);
    data.insert(VM_INFO_DBUS_PORT, dBusPort);

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
    data.insert(VM_INFO_SWAP_SIZE_MB, swapSizeMb);
    data.insert(VM_INFO_CPU_COUNT, cpuCount);
    data.insert(VM_INFO_STORAGE_SIZE_MB, storageSizeMb);
    data.insert(VM_INFO_SNAPSHOTS, snapshots);

    return data;
}

} // namespace Sfdk

#include "virtualmachine.moc"
