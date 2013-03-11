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

#ifndef MERSDK_H
#define MERSDK_H

#include <ssh/sshconnection.h>
#include <coreplugin/id.h>

#include <QStringList>

namespace Mer {
namespace Internal {

class MerSdk
{
public:
    explicit MerSdk(bool autoDetected = false);
    virtual ~MerSdk() {}

    bool isAutoDetected() const { return m_autoDetected; }

    void setVirtualMachineName(const QString &name) { m_name = name; }
    QString virtualMachineName() const { return m_name; }

    void setSharedHomePath(const QString &homePath) { m_sharedHomePath = homePath; }
    QString sharedHomePath() const { return m_sharedHomePath; }

    void setSharedTargetPath(const QString &targetPath) { m_sharedTargetPath = targetPath; }
    QString sharedTargetPath() const { return m_sharedTargetPath; }

    void setSharedSshPath(const QString &sshPath) { m_sharedSshPath = sshPath; }
    QString sharedSshPath() const { return m_sharedSshPath; }

    void setSshConnectionParameters(const QSsh::SshConnectionParameters &connectionParams)
    {
        m_connectionParams = connectionParams;
    }

    QSsh::SshConnectionParameters sshConnectionParams() const { return m_connectionParams; }

    void setToolChains(const QHash<QString, QString> &toolChains) { m_toolChains = toolChains; }
    QHash<QString, QString> toolChains() const { return m_toolChains; }

    void setQtVersions(const QHash<QString, int> &qmakeIds) { m_qtVersions = qmakeIds; }
    QHash<QString, int> qtVersions() const { return m_qtVersions; }

    QStringList targets() const { return m_toolChains.keys(); }

private:
    QString m_name;
    QString m_sharedHomePath;
    QString m_sharedTargetPath;
    QString m_sharedSshPath;
    QSsh::SshConnectionParameters m_connectionParams;
    QHash<QString, QString> m_toolChains;       // QHash<target, id>
    QHash<QString, int> m_qtVersions;           // QHash<target, id>
    bool m_autoDetected;                        // true if the VM was packaged
};

} // Internal
} // Mer

Q_DECLARE_METATYPE(Mer::Internal::MerSdk)

#endif // MERSDK_H
