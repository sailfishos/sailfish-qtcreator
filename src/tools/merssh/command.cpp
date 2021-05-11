/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#include "command.h"

#include <mer/merconstants.h>
#include <sfdk/sfdkconstants.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#ifdef Q_OS_WIN
# include <windows.h>
#else
# include <QSocketNotifier>
# include <signal.h>
# include <sys/socket.h>
# include <unistd.h>
#endif

using namespace Utils;

namespace {

class SignalHandler : public QObject
{
    Q_OBJECT

public:
    SignalHandler();
    ~SignalHandler();

signals:
    void terminate();

private:
    void setUp();
    void restore();

    static SignalHandler *instance;

#ifdef Q_OS_WIN
    static BOOL WINAPI ctrlHandler(DWORD fdwCtrlType);
#else
    static void signalHandler(int signum);
    int signalFd[2] = {-1, -1};
    std::unique_ptr<QSocketNotifier> signalNotifier;
#endif
};

} // namespace anonymous

Command::Command()
{
}

Command::~Command()
{
}

int Command::executeSfdk(const QStringList &arguments)
{
    QTextStream qerr(stderr);

    static const FilePath program = FilePath::fromString(QCoreApplication::applicationDirPath())
        .resolvePath(RELATIVE_REVERSE_LIBEXEC_PATH)
        .pathAppended("sfdk" QTC_HOST_EXE_SUFFIX);

    qerr << "+ " << program.toUserOutput() << ' ' << QtcProcess::joinArgs(arguments, Utils::OsTypeLinux) << endl;

    QtcProcess process;
    process.setUseCtrlCStub(Utils::HostOsInfo::isWindowsHost());
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setCommand({program, arguments});

    SignalHandler signalHandler;
    connect(&signalHandler, &SignalHandler::terminate,
            &process, &QtcProcess::terminate);

    QEventLoop loop;
    connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            &loop, &QEventLoop::quit);

    process.start();
    if (!process.waitForStarted(-1))
        return -2; // like QProcess::execute does

    loop.exec();

    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

QString Command::sdkToolsPath() const
{
    return m_toolsPath;
}

void Command::setSdkToolsPath(const QString& path)
{
    m_toolsPath = QDir::fromNativeSeparators(path);
}

void Command::setArguments(const QStringList &args)
{
    m_args = args;
}

QStringList Command::arguments() const
{
    return m_args;
}

bool Command::isValid() const
{
    return true;
}

/*!
 * \class SignalHandler
 * \internal
 */

SignalHandler *SignalHandler::instance = nullptr;

SignalHandler::SignalHandler()
{
    Q_ASSERT(instance == nullptr);
    instance = this;
    setUp();
}

SignalHandler::~SignalHandler()
{
    instance = nullptr;
    restore();
}

#ifdef Q_OS_WIN
void SignalHandler::setUp()
{
    if (!SetConsoleCtrlHandler(ctrlHandler, TRUE))
        qFatal("merssh: Failed to set up console signal handling");
}

void SignalHandler::restore()
{
    if (!SetConsoleCtrlHandler(ctrlHandler, FALSE))
        qFatal("merssh: Failed to restore console signal handling");
}

BOOL WINAPI SignalHandler::ctrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        emit instance->terminate();
        return TRUE;

    // The CTRL_LOGOFF_EVENT and CTRL_SHUTDOWN_EVENT even handling is likely a
    // dead code. With Windows 7 and later, if a console application loads the
    // gdi32.dll or user32.dll library, the HandlerRoutine function does not get
    // called for these events.
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        emit instance->terminate();
        // The handler is run in a new thread. The process would be terminated
        // immediately after returning.
        Sleep(INFINITE);
        return TRUE;

    default:
        return FALSE;
    }
}

#else

void SignalHandler::setUp()
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd) != 0)
        qFatal("merssh: Failed to create socket pair for signal handling");

    signalNotifier = std::make_unique<QSocketNotifier>(signalFd[1], QSocketNotifier::Read);
    QObject::connect(signalNotifier.get(), &QSocketNotifier::activated, [this]() {
        signalNotifier->setEnabled(false);

        int signum;
        if (read(signalFd[1], &signum, sizeof(signum)) != sizeof(signum))
            qFatal("merssh: Failed to read signal number");

        switch (signum) {
        case SIGINT:
        case SIGHUP:
        case SIGTERM:
            emit terminate();
            break;

        default:
            qFatal("merssh: Read unexpected signal number");
        }

        signalNotifier->setEnabled(true);
    });

    struct sigaction action;
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &action, nullptr))
        qFatal("merssh: Failed to set up SIGHUP handling");
    if (sigaction(SIGINT, &action, nullptr))
        qFatal("merssh: Failed to set up SIGINT handling");
    if (sigaction(SIGTERM, &action, nullptr))
        qFatal("merssh: Failed to set up SIGTERM handling");
}

void SignalHandler::restore()
{
    if (signal(SIGHUP, SIG_DFL) == SIG_ERR)
        qFatal("merssh: Failed to restore original SIGHUP handling");
    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        qFatal("merssh: Failed to restore original SIGINT handling");
    if (signal(SIGTERM, SIG_DFL) == SIG_ERR)
        qFatal("merssh: Failed to restore original SIGTERM handling");

    close(signalFd[0]), signalFd[0] = -1;
    close(signalFd[1]), signalFd[1] = -1;
}

void SignalHandler::signalHandler(int signum)
{
    if (write(instance->signalFd[0], &signum, sizeof(signum)) != sizeof(signum))
        qFatal("merssh: Failed to write signal number");
}

#endif

#include "command.moc"
