/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
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

#include "meraddvmstartbuildstepprojectlistener.h"

#include "merbuildsteps.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Mer {
namespace Internal {

MerAddVmStartBuildStepProjectListener::MerAddVmStartBuildStepProjectListener(QObject *parent)
    : MerProjectListener(parent)
{
}

MerAddVmStartBuildStepProjectListener::~MerAddVmStartBuildStepProjectListener()
{
}

bool MerAddVmStartBuildStepProjectListener::handleProject(QmakeProject *project)
{
    foreach (Target *target, project->targets()) {
        foreach (BuildConfiguration *bc, target->buildConfigurations()) {
            ensureHasVmStartStep(bc);
        }
    }

    return true;
}

bool MerAddVmStartBuildStepProjectListener::forgetProject(Project *project)
{
    Q_UNUSED(project);
    return true;
}

void MerAddVmStartBuildStepProjectListener::ensureHasVmStartStep(BuildConfiguration *bc)
{
    BuildStepList *buildSteps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(buildSteps);
    if (!buildSteps->contains(MerSdkStartStep::stepId()))
        buildSteps->insertStep(0, new MerSdkStartStep(buildSteps));

    BuildStepList *cleanSteps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    Q_ASSERT(cleanSteps);
    if (!cleanSteps->contains(MerSdkStartStep::stepId()))
        cleanSteps->insertStep(0, new MerSdkStartStep(cleanSteps));
}

} // Internal
} // Mer
