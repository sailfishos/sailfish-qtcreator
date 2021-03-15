/****************************************************************************
**
** Copyright (C) 2016-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "merqmlrunconfiguration.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merrunconfigurationaspect.h"
#include "mersdkkitaspect.h"

#include <sfdk/buildengine.h>

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace RemoteLinux;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const char SAILFISHAPP_ENABLE_QML_DEBUGGING[] = "SAILFISHAPP_ENABLE_QML_DEBUGGING";
const char SAILFISHAPP_QML_CONFIG[] = "sailfishapp_qml";
} // anonymous namespace

MerQmlRunConfiguration::MerQmlRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    addAspect<RemoteLinuxEnvironmentAspect>(target);

    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setExecutable(FilePath::fromString(Constants::SAILFISH_QML_LAUNCHER));

    auto argsAspect = addAspect<ArgumentsAspect>();

    setUpdater([this, argsAspect] {
        auto buildSystem = qobject_cast<QmakeBuildSystem *>(this->target()->buildSystem());
        const QString appName{buildSystem->rootProFile()->targetInformation().target};
        // FIXME Overwrites (unlikely) user changes
        argsAspect->setArguments(appName);
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    connect(target, &Target::activeDeployConfigurationChanged, this, &RunConfiguration::update);
}

QString MerQmlRunConfiguration::disabledReason() const
{
    if (!RunConfiguration::isEnabled())
        return RunConfiguration::disabledReason();

    QTC_ASSERT(target()->kit(), return {});

    DeployConfiguration *const dc = target()->activeDeployConfiguration();
    if (dc->id() == MerMb2RpmBuildConfigurationFactory::configurationId())
        return tr("This deployment method does not support run configuration");

    return {};
}

bool MerQmlRunConfiguration::isEnabled() const
{
    if (!RunConfiguration::isEnabled())
        return false;

    QTC_ASSERT(target()->kit(), return {});

    DeployConfiguration *const dc = target()->activeDeployConfiguration();
    if (dc->id() == MerMb2RpmBuildConfigurationFactory::configurationId())
        return false;

    return true;
}

Runnable MerQmlRunConfiguration::runnable() const
{
    Runnable r = RunConfiguration::runnable();

    // required by qtbase not to direct logs to journald
    // for Qt < 5.4
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    // for Qt >= 5.4
    r.environment.appendOrSet(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));

    auto debuggerAspect = aspect<DebuggerRunConfigurationAspect>();
    if (debuggerAspect->useQmlDebugger())
        r.environment.set(QLatin1String(SAILFISHAPP_ENABLE_QML_DEBUGGING), QLatin1String("1"));

    auto merAspect = aspect<MerRunConfigurationAspect>();
    merAspect->applyTo(&r);

    return r;
}

/*!
 * \class MerQmlRunConfigurationFactory
 * \internal
 */

MerQmlRunConfigurationFactory::MerQmlRunConfigurationFactory()
    : FixedRunConfigurationFactory(RunConfiguration::tr("QML Scene"), true)
{
    registerRunConfiguration<MerQmlRunConfiguration>(Constants::MER_QMLRUNCONFIGURATION);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

bool MerQmlRunConfigurationFactory::canHandle(Target *parent) const
{
    auto qmakeBuildSystem = qobject_cast<QmakeBuildSystem *>(parent->buildSystem());
    if (!qmakeBuildSystem)
        return false;

    QmakeProFile *root = qmakeBuildSystem->rootProFile();
    if (root->projectType() != ProjectType::AuxTemplate ||
            ! root->variableValue(Variable::Config).contains(QLatin1String(SAILFISHAPP_QML_CONFIG))) {
        return false;
    }

    return true;
}

} // Internal
} // Mer
