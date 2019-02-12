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

#include "mersdkmanager.h"

#include "merconnectionmanager.h"
#include "merconstants.h"
#include "merdevicefactory.h"
#include "merdevicexmlparser.h"
#include "meremulatordevice.h"
#include "merhardwaredevice.h"
#include "merlogging.h"
#include "merplugin.h"
#include "merqtversion.h"
#include "mersdkkitinformation.h"
#include "mertarget.h"
#include "mertargetkitinformation.h"
#include "mertoolchain.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <ssh/sshkeygenerator.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QDir>
#include <QMenu>

#ifdef WITH_TESTS
#include <QtTest>
#endif

using namespace ExtensionSystem;
using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Core;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerSdkManager *MerSdkManager::m_instance = 0;

static FileName globalSettingsFileName()
{
    QSettings *globalSettings = PluginManager::globalSettings();
    return FileName::fromString(QFileInfo(globalSettings->fileName()).absolutePath()
                                + QLatin1String(MER_SDK_FILENAME));
}

static FileName settingsFileName()
{
     QFileInfo settingsLocation(PluginManager::settings()->fileName());
     return FileName::fromString(settingsLocation.absolutePath() + QLatin1String(MER_SDK_FILENAME));
}

MerSdkManager::MerSdkManager()
    : m_intialized(false),
      m_writer(0),
      m_version(0)
{
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &MerSdkManager::storeSdks);
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &MerSdkManager::initialize);
    connect(DeviceManager::instance(), &DeviceManager::devicesLoaded,
            this, &MerSdkManager::updateDevices);
    connect(DeviceManager::instance(), &DeviceManager::updated,
            this, &MerSdkManager::updateDevices);
    m_writer = new PersistentSettingsWriter(settingsFileName(), QLatin1String("MerSDKs"));
    m_instance = this;
    KitManager::registerKitInformation<MerSdkKitInformation>();
    KitManager::registerKitInformation<MerTargetKitInformation>();
}

MerSdkManager::~MerSdkManager()
{
    qDeleteAll(m_sdks.values());
    m_instance = 0;
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
            const MerSdk *sdk = m_sdks.value(toolchain->virtualMachineName());
            if (sdk && sdk->targetNames().contains(toolchain->targetName()))
                continue;
            qWarning() << "MerToolChain wihout target found. Removing toolchain.";
            ToolChainManager::deregisterToolChain(toolchain);
        }

        foreach (MerQtVersion *version, qtversions) {
            const MerSdk *sdk = m_sdks.value(version->virtualMachineName());
            if (sdk && sdk->targetNames().contains(version->targetName()))
                continue;
            qWarning() << "MerQtVersion without target found. Removing qtversion.";
            QtVersionManager::removeVersion(version);
        }

        //remove broken kits
        foreach (Kit *kit, kits) {
            if (!validateKit(kit)) {
                qWarning() << "Broken Mer kit found! Removing kit.";
                KitManager::deregisterKit(kit);
            }else{
                kit->validate();
            }
        }

        // If debugger became available
        foreach (MerSdk *sdk, m_sdks) {
            foreach (const MerTarget &target, sdk->targets()) {
                foreach (Kit *kit, kitsForTarget(target.name())) {
                    target.ensureDebuggerIsSet(kit);
                }
            }
        }

        m_intialized = true;
        updateDevices();
        emit initialized();
    }
}

QList<Kit *> MerSdkManager::merKits()
{
    QList<Kit*> kits;
    foreach (Kit *kit, KitManager::kits()) {
        if (isMerKit(kit))
            kits << kit;
    }
    return kits;
}

QList<MerToolChain *> MerSdkManager::merToolChains()
{
    QList<MerToolChain*> toolchains;
    foreach (ToolChain *toolchain, ToolChainManager::toolChains()) {
        if (!toolchain->isAutoDetected())
            continue;
        if (toolchain->typeId() != Constants::MER_TOOLCHAIN_ID)
            continue;
        toolchains << static_cast<MerToolChain*>(toolchain);
    }
    return toolchains;
}

QList<MerQtVersion *> MerSdkManager::merQtVersions()
{
    QList<MerQtVersion*> qtversions;
    foreach (BaseQtVersion *qtVersion, QtVersionManager::versions()) {
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
    QTC_CHECK(m_instance);
    return m_instance;
}

QList<MerSdk*> MerSdkManager::sdks()
{
    return m_instance->m_sdks.values();
}

FileName MerSdkManager::checkLocalConfig(const FileName &local, const FileName &global)
{
    PersistentSettingsReader lReader;
    if (!lReader.load(local)) {
        // local file not found
        return global;
    }

    PersistentSettingsReader gReader;
    if (!gReader.load(global)) {
        // global file read failed, use the local file then.
        return local;
    }

    QVariantMap lData = lReader.restoreValues();
    QVariantMap gData = gReader.restoreValues();

    int lVersion = lData.value(QLatin1String(MER_SDK_FILE_VERSION_KEY), 0).toInt();
    int gVersion = gData.value(QLatin1String(MER_SDK_FILE_VERSION_KEY), 0).toInt();
    QString lInstallDir = lData.value(QLatin1String(MER_SDK_INSTALLDIR)).toString();
    QString gInstallDir = gData.value(QLatin1String(MER_SDK_INSTALLDIR)).toString();

    bool reinstall = false;
    if (lVersion != gVersion) {
        qCDebug(Log::sdks) << "MerSdkManager: version changed => use global config";
        reinstall = true;
    } else if (lInstallDir != gInstallDir) {
        qCDebug(Log::sdks) << "MerSdkManager: installdir changed => use global config";
        reinstall = true;
    } else {
        qCDebug(Log::sdks) << "MerSdkManager: version and installdir same => use local config";
    }

    // This is executed if the version changed or the user has reinstalled
    // MerSDK to a different directory. SDKs will be restored from the global
    // configuration.
    if (reinstall) {
        // Clean up all the existing Mer kits.
        foreach (Kit *kit, KitManager::kits()) {
            if (!kit->isAutoDetected())
                continue;
            ToolChain *tc = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
            ToolChain *tc_c = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::C_LANGUAGE_ID);
            if (!tc)
                continue;

            if (tc->typeId() == Constants::MER_TOOLCHAIN_ID) {
                qCDebug(Log::sdks) << "Removing Mer kit due to reinstall";
                BaseQtVersion *v = QtKitInformation::qtVersion(kit);
                KitManager::deregisterKit(kit);
                ToolChainManager::deregisterToolChain(tc);
                if (tc_c)
                    ToolChainManager::deregisterToolChain(tc_c);
                QtVersionManager::removeVersion(v);
            }
        }

        // Remove tools directories - see MerTarget::deleteScripts()
        int count = lData.value(QLatin1String(MER_SDK_COUNT_KEY), 0).toInt();
        for (int i = 0; i < count; ++i) {
            const QString key = QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(i);
            if (!lData.contains(key))
                break;

            const QVariantMap merSdkMap = lData.value(key).toMap();

            const QString vmName = merSdkMap.value(QLatin1String(VM_NAME)).toString();
            const QString toolsPath = sdkToolsDirectory() + vmName;
            FileUtils::removeRecursively(FileName::fromString(toolsPath));
        }
    }

    return reinstall ? global : local;
}

void MerSdkManager::restore()
{
    FileName settings = checkLocalConfig(settingsFileName(), globalSettingsFileName());
    QList<MerSdk*> sdks = restoreSdks(settings);
    foreach (MerSdk *sdk, sdks) {
        addSdk(sdk);
    }
}

QList<MerSdk*> MerSdkManager::restoreSdks(const FileName &fileName)
{
    QList<MerSdk*> result;
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();

    int version = data.value(QLatin1String(MER_SDK_FILE_VERSION_KEY), 0).toInt();
    if (version < 1) {
        qWarning() << "invalid configuration version: " << version;
        return result;
    }
    m_instance->m_version = version;

    m_instance->m_installDir = data.value(QLatin1String(MER_SDK_INSTALLDIR)).toString();

    int count = data.value(QLatin1String(MER_SDK_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap merSdkMap = data.value(key).toMap();
        MerSdk *sdk = new MerSdk();
        if (!sdk->fromMap(merSdkMap)) {
             qWarning() << sdk->virtualMachineName() << "is configured incorrectly...";
        }
        result << sdk;
    }

    return result;
}

void MerSdkManager::storeSdks()
{
    QVariantMap data;
    data.insert(QLatin1String(MER_SDK_FILE_VERSION_KEY), m_version);
    data.insert(QLatin1String(MER_SDK_INSTALLDIR), m_installDir);
    int count = 0;
    foreach (const MerSdk* sdk, m_instance->m_sdks) {
        if (!sdk->isValid()) {
            qWarning() << sdk->virtualMachineName() << "is configured incorrectly...";
        }
        QVariantMap tmp = sdk->toMap();
        if (tmp.isEmpty())
            continue;
        data.insert(QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    data.insert(QLatin1String(MER_SDK_COUNT_KEY), count);
    m_instance->m_writer->save(data, ICore::mainWindow());
}

bool MerSdkManager::isMerKit(const Kit *kit)
{
    if (!kit)
        return false;
    if (!MerSdkKitInformation::sdk(kit))
        return false;

    ToolChain* tc = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    if (tc && tc->typeId() == Constants::MER_TOOLCHAIN_ID)
        return true;
    if (deviceType.isValid() && MerDeviceFactory::canCreate(deviceType))
        return true;

    return false;
}

QString MerSdkManager::targetNameForKit(const Kit *kit)
{
    if (!kit || !isMerKit(kit))
        return QString();
    ToolChain *toolchain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (toolchain && toolchain->typeId() == Constants::MER_TOOLCHAIN_ID) {
        MerToolChain *mertoolchain = static_cast<MerToolChain *>(toolchain);
        return mertoolchain->targetName();
    }
    return QString();
}

QList<Kit *> MerSdkManager::kitsForTarget(const QString &targetName)
{
    QList<Kit*> kitsForTarget;
    if (targetName.isEmpty())
        return kitsForTarget;
    const QList<Kit*> kits = KitManager::kits();
    foreach (Kit *kit, kits) {
        if (targetNameForKit(kit) == targetName)
            kitsForTarget << kit;
    }
    return kitsForTarget;
}

bool MerSdkManager::hasMerDevice(Kit *kit)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(kit);
    if (dev.isNull())
        return false;
    return MerDeviceFactory::canCreate(dev->type());
}

QString MerSdkManager::sdkToolsDirectory()
{
    return QFileInfo(PluginManager::settings()->fileName()).absolutePath() +
            QLatin1String(Constants::MER_SDK_TOOLS);
}

QString MerSdkManager::globalSdkToolsDirectory()
{
    return QFileInfo(PluginManager::globalSettings()->fileName()).absolutePath() +
            QLatin1String(Constants::MER_SDK_TOOLS);
}

bool MerSdkManager::authorizePublicKey(const QString &authorizedKeysPath,
                                       const QString &pubKeyPath,
                                       QString &error)
{
    bool success = false;
    QFileInfo fi(pubKeyPath);
    if (!fi.exists()) {
        error.append(tr("Error: File %1 is missing.").arg(QDir::toNativeSeparators(pubKeyPath)));
        return success;
    }

    FileReader pubKeyReader;
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
        FileSaver authKeysSaver(authorizedKeysPath, QIODevice::WriteOnly);
        authKeysSaver.write(publicKey);
        success = authKeysSaver.finalize();
        if (!success) {
            error.append(tr("Error: %1").arg(authKeysSaver.errorString()));
            return success;
        }
        QFile::setPermissions(authorizedKeysPath, QFile::ReadOwner|QFile::WriteOwner);
    } else {
        //append
        FileReader authKeysReader;
        success = authKeysReader.fetch(authorizedKeysPath);
        if (!success) {
            error.append(tr("Error: %1").arg(authKeysReader.errorString()));
            return success;
        }
        success = !authKeysReader.data().contains(publicKey);
        if (!success) {
            error.append(tr("Key already authorized!\n %1 already in %2").arg(QDir::toNativeSeparators(pubKeyPath)).arg(QDir::toNativeSeparators(authorizedKeysPath)));
            return success;
        }
        // File does not contain the public key. Append it to file.
        FileSaver authorizedKeysSaver(authorizedKeysPath, QIODevice::Append);
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

bool MerSdkManager::hasSdk(const MerSdk* sdk)
{
    return m_instance->m_sdks.contains(sdk->virtualMachineName());
}

// takes ownership
void MerSdkManager::addSdk(MerSdk *sdk)
{
    if (m_instance->m_sdks.contains(sdk->virtualMachineName()))
        return;
    m_instance->m_sdks.insert(sdk->virtualMachineName(), sdk);
    connect(sdk, &MerSdk::targetsChanged,
            m_instance, &MerSdkManager::sdksUpdated);
    connect(sdk, &MerSdk::privateKeyChanged,
            m_instance, &MerSdkManager::sdksUpdated);
    connect(sdk, &MerSdk::headlessChanged,
            m_instance, &MerSdkManager::sdksUpdated);
    connect(sdk, &MerSdk::sshPortChanged,
            m_instance, &MerSdkManager::sdksUpdated);
    connect(sdk, &MerSdk::wwwPortChanged,
            m_instance, &MerSdkManager::sdksUpdated);
    sdk->attach();
    emit m_instance->sdksUpdated();
}

// pass back the ownership
void MerSdkManager::removeSdk(MerSdk *sdk)
{
    if (!m_instance->m_sdks.contains(sdk->virtualMachineName()))
        return;
    m_instance->m_sdks.remove(sdk->virtualMachineName());
    sdk->disconnect(m_instance);
    sdk->detach();
    emit m_instance->sdksUpdated();
}

//ownership passed to caller
MerSdk* MerSdkManager::createSdk(const QString &vmName)
{
    MerSdk *sdk = new MerSdk();

    VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(vmName);
    sdk->setVirtualMachineName(vmName);
    sdk->setSshPort(info.sshPort);
    sdk->setWwwPort(info.wwwPort);
    //TODO:
    sdk->setHost(QLatin1String(MER_SDK_DEFAULTHOST));
    //TODO:
    sdk->setUserName(QLatin1String(MER_SDK_DEFAULTUSER));

    QString sshDirectory;
    if(info.sharedConfig.isEmpty())
        sshDirectory = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))+ QLatin1String("/.ssh");
    else
        sshDirectory = info.sharedConfig + QLatin1String("/ssh/private_keys/engine/") + QLatin1String(MER_SDK_DEFAULTUSER);
    sdk->setPrivateKeyFile(QDir::toNativeSeparators(sshDirectory));
    sdk->setSharedHomePath(info.sharedHome);
    sdk->setSharedTargetsPath(info.sharedTargets);
    sdk->setSharedConfigPath(info.sharedConfig);
    sdk->setSharedSrcPath(info.sharedSrc);
    sdk->setSharedSshPath(info.sharedSsh);
    return sdk;
}


MerSdk* MerSdkManager::sdk(const QString &sdkName)
{
    return m_instance->m_sdks.value(sdkName);
}

bool MerSdkManager::validateKit(const Kit *kit)
{
    if (!kit)
        return false;
    ToolChain* toolchain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    BaseQtVersion* version = QtKitInformation::qtVersion(kit);
    Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    MerSdk* sdk = MerSdkKitInformation::sdk(kit);

    if (!version || !toolchain || !deviceType.isValid() || !sdk)
        return false;
    if (version->type() != QLatin1String(Constants::MER_QT))
        return false;
    if (toolchain->typeId() != Constants::MER_TOOLCHAIN_ID)
        return false;
    if (!MerDeviceFactory::canCreate(deviceType))
        return false;

    MerToolChain* mertoolchain = static_cast<MerToolChain*>(toolchain);
    MerQtVersion* merqtversion = static_cast<MerQtVersion*>(version);

    return  sdk->virtualMachineName() ==  mertoolchain->virtualMachineName()
            && sdk->virtualMachineName() ==  merqtversion->virtualMachineName()
            && mertoolchain->targetName() == merqtversion->targetName();
}

bool MerSdkManager::generateSshKey(const QString &privKeyPath, QString &error)
{
    if (privKeyPath.isEmpty()) {
        error.append(tr("Error: Key Path is empty.\n"));
        return false;
    }

    QFileInfo finfo(privKeyPath);

    if (finfo.exists()) {
        error.append(tr("Error: File \"%1\" exists.\n").arg(privKeyPath));
        return false;
    }

    if (!finfo.dir().exists()) {
        QDir().mkpath(finfo.dir().absolutePath());
    }

    bool success = true;
    SshKeyGenerator keyGen;
    success = keyGen.generateKeys(SshKeyGenerator::Rsa,
                                  SshKeyGenerator::OpenSsl, 2048,
                                  SshKeyGenerator::DoNotOfferEncryption);
    if (!success) {
        error.append(tr("Error: %1\n").arg(keyGen.error()));
        return false;
    }

    FileSaver privKeySaver(privKeyPath);
    privKeySaver.write(keyGen.privateKey());
    success = privKeySaver.finalize();
    if (!success) {
        error.append(tr("Error: %1\n").arg(privKeySaver.errorString()));
        return false;
    }

    // fix file permissions for private key
    QFile tmp_perm(privKeyPath);
    if (tmp_perm.open(QIODevice::WriteOnly|QIODevice::Append)) {
        QFile::setPermissions(tmp_perm.fileName(), (QFile::ReadOwner|QFile::WriteOwner));
        tmp_perm.close();
    }

    FileSaver pubKeySaver(privKeyPath + QLatin1String(".pub"));
    const QByteArray publicKey = keyGen.publicKey();
    pubKeySaver.write(publicKey);
    success = pubKeySaver.finalize();
    if (!success) {
        error.append(tr("Error: %1\n").arg(pubKeySaver.errorString()));
        return false;
    }
    return true;
}

// this method updates the Mer devices.xml, nothing else
void MerSdkManager::updateDevices()
{
    QList<MerDeviceData> devices;
    int count = DeviceManager::instance()->deviceCount();
    for(int i = 0 ;  i < count; ++i) {
        IDevice::ConstPtr d = DeviceManager::instance()->deviceAt(i);
        if (MerDeviceFactory::canCreate(d->type())) {
            MerDeviceData xmlData;
            if (d->machineType() == IDevice::Hardware) {
                Q_ASSERT(dynamic_cast<const MerHardwareDevice*>(d.data()) != 0);
                const MerHardwareDevice* device = static_cast<const MerHardwareDevice*>(d.data());
                xmlData.m_ip = device->sshParameters().host();
                xmlData.m_name = device->displayName();
                xmlData.m_type = QLatin1String("real");
                xmlData.m_sshPort.setNum(device->sshParameters().port());
                QFileInfo file(device->sshParameters().privateKeyFile);
                QString path = QDir::toNativeSeparators(file.dir().absolutePath());
                if(!device->sharedSshPath().isEmpty())
                    xmlData.m_sshKeyPath = QDir::fromNativeSeparators(
                                path.remove(QDir::toNativeSeparators(device->sharedSshPath() +
                                                                     QDir::separator())));
            } else {
                Q_ASSERT(dynamic_cast<const MerEmulatorDevice*>(d.data()) != 0);
                const MerEmulatorDevice* device = static_cast<const MerEmulatorDevice*>(d.data());
                //TODO: fix me
                QString mac = device->mac();
                xmlData.m_index = mac.at(mac.count()-1);
                xmlData.m_subNet = device->subnet();
                xmlData.m_name = device->displayName();
                xmlData.m_mac = device->mac();
                xmlData.m_type = QLatin1String ("vbox");
                QFileInfo file(device->sshParameters().privateKeyFile);
                QString path = QDir::toNativeSeparators(file.dir().absolutePath());
                if(!device->sharedConfigPath().isEmpty())
                    xmlData.m_sshKeyPath = QDir::fromNativeSeparators(
                                path.remove(QDir::toNativeSeparators(device->sharedConfigPath() +
                                                                     QDir::separator())));
            }
            devices << xmlData;
        }
    }

    foreach(MerSdk* sdk, m_sdks)
    {
        const QString file = sdk->sharedConfigPath() + QLatin1String(Constants::MER_DEVICES_FILENAME);
        MerEngineData xmlData;
        xmlData.m_name = sdk->virtualMachineName();
        xmlData.m_type =  QLatin1String("vbox");
        //hardcoded/magic values on customer request
        xmlData.m_subNet = QLatin1String("10.220.220");
        if (!file.isEmpty()) {
            MerDevicesXmlWriter writer(file, devices, xmlData);
        }
    }
}

#ifdef WITH_TESTS

void MerPlugin::verifyTargets(const QString &vm, QStringList expectedKits, QStringList expectedToolChains, QStringList expectedQtVersions)
{
    QList<Kit*> kits = MerSdkManager::merKits();
    QList<MerToolChain*> toolchains = MerSdkManager::merToolChains();
    QList<MerQtVersion*> qtversions = MerSdkManager::merQtVersions();

    foreach (Kit* x, kits) {
        QString target = MerSdkManager::targetNameForKit(x);
        if (expectedKits.contains(target)) {
            expectedKits.removeAll(target);
            continue;
        }
        QFAIL("Unexpected kit created");
    }
    QVERIFY2(expectedKits.isEmpty(), "Expected kits not created");

    foreach (MerToolChain *x, toolchains) {
        QString target = x->targetName();
        QVERIFY2(x->virtualMachineName() == vm, "VirtualMachine name does not match");
        if (expectedToolChains.contains(target)) {
            expectedToolChains.removeAll(target);
            continue;
        }
        QFAIL("Unexpected toolchain created");
    }
    QVERIFY2(expectedToolChains.isEmpty(), "Expected toolchains not created");

    foreach (MerQtVersion *x, qtversions) {
        QString target = x->targetName();
        QVERIFY2(x->virtualMachineName() == vm, "VirtualMachine name does not match");
        if (expectedQtVersions.contains(target)) {
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
    QTest::newRow("testcase1") << "./testcase1" << "TestVM" << QStringList() << (QStringList() << QLatin1String("SailfishOS-i486-1"));
    QTest::newRow("testcase2") << "./testcase2" << "TestVM" << (QStringList() << QLatin1String("SailfishOS-i486-1")) << (QStringList() << QLatin1String("SailfishOS-i486-2"));
    QTest::newRow("testcase3") << "./testcase3" << "TestVM" << (QStringList() << QLatin1String("SailfishOS-i486-1")) << (QStringList());
}

void MerPlugin::testMerSdkManager()
{
    QFETCH(QString, filepath);
    QFETCH(QString, virtualMachine);
    QFETCH(QStringList, targets);
    QFETCH(QStringList, expectedTargets);

    const QString &initFile = filepath + QDir::separator() + QLatin1String("init.xml");
    const QString &targetFile = filepath + QDir::separator() + QLatin1String("targets.xml");
    const QString &runFile = filepath + QDir::separator() + QLatin1String("run.xml");

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

    foreach (const QString &target, targets) {
        if (target.isEmpty())
            break;
        QVERIFY2(QDir(filepath).mkdir(target), "Could not create fake sysroot");
    }
    foreach (const QString &target, expectedTargets) {
        if (target.isEmpty())
            break;
        QVERIFY2(QDir(filepath).mkdir(target), "Could not create fake sysroot");
    }

    QVERIFY2(QFile::copy(initFile, targetFile), "Could not copy init.xml to target.xml");

    QVERIFY(MerSdkManager::sdks().isEmpty());
    MerSdk *sdk = MerSdkManager::createSdk(virtualMachine);
    QVERIFY(sdk);
    QVERIFY(!sdk->isValid());

    sdk->setSharedSshPath(QDir::toNativeSeparators(filepath));
    sdk->setSharedHomePath(QDir::toNativeSeparators(filepath));
    sdk->setSharedTargetsPath(QDir::toNativeSeparators(filepath));

    QVERIFY(sdk->isValid());
    MerSdkManager::addSdk(sdk);

    QVERIFY(!MerSdkManager::sdks().isEmpty());

    verifyTargets(virtualMachine, initKits, initToolChains, initQtVersions);

    QVERIFY2(QFile::remove(targetFile), "Could not remove target.xml");
    QVERIFY2(QFile::copy(runFile, targetFile), "Could not copy run.xml to target.xml");

    QSignalSpy spy(sdk, &MerSdk::targetsChanged);
    int i = 0;
    while (spy.count() == 0 && i++ < 50)
        QTest::qWait(200);

    verifyTargets(virtualMachine, expectedKits, expectedToolChains, expectedQtVersions);

    MerSdkManager::removeSdk(sdk);

    QList<Kit*> kits = MerSdkManager::merKits();
    QList<MerToolChain*> toolchains = MerSdkManager::merToolChains();
    QList<MerQtVersion*> qtversions = MerSdkManager::merQtVersions();

    QVERIFY2(kits.isEmpty(), "Merkit not removed");
    QVERIFY2(toolchains.isEmpty(), "Toolchain not removed");
    QVERIFY2(qtversions.isEmpty(), "QtVersion not removed");
    QVERIFY(MerSdkManager::sdks().isEmpty());
    //cleanup

    QVERIFY2(QFile::remove(targetFile), "Could not remove target.xml");
    foreach (const QString &target, expectedTargets)
        QDir(filepath).rmdir(target);
    foreach (const QString &target, targets)
        QDir(filepath).rmdir(target);
}
#endif // WITH_TESTS

} // Internal
} // Mer
