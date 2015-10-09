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

#ifndef MERSDKMANAGER_H
#define MERSDKMANAGER_H

#include "mersdk.h"

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
    static bool isMerKit(const ProjectExplorer::Kit *kit);
    static QString targetNameForKit(const ProjectExplorer::Kit *kit);
    static QList<ProjectExplorer::Kit *> kitsForTarget(const QString &targetName);
    static bool hasMerDevice(ProjectExplorer::Kit *kit);
    static bool validateKit(const ProjectExplorer::Kit* kit);
    static bool generateSshKey(const QString &privKeyPath, QString &error);

    ~MerSdkManager() override;
    static QList<MerSdk*> sdks();
    static MerSdk* sdk(const QString &virtualMachineName);
    static MerSdk* createSdk(const QString &vmName);
    static void addSdk(MerSdk *sdk);
    static void removeSdk(MerSdk *sdk);
    static void restoreSdks();
    static bool hasSdk(const MerSdk *sdk);

    static QList<ProjectExplorer::Kit*> merKits();

public slots:
    void storeSdks();

signals:
    void sdksUpdated();
    void initialized();

private slots:
    void initialize();
    void updateDevices();

private:
    MerSdkManager();
    void restore();
    QList<MerSdk*> restoreSdks(const Utils::FileName &fileName);
    QList<MerToolChain*> merToolChains() const;
    QList<MerQtVersion*> merQtVersions() const;
    const Utils::FileName& checkInstallLocation(const Utils::FileName &l, const Utils::FileName &g);
private:
    static MerSdkManager *m_instance;
    QMap<QString, MerSdk*> m_sdks;
    bool m_intialized;
    Utils::PersistentSettingsWriter *m_writer;
    QString m_installDir;
    bool m_reinstall;
    friend class MerPlugin;

public:
    static bool verbose;
};

} // Internal
} // Mer

#endif // MERSDKMANAGER_H
