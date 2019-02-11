/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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
#include "merqmlrunconfigurationwidget.h"
#include "merrunconfigurationaspect.h"
#include "mersdk.h"
#include "mersdkkitinformation.h"
#include "mertargetkitinformation.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace RemoteLinux;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const char SAILFISHAPP_ENABLE_QML_DEBUGGING[] = "SAILFISHAPP_ENABLE_QML_DEBUGGING";
} // anonymous namespace

MerQmlRunConfiguration::MerQmlRunConfiguration(Target *target, Core::Id id)
    : RunConfiguration(target, id)
{
    addExtraAspect(new RemoteLinuxEnvironmentAspect(target));
    connect(target, &Target::activeDeployConfigurationChanged,
            this, &MerQmlRunConfiguration::updateEnabledState);
}

QString MerQmlRunConfiguration::disabledReason() const
{
    if(m_disabledReason.isEmpty())
        return RunConfiguration::disabledReason();
    else
        return m_disabledReason;
}

void MerQmlRunConfiguration::updateEnabledState()
{
    //TODO Hack

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    if(target()->kit())
    {
        if (conf->id() == MerMb2RpmBuildConfiguration::configurationId()) {
            m_disabledReason = tr("This deployment method does not support run configuration");
            setEnabled(false);
            return;
        }
    }

    RunConfiguration::updateEnabledState();
}

Runnable MerQmlRunConfiguration::runnable() const
{
    auto project = qobject_cast<QmakeProject *>(target()->project());
    const QString appName{project->rootProFile()->targetInformation().target};

    Runnable r;
    r.environment = extraAspect<RemoteLinuxEnvironmentAspect>()->environment();
    r.executable = QLatin1String(Constants::SAILFISH_QML_LAUNCHER);
    r.commandLineArguments = appName;

    // required by qtbase not to direct logs to journald
    // for Qt < 5.4
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    // for Qt >= 5.4
    r.environment.appendOrSet(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));

    auto debuggerAspect = extraAspect<DebuggerRunConfigurationAspect>();
    if (debuggerAspect->useQmlDebugger())
        r.environment.set(QLatin1String(SAILFISHAPP_ENABLE_QML_DEBUGGING), QLatin1String("1"));

    auto merAspect = extraAspect<MerRunConfigurationAspect>();
    merAspect->applyTo(&r);

    return r;
}

QWidget *MerQmlRunConfiguration::createConfigurationWidget()
{
    return new MerQmlRunConfigurationWidget(this);
}

QString MerQmlRunConfiguration::localExecutableFilePath() const
{
    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());
    QTC_ASSERT(merSdk, return {});
    const QString merTargetName = MerTargetKitInformation::targetName(target()->kit());
    QTC_ASSERT(!merTargetName.isEmpty(), return {});

    const QString path = merSdk->sharedTargetsPath() + QLatin1Char('/') + merTargetName +
        QLatin1String(Constants::SAILFISH_QML_LAUNCHER);
    return QDir::cleanPath(path);
}

} // Internal
} // Mer
