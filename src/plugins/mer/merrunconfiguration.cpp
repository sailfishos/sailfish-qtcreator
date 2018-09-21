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

#include "merrunconfiguration.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merrunconfigurationaspect.h"
#include "projectexplorer/kitinformation.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Mer {
namespace Internal {

MerRunConfiguration::MerRunConfiguration(Target *parent)
    : RemoteLinuxRunConfiguration(parent, Constants::MER_RUNCONFIGURATION_PREFIX)
{
    connect(target(), &Target::activeDeployConfigurationChanged,
            this, &MerRunConfiguration::updateEnabledState);
}

QString MerRunConfiguration::disabledReason() const
{
    if(m_disabledReason.isEmpty())
        return RemoteLinuxRunConfiguration::disabledReason();
    else
        return m_disabledReason;
}

void MerRunConfiguration::updateEnabledState()
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

    RemoteLinuxRunConfiguration::updateEnabledState();
}

QString MerRunConfiguration::defaultRemoteExecutableFilePath() const
{
    DeployConfiguration* conf = target()->activeDeployConfiguration();
    if (!conf) return QString();

    QString executable = RemoteLinuxRunConfiguration::defaultRemoteExecutableFilePath();
    if (conf->id() == MerRsyncDeployConfiguration::configurationId()) {
        QString projectName = target()->project()->displayName();
        return QLatin1String("/opt/sdk/") + projectName + executable;
    }

    if (conf->id() == MerRpmDeployConfiguration::configurationId()) {
        //TODO:
        return executable;
    }

    return executable;
}

Runnable MerRunConfiguration::runnable() const
{
    auto r = RemoteLinuxRunConfiguration::runnable().as<StandardRunnable>();
    // required by qtbase not to direct logs to journald
    // for Qt < 5.4
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    // for Qt >= 5.4
    r.environment.appendOrSet(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));

    auto merAspect = extraAspect<MerRunConfigurationAspect>();
    merAspect->applyTo(&r);

    return r;
}

bool MerRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RemoteLinuxRunConfiguration::fromMap(map))
        return false;

    setDefaultDisplayName(defaultDisplayName());
    return true;
}

QString MerRunConfiguration::defaultDisplayName() const
{
    if (!buildSystemTarget().isEmpty())
        //: %1 is the name of a project which is being run on Mer device
        return tr("%1 (on Sailfish OS Device)").arg(buildSystemTarget());
    //: Mer run configuration default display name
    return tr("Run on Sailfish OS Device");
}

} // Internal
} // Mer
