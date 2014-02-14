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

#ifndef MERCONNECTIONMANAGER_H
#define MERCONNECTIONMANAGER_H

#include <QObject>

namespace ProjectExplorer {
class Kit;
class Project;
class Target;
class Task;
}

namespace QSsh {
class SshConnection;
class SshConnectionParameters;
}

namespace Mer {
namespace Internal {

class MerSdk;
class MerConnection;

class MerConnectionManager : public QObject
{
    Q_OBJECT
public:
    static MerConnectionManager* instance();
    ~MerConnectionManager();
    static QSsh::SshConnectionParameters parameters(const MerSdk *sdk);
    QString testConnection(const QSsh::SshConnectionParameters &params) const;
    bool isConnected(const QString &vmName) const;
    void connectTo(const QString &vmName);
    void disconnectFrom(const QString &vmName);
private slots:
    void update();
    void handleStartupProjectChanged(ProjectExplorer::Project *project);
    void handleKitUpdated(ProjectExplorer::Kit *kit);
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetRemoved(ProjectExplorer::Target *target);
    void handleBuildStateChanged(ProjectExplorer::Project* project);
private:
    MerConnectionManager();

private:
    static MerConnectionManager *m_instance;
    static ProjectExplorer::Project *m_project;
    QScopedPointer<MerConnection> m_emulatorConnection;
    QScopedPointer<MerConnection> m_sdkConnection;

    friend class MerPlugin;
};

}
}

#endif
