/****************************************************************************
**
** Copyright (C) 2012 - 2016 Jolla Ltd.
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

#ifndef VIRTUALBOXMANAGER_H
#define VIRTUALBOXMANAGER_H

#include <functional>

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Utils {
    class Port;
    class PortList;
}

namespace Mer {
namespace Internal {

class VirtualMachineInfo
{
public:
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
    QString vdiUuid;

    // VdiInfo
    int vdiCapacityMb{0};
    QStringList allRelatedVdiUuids;

    // SnapshotInfo
    QStringList snapshots;
};

// TODO Possible to use QFutureInterface?
// TODO Errors should be reported in the UI
// TODO Use UUIDs instead of names - names may not be unique
class MerVirtualBoxManager : public QObject
{
    Q_OBJECT
public:
    enum ExtraInfo {
        NoExtraInfo = 0x00,
        VdiInfo = 0x01,
        SnapshotInfo = 0x02,
    };
    Q_DECLARE_FLAGS(ExtraInfos, ExtraInfo)

    MerVirtualBoxManager(QObject *parent = 0);
    static MerVirtualBoxManager* instance();
    ~MerVirtualBoxManager() override;
    static void isVirtualMachineRunning(const QString &vmName, QObject *context,
                                        std::function<void(bool,bool)> slot);
    static QStringList fetchRegisteredVirtualMachines();
    static VirtualMachineInfo fetchVirtualMachineInfo(const QString &vmName,
            ExtraInfos extraInfo = NoExtraInfo);
    static void startVirtualMachine(const QString &vmName, bool headless);
    static void shutVirtualMachine(const QString &vmName);
    static void restoreSnapshot(const QString &vmName, const QString &snapshotName,
            QObject *context, std::function<void(bool)> slot);
    static bool updateSharedFolder(const QString &vmName, const QString &mountName, const QString &newFolder);
    static bool updateSdkSshPort(const QString &vmName, quint16 port);
    static bool updateSdkWwwPort(const QString &vmName, quint16 port);
    static bool updateEmulatorSshPort(const QString &vmName, quint16 port);
    static Utils::PortList updateEmulatorQmlLivePorts(const QString &vmName, const QList<Utils::Port> &ports);
    static void setVideoMode(const QString &vmName, const QSize &size, int depth);
    static void setVdiCapacityMb(const QString &vmName, int sizeMb, QObject *context, std::function<void(bool)> slot);
    static bool setMemorySizeMb(const QString &vmName, int sizeMb);
    static bool setCpuCount(const QString &vmName, int count);

    static bool deletePortForwardingRule(const QString &vmName, const QString &ruleName);
    static bool updatePortForwardingRule(const QString &vmName, const QString &protocol,
                                         const QString &ruleName, quint16 hostPort, quint16 vmPort);
    static QList<QMap<QString, quint16>> fetchPortForwardingRules(const QString &vmName);

    static QString getExtraData(const QString &vmName, const QString &key);
    static void getHostTotalMemorySizeMb(QObject *context, std::function<void(int)> slot);
    static int getHostTotalCpuCount();
private:
    static void setExtraData(const QString& vmName, const QString& keyword, const QString& data);

    static MerVirtualBoxManager *m_instance;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MerVirtualBoxManager::ExtraInfos)

} // Internal
} // Mer

#endif // VIRTUALBOXMANAGER_H
