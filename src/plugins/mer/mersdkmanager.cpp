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

#include "mersdkmanager.h"
#include "merconstants.h"
#include "merqtversion.h"
#include "mervirtualmachinemanager.h"
#include "mertoolchain.h"
#include "virtualboxmanager.h"

#include <coreplugin/icore.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/taskhub.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>

using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace QSsh;
using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerSdkManager *MerSdkManager::m_sdkManager = 0;
ProjectExplorer::Project *MerSdkManager::m_previousProject = 0;

MerSdkManager::MerSdkManager()
    : m_intialized(false)
{
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), SLOT(writeSettings()));

    KitManager *km = KitManager::instance();

    connect(km, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            SLOT(handleKitUpdated(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitsLoaded()), SLOT(initialize()));

    SessionManager *session = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            SLOT(handleStartupProjectChanged(ProjectExplorer::Project*)));

    MerVirtualMachineManager *mrmm = MerVirtualMachineManager::instance();
    connect(mrmm, SIGNAL(connectionChanged(QString,bool)), SLOT(updateUI()));
    connect(mrmm, SIGNAL(error(QString,QString)), SLOT(handleConnectionError(QString,QString)));

    TaskHub *th = ProjectExplorerPlugin::instance()->taskHub();
    th->addCategory(Core::Id(MER_TASKHUB_CATEGORY), tr("Virtual Machine Error"));
    connect(th, SIGNAL(taskAdded(ProjectExplorer::Task)),
            SLOT(handleTaskAdded(ProjectExplorer::Task)));

    QIcon emuIcon;
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-run.png"));
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-stop.png"), QSize(), QIcon::Normal,
                    QIcon::On);

    QIcon sdkIcon;
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-run.png"));
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-stop.png"), QSize(), QIcon::Normal, QIcon::On);

    m_remoteEmulatorBtn.setName(tr("Emulator"));
    m_remoteEmulatorBtn.setIcon(emuIcon);
    m_remoteEmulatorBtn.setStartTip(tr("Start Emulator"));
    m_remoteEmulatorBtn.setStopTip(tr("Stop Emulator"));
    m_remoteEmulatorBtn.initialize();

    m_remoteSdkBtn.setName(tr("MerSdk"));
    m_remoteSdkBtn.setIcon(sdkIcon);
    m_remoteSdkBtn.setStartTip(tr("Start Sdk"));
    m_remoteSdkBtn.setStopTip(tr("Stop Sdk"));
    m_remoteSdkBtn.initialize();

    connect(&m_remoteEmulatorBtn, SIGNAL(startRequest()), SLOT(handleStartRemoteRequested()));
    connect(&m_remoteSdkBtn, SIGNAL(startRequest()), SLOT(handleStartRemoteRequested()));
    connect(&m_remoteEmulatorBtn, SIGNAL(stopRequest()), SLOT(handleStopRemoteRequested()));
    connect(&m_remoteSdkBtn, SIGNAL(stopRequest()), SLOT(handleStopRemoteRequested()));
}

MerSdkManager::~MerSdkManager()
{
}

void MerSdkManager::initialize()
{
    if (!m_intialized) {
        //read stored sdks
        readSettings();

        //populate sdks toolchains qtversion
        const QList<ToolChain *> &toolChains = ToolChainManager::instance()->toolChains();
        const QList<QtSupport::BaseQtVersion *> &versions =
                QtSupport::QtVersionManager::instance()->versions();

        QMap<QString, MerSdk> sdks;

        foreach (MerSdk sdk, m_sdks) {
            QHash<QString, QString> toolChainsList;
            foreach (const ToolChain *toolChain, toolChains) {
                if (toolChain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
                    continue;
                const MerToolChain *tc = static_cast<const MerToolChain *>(toolChain);
                if (tc->virtualMachineName() == sdk.virtualMachineName())
                    toolChainsList.insert(tc->targetName(), toolChain->id());
            }
            sdk.setToolChains(toolChainsList);

            QHash<QString, int> qtVersionsList;
            foreach (const QtSupport::BaseQtVersion *version, versions) {
                if (version->type() != QLatin1String(Constants::MER_QT))
                    continue;
                const MerQtVersion *qtv = static_cast<const MerQtVersion *>(version);
                if (qtv->virtualMachineName() == sdk.virtualMachineName())
                    qtVersionsList.insert(qtv->targetName(), version->uniqueId());
            }
            sdk.setQtVersions(qtVersionsList);
            sdks.insert(sdk.virtualMachineName(), sdk);
        }
        m_sdks = sdks;
        m_intialized = true;
        emit sdksUpdated();
    }
}

MerSdkManager *MerSdkManager::instance()
{
    if (m_sdkManager)
        return m_sdkManager;

    m_sdkManager = new MerSdkManager;
    return m_sdkManager;
}

QList<MerSdk> MerSdkManager::sdks() const
{
    return m_sdks.values();
}

MerSdk MerSdkManager::sdk(const QString &sdkName) const
{
    return m_sdks.value(sdkName);
}

void MerSdkManager::readSettings()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(MER_SDK));
    const QStringList sdks = s->childGroups();
    foreach (const QString &sdk, sdks) {
        s->beginGroup(sdk);
        bool autoDetected = s->value(QLatin1String(AUTO_DETECTED)).toBool();
        MerSdk mersdk(autoDetected);

        mersdk.setVirtualMachineName(sdk);

        mersdk.setSharedHomePath(s->value(QLatin1String(SHARED_HOME)).toString());
        mersdk.setSharedTargetPath(s->value(QLatin1String(SHARED_TARGET)).toString());
        mersdk.setSharedSshPath(s->value(QLatin1String(SHARED_SSH)).toString());

        SshConnectionParameters params = defaultConnectionParameters();
        params.userName = s->value(QLatin1String(USERNAME)).toString();
        params.password = s->value(QLatin1String(PASSWORD)).toString();
        params.privateKeyFile = s->value(QLatin1String(PRIVATE_KEY_FILE)).toString();
        params.authenticationType =
                s->value(QLatin1String(AUTHENTICATION_TYPE)).toString() == QLatin1String(PASSWORD)
                ? SshConnectionParameters::AuthenticationByPassword
                : SshConnectionParameters::AuthenticationByKey;
        params.port = s->value(QLatin1String(SSH_PORT)).toUInt();
        mersdk.setSshConnectionParameters(params);
        s->endGroup();
        m_sdks.insert(sdk, mersdk);
    }
    s->endGroup();
}

bool MerSdkManager::isMerKit(Kit *kit)
{
    if (!kit)
        return false;
    return kit->value(Core::Id(TYPE)).toString() == QLatin1String(MER_SDK);
}

QString MerSdkManager::targetNameForKit(Kit *kit)
{
    if (!kit || !isMerKit(kit))
        return QString();
    ToolChain *toolchain = ToolChainKitInformation::toolChain(kit);
    if (toolchain && toolchain->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
        MerToolChain *mertoolchain = static_cast<MerToolChain *>(toolchain);
        return mertoolchain->targetName();
    }
    return QString();
}

QString MerSdkManager::virtualMachineNameForKit(const Kit *kit)
{
    ToolChain *toolchain = ToolChainKitInformation::toolChain(kit);
    if (toolchain && toolchain->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
        MerToolChain *mertoolchain = static_cast<MerToolChain *>(toolchain);
        return mertoolchain->virtualMachineName();
    }
    return QString();
}

bool MerSdkManager::hasMerDevice(Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (dev.isNull())
        return false;
    return dev->type() == Constants::MER_DEVICE_TYPE;
}

QString MerSdkManager::sdkToolsDirectory()
{
    return QFileInfo(ExtensionSystem::PluginManager::settings()->fileName()).absolutePath() +
            QLatin1String(Constants::MER_SDK_TOOLS);
}

QString MerSdkManager::globalSdkToolsDirectory()
{
    return QFileInfo(ExtensionSystem::PluginManager::globalSettings()->fileName()).absolutePath() +
            QLatin1String(Constants::MER_SDK_TOOLS);
}

SshConnectionParameters MerSdkManager::defaultConnectionParameters()
{
    SshConnectionParameters params;
    params.userName = QLatin1String(MER_SDK_DEFAULTUSER);
    params.authenticationType = SshConnectionParameters::AuthenticationByKey;
    params.host = QLatin1String(MER_SDK_DEFAULTHOST);
    params.timeout = 10;
    return params;
}

bool MerSdkManager::authorizePublicKey(const QString &vmName,
                                       const QString &pubKeyPath,
                                       const QString &userName,
                                       QWidget *parent = 0)
{
    VirtualMachineInfo info = VirtualBoxManager::fetchVirtualMachineInfo(vmName);

    const QString sshDirectoryPath = info.sharedSsh + QLatin1Char('/');
    const QStringList authorizedKeysPaths = QStringList()
            << sshDirectoryPath + QLatin1String("root/authorized_keys")
            << sshDirectoryPath + userName
               + QLatin1String("/authorized_keys");

    bool success = false;
    QFileInfo fi(pubKeyPath);
    if (!fi.exists()) {
        QMessageBox::critical(parent, tr("Cannot Authorize Keys"),
                              tr("Error: File %1 is missing \n").arg(pubKeyPath));
        return success;
    }

    Utils::FileReader pubKeyReader;
    success = pubKeyReader.fetch(pubKeyPath);
    if (!success) {
        QMessageBox::critical(parent, tr("Cannot Authorize Keys"),
                              tr("Error: %1 \n").arg(pubKeyReader.errorString()));
        return success;
    }
    const QByteArray publicKey = pubKeyReader.data();
    foreach (const QString &authorizedKeys, authorizedKeysPaths) {
        bool containsKey = false;
        {
            Utils::FileReader authKeysReader;
            success = authKeysReader.fetch(authorizedKeys);
            if (!success) {
                // File could not be read. Does it exist?
                // Check for directory
                QDir sshDirectory(QFileInfo(authorizedKeys).absolutePath());
                if (!sshDirectory.exists() && !sshDirectory.mkpath(sshDirectory.absolutePath())) {
                    QMessageBox::critical(parent, tr("Cannot Authorize Keys"),
                                          tr("Error: Could not create directory %1 \n").arg(
                                              sshDirectory.absolutePath()));
                    success = false;
                    break;
                }

                QFileInfo akFi(authorizedKeys);
                if (!akFi.exists()) {
                    // Create file
                    Utils::FileSaver authKeysSaver(authorizedKeys, QIODevice::WriteOnly);
                    authKeysSaver.write(publicKey);
                    success = authKeysSaver.finalize();
                    if (!success) {
                        QMessageBox::critical(parent, tr("Cannot Authorize Keys"),
                                              tr("Error: %1 \n").arg(authKeysSaver.errorString()));
                        break;
                    }
                    QFile::setPermissions(authorizedKeys, QFile::ReadOwner|QFile::WriteOwner);
                }
            } else {
                containsKey = authKeysReader.data().contains(publicKey);
            }
        }
        if (!containsKey) {
            // File does not contain the public key. Append it to file.
            Utils::FileSaver authorizedKeysSaver(authorizedKeys, QIODevice::Append);
            authorizedKeysSaver.write("\n");
            authorizedKeysSaver.write(publicKey);
            authorizedKeysSaver.write("\n");
            success = authorizedKeysSaver.finalize();
            if (!success) {
                QMessageBox::critical(parent, tr("Cannot Authorize Keys"),
                                      tr("Error: %1 \n").arg(authorizedKeysSaver.errorString()));
                break;
            }
        }
    }
    return success;
}

void MerSdkManager::writeSettings(QSettings *s, const MerSdk &sdk) const
{
    s->beginGroup(sdk.virtualMachineName());

    s->setValue(QLatin1String(AUTO_DETECTED), sdk.isAutoDetected());
    s->setValue(QLatin1String(SHARED_HOME), sdk.sharedHomePath());
    s->setValue(QLatin1String(SHARED_TARGET), sdk.sharedTargetPath());
    s->setValue(QLatin1String(SHARED_SSH), sdk.sharedSshPath());

    SshConnectionParameters params = sdk.sshConnectionParams();
    s->setValue(QLatin1String(USERNAME), params.userName);
    s->setValue(QLatin1String(PASSWORD), params.password);
    s->setValue(QLatin1String(PRIVATE_KEY_FILE), params.privateKeyFile);
    QString authType = QLatin1String(PASSWORD);
    if (params.authenticationType == SshConnectionParameters::AuthenticationByKey)
        authType = QLatin1String(KEY);
    s->setValue(QLatin1String(AUTHENTICATION_TYPE), authType);
    s->setValue(QLatin1String(SSH_PORT), QString::number(params.port));

    s->endGroup();
}

void MerSdkManager::writeSettings() const
{
    QSettings *userSettings = Core::ICore::settings();
    userSettings->remove(QLatin1String(MER_SDK));
    userSettings->beginGroup(QLatin1String(MER_SDK));
    foreach (const MerSdk &sdk, m_sdks) {
        // Auto detected SDKs have been installed by the SDK installer
        // Their settings are in the global scope and are non-modifiable.
        if (sdk.isAutoDetected())
            continue;
        writeSettings(userSettings, sdk);
    }
    userSettings->endGroup();
}

void MerSdkManager::handleKitUpdated(Kit *kit)
{
    if (!isMerKit(kit))
        return;
    updateUI();
}

void MerSdkManager::handleStartupProjectChanged(Project *project)
{
    if (project != m_previousProject && project) {
        // handle all target related changes, add, remove, etc...
        connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                SLOT(updateUI()));
    }

    if (m_previousProject) {
        disconnect(m_previousProject, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        disconnect(m_previousProject, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        disconnect(m_previousProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(updateUI()));
    }

    m_previousProject = project;
    updateUI();
}

void MerSdkManager::handleTargetAdded(Target *target)
{
    if (!target || !isMerKit(target->kit()))
        return;
    updateUI();
}

void MerSdkManager::handleTargetRemoved(Target *target)
{
    if (!target || !isMerKit(target->kit()))
        return;
    updateUI();
}

void MerSdkManager::updateUI()
{
    bool sdkRemoteButonEnabled = false;
    bool emulatorRemoteButtonEnabled = false;
    bool sdkRemoteButonVisible = false;
    bool emulatorRemoteButtonVisible = false;
    bool isSdkRemoteRunning = false;
    bool isEmulatorRemoteRunning = false;

    const Project * const p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (p) {
        const Target * const activeTarget = p->activeTarget();
        const QList<Target *> targets =  p->targets();

        foreach (const Target* t , targets) {
            if (isMerKit(t->kit())) {
                sdkRemoteButonVisible = true;
                if (t == activeTarget) {
                    sdkRemoteButonEnabled = true;
                    isSdkRemoteRunning =
                            MerVirtualMachineManager::instance()->isRemoteConnected(
                                virtualMachineNameForKit(t->kit()));
                }
            }

            if (hasMerDevice(t->kit())) {
                emulatorRemoteButtonVisible = true;
                if (t == activeTarget) {
                    const IDevice::ConstPtr device = DeviceKitInformation::device(t->kit());
                    emulatorRemoteButtonEnabled =
                            device && device->machineType() == IDevice::Emulator;
                    isEmulatorRemoteRunning =
                            MerVirtualMachineManager::instance()->isRemoteConnected(
                                device->id().toString());
                }
            }
        }
    }

    m_remoteEmulatorBtn.setEnabled(emulatorRemoteButtonEnabled);
    m_remoteEmulatorBtn.setVisible(emulatorRemoteButtonVisible);
    m_remoteEmulatorBtn.setRunning(isEmulatorRemoteRunning);
    m_remoteEmulatorBtn.update();

    if (m_remoteSdkBtn.isRunning() != isSdkRemoteRunning)
        emit sdkRunningChanged();
    m_remoteSdkBtn.setEnabled(sdkRemoteButonEnabled);
    m_remoteSdkBtn.setVisible(sdkRemoteButonVisible);
    m_remoteSdkBtn.setRunning(isSdkRemoteRunning);
    m_remoteSdkBtn.update();
}

void MerSdkManager::handleStartRemoteRequested()
{
    QString name;
    SshConnectionParameters params;

    if (sender() == &m_remoteEmulatorBtn && !emulatorParams(name, params))
        return;

    if (sender() == &m_remoteSdkBtn && !sdkParams(name, params))
        return;

    MerVirtualMachineManager::instance()->startRemote(name, params);
}

void MerSdkManager::handleStopRemoteRequested()
{
    QString name;
    SshConnectionParameters params;

    if (sender() == &m_remoteEmulatorBtn && emulatorParams(name, params))
        MerVirtualMachineManager::instance()->stopRemote(name);

    if (sender() == &m_remoteSdkBtn && sdkParams(name, params))
        MerVirtualMachineManager::instance()->stopRemote(name);
}

bool MerSdkManager::emulatorParams(QString &emulatorName,
                                   SshConnectionParameters &params) const
{
    Project *p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (!p)
        return false;
    Target *target = p->activeTarget();
    if (!target)
        return false;
    RemoteLinuxRunConfiguration *mrc =
            qobject_cast<RemoteLinuxRunConfiguration *>(target->activeRunConfiguration());
    if (!mrc)
        return false;
    const IDevice::ConstPtr config = DeviceKitInformation::device(target->kit());
    if (config.isNull() || config->machineType() != IDevice::Emulator)
        return false;
    emulatorName = config->id().toString();
    params = config->sshParameters();
    return true;
}

bool MerSdkManager::sdkParams(QString &sdkName, SshConnectionParameters &params) const
{
    Project *p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (!p)
        return false;
    Target *target = p->activeTarget();
    if (!target)
        return false;
    Kit *kit = target->kit();
    if (!kit)
        return false;
    sdkName = virtualMachineNameForKit(kit);
    if (sdkName.isNull())
        return false;

    foreach (const MerSdk& sdk , m_sdks) {
        if (sdk.virtualMachineName()==sdkName) {
            params = sdk.sshConnectionParams();
            break;
        }
    }
    return true;
}

void MerSdkManager::handleTaskAdded(const Task &task)
{
    static QRegExp regExp(QLatin1String("Virtual Machine '(.*)' is not running!"));
    if (regExp.indexIn(task.description) != -1) {
        QString vm = regExp.cap(1);
        const QMessageBox::StandardButton response =
                QMessageBox::question(0, tr("Start Virtual Machine"),
                                      tr("Virtual Machine '%1' is not running! Please start the "
                                         "Virtual Machine and retry after the Virtual Machine is "
                                         "running.\n\n"
                                         "Start Virtual Machine now?").arg(vm),
                                      QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
        if (response == QMessageBox::Yes) {
            QString name;
            SshConnectionParameters params;
            if (contains(vm))
                sdkParams(name, params);
            else
                emulatorParams(name, params);
            MerVirtualMachineManager::instance()->startRemote(name, params);
        }
    }
}

void MerSdkManager::addSdk(const QString &sdkName, MerSdk sdk)
{
    m_sdks.insert(sdkName, sdk);
    emit sdksUpdated();
}

void MerSdkManager::removeSdk(const QString &sdkName)
{
    m_sdks.remove(sdkName);
    emit sdksUpdated();
}

bool MerSdkManager::contains(const QString &sdkName) const
{
    return m_sdks.contains(sdkName);
}

void MerSdkManager::handleConnectionError(const QString &vmName, const QString &error)
{
    TaskHub* th = ProjectExplorerPlugin::instance()->taskHub();
    th->addTask(Task(Task::Error,
                     tr("%1: %2").arg(vmName, error),
                     Utils::FileName() /* filename */,
                     -1 /* linenumber */,
                     Core::Id(MER_TASKHUB_CATEGORY)));
}

bool MerSdkManager::validateTarget(const MerSdk &sdk, const QString &target)
{
    QString toolchainId = sdk.toolChains().value(target);
    int qtversionId = sdk.qtVersions().value(target);

    if (toolchainId.isEmpty() || qtversionId == 0)
        return false;

    QtSupport::BaseQtVersion* version =
            QtSupport::QtVersionManager::instance()->version(qtversionId);
    ToolChain* toolchain =  ToolChainManager::instance()->findToolChain(toolchainId);

    if (!version || !toolchain)
        return false;
    if (version->type() != QLatin1String(Constants::MER_QT))
        return false;
    if (toolchain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
        return false;

    QList<Kit*> kits = KitManager::instance()->kits();
    foreach (Kit* kit, kits) {
        ToolChain* kitToolChain = ToolChainKitInformation::toolChain(kit);
        QtSupport::BaseQtVersion* kitVersion = QtSupport::QtKitInformation::qtVersion(kit);
        if (kitToolChain == toolchain && kitVersion == version)
            return true;
    }
    return false;
}

bool MerSdkManager::validateKit(const Kit* kit)
{
    if (!kit)
        return false;
    ToolChain* toolchain = ToolChainKitInformation::toolChain(kit);
    QtSupport::BaseQtVersion* version = QtSupport::QtKitInformation::qtVersion(kit);

    if (!version || !toolchain)
        return false;
    if (version->type() != QLatin1String(Constants::MER_QT))
        return false;
    if (toolchain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
        return false;

    MerToolChain* mertoolchain = static_cast<MerToolChain*>(toolchain);
    MerQtVersion* merqtversion = static_cast<MerQtVersion*>(version);

    return mertoolchain->virtualMachineName() == merqtversion->virtualMachineName()
            && mertoolchain->targetName() == merqtversion->targetName();
}

void MerSdkManager::addToEnvironment(const QString &sdkName, Utils::Environment &env)
{
    const VirtualMachineInfo vmInfo = VirtualBoxManager::fetchVirtualMachineInfo(sdkName);
    const MerSdk sdk = MerSdkManager::instance()->sdk(sdkName);
    QSsh::SshConnectionParameters sshParams = sdk.sshConnectionParams();
    const QString sshPort = QString::number(vmInfo.sshPort ? vmInfo.sshPort : sshParams.port);
    const QString sharedHome =
            QDir::fromNativeSeparators(vmInfo.sharedHome.isEmpty() ? sdk.sharedHomePath()
                                                                   : vmInfo.sharedHome);
    const QString sharedTarget =
            QDir::fromNativeSeparators(vmInfo.sharedTarget.isEmpty() ? sdk.sharedTargetPath()
                                                                     : vmInfo.sharedTarget);


    env.appendOrSet(QLatin1String("MER_SSH_USERNAME"),
                    QLatin1String(Constants::MER_SDK_DEFAULTUSER));
    env.appendOrSet(QLatin1String("MER_SSH_PORT"), sshPort);
    env.appendOrSet(QLatin1String("MER_SSH_PRIVATE_KEY"), sshParams.privateKeyFile);
    env.appendOrSet(QLatin1String("MER_SSH_SHARED_HOME"), sharedHome);
    env.appendOrSet(QLatin1String("MER_SSH_SHARED_TARGET"), sharedTarget);
}

} // Internal
} // Mer
