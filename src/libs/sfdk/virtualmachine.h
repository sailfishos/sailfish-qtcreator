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

#include <memory>

namespace QSsh {
class SshConnectionParameters;
}

namespace Sfdk {

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
        Block = 0x02,
    };
    Q_DECLARE_FLAGS(ConnectOptions, ConnectOption)

    enum Synchronization {
        Asynchronous,
        Synchronous
    };

    ~VirtualMachine() override;

    QString name() const; // FIXME uuid
    void setName(const QString &name); // FIXME disallow

    State state() const;
    QString errorString() const;

    QSsh::SshConnectionParameters sshParameters() const; // FIXME internal
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);

    bool isHeadless() const;
    void setHeadless(bool headless);

    bool isAutoConnectEnabled() const; // FIXME internal?
    void setAutoConnectEnabled(bool autoConnectEnabled);

    bool isOff(bool *runningHeadless = 0, bool *startedOutside = 0) const;
    bool lockDown(bool lockDown);

    virtual void hasPortForwarding(quint16 hostPort, const QObject *context,
            const Functor<bool, const QString &, bool> &functor) const = 0;
    virtual void addPortForwarding(const QString &ruleName, const QString &protocol,
            quint16 hostPort, quint16 emulatorVmPort, const QObject *context,
            const Functor<bool> &functor) = 0;
    virtual void removePortForwarding(const QString &ruleName, const QObject *context,
            const Functor<bool> &functor) = 0;

    template<typename ConcreteUi>
    static void registerConnectionUi()
    {
        Q_ASSERT(!s_connectionUiCreator);
        s_connectionUiCreator = [](VirtualMachine *parent) { return std::make_unique<ConcreteUi>(parent); };
    }

public slots:
    void refresh(Sfdk::VirtualMachine::Synchronization synchronization = Asynchronous);
    bool connectTo(Sfdk::VirtualMachine::ConnectOptions options = NoConnectOption);
    void disconnectFrom();

signals:
    void nameChanged(const QString &name);
    void stateChanged();
    void sshParametersChanged();
    void headlessChanged(bool headless);
    void autoConnectEnabledChanged(bool autoConnectEnabled);
    void virtualMachineOffChanged(bool vmOff);
    void lockDownFailed();

protected:
    VirtualMachine(std::unique_ptr<VirtualMachinePrivate> &&dd, QObject *parent);
    std::unique_ptr<VirtualMachinePrivate> d_ptr;

private:
    Q_DISABLE_COPY(VirtualMachine)
    Q_DECLARE_PRIVATE(VirtualMachine)

    using ConnectionUiCreator = std::function<std::unique_ptr<ConnectionUi>(VirtualMachine *)>;
    static ConnectionUiCreator s_connectionUiCreator;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachine::ConnectOptions)

class SFDK_EXPORT VirtualMachine::ConnectionUi : public QObject
{
    Q_OBJECT

public:
    enum Warning {
        AlreadyConnecting,
        AlreadyDisconnecting,
        UnableToCloseVm,
        VmNotRegistered,
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
