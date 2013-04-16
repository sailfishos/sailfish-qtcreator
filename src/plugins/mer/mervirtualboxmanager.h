/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#include <QObject>
#include <QHash>
#include <QString>

namespace ProjectExplorer {
class Task;
}

namespace Mer {
namespace Internal {

class VirtualMachineInfo
{
public:
    VirtualMachineInfo() : sshPort(0), wwwPort(0) {}
    QString sharedHome;
    QString sharedTarget;
    QString sharedSsh;
    quint16 sshPort;
    quint16 wwwPort;
    QList<quint16> freePorts;
};

class MerVirtualBoxManager : public QObject
{
    Q_OBJECT
public:
    MerVirtualBoxManager* instance();
    ~MerVirtualBoxManager();
    static bool isVirtualMachineRunning(const QString &vmName);
    static bool isVirtualMachineRegistered(const QString &vmName);
    static QStringList fetchRegisteredVirtualMachines();
    static VirtualMachineInfo fetchVirtualMachineInfo(const QString &vmName);
    static bool startVirtualMachine(const QString &vmName);
    static bool shutVirtualMachine(const QString &vmName);
private slots:
    void handleTaskAdded(const ProjectExplorer::Task &task);
private:
    MerVirtualBoxManager(QObject *parent = 0);
    bool promptToStart(const QString& vm) const;

private:
    MerVirtualBoxManager *m_instance;
    friend class MerPlugin;
};

} // Internal
} // Mer

#endif // VIRTUALBOXMANAGER_H
