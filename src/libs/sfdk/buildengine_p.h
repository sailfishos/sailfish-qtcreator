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

#include "buildengine.h"

namespace QSsh {
class SshConnectionParameters;
}

namespace Utils {
class FileSystemWatcher;
}

namespace Sfdk {

class VirtualMachineInfo;

class BuildTargetDump
{
public:
    bool operator==(const BuildTargetDump &other) const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &data);

    QString name;
    QString gccDumpMachine;
    QString gccDumpMacros;
    QString gccDumpIncludes;
    QString qmakeQuery;
    QString rpmValidationSuites;
};

class BuildEnginePrivate
{
    Q_DECLARE_PUBLIC(BuildEngine)

public:
    explicit BuildEnginePrivate(BuildEngine *q);
    ~BuildEnginePrivate();

    static BuildEnginePrivate *get(BuildEngine *q) { return q->d_func(); }

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    bool initVirtualMachine(const QUrl &vmUri);
    void enableUpdates();
    void updateOnce();
    void updateVmProperties(const QObject *context, const Functor<bool> &functor);

private:
    void setSharedHomePath(const Utils::FileName &sharedHomePath);
    void setSharedTargetsPath(const Utils::FileName &sharedTargetsPath);
    void setSharedConfigPath(const Utils::FileName &sharedConfigPath);
    void setSharedSrcPath(const Utils::FileName &sharedSrcPath);
    void setSharedSshPath(const Utils::FileName &sharedSshPath);
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);
    void setWwwPort(quint16 wwwPort);

    void syncWwwProxy();
    void updateBuildTargets();
    void updateBuildTargets(QList<BuildTargetDump> newTargets);
    BuildTargetData createTargetData(const BuildTargetDump &targetDump) const;
    static QList<RpmValidationSuiteData> rpmValidationSuitesFromString(const QString &string);
    static QString rpmValidationSuitesToString(const QList<RpmValidationSuiteData> &suites);

    void initBuildTargetAt(int index) const;
    void deinitBuildTargetAt(int index) const;
    bool createCacheFile(const Utils::FileName &fileName, const QString &data) const;
    bool createSimpleWrapper(const QString &targetName, const Utils::FileName &toolsPath,
        const QString &wrapperName) const;
    bool createPkgConfigWrapper(const Utils::FileName &toolsPath,
            const Utils::FileName &sysRoot) const;
    Utils::FileName targetsXmlFile() const;
    Utils::FileName sysRootForTarget(const QString &targetName) const;
    Utils::FileName toolsPathForTarget(const QString &targetName) const;
    Utils::FileName toolsPath() const;

private:
    BuildEngine *const q_ptr;
    std::unique_ptr<VirtualMachine> virtualMachine;
    bool autodetected = false;
    Utils::FileName sharedHomePath;
    Utils::FileName sharedTargetsPath;
    Utils::FileName sharedConfigPath;
    Utils::FileName sharedSrcPath;
    Utils::FileName sharedSshPath;
    quint16 wwwPort = 0;
    QString wwwProxyType;
    QString wwwProxyServers;
    QString wwwProxyExcludes;
    bool headless = true;
    std::unique_ptr<Utils::FileSystemWatcher> targetsXmlWatcher;
    QList<BuildTargetDump> buildTargets;
    QList<BuildTargetData> buildTargetsData;
};

class BuildEngineManager : public QObject
{
    Q_OBJECT

public:
    explicit BuildEngineManager(QObject *parent);
    ~BuildEngineManager() override;
    static BuildEngineManager *instance();

    static QString installDir();

    static QList<BuildEngine *> buildEngines();
    static BuildEngine *buildEngine(const QUrl &uri);
    static void createBuildEngine(const QUrl &virtualMachineUri, const QObject *context,
        const Functor<std::unique_ptr<BuildEngine> &&> &functor);
    static int addBuildEngine(std::unique_ptr<BuildEngine> &&buildEngine);
    static void removeBuildEngine(const QUrl &uri);

signals:
    void buildEngineAdded(int index);
    void aboutToRemoveBuildEngine(int index);

private:
    QVariantMap toMap() const;
    void fromMap(const QVariantMap &data);
    void enableUpdates();
    void updateOnce();
    void checkSystemSettings();
    void saveSettings(QStringList *errorStrings) const;
    static Utils::FileName systemSettingsFile();

private:
    static BuildEngineManager *s_instance;
    const std::unique_ptr<UserSettings> m_userSettings;
    int m_version = 0;
    QString m_installDir;
    std::vector<std::unique_ptr<BuildEngine>> m_buildEngines;
};

} // namespace Sfdk
