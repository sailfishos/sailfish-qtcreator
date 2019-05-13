/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "merprojectlistener.h"

#include "mersdkmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace QmakeProjectManager;

namespace Mer {
namespace Internal {

MerProjectListener::MerProjectListener(QObject *parent)
    : QObject(parent)
{
    connect(MerSdkManager::instance(), &MerSdkManager::initialized,
            this, &MerProjectListener::init);
}

void MerProjectListener::init()
{
    // This is called just once after the MerSdkManager is initialized.
    const QList<Project*> projects = SessionManager::projects();
    // O(n2)
    foreach (Project *project, projects)
        onProjectAdded(project);

    connect(SessionManager::instance(), &SessionManager::projectAdded,
            this, &MerProjectListener::onProjectAdded);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &MerProjectListener::onProjectRemoved);
}

void MerProjectListener::onTargetAddedToProject(Target *target)
{
    Project *project = target->project();
    if (MerSdkManager::merKits().contains(target->kit()))
        handleProject_private(project);
}

void MerProjectListener::onTargetRemovedFromProject(Target *target)
{
    Project *project = target->project();
    const QList<Kit *> merKits = MerSdkManager::merKits();
    int merTargetsCount = 0;
    // Counting how many Mer Targets are left after this removal
    foreach (Target *projectTarget, project->targets())
        if (merKits.contains(projectTarget->kit()))
            merTargetsCount++;
    if (merTargetsCount == 0)
        forgetProject(project);
}

void MerProjectListener::onProjectAdded(Project *project)
{
    connect(project, &Project::addedTarget,
            this, &MerProjectListener::onTargetAddedToProject);
    connect(project, &Project::removedTarget,
            this, &MerProjectListener::onTargetRemovedFromProject);

    const QList<Kit *> merKits = MerSdkManager::merKits();
    foreach (Kit *kit, merKits) {
        if (!containsType(project->projectIssues(kit), Task::TaskType::Error)) {
            if (handleProject_private(project))
                break;
        }
    }
}

void MerProjectListener::onProjectRemoved(Project *project)
{
    forgetProject(project);
}

bool MerProjectListener::handleProject_private(Project *project)
{
    QmakeProject *qmakeProject = qobject_cast<QmakeProject*>(project);
    if (!qmakeProject)
        return false;
    return handleProject(qmakeProject);
}

} // Internal
} // Mer
