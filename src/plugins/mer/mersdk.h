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
#include <QFileSystemWatcher>
#include <QStringList>

namespace ProjectExplorer {
 class Kit;
}

namespace Utils {
 class FileName;
}

namespace Mer {
namespace Internal {

class MerQtVersion;
class MerToolChain;
class MerTarget;

class MerSdk : public QObject
{
    Q_OBJECT
public:
    virtual ~MerSdk();

    bool isAutoDetected() const;
    void setAutodetect(bool autoDetected);

    void setVirtualMachineName(const QString &name);
    QString virtualMachineName() const;

    void setSharedHomePath(const QString &homePath);
    QString sharedHomePath() const;

    void setSharedTargetPath(const QString &targetPath);
    QString sharedTargetPath() const;

    void setSharedSshPath(const QString &sshPath);
    QString sharedSshPath() const;

    void setSshPort(quint16 port);
    quint16 sshPort() const;

    void setWwwPort(quint16 port);
    quint16 wwwPort() const;

    void setPrivateKeyFile(const QString &file);
    QString privateKeyFile() const;

    void setHost(const QString &host);
    QString host() const;

    void setUserName(const QString &username);
    QString userName() const;

    QStringList targets() const;

    bool isValid() const;
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &data);

    void attach();
    void detach();

signals:
    void targetsChanged(const QStringList &targets);

private slots:
    void updateTargets();
    void handleTargetsFileChanged(const QString &file);

private:
    explicit MerSdk(QObject *parent = 0);
    QList<MerTarget> readTargets(const Utils::FileName &fileName);
    bool addTarget(const MerTarget &target);
    bool removeTarget(const MerTarget &target);
    void setTargets(const QList<MerTarget> &targets);

private:
    QString m_name;
    QString m_sharedHomePath;
    QString m_sharedTargetPath;
    QString m_sharedSshPath;
    QString m_host;
    QString m_userName;
    QString m_privateKeyFile;
    quint16 m_sshPort;
    quint16 m_wwwPort;

    bool m_autoDetected;
    QList<MerTarget> m_targets;
    QFileSystemWatcher m_watcher;

friend class MerSdkManager;
};

} // Internal
} // Mer

#endif // MERSDK_H
