/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#ifndef MERSDKMANAGER_H
#define MERSDKMANAGER_H

#include <coreplugin/id.h>

#include <QObject>

#include <memory>

namespace ProjectExplorer {
class Kit;
class Project;
}

namespace Sfdk {
    class BuildEngine;
}

namespace Mer {
namespace Internal {

class MerToolChain;
class MerQtVersion;

class MerSdkManager : public QObject
{
    Q_OBJECT
public:
    MerSdkManager();
    ~MerSdkManager() override;
    static MerSdkManager *instance();

    static bool authorizePublicKey(const QString &authorizedKeysPath, const QString &publicKeyPath, QString &error);
    static bool isMerKit(const ProjectExplorer::Kit *kit);
    static QString targetNameForKit(const ProjectExplorer::Kit *kit);
    static QList<ProjectExplorer::Kit *> kitsForTarget(const QString &targetName);
    static bool hasMerDevice(ProjectExplorer::Kit *kit);
    static bool validateKit(const ProjectExplorer::Kit* kit);
    static bool generateSshKey(const QString &privKeyPath, QString &error);
    static QList<ProjectExplorer::Kit*> merKits();

signals:
    void initialized();

private:
    void initialize();
    void updateDevices();
    static QList<MerToolChain*> merToolChains();
    static QList<MerQtVersion*> merQtVersions();
    void onBuildEngineAdded(int index);
    void onAboutToRemoveBuildEngine(int index);
    static bool addKit(const Sfdk::BuildEngine *buildEngine, const QString &buildTargetName);
    static bool removeKit(const Sfdk::BuildEngine *buildEngine, const QString &buildTargetName);
    static ProjectExplorer::Kit *kit(const Sfdk::BuildEngine *buildEngine,
            const QString &buildTargetName);
    void checkPkgConfigAvailability();
    static bool finalizeKitCreation(const Sfdk::BuildEngine *buildEngine,
            const QString &buildTargetName, ProjectExplorer::Kit* k);
    static void ensureDebuggerIsSet(ProjectExplorer::Kit *k, const Sfdk::BuildEngine *buildEngine,
            const QString &buildTargetName);
    static std::unique_ptr<MerQtVersion> createQtVersion(const Sfdk::BuildEngine *buildEngine,
            const QString &buildTargetName);
    static std::unique_ptr<MerToolChain> createToolChain(const Sfdk::BuildEngine *buildEngine,
            const QString &buildTargetName, Core::Id language);

private:
    static MerSdkManager *m_instance;
    bool m_intialized;
};

} // Internal
} // Mer

#endif // MERSDKMANAGER_H
