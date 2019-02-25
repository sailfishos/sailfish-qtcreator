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

#include <QHash>
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
    VirtualMachineInfo() : sshPort(0), wwwPort(0), headless(false), memorySizeMb(0), cpuCount(0), vdiCapacityMb(0) {}
    QString sharedHome;
    QString sharedTargets;
    QString sharedConfig;
    QString sharedSrc;
    QString sharedSsh;
    quint16 sshPort;
    quint16 wwwPort;
    QList<quint16> freePorts;
    QList<quint16> qmlLivePorts;
    QStringList macs;
    bool headless;
    int memorySizeMb;
    int cpuCount;
    int vdiCapacityMb;
    QString vdiPath;
};

class MerVirtualBoxManager : public QObject
{
    Q_OBJECT
public:
    MerVirtualBoxManager(QObject *parent = 0);
    static MerVirtualBoxManager* instance();
    ~MerVirtualBoxManager() override;
    static void isVirtualMachineRunning(const QString &vmName, QObject *context,
                                        std::function<void(bool,bool)> slot);
    static QStringList fetchRegisteredVirtualMachines();
    static VirtualMachineInfo fetchVirtualMachineInfo(const QString &vmName, bool fetchVdiInfo = false);
    static void startVirtualMachine(const QString &vmName, bool headless);
    static void shutVirtualMachine(const QString &vmName);
    static bool updateSharedFolder(const QString &vmName, const QString &mountName, const QString &newFolder);
    static bool updateSdkSshPort(const QString &vmName, quint16 port);
    static bool updateSdkWwwPort(const QString &vmName, quint16 port);
    static bool updateEmulatorSshPort(const QString &vmName, quint16 port);
    static Utils::PortList updateEmulatorQmlLivePorts(const QString &vmName, const QList<Utils::Port> &ports);
    static void setVideoMode(const QString &vmName, const QSize &size, int depth);
    static void setVdiCapacityMb(const QString &vmName, int sizeMb, QObject *context, std::function<void(bool)> slot);
    static bool setMemorySizeMb(const QString &vmName, int sizeMb);
    static bool setCpuCount(const QString &vmName, int count);

    static QString getExtraData(const QString &vmName, const QString &key);
    static void getHostTotalMemorySizeMb(QObject *context, std::function<void(int)> slot);
    static int getHostTotalCpuCount();
private:
    static MerVirtualBoxManager *m_instance;
};

} // Internal
} // Mer

#endif // VIRTUALBOXMANAGER_H
