/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "session.h"

#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QProcess>
#include <QRegularExpression>

#ifdef Q_OS_WIN
# include <windows.h>
# include <tlhelp32.h>
#endif

#ifdef Q_OS_UNIX
# include <unistd.h>
#endif

using namespace Sfdk;
using namespace Utils;

namespace {
const int SESSION_FIELD = 5;
const int STARTTIME_FIELD = 22;
const char SESSION_ID_DELIMITER = '.';
}

Session *Session::s_instance = nullptr;

Session::Session()
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    if (HostOsInfo::hostOs() == OsTypeWindows) {
        if (!qEnvironmentVariableIsSet(Constants::MSYS_DETECTION_ENV_VAR)) {
            qerr() << tr("Not running under MSYS shell. Session management is not possible.") << endl;
            return;
        }
    }

    m_id = getSessionId();
    if (m_id.isEmpty())
        qCWarning(sfdk) << "Error retrieving process information. Session management is not possible.";
}

Session::~Session()
{
    s_instance = nullptr;
}

bool Session::isValid()
{
    return !s_instance->m_id.isEmpty();
}

QString Session::id()
{
    QTC_ASSERT(isValid(), return QString());
    return s_instance->m_id;
}

bool Session::isSessionActive(const QString &id)
{
    QTC_ASSERT(!id.isEmpty(), return false);
    QString processId = id.left(id.indexOf(SESSION_ID_DELIMITER));
    return getSessionId(processId) == id;
}

QString Session::getSessionId(const QString &processId)
{
    QTC_CHECK(!processId.isEmpty() || processId.isNull());

#ifdef Q_OS_WIN
    const QString cygwinPid = processId.isNull()
        ? firstCygwinAncestorPid()
        : processId;
    if (cygwinPid.isEmpty()) {
        qCWarning(sfdk) << "Error determining first cygwin process ancestor";
        return {};
    }

    const QString sessionId = procfsStat(cygwinPid, SESSION_FIELD, !processId.isNull());
    if (sessionId.isEmpty()) {
        if (processId.isNull())
            qCWarning(sfdk) << "Error retrieving own session leader ID";
        return {};
    }
#else
    auto pidFromString = [](const QString &string) -> pid_t {
        bool ok = true;
        const qlonglong pid = string.toLongLong(&ok);
        QTC_ASSERT(ok, return -1);
        QTC_ASSERT(pid == static_cast<pid_t>(pid), return -1);
        return static_cast<pid_t>(pid);
    };

    const pid_t processId_ = processId.isNull()
        ? ::getpid()
        : pidFromString(processId);
    if (processId_ == -1) {
        qCWarning(sfdk) << "Error converting" << processId << "to pid_t";
        return {};
    }

    const pid_t sessionId_ = ::getsid(processId_);
    if (sessionId_ == -1) {
        if (processId.isNull())
            qCWarning(sfdk) << "Error retrieving own session leader ID";
        return {};
    }

    const QString sessionId = QString::number(sessionId_);
#endif

    qCDebug(sfdk) <<  "Session leader ID:" << sessionId;

#ifdef Q_OS_MACOS
    QProcess getSessionStartTime;
    getSessionStartTime.start("ps", {"-p", sessionId, "-o", "lstart="}, QIODevice::ReadOnly);
    if (!finishedOk(&getSessionStartTime)) {
        qCWarning(sfdk) << "Error getting session leader process start time:"
            << getSessionStartTime.readAllStandardError().constData();
        return {};
    }

    const QString sessionStartTime =
        QString::fromLatin1(getSessionStartTime.readAllStandardOutput());
#else
    const QString sessionStartTime = procfsStat(sessionId, STARTTIME_FIELD, false);
    if (sessionStartTime.isEmpty()) {
        qCWarning(sfdk) << "Error gerring session leader process start time";
        return {};
    }
#endif

    return sessionId + SESSION_ID_DELIMITER + sessionStartTime;
}

bool Session::finishedOk(QProcess *process)
{
    if (!process->waitForFinished()
        || process->exitStatus() != QProcess::NormalExit
        || process->exitCode() != 0) {
        qCDebug(sfdk) << "Process" << process->program() << process->arguments()
            << "exited with error" << process->error()
            << "exitStatus" << process->exitStatus()
            << "exitCode" << process->exitCode();
        return false;
    }
    return true;
};

#ifdef Q_OS_WIN
struct Session::WindowsPidToCygwinPid
{
    WindowsPidToCygwinPid()
    {
        QProcess ps;
        ps.start("ps");
        if (!finishedOk(&ps)) {
            qCWarning(sfdk) << "Error listing cygwin processes:"
                << ps.readAllStandardError().constData();
            return;
        }
        QTextStream in(&ps);

        const QStringList header = in.readLine().split(QRegularExpression("\\s+"),
                QString::SkipEmptyParts);
        if (header.isEmpty()) {
            qCWarning(sfdk) << "No header in cygwin process list";
            return;
        }
        if (header.count() < 4 || header.at(0) != "PID" || header.at(3) != "WINPID") {
            qCWarning(sfdk) << "Unexpected columns in cygwin process list:" << header;
            return;
        }

        const QRegularExpression leadingStatusInformation("^[^0-9]+");
        while (!in.atEnd()) {
            QString line = in.readLine();
            line.remove(leadingStatusInformation);
            QTextStream lineIn(&line, QIODevice::ReadOnly);
            QString ignored;
            QString cygwinPid;
            DWORD windowsPid;
            lineIn >> cygwinPid >> ignored >> ignored >> windowsPid;
            if (lineIn.status() != QTextStream::Ok) {
                qCWarning(sfdk) << "Error reading cygwin process list:" << lineIn.status();
                return;
            }
            map.insert(windowsPid, cygwinPid);
        }
        isValid = true;
    }

    bool isValid = false;
    QHash<DWORD, QString> map;
};

QString Session::firstCygwinAncestorPid()
{
#if 0
    QProcess::execute("ps -W");
    QProcess::execute("wmic process get Name,ProcessId,ParentProcessId");
#endif

    WindowsPidToCygwinPid windowsPidToCygwinPid;
    if (!windowsPidToCygwinPid.isValid)
        return {};

    QHash<DWORD, DWORD> parentPids;

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        qCWarning(sfdk) << "CreateToolhelp32Snapshot failed:" << GetLastError();
        return {};
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        qCWarning(sfdk) << "Process32First failed:" << GetLastError();
        CloseHandle(hProcessSnap);
        return {};
    }

    do {
        parentPids.insert(pe32.th32ProcessID, pe32.th32ParentProcessID);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    auto parentPidIt = parentPids.constFind(GetCurrentProcessId());
    if (parentPidIt == parentPids.constEnd()) {
        qCWarning(sfdk) << "Error retrieving current process info";
        return {};
    }

    DWORD parentPid = *parentPidIt;
    while (parentPid > 0) {
        const QString cygwinPid = windowsPidToCygwinPid.map.value(parentPid);
        if (!cygwinPid.isNull()) {
            qCDebug(sfdk) << "First cygwin ancestor:" << cygwinPid;
            return cygwinPid;
        }
        auto parentPidIt = parentPids.constFind(parentPid);
        QTC_ASSERT(parentPidIt != parentPids.constEnd(), return 0);
        parentPid = *parentPidIt;
    }

    return {};
};
#endif // Q_OS_WIN

QString Session::procfsStat(const QString &pid, int field, bool silent)
{
    QProcess catStat;
    catStat.start("cat", {"/proc/" + pid + "/stat"}, QIODevice::ReadOnly);
    if (!finishedOk(&catStat)) {
        if (!silent)
            qCWarning(sfdk) << "Error reading cygwin process status:"
                << catStat.readAllStandardError().constData();
        return {};
    }

    const QStringList stat = QString::fromLatin1(catStat.readAllStandardOutput()).split(' ');
    qCDebug(sfdk) << "Process status:" << stat;

    if (stat.count() < field + 1) {
        qCWarning(sfdk) << "Read corrupted cygwin process status";
        return {};
    }

    return stat.at(field);
}
