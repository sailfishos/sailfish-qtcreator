/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "sfdkglobal.h"
#include "asynchronous.h"

#include <QUrl>

#include <memory>

namespace QSsh {
class SshConnectionParameters;
}

namespace Sfdk {

class SFDK_EXPORT VirtualMachineDescriptor
{
public:
    QUrl uri;
    QString displayType;
    QString name;
};

class VirtualMachinePrivate;
class SFDK_EXPORT VirtualMachine : public QObject
{
    Q_OBJECT

public:
    class ConnectionUi;

    // Keep in sync with VmConnection::str()
    enum State {
        Disconnected,
        Starting,
        Connecting,
        Error,
        Connected,
        Disconnecting,
        Closing
    };
    Q_ENUM(State)

    enum ConnectOption {
        NoConnectOption = 0x00,
        AskStartVm = 0x01,
    };
    Q_DECLARE_FLAGS(ConnectOptions, ConnectOption)

    enum Feature {
        NoFeatures = 0x00,
        LimitMemorySize = 0x01,
        LimitCpuCount = 0x02,
        GrowStorageSize = 0x04,
        ShrinkStorageSize = 0x08,
        OptionalHeadless = 0x10,
        Snapshots = 0x20,
        SwapMemory = 0x40,
    };
    Q_DECLARE_FLAGS(Features, Feature)

    ~VirtualMachine() override;

    QUrl uri() const;
    QString type() const;
    QString displayType() const;
    Features features() const;
    // FIXME add qualifiedName() that would include type information?
    QString name() const;

    State state() const;
    QString errorString() const;

    QSsh::SshConnectionParameters sshParameters() const; // FIXME internal
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);

    bool isHeadless() const;
    void setHeadless(bool headless);

    bool isAutoConnectEnabled() const; // FIXME internal?
    bool setAutoConnectEnabled(bool autoConnectEnabled);

    bool isOff(bool *runningHeadless = 0, bool *startedOutside = 0) const;
    void lockDown(bool lockDown, const QObject *context, const Functor<bool> &functor);
    bool isLockedDown() const;

    int memorySizeMb() const;
    void setMemorySizeMb(int memorySizeMb, const QObject *context,
            const Functor<bool> &functor);
    static int availableMemorySizeMb();

    int swapSizeMb() const;
    void setSwapSizeMb(int swapSizeMb, const QObject *context,
            const Functor<bool> &functor);

    int cpuCount() const;
    void setCpuCount(int cpuCount, const QObject *context,
            const Functor<bool> &functor);
    static int availableCpuCount();

    int storageSizeMb() const;
    void setStorageSizeMb(int storageSizeMb, const QObject *context,
            const Functor<bool> &functor);

    bool hasPortForwarding(quint16 hostPort, QString *ruleName = nullptr) const;
    void addPortForwarding(const QString &ruleName, const QString &protocol,
            quint16 hostPort, quint16 emulatorVmPort, const QObject *context,
            const Functor<bool> &functor);
    void removePortForwarding(const QString &ruleName, const QObject *context,
            const Functor<bool> &functor);

    QStringList snapshots() const;
    void takeSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor);
    void restoreSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor);
    void removeSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor);

    void refreshConfiguration(const QObject *context, const Functor<bool> &functor);
    void refreshState(const QObject *context, const Functor<bool> &functor);

    template<typename ConcreteUi>
    static void registerConnectionUi()
    {
        Q_ASSERT(!s_connectionUiCreator);
        s_connectionUiCreator = [](VirtualMachine *parent) { return std::make_unique<ConcreteUi>(parent); };
    }

    void connectTo(ConnectOptions options, const QObject *context, const Functor<bool> &functor);
    void disconnectFrom(const QObject *context, const Functor<bool> &functor);

signals:
    void stateChanged();
    void sshParametersChanged();
    void headlessChanged(bool headless);
    void autoConnectEnabledChanged(bool autoConnectEnabled);
    void virtualMachineOffChanged(bool vmOff);
    void lockDownFailed();
    void memorySizeMbChanged(int sizeMb);
    void swapSizeMbChanged(int sizeMb);
    void cpuCountChanged(int cpuCount);
    void storageSizeMbChanged(int storageSizeMb);
    void portForwardingChanged();
    void snapshotsChanged();

protected:
    VirtualMachine(std::unique_ptr<VirtualMachinePrivate> &&dd, const QString &type,
            const Features &features, const QString &name, QObject *parent);
    std::unique_ptr<VirtualMachinePrivate> d_ptr;

private:
    Q_DISABLE_COPY(VirtualMachine)
    Q_DECLARE_PRIVATE(VirtualMachine)

    using ConnectionUiCreator = std::function<std::unique_ptr<ConnectionUi>(VirtualMachine *)>;
    static ConnectionUiCreator s_connectionUiCreator;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachine::ConnectOptions)
Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachine::Features)

class SFDK_EXPORT VirtualMachine::ConnectionUi : public QObject
{
    Q_OBJECT

public:
    enum Warning {
        UnableToCloseVm,
        VmNotRegistered,
        SshPortOccupied,
    };

    enum Question {
        StartVm,
        ResetVm,
        CloseVm,
        CancelConnecting,
        CancelLockingDown,
    };

    enum QuestionStatus {
        NotAsked,
        Asked,
        Yes,
        No,
    };

    using QObject::QObject;

    virtual void warn(Warning which) = 0;
    virtual void dismissWarning(Warning which) = 0;

    virtual bool shouldAsk(Question which) const = 0;
    virtual void ask(Question which, std::function<void()> onStatusChanged) = 0;
    virtual void dismissQuestion(Question which) = 0;
    virtual QuestionStatus status(Question which) const = 0;

protected:
    VirtualMachine *virtualMachine() const { return static_cast<VirtualMachine *>(parent()); }
};

} // namespace Sfdk
