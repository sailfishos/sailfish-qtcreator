/****************************************************************************
**
** Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
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


#pragma once

#include <projectexplorer/runcontrol.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QFuture>

namespace Android {

class AndroidDeviceInfo;

namespace Internal {

const int MIN_SOCKET_HANDSHAKE_PORT = 20001;

class AndroidRunnerWorker : public QObject
{
    Q_OBJECT
public:
    AndroidRunnerWorker(ProjectExplorer::RunWorker *runner, const QString &packageName);
    ~AndroidRunnerWorker() override;

    bool runAdb(const QStringList &args, QString *stdOut = nullptr, const QByteArray &writeData = {});
    void adbKill(qint64 pid);
    QStringList selector() const;
    void forceStop();
    void logcatReadStandardError();
    void logcatReadStandardOutput();
    void logcatProcess(const QByteArray &text, QByteArray &buffer, bool onlyError);
    void setAndroidDeviceInfo(const AndroidDeviceInfo &info);
    void setIsPreNougat(bool isPreNougat) { m_isPreNougat = isPreNougat; }
    void setIntentName(const QString &intentName) { m_intentName = intentName; }

    void asyncStart();
    void asyncStop();
    void handleJdbWaiting();
    void handleJdbSettled();

    void removeForwardPort(const QString &port);

signals:
    void remoteProcessStarted(Utils::Port debugServerPort, const QUrl &qmlServer, qint64 pid);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

private:
    void asyncStartHelper();
    bool startDebuggerServer(const QString &packageDir, const QString &debugServerFile, QString *errorStr = nullptr);
    bool deviceFileExists(const QString &filePath);
    bool packageFileExists(const QString& filePath);
    bool uploadDebugServer(const QString &debugServerFileName);
    void asyncStartLogcat();

    enum class JDBState {
        Idle,
        Waiting,
        Settled
    };
    void onProcessIdChanged(qint64 pid);
    using Deleter = void (*)(QProcess *);

    // Create the processes and timer in the worker thread, for correct thread affinity
    bool m_isPreNougat = false;
    QString m_packageName;
    QString m_intentName;
    QStringList m_beforeStartAdbCommands;
    QStringList m_afterFinishAdbCommands;
    QStringList m_amStartExtraArgs;
    qint64 m_processPID = -1;
    std::unique_ptr<QProcess, Deleter> m_adbLogcatProcess;
    std::unique_ptr<QProcess, Deleter> m_psIsAlive;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    QFuture<qint64> m_pidFinder;
    bool m_useCppDebugger = false;
    bool m_useLldb = false; // FIXME: Un-implemented currently.
    QmlDebug::QmlDebugServicesPreset m_qmlDebugServices;
    Utils::Port m_localDebugServerPort; // Local end of forwarded debug socket.
    QUrl m_qmlServer;
    JDBState m_jdbState = JDBState::Idle;
    Utils::Port m_localJdbServerPort;
    std::unique_ptr<QProcess, Deleter> m_debugServerProcess; // gdbserver or lldb-server
    std::unique_ptr<QProcess, Deleter> m_jdbProcess;
    QString m_deviceSerialNumber;
    int m_apiLevel = -1;
    QString m_extraAppParams;
    Utils::Environment m_extraEnvVars;
    QString m_debugServerPath;
    bool m_useAppParamsForQmlDebugger = false;
};

} // namespace Internal
} // namespace Android
