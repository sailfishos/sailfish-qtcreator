/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#pragma once

#include "sfdkglobal.h"

#include "asynchronous.h"

#include <utils/fileutils.h>

namespace Sfdk {

class VirtualMachine;
class UserSettings;

class SFDK_EXPORT RpmValidationSuiteData
{
public:
    bool operator==(const RpmValidationSuiteData &other) const;

    QString id;
    QString name;
    QString website;
    bool essential;
};

class SFDK_EXPORT BuildTargetData
{
public:
    bool operator==(const BuildTargetData &other) const;
    bool isValid() const;

    QString name;
    Utils::FilePath sysRoot;
    Utils::FilePath toolsPath;
    Utils::FilePath gdb;
    QList<RpmValidationSuiteData> rpmValidationSuites;

    static Utils::FilePath toolsPathCommonPrefix();
};

class BuildEngineManager;

class BuildEnginePrivate;
class SFDK_EXPORT BuildEngine : public QObject
{
    Q_OBJECT

public:
    struct PrivateConstructorTag;
    BuildEngine(QObject *parent, const PrivateConstructorTag &);
    ~BuildEngine() override;

    QUrl uri() const;
    QString name() const;

    VirtualMachine *virtualMachine() const;

    bool isAutodetected() const;

    Utils::FilePath sharedInstallPath() const;
    Utils::FilePath sharedHomePath() const;
    Utils::FilePath sharedTargetsPath() const;
    Utils::FilePath sharedConfigPath() const;
    Utils::FilePath sharedSrcPath() const;
    QString sharedSrcMountPoint() const;
    Utils::FilePath sharedSshPath() const;
    void setSharedSrcPath(const Utils::FilePath &sharedSrcPath, const QObject *context,
            const Functor<bool> &functor);

    quint16 sshPort() const;
    void setSshPort(quint16 sshPort, const QObject *context, const Functor<bool> &functor);

    quint16 wwwPort() const;
    void setWwwPort(quint16 wwwPort, const QObject *context, const Functor<bool> &functor);

    quint16 dBusPort() const;
    void setDBusPort(quint16 dBusPort, const QObject *context, const Functor<bool> &functor);

    QString wwwProxyType() const;
    QString wwwProxyServers() const;
    QString wwwProxyExcludes() const;
    void setWwwProxy(const QString &type, const QString &servers, const QString &excludes);

    QStringList buildTargetNames() const;
    QList<BuildTargetData> buildTargets() const;
    BuildTargetData buildTarget(const QString &name) const;

    void importPrivateGpgKey(const QString &id, const Utils::FilePath &passphraseFile,
            const QObject *context, const Functor<bool, QString> &functor);

signals:
    void sharedInstallPathChanged(const Utils::FilePath &sharedInstallPath);
    void sharedHomePathChanged(const Utils::FilePath &sharedHomePath);
    void sharedTargetsPathChanged(const Utils::FilePath &sharedTargetsPath);
    void sharedConfigPathChanged(const Utils::FilePath &sharedConfigPath);
    void sharedSrcPathChanged(const Utils::FilePath &sharedSrcPath);
    void sharedSrcMountPointChanged(const QString &sharedSrcMountPoint);
    void sharedSshPathChanged(const Utils::FilePath &sharedSshPath);
    void sshPortChanged(quint16 sshPort);
    void wwwPortChanged(quint16 wwwPort);
    void dBusPortChanged(quint16 dBusPort);
    void wwwProxyChanged(const QString &type, const QString &servers, const QString &excludes);

    void buildTargetAdded(int index);
    void aboutToRemoveBuildTarget(int index);

private:
    std::unique_ptr<BuildEnginePrivate> d_ptr;
    Q_DISABLE_COPY(BuildEngine)
    Q_DECLARE_PRIVATE(BuildEngine)
};

} // namespace Sfdk
