/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include <coreplugin/id.h>
#include <ssh/sshconnection.h>

#include <QFileSystemWatcher>
#include <QStringList>
#include <QTimer>

namespace ProjectExplorer {
class Kit;
}

namespace Utils {
class FileName;
}

namespace Mer {

class MerConnection;

namespace Internal {

class MerQtVersion;
class MerToolChain;
class MerTarget;

class MerSdk : public QObject
{
    Q_OBJECT
public:
    ~MerSdk() override;

    bool isAutoDetected() const;
    void setAutodetect(bool autoDetected);

    void setVirtualMachineName(const QString &name);
    QString virtualMachineName() const;

    void setSharedHomePath(const QString &homePath);
    QString sharedHomePath() const;

    void setSharedTargetsPath(const QString &targetsPath);
    QString sharedTargetsPath() const;

    void setSharedConfigPath(const QString &configPath);
    QString sharedConfigPath() const;

    void setSharedSshPath(const QString &sshPath);
    QString sharedSshPath() const;

    void setSharedSrcPath(const QString &srcPath);
    QString sharedSrcPath() const;

    void setSshPort(quint16 port);
    quint16 sshPort() const;

    void setWwwPort(quint16 port);
    quint16 wwwPort() const;

    void setTimeout(int timeout);
    int timeout() const;

    void setPrivateKeyFile(const QString &file);
    QString privateKeyFile() const;

    void setHost(const QString &host);
    QString host() const;

    void setUserName(const QString &username);
    QString userName() const;

    void setHeadless(bool enabled);
    bool isHeadless() const;

    QStringList targetNames() const;
    QList<MerTarget> targets() const;

    bool isValid() const;
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &data);

    void attach();
    void detach();

    MerConnection *connection() const;

signals:
    void targetsChanged(const QStringList &targets);
    void privateKeyChanged(const QString &file);
    void headlessChanged(bool);
    void sshPortChanged(quint16 port);
    void wwwPortChanged(quint16 port);

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
    bool m_autoDetected;
    MerConnection *m_connection;
    QString m_sharedHomePath;
    QString m_sharedTargetsPath;
    QString m_sharedSshPath;
    QString m_sharedConfigPath;
    QString m_sharedSrcPath;
    quint16 m_wwwPort;
    QList<MerTarget> m_targets;
    QFileSystemWatcher m_watcher;
    QTimer m_updateTargetsTimer;

friend class MerSdkManager;
};

} // Internal
} // Mer

#endif // MERSDK_H
