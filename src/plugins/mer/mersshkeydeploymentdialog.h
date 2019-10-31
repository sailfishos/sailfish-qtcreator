/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#ifndef MERSSHKEYDEPLOYMENTDIALOG_H
#define MERSSHKEYDEPLOYMENTDIALOG_H

#include <remotelinux/sshkeydeployer.h>
#include <ssh/sshconnection.h>

#include <QProgressDialog>

namespace Mer {
namespace Internal {

class MerSshKeyDeploymentDialog: public QProgressDialog
{
    Q_OBJECT
public:
    MerSshKeyDeploymentDialog(QWidget *parent = 0);
    void setPublicKeyPath(const QString& file);
    void setSShParameters(const QSsh::SshConnectionParameters& parameters);
    int exec() override;
private slots:
    void handleDeploymentError(const QString &errorMsg);
    void handleDeploymentSuccess();
    void handleCanceled();
private:
    void handleDeploymentFinished(const QString &errorMsg);
private:
    RemoteLinux::SshKeyDeployer m_sshDeployer;
    QString m_publicKeyPath;
    QSsh::SshConnectionParameters m_sshParams;
    bool m_done;
};

}
}
#endif // MERSSHKEYDEPLOYMENTDIALOG_H
