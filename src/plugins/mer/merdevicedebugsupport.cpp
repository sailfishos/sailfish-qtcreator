/****************************************************************************
**
** Copyright (C) 2017-2019 Jolla Ltd.
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

#include "merdevicedebugsupport.h"

#include "merconstants.h"
#include "meremulatordevice.h"
#include "merrunconfiguration.h"
#include "merrunconfigurationaspect.h"
#include "merqmllivebenchmanager.h"
#include "merqmlrunconfiguration.h"
#include "mersdkkitaspect.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"

#include <sfdk/buildengine.h>
#include <sfdk/sfdkconstants.h>

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerruncontrol.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/remotelinuxdebugsupport.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QElapsedTimer>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Mer;
using namespace Mer::Internal;
using namespace RemoteLinux;
using namespace Sfdk;
using namespace Utils;

namespace {
const int GDB_SERVER_READY_TIMEOUT_MS = 10000;
}

class GdbServerReadyWatcher : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit GdbServerReadyWatcher(ProjectExplorer::RunControl *runControl,
                             DebugServerPortsGatherer *debugServerPortsGatherer)
        : RunWorker(runControl)
        , m_debugServerPortsGatherer(debugServerPortsGatherer)
    {
        setId("GdbServerReadyWatcher");

        connect(&m_usedPortsGatherer, &DeviceUsedPortsGatherer::error,
                this, &RunWorker::reportFailure);
        connect(&m_usedPortsGatherer, &DeviceUsedPortsGatherer::portListReady,
                this, &GdbServerReadyWatcher::handlePortListReady);
    }
    ~GdbServerReadyWatcher()
    {
    }

private:
    void start() override
    {
        appendMessage(tr("Waiting for gdbserver..."), Utils::NormalMessageFormat);
        m_usedPortsGatherer.start(device());
        m_startTimer.restart();
    }

    void handlePortListReady()
    {
        const Port gdbServerPort(m_debugServerPortsGatherer->gdbServer().port());
        if (!m_usedPortsGatherer.usedPorts().contains(gdbServerPort)) {
            if (m_startTimer.elapsed() > GDB_SERVER_READY_TIMEOUT_MS) {
                reportFailure(tr("Timeout waiting for gdbserver to become ready."));
                return;
            }
            m_usedPortsGatherer.start(device());
        }
        reportDone();
    }

    DebugServerPortsGatherer *m_debugServerPortsGatherer;
    DeviceUsedPortsGatherer m_usedPortsGatherer;
    QElapsedTimer m_startTimer;
};

MerDeviceDebugSupport::MerDeviceDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setId("MerDeviceDebugSupport");

    setUsePortsGatherer(isCppDebugging(), isQmlDebugging());

    auto aspect = runControl->runConfiguration()->aspect<MerRunConfigurationAspect>();
    if (aspect->isDebugBypassOpenSslArmCapEnabled()) {
        Runnable runnable = runControl->runnable();
        runnable.environment.modify(Utils::EnvironmentItems({{"OPENSSL_armcap", "1"}}));
        runControl->setRunnable(runnable);
    }
    auto gdbServer = new DebugServerRunner(runControl, portsGatherer());

    addStartDependency(gdbServer);

    setStartMode(AttachToRemoteServer);
    setCloseMode(KillAndExitMonitorAtClose);
    setUseExtendedRemote(true);

    if (isCppDebugging()) {
        auto gdbServerReadyWatcher = new GdbServerReadyWatcher(runControl, portsGatherer());
        gdbServerReadyWatcher->addStartDependency(gdbServer);
        addStartDependency(gdbServerReadyWatcher);
    }

    connect(this, &DebuggerRunTool::inferiorRunning, this, [runControl]() {
        MerQmlLiveBenchManager::notifyInferiorRunning(runControl);
    });
}

void MerDeviceDebugSupport::start()
{
    RunConfiguration *runConfig = runControl()->runConfiguration();

    if (isCppDebugging()) {
        ProjectNode *const root = runConfig->target()->project()->rootProjectNode();
        root->forEachProjectNode([this](const ProjectNode *node) {
            auto qmakeNode = dynamic_cast<const QmakeProFileNode *>(node);
            if (!qmakeNode || !qmakeNode->includedInExactParse())
                return;
            if (qmakeNode->projectType() != ProjectType::SharedLibraryTemplate)
                return;
            auto qmakeBuildSystem = qobject_cast<QmakeBuildSystem *>(
                    runControl()->runConfiguration()->target()->buildSystem());
            QTC_ASSERT(qmakeBuildSystem, return);
            const FilePath outPwd = qmakeBuildSystem->buildDir(qmakeNode->filePath());
            const FilePath destDir = outPwd.resolvePath(qmakeNode->targetInformation().destDir.toString());
            addSolibSearchDir(destDir.toString());
        });
    }

    BuildEngine *const engine = MerSdkKitAspect::buildEngine(runConfig->target()->kit());
    QTC_ASSERT(engine, return);

    QTC_ASSERT(!engine->sharedSrcPath().isEmpty(), return);
    QTC_ASSERT(!engine->sharedSrcMountPoint().isEmpty(), return);
    addSourcePathMap(engine->sharedSrcMountPoint(), engine->sharedSrcPath().toString());

    DebuggerRunTool::start();
}

#include "merdevicedebugsupport.moc"
