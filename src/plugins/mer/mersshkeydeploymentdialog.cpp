/****************************************************************************
**
** Copyright (C) 2012-2015,2019 Jolla Ltd.
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

#include "mersshkeydeploymentdialog.h"

using namespace RemoteLinux;

namespace Mer {
namespace Internal {

MerSshKeyDeploymentDialog::MerSshKeyDeploymentDialog(QWidget *parent)
    : QProgressDialog(parent),
      m_done(false)
{
    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);
    setLabelText(tr("Deploying..."));
    setValue(0);
    connect(this, &MerSshKeyDeploymentDialog::canceled,
            this, &MerSshKeyDeploymentDialog::handleCanceled);
    connect(&m_sshDeployer, &SshKeyDeployer::error,
            this, &MerSshKeyDeploymentDialog::handleDeploymentError);
    connect(&m_sshDeployer, &SshKeyDeployer::finishedSuccessfully,
            this, &MerSshKeyDeploymentDialog::handleDeploymentSuccess);
}

void MerSshKeyDeploymentDialog::handleDeploymentSuccess()
{
    handleDeploymentFinished(QString());
    setValue(1);
    m_done = true;
}

void MerSshKeyDeploymentDialog::handleDeploymentError(const QString &errorMsg)
{
    handleDeploymentFinished(errorMsg);
}

void MerSshKeyDeploymentDialog::handleDeploymentFinished(const QString &errorMsg)
{
    QString buttonText;
    const char *textColor;
    if (errorMsg.isEmpty()) {
        buttonText = tr("Deployment finished successfully.");
        textColor = "blue";
    } else {
        buttonText = errorMsg;
        textColor = "red";
    }
    setLabelText(QString::fromLatin1("<font color=\"%1\">%2</font>").arg(QLatin1String(textColor), buttonText));
    setCancelButtonText(tr("Close"));
}

void MerSshKeyDeploymentDialog::handleCanceled()
{
    disconnect(&m_sshDeployer, 0, this, 0);
    m_sshDeployer.stopDeployment();
    if (m_done)
        accept();
    else
        reject();
}

void MerSshKeyDeploymentDialog::setPublicKeyPath(const QString& file)
{
    m_publicKeyPath = file;
}

void MerSshKeyDeploymentDialog::setSShParameters(const QSsh::SshConnectionParameters& parameters)
{
    m_sshParams = parameters;
}

int MerSshKeyDeploymentDialog::exec()
{
    if (m_publicKeyPath.isEmpty() || m_sshParams.host().isEmpty())
        return QDialog::Rejected;
    m_sshDeployer.deployPublicKey(m_sshParams, m_publicKeyPath);
    return QProgressDialog::exec();
}

}
}
