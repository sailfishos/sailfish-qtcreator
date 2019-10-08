/****************************************************************************
**
** Copyright (C) 2018-2019 Jolla Ltd.
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

#include "enginectlcommand.h"

#include "merremoteprocess.h"

#include <mer/merconnection.h>
#include <mer/mervirtualboxmanager.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QTextStream>

namespace {
    const char COMMAND_EXEC[] = "exec";
    const char COMMAND_START[] = "start";
    const char COMMAND_STOP[] = "stop";

    const int CONNECT_TIMEOUT_MS = 120000;
    const int LOCK_DOWN_TIMEOUT_MS = 120000;

    QTextStream &qerr()
    {
        static QTextStream qerr(stderr);
        return qerr;
    }
}

using Mer::MerConnection;
using Mer::Internal::MerVirtualBoxManager;
using namespace Utils;

class LogOnlyUi : public MerConnection::Ui
{
    Q_OBJECT

public:
    using MerConnection::Ui::Ui;

    void warn(Warning which) override
    {
        switch (which) {
        case Ui::AlreadyConnecting:
            QTC_CHECK(false);
            break;
        case Ui::AlreadyDisconnecting:
            QTC_CHECK(false);
            break;
        case Ui::UnableToCloseVm:
            qerr() << tr("Timeout waiting for the \"%1\" virtual machine to close.")
                .arg(connection()->virtualMachine());
            break;
        case Ui::VmNotRegistered:
            qerr() << tr("No virtual machine with the name \"%1\" found. Check your installation.")
                .arg(connection()->virtualMachine());
            break;
        }
    }

    void dismissWarning(Warning which) override
    {
        Q_UNUSED(which);
    }

    bool shouldAsk(Question which) const override
    {
        Q_UNUSED(which);
        return false;
    }

    void ask(Question which, OnStatusChanged onStatusChanged) override
    {
        switch (which) {
        case Ui::StartVm:
            QTC_CHECK(false);
            break;
        case Ui::ResetVm:
            QTC_CHECK(false);
            break;
        case Ui::CloseVm:
            QTC_CHECK(false);
            break;
        case Ui::CancelConnecting:
            QTC_CHECK(!m_cancelConnectingTimer);
            m_cancelConnectingTimer = startTimer(CONNECT_TIMEOUT_MS, onStatusChanged,
                tr("Timeout connecting to the \"%1\" virtual machine.")
                    .arg(connection()->virtualMachine()));
            break;
        case Ui::CancelLockingDown:
            QTC_CHECK(!m_cancelLockingDownTimer);
            m_cancelLockingDownTimer = startTimer(LOCK_DOWN_TIMEOUT_MS, onStatusChanged,
                tr("Timeout waiting for the \"%1\" virtual machine to close.")
                    .arg(connection()->virtualMachine()));
            break;
        }
}

    void dismissQuestion(Question which) override
    {
        switch (which) {
        case Ui::StartVm:
        case Ui::ResetVm:
        case Ui::CloseVm:
            break;
        case Ui::CancelConnecting:
            delete m_cancelConnectingTimer;
            break;
        case Ui::CancelLockingDown:
            delete m_cancelLockingDownTimer;
            break;
        }
    }

    QuestionStatus status(Question which) const override
    {
        switch (which) {
        case Ui::StartVm:
            QTC_CHECK(false);
            return NotAsked;
        case Ui::ResetVm:
            QTC_CHECK(false);
            return NotAsked;
        case Ui::CloseVm:
            QTC_CHECK(false);
            return NotAsked;
        case Ui::CancelConnecting:
            return status(m_cancelConnectingTimer);
        case Ui::CancelLockingDown:
            return status(m_cancelLockingDownTimer);
        }

        QTC_CHECK(false);
        return NotAsked;
    }

private:
    QTimer *startTimer(int timeout, OnStatusChanged onStatusChanged, const QString &timeoutMessage)
    {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, [=] {
            qerr() << timeoutMessage << endl;
            (connection()->*onStatusChanged)();
        });
        timer->setSingleShot(true);
        timer->start(timeout);
        return timer;
    }

    static QuestionStatus status(QTimer *timer)
    {
        if (!timer)
            return NotAsked;
        else if (timer->isActive())
            return Asked;
        else
            return Yes;
    }

private:
    QPointer<QTimer> m_cancelConnectingTimer;
    QPointer<QTimer> m_cancelLockingDownTimer;
};

EngineCtlCommand::EngineCtlCommand()
{
}

QString EngineCtlCommand::name() const
{
    return QLatin1String("enginectl");
}

int EngineCtlCommand::execute()
{
    const QString command = arguments().at(1);

    MerVirtualBoxManager virtualBoxManager; // singleton

    // Do not require SSH port from environment - it's not easy to be determined for the installer
    auto info = virtualBoxManager.fetchVirtualMachineInfo(engineName());
    QSsh::SshConnectionParameters sshParameters = this->sshParameters();
    sshParameters.setPort(info.sshPort);

    if (command == COMMAND_EXEC) {
        QString command = QtcProcess::joinArgs(arguments().mid(2), Utils::OsTypeLinux);
        MerRemoteProcess process;
        process.setSshParameters(sshParameters);
        process.setCommand(command);
        return process.executeAndWait();
    }

    MerConnection::registerUi<LogOnlyUi>();
    MerConnection connection(nullptr);
    connection.setVirtualMachine(engineName());
    connection.setHeadless(true);
    connection.setSshParameters(sshParameters);
    connection.refresh(MerConnection::Synchronous);

    if (!connection.connectTo(MerConnection::Block)) {
        qerr() << tr("Failed to connect to the '%1' virtual machine: %2")
            .arg(connection.virtualMachine())
            .arg(connection.errorString()) << endl;
        return 1;
    }

    if (command == COMMAND_START)
        return 0;

    if (!connection.lockDown(true)) {
        qerr() << tr("Failed to shut down the '%1' virtual machine")
            .arg(connection.virtualMachine()) << endl;
        return 1;
    }

    return 0;
}

bool EngineCtlCommand::isValid() const
{
    if (arguments().count() < 2)
        goto err;

    if (arguments().at(1) == COMMAND_START || arguments().at(1) == COMMAND_STOP) {
        if (arguments().count() != 2)
            goto err;
    } else if (arguments().at(1) == COMMAND_EXEC) {
        if (arguments().count() < 3)
            goto err;
    } else {
        goto err;
    }

    return true;

err:
    printUsage();
    return false;
}

void EngineCtlCommand::printUsage()
{
    qerr() << tr("usage") << ": enginectl {start|stop|exec command [args...]}" << endl;
}

#include "enginectlcommand.moc"
