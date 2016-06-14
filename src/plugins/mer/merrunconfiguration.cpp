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

MerRunConfiguration::MerRunConfiguration(Target *parent, Core::Id id,
                                         const QString &targetName)
    : RemoteLinuxRunConfiguration(parent, id, targetName)
{
    ctor();
}

MerRunConfiguration::MerRunConfiguration(Target *parent,
                                         MerRunConfiguration *source)
    : RemoteLinuxRunConfiguration(parent, source)
{
    ctor();
}

void MerRunConfiguration::ctor()
{
    connect(target(), &Target::activeDeployConfigurationChanged,
            this, &MerRunConfiguration::enabledChanged);
}

QString MerRunConfiguration::disabledReason() const
{
    if(m_disabledReason.isEmpty())
        return RemoteLinuxRunConfiguration::disabledReason();
    else
        return m_disabledReason;
}

bool MerRunConfiguration::isEnabled() const
{   
    //TODO Hack

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    if(target()->kit())
    {
        if (conf->id() == MerMb2RpmBuildConfiguration::configurationId()) {
            m_disabledReason = tr("This deployment method does not support run configuration");
            return false;
        }
    }

    return RemoteLinuxRunConfiguration::isEnabled();
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
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    return r;
}

} // Internal
} // Mer
