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

#ifndef MERSDKTARGETUTILS_H
#define MERSDKTARGETUTILS_H

#include "mersdk.h"

#include <ssh/sshconnection.h>

#include <QObject>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Mer {
namespace Internal {

class MerQtVersion;
class SdkTargetUtils : public QObject
{
    Q_OBJECT
    enum Command {
        None,
        FetchTargets
    };

    enum State {
        Inactive,
        Active
    };

public:
    explicit SdkTargetUtils(QObject *parent = 0);

    void setSshConnectionParameters(const QSsh::SshConnectionParameters params);

    bool checkInstalledTargets(const QString &sdkName);
    void updateQtVersions(MerSdk &sdk, const QStringList &targets);

signals:
    void installedTargets(const QString &sdkName, const QStringList &targets);
    void processOutput(const QString &output);
    void connectionError();

private slots:
    void onConnectionError();
    void onProcessStarted();
    void onReadReadyStandardOutput();
    void onReadReadyStandardError();
    void onProcessClosed(int);

private:
    MerQtVersion *createQtVersion(const QString &sdkName, const QString &targetDirectory);

private:
    QSsh::SshRemoteProcessRunner *m_remoteProcessRunner;
    QSsh::SshConnectionParameters m_sshParameters;
    QString m_sdkName;
    QString m_output;
    Command m_command;
    State m_state;
};

} // Internal
} // Mer

#endif // MERSDKTARGETUTILS_H
