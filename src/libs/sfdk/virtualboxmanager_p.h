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

// TODO make internal

#pragma once

#include "asynchronous.h"
#include "sfdkglobal.h"
#include "virtualmachine_p.h"

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

namespace Utils {
    class Port;
    class PortList;
}

namespace Sfdk {

// FIXME internal
// TODO Errors should be reported in the UI
// TODO Use UUIDs instead of names - names may not be unique
class CommandSerializer;
class SFDK_EXPORT VirtualBoxManager : public QObject
{
    Q_OBJECT
public:
    VirtualBoxManager(QObject *parent = 0);
    static VirtualBoxManager* instance();
    ~VirtualBoxManager() override;

    static void probe(const QString &vmName, const QObject *context,
            const Functor<VirtualMachinePrivate::BasicState, bool> &functor);
    static void fetchRegisteredVirtualMachines(const QObject *context,
            const Functor<const QStringList &, bool> &functor);
    static void fetchVirtualMachineInfo(const QString &vmName, VirtualMachineInfo::ExtraInfos extraInfo,
            const QObject *context, const Functor<const VirtualMachineInfo &, bool> &functor);

    static void startVirtualMachine(const QString &vmName, bool headless,
            const QObject *context, const Functor<bool> &functor);
    static void shutVirtualMachine(const QString &vmName, const QObject *context,
            const Functor<bool> &functor);

    static void restoreSnapshot(const QString &vmName, const QString &snapshotName,
            const QObject *context, const Functor<bool> &functor);

    static void updateSharedFolder(const QString &vmName, VirtualMachinePrivate::SharedPath which,
            const QString &newFolder, const QObject *context, const Functor<bool> &functor);

    static void updateSdkSshPort(const QString &vmName, quint16 port, const QObject *context,
            const Functor<bool> &functor);
    static void updateSdkWwwPort(const QString &vmName, quint16 port, const QObject *context,
            const Functor<bool> &functor);
    static void updateEmulatorSshPort(const QString &vmName, quint16 port, const QObject *context,
            const Functor<bool> &functor);
    static void updateEmulatorQmlLivePorts(const QString &vmName, const QList<Utils::Port> &ports,
            const QObject *context, const Functor<const Utils::PortList &, bool> &functor);

    static void setVideoMode(const QString &vmName, const QSize &size, int depth,
            const QObject *context, const Functor<bool> &functor);

    static void setVdiCapacityMb(const QString &vmName, int sizeMb, const QObject *context,
            const Functor<bool> &functor);
    static void setMemorySizeMb(const QString &vmName, int sizeMb, const QObject *context,
            const Functor<bool> &functor);
    static void setCpuCount(const QString &vmName, int count, const QObject *context,
            const Functor<bool> &functor);

    static void deletePortForwardingRule(const QString &vmName, const QString &ruleName,
            const QObject *context, const Functor<bool> &functor);
    static void updatePortForwardingRule(const QString &vmName, const QString &protocol,
            const QString &ruleName, quint16 hostPort, quint16 vmPort,
            const QObject *context, const Functor<bool> &functor);
    static void fetchPortForwardingRules(const QString &vmName, const QObject *context,
            const Functor<const QList<QMap<QString, quint16>> &, bool> &functor);

    static void fetchExtraData(const QString &vmName, const QString &key, const QObject *context,
            const Functor<QString, bool> &functor);

    static void fetchHostTotalMemorySizeMb(const QObject *context,
            const Functor<int, bool> &functor);

private:
    static void setExtraData(const QString &vmName, const QString &keyword, const QString &data,
            const QObject *context, const Functor<bool> &functor);

    static VirtualBoxManager *s_instance;
    std::unique_ptr<CommandSerializer> m_serializer;
};

} // Sfdk
