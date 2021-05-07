/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "debugger.h"

#include "remoteprocess.h"
#include "sfdkconstants.h"
#include "sdkmanager.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/sfdkconstants.h>

#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>

using namespace Sfdk;
using namespace Utils;

namespace {
const char GDB_SERVER_CONNECTION_ESTABLISHED_MESSAGE[] = "Remote debugging from host";
const int PORT_SCAN_TIMEOUT_MS = 4000;
const int GDB_SERVER_READY_TIMEOUT_MS = 4000;
const int GDB_SERVER_EXIT_TIMEOUT_MS = 4000;
const char ELF_MAGIC_NUMBER[] = "\x7F" "ELF";
const int ELF_MAGIC_NUMBER_LENGTH = 4; // TODO C++17: Use std::size
}

namespace Sfdk {

class GdbServerRunner
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(Sfdk::GdbServerRunner)

    enum State {
        NotStarted,
        Started,
        Connected,
    };

public:
    GdbServerRunner()
    {
        m_remoteProcess.enableLogAllOutput(sfdk, "gdbserver");

        QObject::connect(&m_remoteProcess, &RemoteProcess::finished, [this]() {
            m_state = NotStarted;
        });
        QObject::connect(&m_remoteProcess, &RemoteProcess::standardError,
                [this](const QByteArray &data) {
            if (data.contains(GDB_SERVER_CONNECTION_ESTABLISHED_MESSAGE))
                m_state = Connected;
        });
    }

    ~GdbServerRunner()
    {
        if (m_state == NotStarted)
            return;

        // For "--once" to take any effect GDB must connect successfully
        if (m_state != Connected) {
            // Give it some time - it might be just under load and terminating
            // it is unnecessary verbose because gdbserver does not handle SIGTERM
            QTimer::singleShot(GDB_SERVER_EXIT_TIMEOUT_MS, &m_remoteProcess,
                    &RemoteProcess::terminate);
        }

        QEventLoop loop;
        QObject::connect(&m_remoteProcess, &RemoteProcess::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(GDB_SERVER_EXIT_TIMEOUT_MS * 2, &loop, []() {
            qCInfo(sfdk).noquote() << tr("Waiting for the on-device gdbserver process to exit…");
        });
        loop.exec();
    }

    void setDryRunEnabled(bool enabled) { m_dryRunEnabled = enabled; }
    void setExecutable(const QString &executable) { m_executable = executable; }
    void setExtraArgs(const QStringList &args) { m_extraArgs = args; }

    bool start(const Device &device, const QString &workingDirectory, Port *port)
    {
        QTC_ASSERT(port, return false);
        QTC_ASSERT(m_state == NotStarted, return false);
        QTC_ASSERT(!m_executable.isEmpty(), return false);

        if (!selectPort(device, port)) {
            qerr() << tr("Failed to detect a free port for gdbserver") << endl;
            return false;
        }

        const QStringList initialArgs{"--multi", "--once"};
        const QStringList commArg{":" + port->toString()};

        if (m_dryRunEnabled) {
            const QStringList args(initialArgs + m_extraArgs + commArg);
            if (!workingDirectory.isEmpty())
                qout() << "cd " << QtcProcess::quoteArgUnix(workingDirectory) << " && ";
            qout() << m_executable << ' ' << QtcProcess::joinArgs(args, Utils::OsTypeLinux) << endl;
            return true;
        }

        m_remoteProcess.setProgram(m_executable);
        m_remoteProcess.setArguments(initialArgs + m_extraArgs + commArg);
        if (!workingDirectory.isEmpty())
            m_remoteProcess.setWorkingDirectory(workingDirectory);
        m_remoteProcess.setRunInTerminal(false);
        m_remoteProcess.setStandardOutputLineBuffered(true);
        if (!qEnvironmentVariableIsSet(Constants::NO_BYPASS_OPENSSL_ARMCAP_ENV_VAR)) {
            QProcessEnvironment extraEnvironment;
            extraEnvironment.insert("OPENSSL_armcap", "1");
            m_remoteProcess.setExtraEnvironment(extraEnvironment);
        }
        if (!SdkManager::prepareForRunOnDevice(device, &m_remoteProcess))
            return false;

        qCDebug(sfdk) << "Starting remote gdbserver" << m_remoteProcess.program() << "with arguments"
            << m_remoteProcess.arguments();

        m_state = Started;
        m_remoteProcess.start();

        return waitForReady(device, *port);
    }

private:
    bool selectPort(const Device &device, Port *port)
    {
        const QString scannerScript(R"(
            port_is_used() {
                local port=$1
                local regex=$(printf '^[^:]*: [[:xdigit:]]*:%04x ' "$port")
                grep -q -e "$regex" /proc/net/tcp*
            }
            # Ensure it really works - assume port 22 is always open
            if ! port_is_used 22; then
                echo "Unable to detect open ports" >&2
                exit 1
            fi
            expand_port_list() {
                local port_list=$1
                local min= max=
                printf '%s\n' ${port_list//,/ } |while IFS=- read min max; do
                    seq ${min?} ${max:-$min}
                done
            }
            expand_port_list "$1" |while read port; do
                ! port_is_used "$port" && echo "$port" && break
            done \
            |grep .
        )");

        const QString freePorts = device.freePorts().toString();

        RemoteProcess scanner;
        scanner.setProgram("/bin/sh");
        scanner.setArguments(QStringList{"-c", scannerScript, scanner.program(), freePorts});
        scanner.setRunInTerminal(false);
        scanner.enableLogAllOutput(sfdk, "port-scanner");
        if (!SdkManager::prepareForRunOnDevice(device, &scanner))
            return false;

        QTimer::singleShot(PORT_SCAN_TIMEOUT_MS, &scanner, []() {
            qCInfo(sfdk).noquote() << tr("Scanning for free port for gdbserver…");
        });

        QObject::connect(&scanner, &RemoteProcess::standardOutput, [port](const QByteArray &data) {
            bool ok;
            int number = data.toInt(&ok);
            QTC_ASSERT(ok, return);
            *port = Port(number);
            QTC_CHECK(port->isValid());
        });

        return scanner.exec() == EXIT_SUCCESS;
    }

    bool waitForReady(const Device &device, Port port)
    {
        const QString watcherScript(R"(
            port_is_used() {
                local port=$1
                local regex=$(printf '^[^:]*: [[:xdigit:]]*:%04x ' "$port")
                grep -q -e "$regex" /proc/net/tcp*
            }
            while ! port_is_used "$1"; do
                sleep 1;
            done
        )");

        RemoteProcess watcher;
        watcher.setProgram("/bin/sh");
        watcher.setArguments(QStringList{"-c", watcherScript, watcher.program(), port.toString()});
        watcher.setRunInTerminal(false);
        watcher.enableLogAllOutput(sfdk, "ready-watcher");
        if (!SdkManager::prepareForRunOnDevice(device, &watcher))
            return false;

        QTimer::singleShot(GDB_SERVER_READY_TIMEOUT_MS, &watcher, []() {
            qCInfo(sfdk).noquote() << tr("Waiting for the on-device gdbserver process to start…");
        });

        return watcher.exec() == EXIT_SUCCESS;
    }

private:
    bool m_dryRunEnabled = false;
    QString m_executable;
    QStringList m_extraArgs;
    RemoteProcess m_remoteProcess;
    State m_state = NotStarted;
};

} // namespace Sfdk

/*!
 * \class Debugger
 */

Debugger::Debugger(const Device *device, const BuildTargetData &target)
    : m_device(device)
    , m_target(target)
{
    QTC_ASSERT(device, return);
    QTC_ASSERT(target.isValid(), return);

    m_gdbServerExecutable = "gdbserver";
    m_gdbExecutable = QCoreApplication::applicationDirPath()
        + '/' + HostOsInfo::withExecutableSuffix(m_target.gdb.toString());
}

int Debugger::execStart(const QString &executable, const QStringList &arguments,
        const QString &workingDirectory) const
{
    QList<QStringList> gdbLateInit;
    gdbLateInit << QStringList{"set", "args"} + arguments;
    return exec(executable, gdbLateInit, workingDirectory);
}

int Debugger::execAttach(const QString &executable, int pid) const
{
    QList<QStringList> gdbLateInit;
    gdbLateInit << QStringList{"attach", QString::number(pid)};
    return exec(executable, gdbLateInit, {});
}

int Debugger::execLoadCore(const QString &executable, const QString &coreFile, bool localCore) const
{
    QList<QStringList> gdbLateInit;

    QString localCoreFile;
    if (localCore) {
        localCoreFile = coreFile;
    } else {
        TemporaryFile localFile("sfdkcore-XXXXXX");
        localFile.open();
        localCoreFile = localFile.fileName();
        gdbLateInit << QStringList{"remote", "get", coreFile, localCoreFile};
    }

    gdbLateInit << QStringList{"core-file", localCoreFile};
    return exec(executable, gdbLateInit, {});
}

int Debugger::exec(const QString &executable, const QList<QStringList> &gdbLateInit,
        const QString &gdbServerWorkingDirectory) const
{
    Port gdbPort;

    GdbServerRunner gdbServer;
    gdbServer.setDryRunEnabled(m_dryRunEnabled);
    gdbServer.setExecutable(m_gdbServerExecutable);
    gdbServer.setExtraArgs(m_gdbServerExtraArgs);
    if (!gdbServer.start(*m_device, gdbServerWorkingDirectory, &gdbPort)) {
        qerr() << tr("Failed to start gdbserver on the device") << endl;
        return SFDK_EXIT_ABNORMAL;
    }

    const QString gdbRemote = "tcp:" + m_device->sshParameters().host() + ":" + gdbPort.toString();

    QList<QStringList> gdbInit;

    gdbInit << QStringList{"set", "substitute-path",
                SdkManager::engine()->sharedSrcMountPoint(),
                SdkManager::engine()->sharedSrcPath().toString()};

    gdbInit << QStringList{"set", "sysroot", m_target.sysRoot.toString()};
    gdbInit << QStringList{"set", "substitute-path", "/usr/src",
        m_target.sysRoot.pathAppended("usr/src").toString()};

    gdbInit << QStringList{"target", "extended-remote", gdbRemote};
    gdbInit << QStringList{"set", "remote", "exec-file", executable};

    const QStringList localExecutableCandidates = findLocalExecutable(executable);
    if (localExecutableCandidates.count() == 1) {
        gdbInit << QStringList{"file", localExecutableCandidates.first()};
    } else if (localExecutableCandidates.isEmpty()) {
        gdbInit << QStringList{"echo", tr("!!! Could not determine local executable. "
                "Please select some using the \"file\" command.")};
    } else {
        gdbInit << QStringList{"echo", tr("!!! Multiple candidates found for the local executable:")};
        for (const QString &candidate : localExecutableCandidates)
            gdbInit << QStringList{"echo", tr("!!!     - %1").arg(candidate)};
        gdbInit << QStringList{"echo", tr("!!! Please select one using the \"file\" command.")};
    }

    QStringList gdbArgs;
    for (const QStringList &gdbInitCommand : gdbInit + gdbLateInit) {
        const QString joined = QtcProcess::joinArgs(gdbInitCommand, Utils::OsTypeLinux);
        gdbArgs << "--init-eval-command" << joined;
    }

    gdbArgs += m_gdbExtraArgs;

    if (m_dryRunEnabled) {
        qout() << m_gdbExecutable;
        bool previousWasOption = false;
        for (const QString &arg : gdbArgs) {
            bool isOption = arg.startsWith("--");
            const char *const delim = !isOption && previousWasOption ? " " : " \\\n\t";
            qout() << delim << QtcProcess::quoteArgUnix(arg);
            previousWasOption = isOption;
        }
        qout() << endl;
        return EXIT_SUCCESS;
    }

    if (sfdk().isDebugEnabled()) {
        qCDebug(sfdk) << "Starting GDB:" << m_gdbExecutable;
        for (const QString &arg : gdbArgs)
            qCDebug(sfdk) << '\t' << arg;
    }

    QProcess gdb;
    gdb.setProgram(m_gdbExecutable);
    gdb.setArguments(gdbArgs);
    gdb.setProcessChannelMode(QProcess::ForwardedChannels);
    gdb.setInputChannelMode(QProcess::ForwardedInputChannel);

    gdb.start();

    if (!gdb.waitForStarted(-1)) {
        qerr() << tr("Error starting GDB") << endl;
        return SFDK_EXIT_ABNORMAL;
    }

    TaskManager::setCtrlCIgnored(true);

    while (gdb.state() != QProcess::NotRunning)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

    if (gdb.exitStatus() != QProcess::NormalExit) {
        // Assume GDB was already verbose enough
        return SFDK_EXIT_ABNORMAL;
    }
    return gdb.exitCode();
}

QStringList Debugger::findLocalExecutable(const QString &remoteExecutable) const
{
    const QString executableFileName = QFileInfo(remoteExecutable).fileName();
    const QString buildDir = QDir::currentPath();

    QStringList candidates;

    qCDebug(sfdk) << "Searching for the local executable";

    QDirIterator it(buildDir, {executableFileName}, QDir::Files | QDir::NoSymLinks,
            QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString candidate = it.next();
        if (!isElfExecutable(candidate)) {
            qCDebug(sfdk) << "Does not seem to be an ELF executable, ignoring" << candidate;
            continue;
        }
        candidates << candidate;
    }

    return candidates;
}

bool Debugger::isElfExecutable(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(sfdk).noquote() << tr("Failed to open file for reading: %1").arg(fileName);
        return false;
    }

    const QByteArray header = file.read(ELF_MAGIC_NUMBER_LENGTH);
    if (header.size() < ELF_MAGIC_NUMBER_LENGTH) {
        qCDebug(sfdk) << "Could not read enough bytes to fit an ELF magic number:" << fileName;
        return false;
    }

    return header == ELF_MAGIC_NUMBER;
}
