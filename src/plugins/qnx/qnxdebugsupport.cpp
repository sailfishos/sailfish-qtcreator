/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qnxdebugsupport.h"
#include "qnxconstants.h"
#include "qnxdevice.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"
#include "qnxqtversion.h"
#include "qnxutils.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

// QnxDebuggeeRunner

class QnxDebuggeeRunner : public ProjectExplorer::SimpleTargetRunner
{
public:
    QnxDebuggeeRunner(RunControl *runControl, GdbServerPortsGatherer *portsGatherer)
        : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
    {
        setDisplayName("QnxDebuggeeRunner");
    }

private:
    void start() override
    {
        StandardRunnable r = runnable().as<StandardRunnable>();
        QStringList arguments;
        if (m_portsGatherer->useGdbServer()) {
            Utils::Port pdebugPort = m_portsGatherer->gdbServerPort();
            r.executable = Constants::QNX_DEBUG_EXECUTABLE;
            arguments.append(pdebugPort.toString());
        }
        if (m_portsGatherer->useQmlServer()) {
            arguments.append(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                            m_portsGatherer->qmlServerPort()));
        }
        arguments.append(Utils::QtcProcess::splitArgs(r.commandLineArguments));
        r.commandLineArguments = Utils::QtcProcess::joinArgs(arguments);

        setRunnable(r);

        SimpleTargetRunner::start();
    }

    GdbServerPortsGatherer *m_portsGatherer;
};


// QnxDebugSupport

QnxDebugSupport::QnxDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl)
{
    setDisplayName("QnxDebugSupport");
    appendMessage(tr("Preparing remote side..."), Utils::LogMessageFormat);

    m_portsGatherer = new GdbServerPortsGatherer(runControl);
    m_portsGatherer->setUseGdbServer(isCppDebugging());
    m_portsGatherer->setUseQmlServer(isQmlDebugging());

    auto debuggeeRunner = new QnxDebuggeeRunner(runControl, m_portsGatherer);
    debuggeeRunner->addStartDependency(m_portsGatherer);

    auto slog2InfoRunner = new Slog2InfoRunner(runControl);
    debuggeeRunner->addStartDependency(slog2InfoRunner);

    addStartDependency(debuggeeRunner);
}

void QnxDebugSupport::start()
{
    Utils::Port pdebugPort = m_portsGatherer->gdbServerPort();

    auto runConfig = qobject_cast<QnxRunConfiguration *>(runControl()->runConfiguration());
    QTC_ASSERT(runConfig, return);
    Target *target = runConfig->target();
    Kit *k = target->kit();

    DebuggerStartParameters params;
    params.startMode = AttachToRemoteServer;
    params.useCtrlCStub = true;
    params.inferior.executable = runConfig->remoteExecutableFilePath();
    params.symbolFile = runConfig->localExecutableFilePath();
    params.remoteChannel = QString("%1:%2").arg(device()->sshParameters().host).arg(pdebugPort.number());
    params.closeMode = KillAtClose;
    params.inferior.commandLineArguments = runConfig->arguments();

    if (isQmlDebugging()) {
        params.qmlServer.host = device()->sshParameters().host;
        params.qmlServer.port = m_portsGatherer->qmlServerPort();
        params.inferior.commandLineArguments.replace("%qml_port%", params.qmlServer.port.toString());
    }

    auto qtVersion = dynamic_cast<QnxQtVersion *>(QtSupport::QtKitInformation::qtVersion(k));
    if (qtVersion) {
        params.solibSearchPath = QnxUtils::searchPaths(qtVersion);
        params.sysRoot = qtVersion->qnxTarget();
    }

    setStartParameters(params);

    DebuggerRunTool::start();
}

} // namespace Internal
} // namespace Qnx
