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

#pragma once

#include "virtualmachine_p.h"

namespace Sfdk {

class VBoxVirtualMachinePrivate;
class VBoxVirtualMachine : public VirtualMachine
{
    Q_OBJECT

public:
    explicit VBoxVirtualMachine(const QString &name, QObject *parent = nullptr);
    ~VBoxVirtualMachine() override;

    static QString staticType();
    static QString staticDisplayType();
    static void fetchRegisteredVirtualMachines(const QObject *context,
            const Functor<const QStringList &, bool> &functor);
    static void fetchHostTotalMemorySizeMb(const QObject *context,
            const Functor<int, bool> &functor);

private:
    Q_DISABLE_COPY(VBoxVirtualMachine)
    Q_DECLARE_PRIVATE(VBoxVirtualMachine)
};

class VBoxVirtualMachineInfo;
class VBoxVirtualMachinePrivate : public VirtualMachinePrivate
{
    Q_DECLARE_PUBLIC(VBoxVirtualMachine)

public:
    using VirtualMachinePrivate::VirtualMachinePrivate;

    void fetchInfo(VirtualMachineInfo::ExtraInfos extraInfo, const QObject *context,
            const Functor<const VirtualMachineInfo &, bool> &functor) const override;

    void start(const QObject *context, const Functor<bool> &functor) override;
    void stop(const QObject *context, const Functor<bool> &functor) override;
    void probe(const QObject *context,
            const Functor<BasicState, bool> &functor) const override;

    void setVideoMode(const QSize &size, int depth, const QObject *context,
            const Functor<bool> &functor) override;

protected:
    void doSetMemorySizeMb(int memorySizeMb, const QObject *context,
            const Functor<bool> &functor) override;
    void doSetCpuCount(int cpuCount, const QObject *context,
            const Functor<bool> &functor) override;
    void doSetVdiCapacityMb(int vdiCapacityMb, const QObject *context,
            const Functor<bool> &functor) override;

    void doSetSharedPath(SharedPath which, const Utils::FileName &path, const QObject *context,
            const Functor<bool> &functor) override;

    void doAddPortForwarding(const QString &ruleName,
        const QString &protocol, quint16 hostPort, quint16 emulatorVmPort,
        const QObject *context, const Functor<bool> &functor) override;
    void doRemovePortForwarding(const QString &ruleName,
        const QObject *context, const Functor<bool> &functor) override;
    void doSetReservedPortForwarding(ReservedPort which, quint16 port,
            const QObject *context, const Functor<bool> &functor) override;
    void doSetReservedPortListForwarding(ReservedPortList which, const QList<Utils::Port> &ports,
            const QObject *context, const Functor<const QMap<QString, quint16> &, bool> &functor)
        override;

    void doRestoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor) override;

private:
    void fetchExtraData(const QString &key, const QObject *context,
            const Functor<QString, bool> &functor) const;
    void setExtraData(const QString &keyword, const QString &data, const QObject *context,
            const Functor<bool> &functor);

    static bool isVirtualMachineRunningFromInfo(const QString &vmInfo, bool *headless);
    static QStringList listedVirtualMachines(const QString &output);
    static VBoxVirtualMachineInfo virtualMachineInfoFromOutput(const QString &output);
    static void vdiInfoFromOutput(const QString &output,
            VBoxVirtualMachineInfo *virtualMachineInfo);
    static int ramSizeFromOutput(const QString &output, bool *matched);
    static void snapshotInfoFromOutput(const QString &output,
            VBoxVirtualMachineInfo *virtualMachineInfo);
};

} // namespace Sfdk
