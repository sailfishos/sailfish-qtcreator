/****************************************************************************
**
** Copyright (C) 2012-2015,2017-2019 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "merdeployconfiguration.h"

#include "merconstants.h"
#include "merdeploysteps.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QLabel>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace RemoteLinux;

namespace Mer {
namespace Internal {

namespace {
const char SAILFISH_AMBIENCE_CONFIG[] = "sailfish-ambience";
const int ADD_REMOVE_SPECIAL_STEPS_DELAY_MS = 1000;
} // anonymous namespace

MerRpmDeployConfigurationFactory::MerRpmDeployConfigurationFactory()
{
    addInitialStep(MerPrepareTargetStep::stepId());
    addInitialStep(MerMb2RpmDeployStep::stepId());
}

QString MerRpmDeployConfigurationFactory::displayName()
{
    return tr("Deploy As RPM Package");
}

Core::Id MerRpmDeployConfigurationFactory::configurationId()
{
    return Core::Id("QmakeProjectManager.MerRpmDeployConfiguration");
}

////////////////////////////////////////////////////////////////////////////////////////////

MerRsyncDeployConfigurationFactory::MerRsyncDeployConfigurationFactory()
{
    addInitialStep(MerPrepareTargetStep::stepId());
    addInitialStep(MerMb2RpmDeployStep::stepId());
}

QString MerRsyncDeployConfigurationFactory::displayName()
{
    return tr("Deploy By Copying Binaries");
}

Core::Id MerRsyncDeployConfigurationFactory::configurationId()
{
    return Core::Id("QmakeProjectManager.MerRSyncDeployConfiguration");
}

/////////////////////////////////////////////////////////////////////////////////////
//TODO:HACK
MerMb2RpmBuildConfigurationFactory::MerMb2RpmBuildConfigurationFactory()
{
    addInitialStep(MerMb2RpmBuildStep::stepId());
}

QString MerMb2RpmBuildConfigurationFactory::displayName()
{
    return tr("Build RPM Package For Manual Deployment");
}

Core::Id MerMb2RpmBuildConfigurationFactory::configurationId()
{
    return Core::Id("QmakeProjectManager.MerMb2RpmBuildConfiguration");
}

////////////////////////////////////////////////////////////////////////////////////

bool MerAddRemoveSpecialDeployStepsProjectListener::handleProject(QmakeProject *project)
{
    connect(project, &Project::parsingFinished,
            this, &MerAddRemoveSpecialDeployStepsProjectListener::scheduleProjectUpdate,
            Qt::UniqueConnection);

    return true;
}

bool MerAddRemoveSpecialDeployStepsProjectListener::forgetProject(Project *project)
{
    // Who knows if it still can be downcasted to QmakeProject at this point
    Utils::erase(m_updateProjectsQueue, [project](QmakeProject *enqueued) {
        return enqueued == project;
    });

    disconnect(project, &Project::parsingFinished,
            this, &MerAddRemoveSpecialDeployStepsProjectListener::scheduleProjectUpdate);

    return true;
}

void MerAddRemoveSpecialDeployStepsProjectListener::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_updateProjectsTimer.timerId()) {
        m_updateProjectsTimer.stop();
        while (!m_updateProjectsQueue.isEmpty())
            updateProject(m_updateProjectsQueue.dequeue());
    } else {
        MerProjectListener::timerEvent(event);
    }
}

// Avoid temporary changes to project configuration
void MerAddRemoveSpecialDeployStepsProjectListener::scheduleProjectUpdate()
{
    QmakeProject *project = qobject_cast<QmakeProject *>(sender());
    QTC_ASSERT(project, return);

    m_updateProjectsQueue.enqueue(project);
    m_updateProjectsTimer.start(ADD_REMOVE_SPECIAL_STEPS_DELAY_MS, this);
}

void MerAddRemoveSpecialDeployStepsProjectListener::updateProject(QmakeProject *project)
{
    const bool isAmbienceProject = this->isAmbienceProject(project);

    for (Target *const target : project->targets()) {
        for (DeployConfiguration *const dc : target->deployConfigurations()) {
            if (dc->id() == MerMb2RpmBuildConfigurationFactory::configurationId()) {
                if (!isAmbienceProject) {
                    if (!dc->stepList()->contains(MerRpmValidationStep::stepId()))
                        dc->stepList()->appendStep(new MerRpmValidationStep(dc->stepList()));
                } else {
                    removeStep(dc->stepList(), MerRpmValidationStep::stepId());
                }
            } else if (dc->id() == MerRpmDeployConfigurationFactory::configurationId()
                    || dc->id() == MerRsyncDeployConfigurationFactory::configurationId()) {
                if (isAmbienceProject) {
                    if (!dc->stepList()->contains(MerResetAmbienceDeployStep::stepId()))
                        dc->stepList()->appendStep(new MerResetAmbienceDeployStep(dc->stepList()));
                } else {
                    removeStep(dc->stepList(), MerResetAmbienceDeployStep::stepId());
                }
            }
        }
    }
}

bool MerAddRemoveSpecialDeployStepsProjectListener::isAmbienceProject(QmakeProject *project)
{
    QmakeProFile *root = project->rootProFile();
    return root->projectType() == ProjectType::AuxTemplate &&
        root->variableValue(Variable::Config).contains(QLatin1String(SAILFISH_AMBIENCE_CONFIG));
}

// TODO add BuildStepList::removeStep(Core::Id)
void MerAddRemoveSpecialDeployStepsProjectListener::removeStep(BuildStepList *stepList, Core::Id stepId)
{
    for (int i = 0; i < stepList->count(); ++i) {
        if (stepList->at(i)->id() == stepId) {
            stepList->removeStep(i);
            break;
        }
    }
}

} // namespace Internal
} // namespace Mer
