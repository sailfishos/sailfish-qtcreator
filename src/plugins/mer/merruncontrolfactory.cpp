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
#include "merrunconfigurationaspect.h"
#include "merqmlrunconfiguration.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"
#include "mervirtualboxmanager.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerruncontrol.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/remotelinuxanalyzesupport.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Mer;
using namespace Mer::Internal;
using namespace RemoteLinux;

// Based on RemoteLinux::LinuxDeviceDebugSupport
class MerDeviceDebugSupport : public DebuggerRunTool
{
    Q_OBJECT

public:
    MerDeviceDebugSupport(RunControl *runControl)
        : DebuggerRunTool(runControl)
    {
        setDisplayName("DebugSupport");

        auto portsGatherer = new GdbServerPortsGatherer(runControl);
        portsGatherer->setUseGdbServer(isCppDebugging());
        portsGatherer->setUseQmlServer(isQmlDebugging());

        auto gdbServer = new GdbServerRunner(runControl);
        gdbServer->addDependency(portsGatherer);

        addDependency(gdbServer);
    }

protected:
    void start() override
    {
        auto portsGatherer = runControl()->worker<GdbServerPortsGatherer>();
        QTC_ASSERT(portsGatherer, reportFailure(); return);

        const QString host = device()->sshParameters().host;
        const Port gdbServerPort = portsGatherer->gdbServerPort();
        const Port qmlServerPort = portsGatherer->qmlServerPort();

        RunConfiguration *runConfig = runControl()->runConfiguration();

        QString symbolFile;
        if (auto rc = qobject_cast<MerRunConfiguration *>(runConfig))
            symbolFile = rc->localExecutableFilePath();
        else if (auto rc = qobject_cast<MerQmlRunConfiguration *>(runConfig))
            symbolFile = rc->localExecutableFilePath();
        if (symbolFile.isEmpty()) {
            //*errorMessage = tr("Cannot debug: Local executable is not set.");
            return;
        }

        DebuggerStartParameters params;
        params.startMode = AttachToRemoteServer;
        params.closeMode = KillAndExitMonitorAtClose;
        params.remoteSetupNeeded = true;

        if (isQmlDebugging()) {
            params.qmlServer.host = host;
            params.qmlServer.port = qmlServerPort;
            params.inferior.commandLineArguments.replace("%qml_port%",
                            QString::number(qmlServerPort.number()));
        }
        if (isCppDebugging()) {
            Runnable r = runnable();
            QTC_ASSERT(r.is<StandardRunnable>(), return);
            auto stdRunnable = r.as<StandardRunnable>();
            params.useExtendedRemote = true;
            params.inferior.executable = stdRunnable.executable;
            params.inferior.commandLineArguments = stdRunnable.commandLineArguments;
            if (isQmlDebugging()) {
                params.inferior.commandLineArguments.prepend(' ');
                params.inferior.commandLineArguments.prepend(
                    QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices));
            }

            params.remoteChannel = QString("%1:%2").arg(host).arg(gdbServerPort.number());
            params.symbolFile = symbolFile;

            QmakeProject *project = qobject_cast<QmakeProject *>(runConfig->target()->project());
            QTC_ASSERT(project, return 0);
            foreach (QmakeProFileNode *node, project->allProFiles(QList<QmakeProjectManager::ProjectType>() << ProjectType::SharedLibraryTemplate))
                params.solibSearchPath.append(node->targetInformation().destDir);
        }

        MerSdk* mersdk = MerSdkKitInformation::sdk(runConfig->target()->kit());

        if (mersdk && !mersdk->sharedHomePath().isEmpty())
            params.sourcePathMap.insert(QLatin1String("/home/mersdk/share"), mersdk->sharedHomePath());
        if (mersdk && !mersdk->sharedSrcPath().isEmpty())
            params.sourcePathMap.insert(QLatin1String("/home/src1"), mersdk->sharedSrcPath());

        setStartParameters(params);

        DebuggerRunTool::start();
    }
};

MerRunControlFactory::MerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool MerRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != ProjectExplorer::Constants::NORMAL_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE
            && mode != ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN
            && mode != ProjectExplorer::Constants::QML_PROFILER_RUN_MODE
//            && mode != ProjectExplorer::Constants::PERFPROFILER_RUN_MODE
            ) {
            return false;
    }

    if (!runConfiguration->isEnabled())
        return false;

    const Core::Id id = runConfiguration->id();
    if (!id.toString().startsWith(QLatin1String(Constants::MER_RUNCONFIGURATION_PREFIX)) &&
            id != Constants::MER_QMLRUNCONFIGURATION) {
        return false;
    }

    IDevice::ConstPtr dev = DeviceKitInformation::device(runConfiguration->target()->kit());
    if (dev.isNull())
        return false;

    if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE) {
        auto aspect = runConfiguration->extraAspect<DebuggerRunConfigurationAspect>();
        int portsUsed = aspect ? aspect->portsUsedByDebugger() : 0;
        if (portsUsed > dev->freePorts().count())
            return false;
    }

    return true;
}

RunControl *MerRunControlFactory::create(RunConfiguration *runConfig, Core::Id mode,
                                         QString *)
{
    QTC_ASSERT(canRun(runConfig, mode), return 0);

    const auto rcRunnable = runConfig->runnable();
    QTC_ASSERT(rcRunnable.is<StandardRunnable>(), return 0);
    const auto stdRunnable = rcRunnable.as<StandardRunnable>();

    if (mode == ProjectExplorer::Constants::NORMAL_RUN_MODE) {
        auto runControl = new RunControl(runConfig, mode);
        (void) new SimpleTargetRunner(runControl);
        return runControl;
    } else if (mode == ProjectExplorer::Constants::DEBUG_RUN_MODE
               || mode == ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN) {

        auto runControl = new RunControl(runConfig, mode);
        (void) new MerDeviceDebugSupport(runControl);
        return runControl;
    } else if (mode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE
//            || mode == ProjectExplorer::Constants::PERFPROFILER_RUN_MODE
            ) {
        auto runControl = new RunControl(runConfig, mode);
//        AnalyzerConnection connection;
//        connection.connParams =
//            DeviceKitInformation::device(runConfig->target()->kit())->sshParameters();
//        connection.analyzerHost = connection.connParams.host;
//        runControl->setConnection(connection);
//        (void) new SimpleTargetRunner(runControl);
//        (void) new PortsGatherer(runControl);
//        (void) new FifoGatherer(runControl);
//        (void) new RemoteLinuxAnalyzeSupport(runControl);
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

ProjectExplorer::IRunConfigurationAspect *MerRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    return new MerRunConfigurationAspect(rc);
}

#include "merruncontrolfactory.moc"
