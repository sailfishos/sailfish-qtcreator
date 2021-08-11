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

#include "merbuildconfigurationaspect.h"
#include "merconstants.h"
#include "merbuildsteps.h"
#include "merlogging.h"
#include "mersettings.h"
#include "mersdkkitaspect.h"

#include <sfdk/buildengine.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <cmakeprojectmanager/cmakeprojectmanager.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <cmakeprojectmanager/cmakebuildstep.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace CMakeProjectManager;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerCMakeBuildConfiguration::MerCMakeBuildConfiguration(Target *target, Utils::Id id)
    : CMakeBuildConfiguration(target, id)
{
    auto aspect = addAspect<MerBuildConfigurationAspect>(this);
    connect(aspect, &BaseAspect::changed,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);

    connect(target, &Target::parsingStarted,
            this, &MerCMakeBuildConfiguration::ensureBuildEngineRuns);
}

void MerCMakeBuildConfiguration::doInitialize(const ProjectExplorer::BuildInfo &info)
{
    CMakeBuildConfiguration::doInitialize(info);

    BuildStepList *buildSteps = this->buildSteps();
    QTC_ASSERT(buildSteps, return);
    buildSteps->insertStep(0, MerSdkStartStep::stepId());

    BuildStepList *cleanSteps = this->cleanSteps();
    QTC_ASSERT(cleanSteps, return);
    cleanSteps->insertStep(0, MerSdkStartStep::stepId());
    cleanSteps->insertStep(1, MerClearBuildEnvironmentStep::stepId());
}

QList<NamedWidget *> MerCMakeBuildConfiguration::createSubConfigWidgets()
{
    auto retv = CMakeBuildConfiguration::createSubConfigWidgets();

    auto aspect = this->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return retv);

    auto widget = qobject_cast<NamedWidget *>(aspect->createConfigWidget());
    QTC_ASSERT(widget, return retv);

    retv += widget;
    return retv;
}

void MerCMakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    CMakeBuildConfiguration::addToEnvironment(env);

    auto aspect = this->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return);
    aspect->addToEnvironment(env);
}

MerCMakeBuildConfigurationFactory::MerCMakeBuildConfigurationFactory()
{
    registerBuildConfiguration<MerCMakeBuildConfiguration>(CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

void MerCMakeBuildConfiguration::ensureBuildEngineRuns()
{
    BuildEngine *const engine = MerSdkKitAspect::buildEngine(target()->kit());
    if (engine->virtualMachine()->state() == VirtualMachine::Connected)
        return;

    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::connectTo),
            engine->virtualMachine(), VirtualMachine::AskStartVm);
    // FIXME handle failure?
}

} // Internal
} // Mer
