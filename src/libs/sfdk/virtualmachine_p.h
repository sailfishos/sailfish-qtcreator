/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#pragma once

#include "virtualmachine.h"

#include "asynchronous.h"
#include "vmconnection_p.h"

#include <ssh/sshconnection.h>
#include <utils/optional.h>

namespace Utils {
class FileName;
class Port;
class PortList;
}

namespace Sfdk {

class VirtualMachineInfo
{
    Q_GADGET

public:
    enum ExtraInfo {
        NoExtraInfo = 0x00,
        StorageInfo = 0x01,
        SnapshotInfo = 0x02,
    };
    Q_DECLARE_FLAGS(ExtraInfos, ExtraInfo)

    void fromMap(const QVariantMap &data);
    QVariantMap toMap() const;

    QString sharedHome;
    QString sharedTargets;
    QString sharedConfig;
    QString sharedSrc;
    QString sharedSsh;
    quint16 sshPort{0};
    quint16 wwwPort{0};
    QMap<QString, quint16> freePorts;
    QMap<QString, quint16> qmlLivePorts;
    QMap<QString, quint16> otherPorts;
    QStringList macs;
    bool headless{false};
    int memorySizeMb{0};
    int cpuCount{0};

    // StorageInfo
    int storageSizeMb{0};

    // SnapshotInfo
    QStringList snapshots;
};

class VirtualMachinePrivate
{
    Q_GADGET
    Q_DECLARE_PUBLIC(VirtualMachine)

public:
    enum BasicStateFlag {
        NullState = 0x0,
        Existing = 0x1,
        Running = 0x2,
        Headless = 0x4,
    };
    Q_DECLARE_FLAGS(BasicState, BasicStateFlag)

    enum SharedPath {
        // Valid for both build engine and emulator kind of VMs
        SharedConfig,
        SharedSsh,

        // Valid for build engine kind of VMs only
        SharedHome,
        SharedTargets,
        SharedSrc,
    };
    Q_ENUM(SharedPath)

    enum ReservedPort {
        SshPort,
        // Valid for build engine kind of VMs only
        WwwPort,
    };
    Q_ENUM(ReservedPort)

    enum ReservedPortList {
        // Valid for emulator kind of VMs only
        FreePortList,
        QmlLivePortList,
    };
    Q_ENUM(ReservedPortList)

    VirtualMachinePrivate(VirtualMachine *q) : q_ptr(q) {}

    static VirtualMachinePrivate *get(VirtualMachine *q) { return q->d_func(); }

    virtual void fetchInfo(VirtualMachineInfo::ExtraInfos extraInfo, const QObject *context,
            const Functor<const VirtualMachineInfo &, bool> &functor) const = 0;
    VirtualMachineInfo cachedInfo() const { return virtualMachineInfo; }

    virtual void start(const QObject *context, const Functor<bool> &functor) = 0;
    virtual void stop(const QObject *context, const Functor<bool> &functor) = 0;
    virtual void probe(const QObject *context,
            const Functor<BasicState, bool> &functor) const = 0;

    void setSharedPath(SharedPath which, const Utils::FileName &path,
            const QObject *context, const Functor<bool> &functor);

    void setReservedPortForwarding(ReservedPort which, quint16 port,
            const QObject *context, const Functor<bool> &functor);
    void setReservedPortListForwarding(ReservedPortList which,
            const QList<Utils::Port> &ports, const QObject *context,
            const Functor<const QList<Utils::Port> &, bool> &functor);

    virtual void setVideoMode(const QSize &size, int depth, const QString &deviceModelName,
            Qt::Orientation orientation, int scaleDownFactor, const QObject *context,
            const Functor<bool> &functor) = 0;

    void restoreSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor);

    VirtualMachine::ConnectionUi *connectionUi() const { return connectionUi_.get(); }

protected:
    virtual void doSetMemorySizeMb(int memorySizeMb, const QObject *context,
            const Functor<bool> &functor) = 0;
    virtual void doSetCpuCount(int cpuCount, const QObject *context,
            const Functor<bool> &functor) = 0;
    virtual void doSetStorageSizeMb(int storageSizeMb, const QObject *context,
            const Functor<bool> &functor) = 0;

    virtual void doSetSharedPath(SharedPath which, const Utils::FileName &path,
            const QObject *context, const Functor<bool> &functor) = 0;

    virtual void doAddPortForwarding(const QString &ruleName,
        const QString &protocol, quint16 hostPort, quint16 emulatorVmPort,
        const QObject *context, const Functor<bool> &functor) = 0;
    virtual void doRemovePortForwarding(const QString &ruleName,
        const QObject *context, const Functor<bool> &functor) = 0;
    virtual void doSetReservedPortForwarding(ReservedPort which, quint16 port,
            const QObject *context, const Functor<bool> &functor) = 0;
    virtual void doSetReservedPortListForwarding(ReservedPortList which,
            const QList<Utils::Port> &ports, const QObject *context,
            const Functor<const QMap<QString, quint16> &, bool> &functor) = 0;

    virtual void doRestoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor) = 0;

    bool initialized() const { return initialized_; }
    void setDisplayType(const QString &displayType) { this->displayType = displayType; }

    VirtualMachine *const q_ptr;

private:
    void enableUpdates();

private:
    QString type;
    QString displayType;
    QString name;
    VirtualMachineInfo virtualMachineInfo;
    bool initialized_ = false;
    std::unique_ptr<VirtualMachine::ConnectionUi> connectionUi_;
    std::unique_ptr<VmConnection> connection;
    QSsh::SshConnectionParameters sshParameters;
    bool headless = false;
    bool autoConnectEnabled = true;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachineInfo::ExtraInfos)
Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachinePrivate::BasicState);

class VirtualMachineInfoCache;
class VirtualMachineFactory : public QObject
{
    class Meta
    {
// Not idea which version fixes it
#if Q_CC_MSVC && Q_CC_MSVC <= 1900
        template<typename T>
        static std::unique_ptr<VirtualMachine> creator(const QString &name)
        {
            return std::make_unique<T>(name);
        }
#endif

    public:
        Meta() {};

        template<typename T>
        Meta(T *)
            : type(T::staticType())
            , displayType(T::staticDisplayType())
            , fetchRegisteredVirtualMachines(T::fetchRegisteredVirtualMachines)
#if Q_CC_MSVC && Q_CC_MSVC <= 1900
            , create(creator<T>)
#else
            , create([](const QString &name) { return std::make_unique<T>(name); })
#endif
        {
        }

        bool isNull() const { return type.isNull(); }

        QString type;
        QString displayType;
        void (*fetchRegisteredVirtualMachines)(const QObject *,
                const Functor<const QStringList &, bool> &) = nullptr;
        std::function<std::unique_ptr<VirtualMachine>(const QString &)> create = {};
    };

public:
    explicit VirtualMachineFactory(QObject *parent = nullptr);
    ~VirtualMachineFactory() override;

    template<typename T>
    static void registerType() { s_instance->m_metas.emplace_back(static_cast<T *>(nullptr)); }

    static void unusedVirtualMachines(const QObject *context,
            const Functor<const QList<VirtualMachineDescriptor> &, bool> &functor);
    static std::unique_ptr<VirtualMachine> create(const QUrl &uri);

    // FIXME use UUID instead of name
    static QUrl makeUri(const QString &type, const QString &name);
    static QString typeFromUri(const QUrl &uri);
    static QString nameFromUri(const QUrl &uri);

private:
    static VirtualMachineFactory *s_instance;
    std::unique_ptr<VirtualMachineInfoCache> m_vmInfoCache;
    std::vector<Meta> m_metas;
    QMap<QUrl, int> m_used;
};

} // namespace Sfdk
