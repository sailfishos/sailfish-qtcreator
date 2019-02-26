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
#include <projectexplorer/target.h>
#include <remotelinux/remotelinuxenvironmentaspect.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Mer {
namespace Internal {

MerRunConfiguration::MerRunConfiguration(Target *target, Core::Id id)
    : RemoteLinuxRunConfiguration(target, id)
{
    connect(target, &Target::activeDeployConfigurationChanged,
            this, &MerRunConfiguration::updateEnabledState);
}

QString MerRunConfiguration::disabledReason() const
{
    if (m_disabledReason.isEmpty())
        return RemoteLinuxRunConfiguration::disabledReason();
    else
        return m_disabledReason;
}

void MerRunConfiguration::updateEnabledState()
{   
    //TODO Hack

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    if (target()->kit())
    {
        if (conf->id() == MerMb2RpmBuildConfiguration::configurationId()) {
            m_disabledReason = tr("This deployment method does not support run configuration");
            setEnabled(false);
            return;
        }
    }

    RemoteLinuxRunConfiguration::updateEnabledState();
}

Runnable MerRunConfiguration::runnable() const
{
    auto r = RemoteLinuxRunConfiguration::runnable();
    // required by qtbase not to direct logs to journald
    // for Qt < 5.4
    r.environment.appendOrSet(QLatin1String("QT_NO_JOURNALD_LOG"), QLatin1String("1"));
    // for Qt >= 5.4
    r.environment.appendOrSet(QLatin1String("QT_LOGGING_TO_CONSOLE"), QLatin1String("1"));

    auto merAspect = aspect<MerRunConfigurationAspect>();
    merAspect->applyTo(&r);

    DeployConfiguration* conf = target()->activeDeployConfiguration();
    QTC_ASSERT(conf, return r);;

    if (conf->id() == MerRsyncDeployConfiguration::configurationId()) {
        QString projectName = target()->project()->displayName();
        r.executable = QLatin1String("/opt/sdk/") + projectName + r.executable;
    }

    if (conf->id() == MerRpmDeployConfiguration::configurationId()) {
        //TODO:
        //r.executable = ...;
    }

    return r;
}

} // Internal
} // Mer
