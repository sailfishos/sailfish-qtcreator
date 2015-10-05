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

#include "merruncontrolfactory.h"

#include "merconstants.h"
#include "meremulatordevice.h"
#include "merrunconfiguration.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"
#include "mervirtualboxmanager.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerruncontrol.h>
#include <debugger/debuggerstartparameters.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/remotelinuxanalyzesupport.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <remotelinux/remotelinuxruncontrol.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Mer;
using namespace Mer::Internal;
using namespace RemoteLinux;

MerRunControlFactory::MerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool MerRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{

    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
            return false;
    }

    if (!runConfiguration->isEnabled()
            || !runConfiguration->id().toString().startsWith(
                QLatin1String(Constants::MER_RUNCONFIGURATION_PREFIX))) {
        return false;
    }

    IDevice::ConstPtr dev = DeviceKitInformation::device(runConfiguration->target()->kit());
    if (dev.isNull())
        return false;

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE) {
        const RemoteLinuxRunConfiguration * const remoteRunConfig
                = qobject_cast<RemoteLinuxRunConfiguration *>(runConfiguration);
        if (remoteRunConfig)
            return remoteRunConfig->portsUsedByDebuggers() <= dev->freePorts().count();
        return false;
    }
    return true;
}

RunControl *MerRunControlFactory::create(RunConfiguration *runConfig, Core::Id mode,
                                         QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfig, mode), return 0);

    MerRunConfiguration *rc = qobject_cast<MerRunConfiguration *>(runConfig);
    QTC_ASSERT(rc, return 0);

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE) {
        return new RemoteLinuxRunControl(rc);
    } else if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
               || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN) {
        IDevice::ConstPtr dev = DeviceKitInformation::device(rc->target()->kit());
        if (!dev) {
            *errorMessage = tr("Cannot debug: Kit has no device.");
            return 0;
        }
        if (rc->portsUsedByDebuggers() > dev->freePorts().count()) {
            *errorMessage = tr("Cannot debug: Not enough free ports available.");
            return 0;
        }
        DebuggerStartParameters params = LinuxDeviceDebugSupport::startParameters(rc);

        MerSdk* mersdk = MerSdkKitInformation::sdk(rc->target()->kit());

        if (mersdk && !mersdk->sharedHomePath().isEmpty())
            params.sourcePathMap.insert(QLatin1String("/home/mersdk/share"), mersdk->sharedHomePath());
        if (mersdk && !mersdk->sharedSrcPath().isEmpty())
            params.sourcePathMap.insert(QLatin1String("/home/src1"), mersdk->sharedSrcPath());

        DebuggerRunControl * const runControl
                = Debugger::createDebuggerRunControl(params, rc, errorMessage, mode);
        if (!runControl)
            return 0;
        LinuxDeviceDebugSupport * const debugSupport =
                new LinuxDeviceDebugSupport(rc, runControl);
        // TODO: handleDebuggingFinished is private
        connect(runControl, SIGNAL(finished()), debugSupport, SLOT(handleDebuggingFinished()));
        return runControl;
    } else if (mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE) {
        Analyzer::AnalyzerStartParameters params = RemoteLinuxAnalyzeSupport::startParameters(rc, mode);
        Analyzer::AnalyzerRunControl * const runControl = Analyzer::AnalyzerManager::createRunControl(params, runConfig);
        RemoteLinuxAnalyzeSupport * const analyzeSupport =
                new RemoteLinuxAnalyzeSupport(rc, runControl, mode);
        // TODO: handleProfilingFinished is private
        connect(runControl, SIGNAL(finished()), analyzeSupport, SLOT(handleProfilingFinished()));
        return runControl;
    } else {
        QTC_ASSERT(false, return 0);
    }

//    const MerEmulatorDevice::ConstPtr &merDevice
//            = DeviceKitInformation::device(k).dynamicCast<const MerEmulatorDevice>();
//    if (merDevice) {
//        const QString deviceIpAsSeenByBuildEngine = QString::fromLatin1("10.220.220.%1")
//                .arg(merDevice->index());
//        params.remoteChannel = deviceIpAsSeenByBuildEngine + QLatin1String(":-1");
//    }
    return 0;
}
