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
#include "merplugin.h"

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
#include <ssh/sshkeygenerator.h>

#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>
#include <QDesktopServices>

#ifdef WITH_TESTS
#include <QtTest>
#endif

using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace RemoteLinux;
using namespace QSsh;
using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerSdkManager *MerSdkManager::m_sdkManager = 0;
ProjectExplorer::Project *MerSdkManager::m_previousProject = 0;

static Utils::FileName globalSettingsFileName()
{
    QSettings *globalSettings = ExtensionSystem::PluginManager::globalSettings();
    return Utils::FileName::fromString(QFileInfo(globalSettings->fileName()).absolutePath()
                                       + QLatin1String(MER_SDK_FILENAME));
}

static Utils::FileName settingsFileName()
{
     QFileInfo settingsLocation(ExtensionSystem::PluginManager::settings()->fileName());
     return Utils::FileName::fromString(settingsLocation.absolutePath() + QLatin1String(MER_SDK_FILENAME));
}

MerSdkManager::MerSdkManager()
    : m_intialized(false),
      m_writer(0)
{
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), SLOT(storeSdks()));

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

    m_writer = new Utils::PersistentSettingsWriter(settingsFileName(), QLatin1String("MerSDKs"));
    m_sdkManager = this;
}

MerSdkManager::~MerSdkManager()
{
    qDeleteAll(m_sdks.values());
    m_sdkManager = 0;
}

void MerSdkManager::initialize()
{
    if (!m_intialized) {
        restore();
        //read kits
        QList<Kit*> kits = merKits();
        QList<MerToolChain*> toolchains = merToolChains();
        QList<MerQtVersion*> qtversions = merQtVersions();
        //cleanup
        foreach (MerToolChain *toolchain, toolchains) {
            const MerSdk *sdk = m_sdks[toolchain->virtualMachineName()];
            if (sdk && sdk->targets().contains(toolchain->targetName()))
                continue;
            qWarning() << "MerToolChain wihout target found. Removing toolchain.";
            ToolChainManager::instance()->deregisterToolChain(toolchain);
        }

        foreach (MerQtVersion *version, qtversions) {
            const MerSdk *sdk = m_sdks[version->virtualMachineName()];
            if (sdk && sdk->targets().contains(version->targetName()))
                continue;
            qWarning() << "MerQtVersion without target found. Removing qtversion.";
            QtVersionManager::instance()->removeVersion(version);
        }

        //remove broken kits
        foreach (Kit *kit, kits) {
            if (!validateKit(kit)) {
                qWarning() << "Broken Mer kit found !. Removing kit.";
                KitManager::instance()->deregisterKit(kit);
            }
        }
        m_intialized = true;
    }
}

QList<Kit *> MerSdkManager::merKits() const
{
    QList<Kit*> kits;
    foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits()) {
        if (isMerKit(kit))
            kits << kit;
    }
    return kits;
}

QList<MerToolChain *> MerSdkManager::merToolChains() const
{
    QList<MerToolChain*> toolchains;
    foreach (ProjectExplorer::ToolChain *toolchain, ProjectExplorer::ToolChainManager::instance()->toolChains()) {
        if (!toolchain->isAutoDetected())
            continue;
        if (toolchain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
            continue;
        toolchains << static_cast<MerToolChain*>(toolchain);
    }
    return toolchains;
}

QList<MerQtVersion *> MerSdkManager::merQtVersions() const
{
    QList<MerQtVersion*> qtversions;
    foreach (BaseQtVersion *qtVersion, QtSupport::QtVersionManager::instance()->versions()) {
        if (!qtVersion->isAutodetected())
            continue;
        if (qtVersion->type() != QLatin1String(Constants::MER_QT))
            continue;
        qtversions << static_cast<MerQtVersion*>(qtVersion);
    }
    return qtversions;
}

MerSdkManager *MerSdkManager::instance()
{
    QTC_CHECK(m_sdkManager);
    return m_sdkManager;
}

QList<MerSdk*> MerSdkManager::sdks() const
{
    return m_sdks.values();
}

void MerSdkManager::restore()
{
    //first global location
    QList<MerSdk*> globalSDK = restoreSdks(globalSettingsFileName());
    Q_UNUSED(globalSDK)
    //TODO some logic :)
    //local location
    QList<MerSdk*> localSDK = restoreSdks(settingsFileName());

    foreach (MerSdk *sdk, localSDK)
        addSdk(sdk);
}

QList<MerSdk*> MerSdkManager::restoreSdks(const Utils::FileName &fileName)
{
    QList<MerSdk*> result;

    Utils::PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();

    int version = data.value(QLatin1String(MER_SDK_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return result;

    int count = data.value(QLatin1String(MER_SDK_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap merSdkMap = data.value(key).toMap();
        MerSdk *sdk = new MerSdk();
        if (!sdk->fromMap(merSdkMap)) {
             qWarning() << sdk->virtualMachineName()<<"is configured incorrectly...";
        }
        result << sdk;
    }
    return result;
}

void MerSdkManager::storeSdks() const
{
    QVariantMap data;
    data.insert(QLatin1String(MER_SDK_FILE_VERSION_KEY), 1);
    int count = 0;
    foreach (const MerSdk* sdk, m_sdks) {
        if (!sdk->isValid()) {
            qWarning() << sdk->virtualMachineName()<<"is configured incorrectly...";
        }
        QVariantMap tmp = sdk->toMap();
        if (tmp.isEmpty())
            continue;
        data.insert(QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    data.insert(QLatin1String(MER_SDK_COUNT_KEY), count);
    m_writer->save(data, Core::ICore::mainWindow());
}

bool MerSdkManager::isMerKit(Kit *kit)
{
    if (!kit)
        return false;
    if (!kit->isAutoDetected())
        return false;

    ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    if (tc && tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
        return true;
    if (deviceType.isValid() && deviceType == Core::Id(Constants::MER_DEVICE_TYPE))
        return true;

    return false;
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

bool MerSdkManager::authorizePublicKey(const QString &authorizedKeysPath,
                                       const QString &pubKeyPath,
                                       QString &error)
{
    bool success = false;
    QFileInfo fi(pubKeyPath);
    if (!fi.exists()) {
        error.append(tr("Error: File %1 is missing.").arg(pubKeyPath));
        return success;
    }

    Utils::FileReader pubKeyReader;
    success = pubKeyReader.fetch(pubKeyPath);
    if (!success) {
        error.append(tr("Error: %1").arg(pubKeyReader.errorString()));
        return success;
    }

    const QByteArray publicKey = pubKeyReader.data();

    QDir sshDirectory(QFileInfo(authorizedKeysPath).absolutePath());
    if (!sshDirectory.exists() && !sshDirectory.mkpath(sshDirectory.absolutePath())) {
        error.append(tr("Error: Could not create directory %1").arg(sshDirectory.absolutePath()));
        success = false;
        return success;
    }

    QFileInfo akFi(authorizedKeysPath);
    if (!akFi.exists()) {
        //create new key
        Utils::FileSaver authKeysSaver(authorizedKeysPath, QIODevice::WriteOnly);
        authKeysSaver.write(publicKey);
        success = authKeysSaver.finalize();
        if (!success) {
            error.append(tr("Error: %1").arg(authKeysSaver.errorString()));
            return success;
        }
        QFile::setPermissions(authorizedKeysPath, QFile::ReadOwner|QFile::WriteOwner);
    } else {
        //append
        Utils::FileReader authKeysReader;
        success = authKeysReader.fetch(authorizedKeysPath);
        if (!success) {
            error.append(tr("Error: %1").arg(authKeysReader.errorString()));
            return success;
        }
        success = !authKeysReader.data().contains(publicKey);
        if (!success) {
            error.append(tr("Key already authorized ! \n %1 already in %2").arg(pubKeyPath).arg(authorizedKeysPath));
            return success;
        }
        // File does not contain the public key. Append it to file.
        Utils::FileSaver authorizedKeysSaver(authorizedKeysPath, QIODevice::Append);
        authorizedKeysSaver.write("\n");
        authorizedKeysSaver.write(publicKey);
        success = authorizedKeysSaver.finalize();
        if (!success) {
            error.append(tr("Error: %1").arg(authorizedKeysSaver.errorString()));
            return success;
        }
    }

    return success;
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
        MerVirtualMachineManager::instance()->stopRemote(name, params);

    if (sender() == &m_remoteSdkBtn && sdkParams(name, params))
        MerVirtualMachineManager::instance()->stopRemote(name, params);
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

    const MerSdk *sdk = m_sdks[sdkName];
    QTC_ASSERT(sdk, return false);
    params.port = sdk->sshPort();
    params.userName = sdk->userName();
    params.authenticationType = SshConnectionParameters::AuthenticationByKey;
    params.privateKeyFile = sdk->privateKeyFile();
    params.host = sdk->host();

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
            if (sdk(vm))
                sdkParams(name, params);
            else
                emulatorParams(name, params);
            MerVirtualMachineManager::instance()->startRemote(name, params);
        }
    }
}

bool MerSdkManager::hasSdk(const MerSdk* sdk) const
{
    return m_sdks.contains(sdk->virtualMachineName());
}

// takes ownership
void MerSdkManager::addSdk(MerSdk* sdk)
{
    if (m_sdks.contains(sdk->virtualMachineName()))
        return;
    m_sdks.insert(sdk->virtualMachineName(), sdk);
    connect(sdk,SIGNAL(targetsChanged(QStringList)),this,SIGNAL(sdksUpdated()));
    sdk->attach();
    emit sdksUpdated();
}

// pass back the ownership
void MerSdkManager::removeSdk(MerSdk* sdk)
{
    if (!m_sdks.contains(sdk->virtualMachineName()))
        return;
    m_sdks.remove(sdk->virtualMachineName());
    disconnect(sdk,SIGNAL(targetsChanged(QStringList)),this,SIGNAL(sdksUpdated()));
    sdk->detach();
    emit sdksUpdated();
}

//ownership passed to caller
MerSdk* MerSdkManager::createSdk(const QString &vmName)
{
    MerSdk *sdk = new MerSdk();

    VirtualMachineInfo info =
            VirtualBoxManager::fetchVirtualMachineInfo(vmName);
    sdk->setVirtualMachineName(vmName);
    sdk->setSshPort(info.sshPort);
    sdk->setWwwPort(info.wwwPort);
    //TODO:
    sdk->setHost(QLatin1String(MER_SDK_DEFAULTHOST));
    //TODO:
    sdk->setUserName(QLatin1String(MER_SDK_DEFAULTUSER));
    const QString sshDirectory(QDir::fromNativeSeparators(QDesktopServices::storageLocation(
                                                              QDesktopServices::HomeLocation))+ QLatin1String("/.ssh"));
    sdk->setPrivateKeyFile(QDir::toNativeSeparators(QString::fromLatin1("%1/id_rsa").arg(sshDirectory)));
    sdk->setSharedHomePath(info.sharedHome);
    sdk->setSharedTargetPath(info.sharedTarget);
    sdk->setSharedSshPath(info.sharedSsh);
    return sdk;
}


MerSdk* MerSdkManager::sdk(const QString &sdkName) const
{
    return m_sdks[sdkName];
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

bool MerSdkManager::validateKit(const Kit* kit)
{
    if (!kit)
        return false;
    ToolChain* toolchain = ToolChainKitInformation::toolChain(kit);
    QtSupport::BaseQtVersion* version = QtSupport::QtKitInformation::qtVersion(kit);
    Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);

    if (!version || !toolchain || !deviceType.isValid())
        return false;
    if (version->type() != QLatin1String(Constants::MER_QT))
        return false;
    if (toolchain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
        return false;
    if (deviceType != Core::Id(Constants::MER_DEVICE_TYPE))
        return false;

    MerToolChain* mertoolchain = static_cast<MerToolChain*>(toolchain);
    MerQtVersion* merqtversion = static_cast<MerQtVersion*>(version);

    return mertoolchain->virtualMachineName() == merqtversion->virtualMachineName()
            && mertoolchain->targetName() == merqtversion->targetName();
}

void MerSdkManager::addToEnvironment(const QString &sdkName, Utils::Environment &env)
{
    const VirtualMachineInfo vmInfo = VirtualBoxManager::fetchVirtualMachineInfo(sdkName);
    const MerSdk *sdk = MerSdkManager::instance()->sdk(sdkName);
    QTC_CHECK(sdk);
    const QString sshPort = QString::number(vmInfo.sshPort ? vmInfo.sshPort : sdk->sshPort());
    const QString sharedHome =
            QDir::fromNativeSeparators(vmInfo.sharedHome.isEmpty() ? sdk->sharedHomePath()
                                                                   : vmInfo.sharedHome);
    const QString sharedTarget =
            QDir::fromNativeSeparators(vmInfo.sharedTarget.isEmpty() ? sdk->sharedTargetPath()
                                                                     : vmInfo.sharedTarget);


    env.appendOrSet(QLatin1String("MER_SSH_USERNAME"),
                    QLatin1String(Constants::MER_SDK_DEFAULTUSER));
    env.appendOrSet(QLatin1String("MER_SSH_PORT"), sshPort);
    env.appendOrSet(QLatin1String("MER_SSH_PRIVATE_KEY"), sdk->privateKeyFile());
    env.appendOrSet(QLatin1String("MER_SSH_SHARED_HOME"), sharedHome);
    env.appendOrSet(QLatin1String("MER_SSH_SHARED_TARGET"), sharedTarget);
}

bool MerSdkManager::generateSshKey(const QString &privKeyPath, QString &error)
{
    if (QFileInfo(privKeyPath).exists()) {
        error.append(tr("Error: File '%1' exists.\n").arg(privKeyPath));
        return false;
    }

    bool success = true;
    QSsh::SshKeyGenerator keyGen;
    success = keyGen.generateKeys(QSsh::SshKeyGenerator::Rsa,
                                  QSsh::SshKeyGenerator::Mixed, 2048,
                                  QSsh::SshKeyGenerator::DoNotOfferEncryption);
    if (!success) {
        error.append(tr("Error: %1\n").arg(keyGen.error()));
        return false;
    }

    Utils::FileSaver privKeySaver(privKeyPath);
    privKeySaver.write(keyGen.privateKey());
    success = privKeySaver.finalize();
    if (!success) {
        error.append(tr("Error: %1\n").arg(privKeySaver.errorString()));
        return false;
    }

    Utils::FileSaver pubKeySaver(privKeyPath + QLatin1String(".pub"));
    const QByteArray publicKey = keyGen.publicKey();
    pubKeySaver.write(publicKey);
    success = pubKeySaver.finalize();
    if (!success) {
        error.append(tr("Error: %1\n").arg(pubKeySaver.errorString()));
        return false;
    }
    return true;
}

#ifdef WITH_TESTS

void MerPlugin::verifyTargets(QString vm ,QStringList expectedKits,QStringList expectedToolChains,QStringList expectedQtVersions)
{
    MerSdkManager* sdkManager = MerSdkManager::instance();
    QList<ProjectExplorer::Kit *> kits = sdkManager->merKits();
    QList<MerToolChain *> toolchains = sdkManager->merToolChains();
    QList<MerQtVersion *> qtversions = sdkManager->merQtVersions();

    foreach(ProjectExplorer::Kit* x , kits) {
        QString target = MerSdkManager::targetNameForKit(x);
        if(expectedKits.contains(target)) {
            expectedKits.removeAll(target);
            continue;
        }
        QFAIL("Unexpected kit created");
    }
    QVERIFY2(expectedKits.isEmpty(), "Expected kits not created");

    foreach(MerToolChain* x , toolchains) {
        QString target = x->targetName();
        QVERIFY2(x->virtualMachineName() == vm, "VirtualMachine name does not match");
        if(expectedToolChains.contains(target)) {
            expectedToolChains.removeAll(target);
            continue;
        }
        QFAIL("Unexpected toolchain created");
    }
    QVERIFY2(expectedToolChains.isEmpty(), "Expected toolchains not created");

    foreach(MerQtVersion* x , qtversions) {
        QString target = x->targetName();
        QVERIFY2(x->virtualMachineName() == vm, "VirtualMachine name does not match");
        if(expectedQtVersions.contains(target)) {
            expectedQtVersions.removeAll(target);
            continue;
        }
        QFAIL("Unexpected qtversion created");
    }
    QVERIFY2(expectedQtVersions.isEmpty(), "Expected qtverion not created");
}

void MerPlugin::testMerSdkManager_data()
{
    QTest::addColumn<QString>("filepath");
    QTest::addColumn<QString>("virtualMachine");
    QTest::addColumn<QStringList>("targets");
    QTest::addColumn<QStringList>("expectedTargets");
    QTest::newRow("testcase1") << "./testcase1" << "TestVM" << QStringList() << (QStringList() << QLatin1String("SailfishOS-i486-x86-1"));
    QTest::newRow("testcase2") << "./testcase2" << "TestVM" << (QStringList() << QLatin1String("SailfishOS-i486-x86-1")) << (QStringList() << QLatin1String("SailfishOS-i486-x86-2"));
    QTest::newRow("testcase3") << "./testcase3" << "TestVM" << (QStringList() << QLatin1String("SailfishOS-i486-x86-1")) << (QStringList());
}

void MerPlugin::testMerSdkManager()
{
    QFETCH(QString, filepath);
    QFETCH(QString, virtualMachine);
    QFETCH(QStringList, targets);
    QFETCH(QStringList, expectedTargets);

    const QString& initFile = filepath + QDir::separator() + QLatin1String("init.xml");
    const QString& targetFile = filepath + QDir::separator() + QLatin1String("targets.xml");
    const QString& runFile = filepath + QDir::separator() + QLatin1String("run.xml");


    QFileInfo initfi(initFile);
    QVERIFY2(initfi.exists(),"Missing init.xml");
    QFileInfo runfi(runFile);
    QVERIFY2(runfi.exists(),"Missing run.xml");

    QStringList initToolChains = targets;
    QStringList initQtVersions = targets;
    QStringList initKits = targets;

    QStringList expectedToolChains = expectedTargets;
    QStringList expectedQtVersions = expectedTargets;
    QStringList expectedKits = expectedTargets;

    foreach(const QString& target, targets) {
        if(target.isEmpty()) break;
        QVERIFY2(QDir(filepath).mkdir(target),"Could not create fake sysroot");
    }
    foreach(const QString& target, expectedTargets) {
        if(target.isEmpty()) break;
        QVERIFY2(QDir(filepath).mkdir(target),"Could not create fake sysroot");
    }


    QVERIFY2(QFile::copy(initFile,targetFile),"Could not copy init.xml to target.xml");

    MerSdkManager* sdkManager = MerSdkManager::instance();
    QVERIFY(sdkManager);
    QVERIFY(sdkManager->sdks().isEmpty());
    MerSdk* sdk = sdkManager->createSdk(virtualMachine);
    QVERIFY(sdk);
    QVERIFY(!sdk->isValid());

    sdk->setSharedSshPath(QDir::toNativeSeparators(filepath));
    sdk->setSharedHomePath(QDir::toNativeSeparators(filepath));
    sdk->setSharedTargetPath(QDir::toNativeSeparators(filepath));

    QVERIFY(sdk->isValid());
    sdkManager->addSdk(sdk);

    QVERIFY(!sdkManager->sdks().isEmpty());

    verifyTargets(virtualMachine,initKits,initToolChains,initQtVersions);

    QVERIFY2(QFile::remove(targetFile),"Could not remove target.xml");
    QVERIFY2(QFile::copy(runFile,targetFile),"Could not copy run.xml to target.xml");

    QSignalSpy spy(sdk, SIGNAL(targetsChanged(QStringList)));
    int i = 0;
    while (spy.count() == 0 && i++ < 50)
        QTest::qWait(200);

    verifyTargets(virtualMachine,expectedKits,expectedToolChains,expectedQtVersions);

    sdkManager->removeSdk(sdk);

    QList<ProjectExplorer::Kit *> kits = sdkManager->merKits();
    QList<MerToolChain *> toolchains = sdkManager->merToolChains();
    QList<MerQtVersion *> qtversions = sdkManager->merQtVersions();

    QVERIFY2(kits.isEmpty(), "Merkit not removed");
    QVERIFY2(toolchains.isEmpty(), "Toolchain not removed");
    QVERIFY2(qtversions.isEmpty(), "QtVersion not removed");
    QVERIFY(sdkManager->sdks().isEmpty());
    //cleanup
    QVERIFY2(QFile::remove(targetFile),"Could not remove target.xml");
    foreach(const QString& target, expectedTargets) {
            QDir(filepath).rmdir(target);
    }
    foreach(const QString& target, targets) {
            QDir(filepath).rmdir(target);
    }
}
#endif

} // Internal
} // Mer
