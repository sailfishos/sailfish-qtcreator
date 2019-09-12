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

namespace Sfdk {

// FIXME internal
class SFDK_EXPORT VirtualMachineInfo
{
    Q_GADGET

public:
    enum ExtraInfo {
        NoExtraInfo = 0x00,
        VdiInfo = 0x01,
        SnapshotInfo = 0x02,
    };
    Q_DECLARE_FLAGS(ExtraInfos, ExtraInfo)

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

    // VdiInfo
    int vdiCapacityMb{0};

    // SnapshotInfo
    QStringList snapshots;
};

class VirtualMachinePrivate
{
    Q_DECLARE_PUBLIC(VirtualMachine)

public:
    enum BasicStateFlag {
        NullState = 0x0,
        Existing = 0x1,
        Running = 0x2,
        Headless = 0x4,
    };
    Q_DECLARE_FLAGS(BasicState, BasicStateFlag)

    VirtualMachinePrivate(VirtualMachine *q) : q_ptr(q) {}

    static VirtualMachinePrivate *get(VirtualMachine *q) { return q->d_func(); }

    virtual void fetchInfo(VirtualMachineInfo::ExtraInfos extraInfo, const QObject *context,
            const Functor<const VirtualMachineInfo &, bool> &functor) const = 0;

    virtual void start(const QObject *context, const Functor<bool> &functor) = 0;
    virtual void stop(const QObject *context, const Functor<bool> &functor) = 0;
    virtual void probe(const QObject *context,
            const Functor<BasicState, bool> &functor) const = 0;

    VirtualMachine::ConnectionUi *connectionUi() const { return connectionUi_.get(); }

protected:
    virtual void prepareForNameChange() {};
    bool initialized() const { return initialized_; }
    void setInitialized() { initialized_ = true; }

    VirtualMachine *const q_ptr;

private:
    static int s_availableMemmorySizeMb;
    QString name;
    bool initialized_ = false;
    std::unique_ptr<VirtualMachine::ConnectionUi> connectionUi_;
    std::unique_ptr<VmConnection> connection;
    QSsh::SshConnectionParameters sshParameters;
    bool headless = false;
    bool autoConnectEnabled = true;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachineInfo::ExtraInfos)
Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualMachinePrivate::BasicState);

} // namespace Sfdk
