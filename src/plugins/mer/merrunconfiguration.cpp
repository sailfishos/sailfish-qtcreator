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

static const char LIST_SEP[] = ":";
static const char QMLLIVE_SAILFISH_PRELOAD[] = "/usr/lib/qmllive-sailfish/libsailfishapp-preload.so";

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

    auto merAspect = extraAspect<MerRunConfigurationAspect>();
    if (merAspect->isQmlLiveEnabled()) {
        r.environment.appendOrSet(QLatin1String("LD_PRELOAD"),
                                  QLatin1String(QMLLIVE_SAILFISH_PRELOAD),
                                  QLatin1String(LIST_SEP));

        if (merAspect->qmlLiveIpcPort() > 0) {
            r.environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_IPC_PORT"),
                              QString::number(merAspect->qmlLiveIpcPort()));
        }

        if (!merAspect->qmlLiveTargetWorkspace().isEmpty()) {
            r.environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_WORKSPACE"),
                              merAspect->qmlLiveTargetWorkspace());
        }

        if (merAspect->qmlLiveOptions() & MerRunConfigurationAspect::UpdateOnConnect) {
            r.environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_UPDATE_ON_CONNECT"),
                              QLatin1String("1"));
        }

        if (!(merAspect->qmlLiveOptions() & MerRunConfigurationAspect::UpdatesAsOverlay)) {
            r.environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_NO_UPDATES_AS_OVERLAY"),
                              QLatin1String("1"));
        }

        if (merAspect->qmlLiveOptions() & MerRunConfigurationAspect::LoadDummyData) {
            r.environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_LOAD_DUMMY_DATA"),
                              QLatin1String("1"));
        }
    }

    return r;
}

} // Internal
} // Mer
