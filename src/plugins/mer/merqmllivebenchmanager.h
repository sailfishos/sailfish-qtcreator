/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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
#include <QObject>
#include <QQueue>
#include <QSet>

#include <coreplugin/id.h>

QT_FORWARD_DECLARE_CLASS(QProcess)

namespace ProjectExplorer {
    class RunControl;
}

namespace Utils {
    class PortList;
}

namespace Mer {
namespace Internal {

class MerQmlLiveBenchManager : public QObject
{
    Q_OBJECT

    struct DeviceInfo
    {
        QString name;
        QSet<int> ports;
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
    static MerQmlLiveBenchManager* instance();
    ~MerQmlLiveBenchManager() override;

    static void startBench();
    static void offerToStartBenchIfNotRunning();

private:
    MerQmlLiveBenchManager(QObject *parent = nullptr);

    static QString qmlLiveHostName(const QString &merDeviceName, int port);
    void addHostsToBench(const QString &merDeviceName, const QString &address, const QSet<int> &ports);
    void removeHostsFromBench(const QString &merDeviceName, const QSet<int> &ports);
    void enqueueCommand(Command *command);
    void processCommandsQueue();

private slots:
    void onBenchLocationChanged();
    void onDeviceAdded(Core::Id id);
    void onDeviceRemoved(Core::Id id);
    void onDeviceListReplaced();
    void onRunControlStarted(ProjectExplorer::RunControl *rc);

private:
    static MerQmlLiveBenchManager *m_instance;
    friend class MerPlugin;

    bool m_enabled;
    QHash<Core::Id, DeviceInfo *> m_deviceInfoCache;
    QQueue<Command *> m_commands;
    Command *m_currentCommand{};
};

} // Internal
} // Mer

#endif // MERQMLLIVEBENCHMANAGER_H
