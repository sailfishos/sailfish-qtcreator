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

#include "mercompilationdatabasebuildconfiguration.h"

#include "merbuildconfigurationaspect.h"
#include "merconstants.h"
#include "merbuildsteps.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

/*!
 * \class MerCompilationDatabaseBuildConfiguration
 * \internal
 */

MerCompilationDatabaseBuildConfiguration::MerCompilationDatabaseBuildConfiguration(Target *target,
        Utils::Id id)
    : BuildConfiguration(target, id)
{
    auto aspect = addAspect<MerBuildConfigurationAspect>(this);
    connect(aspect, &BaseAspect::changed,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    updateCacheAndEmitEnvironmentChanged();

    appendInitialBuildStep(MerSdkStartStep::stepId());
    appendInitialBuildStep(Constants::MER_COMPILATION_DATABASE_MS_ID);

    appendInitialCleanStep(MerSdkStartStep::stepId());
    appendInitialCleanStep(MerClearBuildEnvironmentStep::stepId());
    appendInitialCleanStep(Constants::MER_COMPILATION_DATABASE_MS_ID);
}

QList<NamedWidget *> MerCompilationDatabaseBuildConfiguration::createSubConfigWidgets()
{
    auto retv = BuildConfiguration::createSubConfigWidgets();

    auto aspect = this->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return retv);

    auto widget = qobject_cast<NamedWidget *>(aspect->createConfigWidget());
    QTC_ASSERT(widget, return retv);

    retv += widget;
    return retv;
}

void MerCompilationDatabaseBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    BuildConfiguration::addToEnvironment(env);

    prependCompilerPathToEnvironment(target()->kit(), env);

    auto aspect = this->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return);
    aspect->addToEnvironment(env);
}

/*!
 * \class MerCompilationDatabaseBuildConfigurationFactory
 * \internal
 */

MerCompilationDatabaseBuildConfigurationFactory::MerCompilationDatabaseBuildConfigurationFactory()
{
    registerBuildConfiguration<MerCompilationDatabaseBuildConfiguration>(
            CompilationDatabaseProjectManager::Constants::COMPILATIONDATABASE_BC_ID);
    setSupportedProjectType(
            CompilationDatabaseProjectManager::Constants::COMPILATIONDATABASEPROJECT_ID);
    setSupportedProjectMimeTypeName(
            CompilationDatabaseProjectManager::Constants::COMPILATIONDATABASEMIMETYPE);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);

    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool) {
        const QString name = BuildConfiguration::tr("Release");
        ProjectExplorer::BuildInfo info;
        info.typeName = name;
        info.displayName = name;
        info.buildType = BuildConfiguration::Release;
        info.buildDirectory = projectPath.parentDir();
        return QList<BuildInfo>{info};
    });
}

/*!
 * \class MerCompilationDatabaseMakeStep
 * \internal
 */

MerCompilationDatabaseMakeStep::MerCompilationDatabaseMakeStep(BuildStepList *parent, Utils::Id id)
    : MakeStep(parent, id)
{
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        setIgnoreReturnValue(true);
        setUserArguments("clean");
    }
}

/*!
 * \class MerCompilationDatabaseMakeStepFactory
 * \internal
 */

MerCompilationDatabaseMakeStepFactory::MerCompilationDatabaseMakeStepFactory()
{
    registerStep<MerCompilationDatabaseMakeStep>(Constants::MER_COMPILATION_DATABASE_MS_ID);
    setDisplayName(MakeStep::defaultDisplayName());
    setSupportedProjectType(
            CompilationDatabaseProjectManager::Constants::COMPILATIONDATABASEPROJECT_ID);
}

} // namespace Internal
} // namespace Mer
