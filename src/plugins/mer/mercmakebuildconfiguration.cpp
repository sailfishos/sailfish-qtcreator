/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#include "mercmakebuildconfiguration.h"

#include "merconstants.h"
#include "merbuildsteps.h"
#include "merlogging.h"
#include "mersettings.h"
#include "mersdkkitinformation.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <cmakeprojectmanager/cmakeprojectmanager.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmakebuildstep.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace CMakeProjectManager::Internal;

namespace Mer {
namespace Internal {

MerCMakeBuildConfiguration::MerCMakeBuildConfiguration(Target *target, Core::Id id)
    : CMakeBuildConfiguration(target, id)
{
}

void MerCMakeBuildConfiguration::initialize(const ProjectExplorer::BuildInfo &info)
{
    CMakeBuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(buildSteps);
    buildSteps->insertStep(0, new MerSdkStartStep(buildSteps));

    BuildStepList *cleanSteps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    Q_ASSERT(cleanSteps);
    cleanSteps->insertStep(0, new MerSdkStartStep(cleanSteps));
}

bool MerCMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!CMakeBuildConfiguration::fromMap(map))
        return false;
    return true;
}

/// returns whether this is a shadow build configuration or not
/// note, even if shadowBuild() returns true, it might be using the
/// source directory as the shadow build directory, thus it
/// still is a in-source build
bool MerCMakeBuildConfiguration::isShadowBuild() const
{
    return buildDirectory() != target()->project()->projectDirectory();
}

MerCMakeBuildConfigurationFactory::MerCMakeBuildConfigurationFactory()
{
    registerBuildConfiguration<MerCMakeBuildConfiguration>(CMakeProjectManager::Constants::CMAKEPROJECT_BC_ID);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

} // Internal
} // Mer
