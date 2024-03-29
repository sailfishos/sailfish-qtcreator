/****************************************************************************
**
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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
** conditions contained in a signed written agreement between you and
** The Qt Company.
**
****************************************************************************/

#pragma once

#include "virtualmachine_p.h"

namespace Sfdk {

class DockerVirtualMachinePrivate;
class DockerVirtualMachine : public VirtualMachine
{
    Q_OBJECT

public:
    explicit DockerVirtualMachine(const QString &name, VirtualMachine::Features featureMask,
            std::unique_ptr<ConnectionUi> &&connectionUi, QObject *parent = nullptr);
    ~DockerVirtualMachine() override;

    QString instanceName() const;

    static bool isAvailable();
    static QString staticType();
    static QString staticDisplayType();
    static VirtualMachine::Features staticFeatures();
    static void fetchRegisteredVirtualMachines(const QObject *context,
            const Functor<const QStringList &, bool> &functor);

private:
    Q_DISABLE_COPY(DockerVirtualMachine)
    Q_DECLARE_PRIVATE(DockerVirtualMachine)
};

class DockerVirtualMachinePrivate : public VirtualMachinePrivate
{
    Q_DECLARE_PUBLIC(DockerVirtualMachine)

public:
    using VirtualMachinePrivate::VirtualMachinePrivate;

    void fetchInfo(VirtualMachineInfo::ExtraInfos extraInfo, const QObject *context,
            const Functor<const VirtualMachineInfo &, bool> &functor) const override;

    void start(const QObject *context, const Functor<bool> &functor) override;
    void stop(const QObject *context, const Functor<bool> &functor) override;
    void commit(const QObject *context, const Functor<bool> &functor) override;
    void probe(const QObject *context,
            const Functor<BasicState, bool> &functor) const override;

    void setVideoMode(const QSize &size, int depth, const QString &deviceModelName,
            Qt::Orientation orientation, int scaleDownFactor, const QObject *context,
            const Functor<bool> &functor) override;

protected:
    void doSetMemorySizeMb(int memorySizeMb, const QObject *context,
            const Functor<bool> &functor) override;
    void doSetSwapSizeMb(int swapSizeMb, const QObject *context,
            const Functor<bool> &functor) override;
    void doSetCpuCount(int cpuCount, const QObject *context,
            const Functor<bool> &functor) override;
    void doSetStorageSizeMb(int storageSizeMb, const QObject *context,
            const Functor<bool> &functor) override;

    void doSetSharedPath(SharedPath which, const Utils::FilePath &path, const QObject *context,
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

    void doTakeSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor) override;
    void doRestoreSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor) override;
    void doRemoveSnapshot(const QString &snapshotName, const QObject *context,
            const Functor<bool> &functor) override;

private:
    static QStringList listedImages(const QString &output);
    static VirtualMachineInfo virtualMachineInfoFromOutput(const QByteArray &output);

    QStringList makeCreateArguments() const;

    void rebuildWithLabel(const QString& key, const QString& value,
                        const QObject *context, const Functor<bool> &functor);
    static QString labelFor(SharedPath which);
    static QString labelFor(ReservedPort which);
};

} // namespace Sfdk
