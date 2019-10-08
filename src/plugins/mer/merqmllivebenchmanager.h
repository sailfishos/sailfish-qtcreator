/****************************************************************************
**
** Copyright (C) 2016-2018 Jolla Ltd.
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

#ifndef MERQMLLIVEBENCHMANAGER_H
#define MERQMLLIVEBENCHMANAGER_H

#include <functional>

#include <QHash>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QSet>

#include <coreplugin/id.h>

QT_FORWARD_DECLARE_CLASS(QProcess)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace ProjectExplorer {
    class Project;
    class RunConfiguration;
    class RunControl;
    class Target;
}

namespace Utils {
    class Port;
}

namespace Mer {
namespace Internal {

class MerQmlLiveBenchManager : public QObject
{
    Q_OBJECT

    struct DeviceInfo
    {
        QString name;
        QList<Utils::Port> ports;
    };

    struct Command
    {
        QStringList arguments;
#ifdef Q_CC_MSVC // Workaround C2797
        QSet<int> expectedExitCodes = QSet<int>{0};
#else
        QSet<int> expectedExitCodes{0};
#endif
        std::function<void(int)> onFinished;

        // Private to processCommandsQueue()
        QProcess *process{};
    };

public:
    MerQmlLiveBenchManager(QObject *parent = nullptr);
    static MerQmlLiveBenchManager* instance();
    ~MerQmlLiveBenchManager() override;

    static void startBench();
    static void offerToStartBenchIfNotRunning();
    static void notifyInferiorRunning(ProjectExplorer::RunControl *rc);

private:
    static void warnBenchLocationNotSet();
    static QString qmlLiveHostName(const QString &merDeviceName, Utils::Port port);
    void addHostsToBench(const QString &merDeviceName, const QString &address, const QList<Utils::Port> &ports);
    void removeHostsFromBench(const QString &merDeviceName, const QList<Utils::Port> &ports);
    void letRunningBenchProbeHosts(const QString &merDeviceName, const QList<Utils::Port> &ports);
    void enqueueCommand(Command *command);
    void processCommandsQueue();
    void startProbing(ProjectExplorer::RunControl *rc);

private slots:
    void onBenchLocationChanged();
    void onDeviceAdded(Core::Id id);
    void onDeviceRemoved(Core::Id id);
    void onDeviceListReplaced();
    void onStartupProjectChanged(ProjectExplorer::Project *project);
    void onActiveTargetChanged(ProjectExplorer::Target *target);
    void onActiveRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);
    void onQmlLiveEnabledChanged(bool enabled);
    void onQmlLiveBenchWorkspaceChanged(const QString &benchWorkspace);
    void onAboutToExecuteProject(ProjectExplorer::RunControl *rc);

private:
    static MerQmlLiveBenchManager *m_instance;
    bool m_enabled;
    QHash<Core::Id, DeviceInfo *> m_deviceInfoCache;
    QQueue<Command *> m_commands;
    Command *m_currentCommand{};
    QMap<ProjectExplorer::RunControl *, QTimer *> m_probeTimeouts;

    QMetaObject::Connection m_activeTargetChangedConnection;
    QMetaObject::Connection m_activeRunConfigurationChangedConnection;
    QMetaObject::Connection m_qmlLiveEnabledChangedConnection;
    QMetaObject::Connection m_qmlLiveBenchWorkspaceChangedConnection;
};

} // Internal
} // Mer

#endif // MERQMLLIVEBENCHMANAGER_H
