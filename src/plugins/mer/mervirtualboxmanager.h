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

#include <coreplugin/id.h>

namespace Utils {
    class PortList;
}

namespace Mer {
namespace Internal {

class VirtualMachineInfo
{
public:
    VirtualMachineInfo() : sshPort(0), wwwPort(0), headless(false) {}
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
};

class MerVirtualBoxManager : public QObject
{
    Q_OBJECT
public:
    static MerVirtualBoxManager* instance();
    ~MerVirtualBoxManager() override;
    static bool isVirtualMachineRunning(const QString &vmName);
    static void isVirtualMachineRunning(const QString &vmName, QObject *context,
                                        std::function<void(bool)> slot);
    static bool isVirtualMachineRegistered(const QString &vmName);
    static QStringList fetchRegisteredVirtualMachines();
    static VirtualMachineInfo fetchVirtualMachineInfo(const QString &vmName);
    static void startVirtualMachine(const QString &vmName, bool headless);
    static void shutVirtualMachine(const QString &vmName);
    static bool updateSharedFolder(const QString &vmName, const QString &mountName, const QString &newFolder);
    static bool setVideoMode(const QString &vmName, const QSize &size, int depth);
    static QString getExtraData(const QString &vmName, const QString &key);

private:
    MerVirtualBoxManager(QObject *parent = 0);
    void setUpQmlLivePortsForwarding(const QString &vmName, const QSet<int> &ports);
    QString qmlLivePortsForwardingRuleName(int index);
    QString qmlLivePortsForwardingRule(int index, int port);

private slots:
    void onDeviceAdded(Core::Id id);
    void onDeviceRemoved(Core::Id id);
    void onDeviceListReplaced();

private:
    static MerVirtualBoxManager *m_instance;
    friend class MerPlugin;

    QHash<Core::Id, QSet<int>> m_deviceQmlLivePortsCache;
};

} // Internal
} // Mer

#endif // VIRTUALBOXMANAGER_H
