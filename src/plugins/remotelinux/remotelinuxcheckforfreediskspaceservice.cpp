/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "remotelinuxcheckforfreediskspaceservice.h"

#include <ssh/sshremoteprocessrunner.h>

namespace RemoteLinux {
namespace Internal {
class RemoteLinuxCheckForFreeDiskSpaceServicePrivate
{
public:
    QString pathToCheck;
    quint64 requiredSpaceInBytes;
    QSsh::SshRemoteProcessRunner *processRunner;
};
} // namespace Internal

RemoteLinuxCheckForFreeDiskSpaceService::RemoteLinuxCheckForFreeDiskSpaceService(QObject *parent)
        : AbstractRemoteLinuxDeployService(parent),
          d(new Internal::RemoteLinuxCheckForFreeDiskSpaceServicePrivate)
{
    d->processRunner = nullptr;
    d->requiredSpaceInBytes = 0;
}

RemoteLinuxCheckForFreeDiskSpaceService::~RemoteLinuxCheckForFreeDiskSpaceService()
{
    cleanup();
    delete d;
}

void RemoteLinuxCheckForFreeDiskSpaceService::setPathToCheck(const QString &path)
{
    d->pathToCheck = path;
}

void RemoteLinuxCheckForFreeDiskSpaceService::setRequiredSpaceInBytes(quint64 sizeInBytes)
{
    d->requiredSpaceInBytes = sizeInBytes;
}

void RemoteLinuxCheckForFreeDiskSpaceService::handleStdErr()
{
    emit stdErrData(QString::fromUtf8(d->processRunner->readAllStandardError()));
}

void RemoteLinuxCheckForFreeDiskSpaceService::handleProcessFinished()
{
    if (!d->processRunner->processErrorString().isEmpty()) {
        emit errorMessage(tr("Remote process failed: %1")
                          .arg(d->processRunner->processErrorString()));
        stopDeployment();
        return;

    }

    bool isNumber;
    QByteArray processOutput = d->processRunner->readAllStandardOutput();
    processOutput.chop(1); // newline
    quint64 freeSpace = processOutput.toULongLong(&isNumber);
    quint64 requiredSpaceInMegaBytes = d->requiredSpaceInBytes / (1024 * 1024);
    if (!isNumber) {
        emit errorMessage(tr("Unexpected output from remote process: \"%1\"")
                .arg(QString::fromUtf8(processOutput)));
        stopDeployment();
        return;
    }

    freeSpace /= 1024; // convert kilobyte to megabyte
    if (freeSpace < requiredSpaceInMegaBytes) {
        emit errorMessage(tr("The remote file system has only %n megabytes of free space, "
                "but %1 megabytes are required.", nullptr, freeSpace).arg(requiredSpaceInMegaBytes));
        stopDeployment();
        return;
    }

    emit progressMessage(tr("The remote file system has %n megabytes of free space, going ahead.",
                            nullptr, freeSpace));
    stopDeployment();
}

CheckResult RemoteLinuxCheckForFreeDiskSpaceService::isDeploymentPossible() const
{
    if (!d->pathToCheck.startsWith(QLatin1Char('/'))) {
        return CheckResult::failure(
           tr("Cannot check for free disk space: \"%1\" is not an absolute path.")
                    .arg(d->pathToCheck));
    }

    return AbstractRemoteLinuxDeployService::isDeploymentPossible();
}

void RemoteLinuxCheckForFreeDiskSpaceService::doDeploy()
{
    d->processRunner = new QSsh::SshRemoteProcessRunner;
    connect(d->processRunner, &QSsh::SshRemoteProcessRunner::processClosed,
            this, &RemoteLinuxCheckForFreeDiskSpaceService::handleProcessFinished);
    connect(d->processRunner, &QSsh::SshRemoteProcessRunner::readyReadStandardError,
            this, &RemoteLinuxCheckForFreeDiskSpaceService::handleStdErr);
    const QString command = QString::fromLatin1("df -k %1 |tail -n 1 |sed 's/  */ /g' "
            "|cut -d ' ' -f 4").arg(d->pathToCheck);
    d->processRunner->run(command, deviceConfiguration()->sshParameters());
}

void RemoteLinuxCheckForFreeDiskSpaceService::stopDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void RemoteLinuxCheckForFreeDiskSpaceService::cleanup()
{
    if (d->processRunner) {
        disconnect(d->processRunner, nullptr, this, nullptr);
        d->processRunner->cancel();
        delete d->processRunner;
        d->processRunner = nullptr;
    }
}

} // namespace RemoteLinux
