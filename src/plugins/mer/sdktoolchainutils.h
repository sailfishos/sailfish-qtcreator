/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#ifndef MERSDKTOOLCHAINUTILS_H
#define MERSDKTOOLCHAINUTILS_H

#include "mersdk.h"

#include <ssh/sshconnection.h>

#include <QObject>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Mer {
namespace Internal {

class MerToolChain;
class SdkToolChainUtils : public QObject
{
    Q_OBJECT
public:
    explicit SdkToolChainUtils(QObject *parent = 0);

    void setSshConnectionParameters(const QSsh::SshConnectionParameters params);

    bool checkInstalledToolChains(const QString &sdkName);

    void updateToolChains(MerSdk &sdk,const QStringList &targets);

signals:
    void installedToolChains(const QString &sdkName, const QStringList &toolChains);
    void processOutput(const QString &output);
    void connectionError();

private slots:
    void onConnectionError();
    void onReadReadyStandardOutput();
    void onReadReadyStandardError();
    void onProcessClosed(int);

private:
    MerToolChain *createToolChain(const QString &targetToolsDir);

private:
    QSsh::SshRemoteProcessRunner *m_remoteProcessRunner;
    QSsh::SshConnectionParameters m_sshParameters;
    QString m_sdkName;
    QString m_output;
};

} // Internal
} // Mer

#endif // MERSDKTOOLCHAINUTILS_H
