/****************************************************************************
**
** Copyright (C) 2012 - 2017 Jolla Ltd.
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

namespace {
const int GDB_SERVER_READY_TIMEOUT_MS = 10000;
}

class GdbServerReadyWatcher : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit GdbServerReadyWatcher(ProjectExplorer::RunControl *runControl,
                             GdbServerPortsGatherer *gdbServerPortsGatherer)
        : RunWorker(runControl)
        , m_gdbServerPortsGatherer(gdbServerPortsGatherer)
    {
        setDisplayName("GdbServerReadyWatcher");

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
        m_startTime.restart();
    }

    void handlePortListReady()
    {
        if (!m_usedPortsGatherer.usedPorts().contains(m_gdbServerPortsGatherer->gdbServerPort())) {
            if (m_startTime.elapsed() > GDB_SERVER_READY_TIMEOUT_MS) {
                reportFailure(tr("Timeout waiting for gdbserver to become ready."));
                return;
            }
            m_usedPortsGatherer.start(device());
        }
        reportDone();
    }

    GdbServerPortsGatherer *m_gdbServerPortsGatherer;
    DeviceUsedPortsGatherer m_usedPortsGatherer;
    QTime m_startTime;
};

MerDeviceDebugSupport::MerDeviceDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setDisplayName("MerDeviceDebugSupport");

    m_portsGatherer = new GdbServerPortsGatherer(runControl);
    m_portsGatherer->setUseGdbServer(isCppDebugging());
    m_portsGatherer->setUseQmlServer(isQmlDebugging());

    auto gdbServer = new GdbServerRunner(runControl, m_portsGatherer);
    gdbServer->addStartDependency(m_portsGatherer);

    addStartDependency(gdbServer);

    setStartMode(AttachToRemoteServer);
    setCloseMode(KillAndExitMonitorAtClose);
    setUseExtendedRemote(true);

    if (isCppDebugging()) {
        auto gdbServerReadyWatcher = new GdbServerReadyWatcher(runControl, m_portsGatherer);
        gdbServerReadyWatcher->addStartDependency(gdbServer);
        addStartDependency(gdbServerReadyWatcher);
    }

    RunConfiguration *runConfig = runControl->runConfiguration();
    if (auto rc = qobject_cast<MerRunConfiguration *>(runConfig))
        setSymbolFile(rc->localExecutableFilePath());
    else if (auto rc = qobject_cast<MerQmlRunConfiguration *>(runConfig))
        setSymbolFile(rc->localExecutableFilePath());

    connect(this, &DebuggerRunTool::inferiorRunning, this, [runControl]() {
        MerQmlLiveBenchManager::notifyInferiorRunning(runControl);
    });
}

void MerDeviceDebugSupport::start()
{
    RunConfiguration *runConfig = runControl()->runConfiguration();

    if (isCppDebugging()) {
        QmakeProject *project = qobject_cast<QmakeProject *>(runConfig->target()->project());
        QTC_ASSERT(project, return);
        foreach (QmakeProFile *proFile, project->allProFiles(QList<QmakeProjectManager::ProjectType>() << ProjectType::SharedLibraryTemplate))
            addSolibSearchDir(proFile->targetInformation().destDir.toString());
    }

    MerSdk* mersdk = MerSdkKitInformation::sdk(runConfig->target()->kit());

    if (mersdk && !mersdk->sharedHomePath().isEmpty())
        addSourcePathMap(QLatin1String("/home/mersdk/share"), mersdk->sharedHomePath());
    if (mersdk && !mersdk->sharedSrcPath().isEmpty())
        addSourcePathMap(QLatin1String("/home/src1"), mersdk->sharedSrcPath());

    setGdbServerChannel(m_portsGatherer->gdbServerChannel());
    setQmlServer(m_portsGatherer->qmlServer());
    addQmlServerInferiorCommandLineArgumentIfNeeded();

    DebuggerRunTool::start();
}

#include "merdevicedebugsupport.moc"
