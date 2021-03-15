/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "mercustomrunconfiguration.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merrunconfigurationaspect.h"
#include "projectexplorer/kitinformation.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>
#include <remotelinux/remotelinuxx11forwardingaspect.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Mer {
namespace Internal {

MerCustomRunConfiguration::MerCustomRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setSettingsKey("Mer.CustomRunConfig.RemoteExecutable");
    exeAspect->setLabelText(tr("Executable on device:"));
    exeAspect->setExecutablePathStyle(OsTypeLinux);
    exeAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    exeAspect->setPlaceHolderText(tr("Remote path not set"));
    exeAspect->setHistoryCompleter("Mer.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::Any);

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setSettingsKey("Mer.CustomRunConfig.LocalExecutable");
    symbolsAspect->setLabelText(tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::PathChooserDisplay);

    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>();
    if (HostOsInfo::isAnyUnixHost())
        addAspect<TerminalAspect>();
    addAspect<RemoteLinuxEnvironmentAspect>(target);
    if (HostOsInfo::isAnyUnixHost())
        addAspect<X11ForwardingAspect>();

    setDefaultDisplayName(defaultDisplayName());
}

QString MerCustomRunConfiguration::defaultDisplayName() const
{
    const QString remoteExecutable = aspect<ExecutableAspect>()->executable().toString();
    const QString displayName = remoteExecutable.isEmpty()
            ? tr("Custom Executable")
            : tr("Run \"%1\"").arg(remoteExecutable);
    return RunConfigurationFactory::decoratedTargetName(displayName, target());
}

QString MerCustomRunConfiguration::disabledReason() const
{
    if (!RunConfiguration::isEnabled())
        return RunConfiguration::disabledReason();

    QTC_ASSERT(target()->kit(), return {});

    DeployConfiguration *const dc = target()->activeDeployConfiguration();
    if (dc->id() == MerMb2RpmBuildConfigurationFactory::configurationId())
        return tr("This deployment method does not support run configuration");

    return {};
}

bool MerCustomRunConfiguration::isEnabled() const
{
    if (!RunConfiguration::isEnabled())
        return false;

    QTC_ASSERT(target()->kit(), return {});

    DeployConfiguration *const dc = target()->activeDeployConfiguration();
    if (dc->id() == MerMb2RpmBuildConfigurationFactory::configurationId())
        return false;

    return true;
}

Tasks MerCustomRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (aspect<ExecutableAspect>()->executable().isEmpty()) {
        tasks << createConfigurationIssue(tr("The remote executable must be set in order to run "
                                             "a custom remote run configuration."));
    }
    return tasks;
}

Runnable MerCustomRunConfiguration::runnable() const
{
    Runnable r = RunConfiguration::runnable();
    const auto * const forwardingAspect = aspect<X11ForwardingAspect>();
    if (forwardingAspect)
        r.extraData.insert("Ssh.X11ForwardToDisplay", forwardingAspect->display(macroExpander()));

    // required by qtbase not to direct logs to journald
    // for Qt < 5.4
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    // for Qt >= 5.4
    r.environment.appendOrSet(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));

    auto merAspect = aspect<MerRunConfigurationAspect>();
    merAspect->applyTo(&r);

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    QTC_ASSERT(conf, return r);;

    if (conf->id() == MerRsyncDeployConfigurationFactory::configurationId()) {
        QString projectName = target()->project()->displayName();
        r.executable = FilePath::fromString("/opt/sdk/").pathAppended(projectName)
            .stringAppended(r.executable.toString());
    }

    return r;
}

/*!
 * \class MerCustomRunConfigurationFactory
 * \internal
 */

MerCustomRunConfigurationFactory::MerCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(MerCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<MerCustomRunConfiguration>(Constants::MER_CUSTOMRUNCONFIGURATION_PREFIX);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

} // Internal
} // Mer
