/****************************************************************************
**
** Copyright (C) 2012-2015,2018 Jolla Ltd.
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

#include <coreplugin/id.h>

#include <QObject>

namespace ProjectExplorer {
class Kit;
class Project;
class Target;
class Task;
}

namespace QSsh {
class SshConnectionParameters;
}

namespace Mer {
namespace Internal {

class MerConnectionAction;

class MerConnectionManager : public QObject
{
    Q_OBJECT
public:
    MerConnectionManager();
    static MerConnectionManager* instance();
    ~MerConnectionManager() override;
    static QString testConnection(const QSsh::SshConnectionParameters &params, bool *ok = 0);

private slots:
    void update();
    void handleStartupProjectChanged(ProjectExplorer::Project *project);
    void handleKitUpdated(ProjectExplorer::Kit *kit);
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetRemoved(ProjectExplorer::Target *target);

private:
    static MerConnectionManager *m_instance;
    static ProjectExplorer::Project *m_project;
    MerConnectionAction *m_emulatorAction;
    MerConnectionAction *m_sdkAction;
};

}
}

#endif
