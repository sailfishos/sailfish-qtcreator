/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "iossimulator.h"
#include "simulatorcontrol.h"
#include "iosprobe.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtversionfactory.h>

#include <QDomDocument>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QSettings>
#include <QStringList>
#include <QTimer>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace {
Q_LOGGING_CATEGORY(kitSetupLog, "qtc.ios.kitSetup")
Q_LOGGING_CATEGORY(iosCommonLog, "qtc.ios.common")
}

using ToolChainPair = std::pair<ClangToolChain *, ClangToolChain *>;
namespace Ios {
namespace Internal {

const QLatin1String SettingsGroup("IosConfigurations");
const QLatin1String ignoreAllDevicesKey("IgnoreAllDevices");

static Core::Id deviceId(const Platform &platform)
{
    if (platform.name.startsWith(QLatin1String("iphoneos-")))
        return Constants::IOS_DEVICE_TYPE;
    else if (platform.name.startsWith(QLatin1String("iphonesimulator-")))
        return Constants::IOS_SIMULATOR_TYPE;
    return Core::Id();
}

static bool handledPlatform(const Platform &platform)
{
    // do not want platforms that
    // - are not iphone (e.g. watchos)
    // - are not base
    // - are C++11
    // - are not clang
    return deviceId(platform).isValid()
            && (platform.platformKind & Platform::BasePlatform) != 0
            && (platform.platformKind & Platform::Cxx11Support) == 0
            && platform.type == Platform::CLang;
}

static QList<Platform> handledPlatforms()
{
    QList<Platform> platforms = IosProbe::detectPlatforms().values();
    return Utils::filtered(platforms, handledPlatform);
}

static QList<ClangToolChain *> clangToolChains(const QList<ToolChain *> &toolChains)
{
    QList<ClangToolChain *> clangToolChains;
    foreach (ToolChain *toolChain, toolChains)
        if (toolChain->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID)
            clangToolChains.append(static_cast<ClangToolChain *>(toolChain));
    return clangToolChains;
}

static QList<ClangToolChain *> autoDetectedIosToolChains()
{
    const QList<ClangToolChain *> toolChains = clangToolChains(ToolChainManager::toolChains());
    return Utils::filtered(toolChains, [](ClangToolChain *toolChain) {
        return toolChain->isAutoDetected()
               && toolChain->displayName().startsWith(QLatin1String("iphone")); // TODO tool chains should be marked directly
    });
}

static ToolChainPair findToolChainForPlatform(const Platform &platform, const QList<ClangToolChain *> &toolChains)
{
    ToolChainPair platformToolChains;
    auto toolchainMatch = [](ClangToolChain *toolChain, const Utils::FileName &compilerPath, const QStringList &flags) {
        return compilerPath == toolChain->compilerCommand()
                && flags == toolChain->platformCodeGenFlags()
                && flags == toolChain->platformLinkerFlags();
    };
    platformToolChains.first = Utils::findOrDefault(toolChains, std::bind(toolchainMatch, std::placeholders::_1,
                                                                  platform.cCompilerPath,
                                                                  platform.backendFlags));
    platformToolChains.second = Utils::findOrDefault(toolChains, std::bind(toolchainMatch, std::placeholders::_1,
                                                                   platform.cxxCompilerPath,
                                                                   platform.backendFlags));
    return platformToolChains;
}

static QHash<Platform, ToolChainPair> findToolChains(const QList<Platform> &platforms)
{
    QHash<Platform, ToolChainPair> platformToolChainHash;
    const QList<ClangToolChain *> toolChains = autoDetectedIosToolChains();
    foreach (const Platform &platform, platforms) {
        ToolChainPair platformToolchains = findToolChainForPlatform(platform, toolChains);
        if (platformToolchains.first || platformToolchains.second)
            platformToolChainHash.insert(platform, platformToolchains);
    }
    return platformToolChainHash;
}

static QHash<Abi::Architecture, QSet<BaseQtVersion *>> iosQtVersions()
{
    QHash<Abi::Architecture, QSet<BaseQtVersion *>> versions;
    foreach (BaseQtVersion *qtVersion, QtVersionManager::unsortedVersions()) {
        if (!qtVersion->isValid() || qtVersion->type() != QLatin1String(Constants::IOSQT))
            continue;
        foreach (const Abi &abi, qtVersion->qtAbis())
            versions[abi.architecture()].insert(qtVersion);
    }
    return versions;
}

static void printQtVersions(const QHash<Abi::Architecture, QSet<BaseQtVersion *> > &map)
{
    foreach (const Abi::Architecture &arch, map.keys()) {
        qCDebug(kitSetupLog) << "-" << Abi::toString(arch);
        foreach (const BaseQtVersion *version, map.value(arch))
            qCDebug(kitSetupLog) << "  -" << version->displayName() << version;
    }
}

static QSet<Kit *> existingAutoDetectedIosKits()
{
    return Utils::filtered(KitManager::kits(), [](Kit *kit) -> bool {
        Core::Id deviceKind = DeviceTypeKitInformation::deviceTypeId(kit);
        return kit->isAutoDetected() && (deviceKind == Constants::IOS_DEVICE_TYPE
                                         || deviceKind == Constants::IOS_SIMULATOR_TYPE);
    }).toSet();
}

static void printKits(const QSet<Kit *> &kits)
{
    foreach (const Kit *kit, kits)
        qCDebug(kitSetupLog) << "  -" << kit->displayName();
}

static void setupKit(Kit *kit, Core::Id pDeviceType, const ToolChainPair& toolChains,
                     const QVariant &debuggerId, const Utils::FileName &sdkPath, BaseQtVersion *qtVersion)
{
    DeviceTypeKitInformation::setDeviceTypeId(kit, pDeviceType);
    ToolChainKitInformation::setToolChain(kit, ToolChain::Language::C, toolChains.first);
    ToolChainKitInformation::setToolChain(kit, ToolChain::Language::Cxx, toolChains.second);
    QtKitInformation::setQtVersion(kit, qtVersion);
    // only replace debugger with the default one if we find an unusable one here
    // (since the user could have changed it)
    if ((!DebuggerKitInformation::debugger(kit)
            || !DebuggerKitInformation::debugger(kit)->isValid()
            || DebuggerKitInformation::debugger(kit)->engineType() != LldbEngineType)
            && debuggerId.isValid())
        DebuggerKitInformation::setDebugger(kit, debuggerId);

    kit->setMutable(DeviceKitInformation::id(), true);
    kit->setSticky(QtKitInformation::id(), true);
    kit->setSticky(ToolChainKitInformation::id(), true);
    kit->setSticky(DeviceTypeKitInformation::id(), true);
    kit->setSticky(SysRootKitInformation::id(), true);
    kit->setSticky(DebuggerKitInformation::id(), false);

    SysRootKitInformation::setSysRoot(kit, sdkPath);
}

static QVersionNumber findXcodeVersion()
{
    Utils::SynchronousProcess pkgUtilProcess;
    Utils::SynchronousProcessResponse resp =
            pkgUtilProcess.runBlocking("pkgutil", QStringList("--pkg-info-plist=com.apple.pkg.Xcode"));
    if (resp.result == Utils::SynchronousProcessResponse::Finished) {
        QDomDocument xcodeVersionDoc;
        if (xcodeVersionDoc.setContent(resp.allRawOutput())) {
            QDomNodeList nodes = xcodeVersionDoc.elementsByTagName(QStringLiteral("key"));
            for (int i = 0; i < nodes.count(); ++i) {
                QDomElement elem = nodes.at(i).toElement();
                if (elem.text().compare(QStringLiteral("pkg-version")) == 0) {
                    QString versionStr = elem.nextSiblingElement().text();
                    return  QVersionNumber::fromString(versionStr);
                }
            }
        } else {
            qCDebug(iosCommonLog) << "Error finding Xcode version. Cannot parse xml output from pkgutil.";
        }
    } else {
        qCDebug(iosCommonLog) << "Error finding Xcode version. pkgutil command failed.";
    }

    qCDebug(iosCommonLog) << "Error finding Xcode version. Unknow error.";
    return QVersionNumber();
}

void IosConfigurations::updateAutomaticKitList()
{
    const QList<Platform> platforms = handledPlatforms();
    qCDebug(kitSetupLog) << "Used platforms:" << platforms;
    if (!platforms.isEmpty())
        setDeveloperPath(platforms.first().developerPath);
    qCDebug(kitSetupLog) << "Developer path:" << developerPath();

    // platform name -> tool chain
    const QHash<Platform, ToolChainPair> platformToolChainHash = findToolChains(platforms);

    const QHash<Abi::Architecture, QSet<BaseQtVersion *> > qtVersionsForArch = iosQtVersions();
    qCDebug(kitSetupLog) << "iOS Qt versions:";
    printQtVersions(qtVersionsForArch);

    const DebuggerItem *possibleDebugger = DebuggerItemManager::findByEngineType(LldbEngineType);
    const QVariant debuggerId = (possibleDebugger ? possibleDebugger->id() : QVariant());

    QSet<Kit *> existingKits = existingAutoDetectedIosKits();
    qCDebug(kitSetupLog) << "Existing auto-detected iOS kits:";
    printKits(existingKits);
    QSet<Kit *> resultingKits;
    // match existing kits and create missing kits
    foreach (const Platform &platform, platforms) {
        qCDebug(kitSetupLog) << "Guaranteeing kits for " << platform.name ;
        const ToolChainPair &platformToolchains = platformToolChainHash.value(platform);
        if (!platformToolchains.first && !platformToolchains.second) {
            qCDebug(kitSetupLog) << "  - No tool chain found";
            continue;
        }
        Core::Id pDeviceType = deviceId(platform);
        QTC_ASSERT(pDeviceType.isValid(), continue);
        Abi::Architecture arch = platformToolchains.second ? platformToolchains.second->targetAbi().architecture() :
                                                            platformToolchains.first->targetAbi().architecture();

        QSet<BaseQtVersion *> qtVersions = qtVersionsForArch.value(arch);
        foreach (BaseQtVersion *qtVersion, qtVersions) {
            qCDebug(kitSetupLog) << "  - Qt version:" << qtVersion->displayName();
            Kit *kit = Utils::findOrDefault(existingKits, [&pDeviceType, &platformToolchains, &qtVersion](const Kit *kit) {
                // we do not compare the sdk (thus automatically upgrading it in place if a
                // new Xcode is used). Change?
                return DeviceTypeKitInformation::deviceTypeId(kit) == pDeviceType
                        && ToolChainKitInformation::toolChain(kit, ToolChain::Language::Cxx) == platformToolchains.second
                        && ToolChainKitInformation::toolChain(kit, ToolChain::Language::C) == platformToolchains.first
                        && QtKitInformation::qtVersion(kit) == qtVersion;
            });
            QTC_ASSERT(!resultingKits.contains(kit), continue);
            if (kit) {
                qCDebug(kitSetupLog) << "    - Kit matches:" << kit->displayName();
                kit->blockNotification();
                setupKit(kit, pDeviceType, platformToolchains, debuggerId, platform.sdkPath, qtVersion);
                kit->unblockNotification();
            } else {
                qCDebug(kitSetupLog) << "    - Setting up new kit";
                kit = new Kit;
                kit->blockNotification();
                kit->setAutoDetected(true);
                const QString baseDisplayName = tr("%1 %2").arg(platform.name, qtVersion->unexpandedDisplayName());
                kit->setUnexpandedDisplayName(baseDisplayName);
                setupKit(kit, pDeviceType, platformToolchains, debuggerId, platform.sdkPath, qtVersion);
                kit->unblockNotification();
                KitManager::registerKit(kit);
            }
            resultingKits.insert(kit);
        }
    }
    // remove unused kits
    existingKits.subtract(resultingKits);
    qCDebug(kitSetupLog) << "Removing unused kits:";
    printKits(existingKits);
    foreach (Kit *kit, existingKits)
        KitManager::deregisterKit(kit);
}

static IosConfigurations *m_instance = 0;

QObject *IosConfigurations::instance()
{
    return m_instance;
}

void IosConfigurations::initialize()
{
    QTC_CHECK(m_instance == 0);
    m_instance = new IosConfigurations(0);
}

bool IosConfigurations::ignoreAllDevices()
{
    return m_instance->m_ignoreAllDevices;
}

void IosConfigurations::setIgnoreAllDevices(bool ignoreDevices)
{
    if (ignoreDevices != m_instance->m_ignoreAllDevices) {
        m_instance->m_ignoreAllDevices = ignoreDevices;
        m_instance->save();
    }
}

FileName IosConfigurations::developerPath()
{
    return m_instance->m_developerPath;
}

QVersionNumber IosConfigurations::xcodeVersion()
{
    return m_instance->m_xcodeVersion;
}

void IosConfigurations::save()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(ignoreAllDevicesKey, m_ignoreAllDevices);
    settings->endGroup();
}

IosConfigurations::IosConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
}

void IosConfigurations::load()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    m_ignoreAllDevices = settings->value(ignoreAllDevicesKey, false).toBool();
    settings->endGroup();
}

void IosConfigurations::updateSimulators()
{
    // currently we have just one simulator
    DeviceManager *devManager = DeviceManager::instance();
    Core::Id devId = Constants::IOS_SIMULATOR_DEVICE_ID;
    IDevice::ConstPtr dev = devManager->find(devId);
    if (dev.isNull()) {
        dev = IDevice::ConstPtr(new IosSimulator(devId));
        devManager->addDevice(dev);
    }
    SimulatorControl::updateAvailableSimulators();
}

void IosConfigurations::setDeveloperPath(const FileName &devPath)
{
    static bool hasDevPath = false;
    if (devPath != m_instance->m_developerPath) {
        m_instance->m_developerPath = devPath;
        m_instance->save();
        if (!hasDevPath && !devPath.isEmpty()) {
            hasDevPath = true;
            QTimer::singleShot(1000, IosDeviceManager::instance(),
                               &IosDeviceManager::monitorAvailableDevices);
            m_instance->updateSimulators();

            // Find xcode version.
            m_instance->m_xcodeVersion = findXcodeVersion();
        }
    }
}

static ClangToolChain *createToolChain(const Platform &platform, ToolChain::Language l)
{
    if (l == ToolChain::Language::None)
        return nullptr;

    ClangToolChain *toolChain = new ClangToolChain(ToolChain::AutoDetection);
    toolChain->setLanguage(l);
    toolChain->setDisplayName(l == ToolChain::Language::Cxx ? platform.name + "++" : platform.name);
    toolChain->setPlatformCodeGenFlags(platform.backendFlags);
    toolChain->setPlatformLinkerFlags(platform.backendFlags);
    toolChain->resetToolChain(l == ToolChain::Language::Cxx ?
                                  platform.cxxCompilerPath : platform.cCompilerPath);
    return toolChain;
}

QSet<ToolChain::Language> IosToolChainFactory::supportedLanguages() const
{
    return { ProjectExplorer::ToolChain::Language::Cxx };
}

QList<ToolChain *> IosToolChainFactory::autoDetect(const QList<ToolChain *> &existingToolChains)
{
    QList<ClangToolChain *> existingClangToolChains = clangToolChains(existingToolChains);
    const QList<Platform> platforms = handledPlatforms();
    QList<ClangToolChain *> toolChains;
    toolChains.reserve(platforms.size());
    foreach (const Platform &platform, platforms) {
        ToolChainPair platformToolchains = findToolChainForPlatform(platform, existingClangToolChains);
        auto createOrAdd = [&](ClangToolChain* toolChain, ToolChain::Language l) {
            if (!toolChain) {
                toolChain = createToolChain(platform, l);
                existingClangToolChains.append(toolChain);
            }
            toolChains.append(toolChain);
        };

        createOrAdd(platformToolchains.first, ToolChain::Language::C);
        createOrAdd(platformToolchains.second, ToolChain::Language::Cxx);
    }
    return Utils::transform(toolChains, [](ClangToolChain *tc) -> ToolChain * { return tc; });
}

} // namespace Internal
} // namespace Ios
