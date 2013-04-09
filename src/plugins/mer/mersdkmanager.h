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

#ifndef MERSDKMANAGER_H
#define MERSDKMANAGER_H

#include "mersdk.h"
#include "mervirtualmachinebutton.h"

#include <ssh/sshconnection.h>
#include <utils/environment.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class PersistentSettingsWriter;
class FileName;
}

namespace ProjectExplorer {
class Kit;
class Project;
class Target;
class Task;
}

namespace Mer {
namespace Internal {

class MerSdkManager : public QObject
{
    Q_OBJECT
public:
    static MerSdkManager *instance();

    static QString sdkToolsDirectory();
    static QString globalSdkToolsDirectory();
    static bool authorizePublicKey(const QString &authorizedKeysPath, const QString &publicKeyPath, QString &error);
    static bool isMerKit(ProjectExplorer::Kit *kit);
    static QString targetNameForKit(ProjectExplorer::Kit *kit);
    static QString virtualMachineNameForKit(const ProjectExplorer::Kit *kit);
    static bool hasMerDevice(ProjectExplorer::Kit *k);
    static bool validateKit(const ProjectExplorer::Kit* kit);
    static bool generateSshKey(const QString &privKeyPath, QString &error);

    static void addToEnvironment(const QString &sdkName, Utils::Environment &env);

    QList<MerSdk*> sdks() const;
    MerSdk* sdk(const QString &virtualMachineName) const;
    MerSdk* createSdk(const QString &vmName);
    void addSdk(MerSdk* sdk);
    void removeSdk(MerSdk* sdk);
    void restoreSdks();
    bool hasSdk(const MerSdk* sdk) const;

public slots:
    void storeSdks() const;

signals:
    void sdksUpdated();
    void sdkRunningChanged();

private slots:
    void handleStartupProjectChanged(ProjectExplorer::Project *project);
    void handleKitUpdated(ProjectExplorer::Kit *kit);
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetRemoved(ProjectExplorer::Target *target);
    void handleTaskAdded(const ProjectExplorer::Task &task);
    void handleStartRemoteRequested();
    void handleStopRemoteRequested();
    void initialize();
    void updateUI();
    void handleConnectionError(const QString &vmName, const QString &error);

private:
    explicit MerSdkManager();
    ~MerSdkManager();
    MerSdkManager(const MerSdkManager &);
    MerSdkManager &operator=(const MerSdkManager &);

    void restore();
    bool sdkParams(QString &sdkName, QSsh::SshConnectionParameters &params) const;
    bool emulatorParams(QString &emulatorName, QSsh::SshConnectionParameters &params) const;
    QList<MerSdk*> restoreSdks(const Utils::FileName &fileName);
    QList<ProjectExplorer::Kit*> merKits() const;
    QList<MerToolChain*> merToolChains() const;
    QList<MerQtVersion*> merQtVersions() const;
private:
    static MerSdkManager *m_sdkManager;
    static ProjectExplorer::Project *m_previousProject;
    mutable QMap<QString, MerSdk*> m_sdks;
    mutable bool m_intialized;

    MerVirtualMachineButton m_remoteEmulatorBtn;
    MerVirtualMachineButton m_remoteSdkBtn;

    Utils::PersistentSettingsWriter *m_writer;

    friend class MerPlugin;
};

} // Internal
} // Mer

#endif // MERSDKMANAGER_H
