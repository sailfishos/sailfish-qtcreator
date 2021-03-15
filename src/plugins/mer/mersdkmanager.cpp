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

#include "mersdkmanager.h"

#include "merconnectionmanager.h"
#include "merconstants.h"
#include "merdevicefactory.h"
#include "meremulatordevice.h"
#include "merhardwaredevice.h"
#include "merlogging.h"
#include "merplugin.h"
#include "merqtversion.h"
#include "mersdkkitaspect.h"
#include "mertoolchain.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/virtualmachine.h>

#include <app/app_version.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketool.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <ssh/sshconnection.h>
#include <ssh/sshsettings.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QMenu>
#include <QProcess>

using namespace CMakeProjectManager;
using namespace Debugger;
using namespace ExtensionSystem;
using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Core;
using namespace QSsh;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerSdkManager *MerSdkManager::m_instance = 0;

MerSdkManager::MerSdkManager()
    : m_intialized(false)
{
    Q_ASSERT(!m_instance);
    m_instance = this;
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &MerSdkManager::initialize);
}

MerSdkManager::~MerSdkManager()
{
    m_instance = 0;
}

void MerSdkManager::initialize()
{
    if (!m_intialized) {
        //read kits
        QList<Kit*> kits = merKits();
        QList<MerToolChain*> toolchains = merToolChains();
        QList<MerQtVersion*> qtversions = merQtVersions();
        //cleanup
        foreach (MerToolChain *toolchain, toolchains) {
            BuildEngine *const engine = Sdk::buildEngine(toolchain->buildEngineUri());
            if (engine && engine->buildTargetNames().contains(toolchain->buildTargetName()))
                continue;
            qCWarning(Log::mer) << "MerToolChain without target found. Removing toolchain.";
            ToolChainManager::deregisterToolChain(toolchain);
        }

        foreach (MerQtVersion *version, qtversions) {
            BuildEngine *const engine = Sdk::buildEngine(version->buildEngineUri());
            if (engine && engine->buildTargetNames().contains(version->buildTargetName()))
                continue;
            qCWarning(Log::mer) << "MerQtVersion without target found. Removing qtversion.";
            QtVersionManager::removeVersion(version);
        }

        //remove broken kits
        foreach (Kit *kit, kits) {
            if (!validateKit(kit)) {
                qCWarning(Log::mer) << "Broken Mer kit found! Removing kit.";
                KitManager::deregisterKit(kit);
            }else{
                kit->validate();
            }
        }

        for (BuildEngine *const buildEngine : Sdk::buildEngines())
            startWatching(buildEngine);
        connect(Sdk::instance(), &Sdk::buildEngineAdded,
                this, &MerSdkManager::onBuildEngineAdded);
        connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
                this, &MerSdkManager::onAboutToRemoveBuildEngine);

        // If debugger and/or cmake became available
        for (BuildEngine *const engine : Sdk::buildEngines()) {
            for (const QString &targetName : engine->buildTargetNames()) {
                for (Kit *const kit : kitsForTarget(targetName)) {
                    ensureDebuggerIsSet(kit, engine, targetName);
                    ensureCmakeToolIsSet(kit, engine, targetName);
                }
            }
        }

        checkPkgConfigAvailability();

        m_intialized = true;
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

bool MerSdkManager::isMerKit(const Kit *kit)
{
    if (!kit)
        return false;
    if (!MerSdkKitAspect::buildEngine(kit))
        return false;

    ToolChain* tc = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    const Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(kit);
    if (tc && tc->typeId() == Constants::MER_TOOLCHAIN_ID)
        return true;
    if (deviceType.isValid() && deviceType == Constants::MER_DEVICE_TYPE)
        return true;

    return false;
}

QString MerSdkManager::buildTargetNameForKit(const Kit *kit)
{
    if (!kit || !isMerKit(kit))
        return QString();
    ToolChain *toolchain = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (toolchain && toolchain->typeId() == Constants::MER_TOOLCHAIN_ID) {
        MerToolChain *mertoolchain = static_cast<MerToolChain *>(toolchain);
        return mertoolchain->buildTargetName();
    }
    return QString();
}

QList<Kit *> MerSdkManager::kitsForTarget(const QString &buildTargetName)
{
    QList<Kit*> kitsForTarget;
    if (buildTargetName.isEmpty())
        return kitsForTarget;
    const QList<Kit*> kits = KitManager::kits();
    foreach (Kit *kit, kits) {
        if (buildTargetNameForKit(kit) == buildTargetName)
            kitsForTarget << kit;
    }
    return kitsForTarget;
}

bool MerSdkManager::hasMerDevice(Kit *kit)
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(kit);
    if (dev.isNull())
        return false;
    return dev->type() == Constants::MER_DEVICE_TYPE;
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

bool MerSdkManager::validateKit(const Kit *kit)
{
    if (!kit)
        return false;
    ToolChain* toolchain = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    BaseQtVersion* version = QtKitAspect::qtVersion(kit);
    Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(kit);
    BuildEngine* engine = MerSdkKitAspect::buildEngine(kit);

    if (!version || !toolchain || !deviceType.isValid() || !engine)
        return false;
    if (version->type() != QLatin1String(Constants::MER_QT))
        return false;
    if (toolchain->typeId() != Constants::MER_TOOLCHAIN_ID)
        return false;
    if (deviceType != Constants::MER_DEVICE_TYPE)
        return false;

    MerToolChain* mertoolchain = static_cast<MerToolChain*>(toolchain);
    MerQtVersion* merqtversion = static_cast<MerQtVersion*>(version);

    return  engine->uri() ==  mertoolchain->buildEngineUri()
            && engine->uri() ==  merqtversion->buildEngineUri()
            && mertoolchain->buildTargetName() == merqtversion->buildTargetName();
}

bool MerSdkManager::generateSshKey(const QString &privKeyPath, QString &error)
{
    if (SshSettings::keygenFilePath().isEmpty()) {
        error.append(tr("The ssh-keygen tool was not found."));
        return false;
    }

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

    QProcess keygen;
    const QString keyComment("QtCreator/" + QDateTime::currentDateTime().toString(Qt::ISODate));
    const QStringList args{"-t", "rsa", "-b", "2048", "-N", QString(), "-C", keyComment, "-f", privKeyPath};
    QString errorMsg;
    keygen.start(SshSettings::keygenFilePath().toString(), args);
    keygen.closeWriteChannel();

    QApplication::setOverrideCursor(Qt::BusyCursor);
    if (!keygen.waitForStarted() || !keygen.waitForFinished())
        errorMsg = keygen.errorString();
    else if (keygen.exitStatus() != QProcess::NormalExit || keygen.exitCode() != 0)
        errorMsg = QString::fromLocal8Bit(keygen.readAllStandardError());
    QApplication::restoreOverrideCursor();

    if (!errorMsg.isEmpty()) {
        error.append(tr("The ssh-keygen tool at \"%1\" failed: %2")
                .arg(SshSettings::keygenFilePath().toUserOutput(), errorMsg));
        return false;
    }

    return true;
}

void MerSdkManager::onBuildEngineAdded(int index)
{
    BuildEngine *const buildEngine = Sdk::buildEngines().at(index);
    for (const QString &buildTargetName : buildEngine->buildTargetNames())
        addKit(buildEngine, buildTargetName);
    startWatching(buildEngine);
}

void MerSdkManager::startWatching(BuildEngine *buildEngine)
{
    connect(buildEngine, &BuildEngine::buildTargetAdded, this, [=](int index) {
        addKit(buildEngine, buildEngine->buildTargetNames().at(index));
    });
    connect(buildEngine, &BuildEngine::aboutToRemoveBuildTarget, this, [=](int index) {
        removeKit(buildEngine, buildEngine->buildTargetNames().at(index));
    });

    // FIXME Let MerSdkKitAspect take care of these?
    auto notifyKitsUpdated = [=]() { MerSdkKitAspect::notifyAboutUpdate(buildEngine); };
    connect(buildEngine, &BuildEngine::sharedHomePathChanged, this, notifyKitsUpdated);
    connect(buildEngine, &BuildEngine::sharedTargetsPathChanged, this, notifyKitsUpdated);
    connect(buildEngine, &BuildEngine::sharedConfigPathChanged, this, notifyKitsUpdated);
    connect(buildEngine, &BuildEngine::sharedSrcPathChanged, this, notifyKitsUpdated);
    connect(buildEngine, &BuildEngine::sshPortChanged, this, notifyKitsUpdated);
    connect(buildEngine->virtualMachine(), &VirtualMachine::sshParametersChanged, this, notifyKitsUpdated);
}

void MerSdkManager::onAboutToRemoveBuildEngine(int index)
{
    BuildEngine *const buildEngine = Sdk::buildEngines().at(index);
    buildEngine->virtualMachine()->disconnect(this);
    buildEngine->disconnect(this);
    for (const QString &buildTargetName : buildEngine->buildTargetNames())
        removeKit(buildEngine, buildTargetName);
}

bool MerSdkManager::addKit(const BuildEngine *buildEngine, const QString &buildTargetName)
{
    qCDebug(Log::sdks) << "Adding kit for" << buildTargetName << "inside"
        << buildEngine->uri().toString();

    const BuildTargetData buildTarget = buildEngine->buildTarget(buildTargetName);
    if (!buildTarget.sysRoot.exists()) {
        qCWarning(Log::mer) << "Sysroot does not exist" << buildTarget.sysRoot.toString();
        return false;
    }

    std::unique_ptr<MerToolChain> toolchain = createToolChain(buildEngine, buildTargetName,
            ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!toolchain)
        return false;
    std::unique_ptr<MerToolChain> toolchain_c = createToolChain(buildEngine, buildTargetName,
            ProjectExplorer::Constants::C_LANGUAGE_ID);
    if (!toolchain_c)
        return false;
    std::unique_ptr<MerQtVersion> version = createQtVersion(buildEngine, buildTargetName);
    if (!version)
        return false;

    Utils::Id id;

    bool sdkProvided = false;

    // Incomplete kits are precreated with sdktool to avoid automatic creation
    // of the Desktop kit
    if (Kit* placeholderKit = kit(buildEngine, buildTargetName)) {
        id = placeholderKit->id();
        KitManager::deregisterKit(placeholderKit);
        sdkProvided = true;
    }

    ToolChainManager::registerToolChain(toolchain.get());
    ToolChainManager::registerToolChain(toolchain_c.get());
    QtVersionManager::addVersion(version.get());

    auto initializeKit = [&](Kit *kit) {
        finalizeKitCreation(kit, buildEngine, buildTargetName, sdkProvided);

        QtKitAspect::setQtVersion(kit, version.get());
        ToolChainKitAspect::setToolChain(kit, toolchain.get());
        ToolChainKitAspect::setToolChain(kit, toolchain_c.get());
    };
    KitManager::registerKit(initializeKit, id);

    toolchain.release();
    toolchain_c.release();
    version.release();
    return true;
}

bool MerSdkManager::removeKit(const BuildEngine *buildEngine, const QString &buildTargetName)
{
    qCDebug(Log::sdks) << "Removing kit for" << buildTargetName << "inside"
        << buildEngine->uri().toString();

    //delete kit
    foreach (Kit *kit, KitManager::kits()) {
        if (!kit->isAutoDetected())
            continue;
        ToolChain* tc = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        ToolChain* tc_c = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::C_LANGUAGE_ID);
        if (!tc ) {
            continue;
        }
        if (tc->typeId() == Constants::MER_TOOLCHAIN_ID) {
            MerToolChain *mertoolchain = static_cast<MerToolChain*>(tc);
            if (mertoolchain->buildEngineUri() == buildEngine->uri() &&
                    mertoolchain->buildTargetName() == buildTargetName) {
                 BaseQtVersion *v = QtKitAspect::qtVersion(kit);
                 Utils::Id cmakeId = CMakeKitAspect::cmakeToolId(kit);
                 KitManager::deregisterKit(kit);
                 ToolChainManager::deregisterToolChain(tc);
                 if (tc_c)
                     ToolChainManager::deregisterToolChain(tc_c);
                 QTC_ASSERT(v && v->type() == QLatin1String(Constants::MER_QT), continue); //serious bug
                 QtVersionManager::removeVersion(v);
                 if (cmakeId.isValid())
                     CMakeToolManager::deregisterCMakeTool(cmakeId);
                 return true;
            }
        }
    }
    return false;
}

Kit *MerSdkManager::kit(const BuildEngine *buildEngine, const QString &buildTargetName)
{
    // FIXME multiple build engines
    Q_UNUSED(buildEngine);
    return KitManager::kit(Utils::Id::fromSetting(QVariant(buildTargetName)));
}

void MerSdkManager::finalizeKitCreation(Kit* k, const BuildEngine *buildEngine,
        const QString &buildTargetName, bool sdkProvided)
{
    const BuildTargetData buildTarget = buildEngine->buildTarget(buildTargetName);
    QTC_ASSERT(buildTarget.sysRoot.exists(), return);

    k->setAutoDetected(true);
    if (sdkProvided)
        k->setSdkProvided(true);
    k->setUnexpandedDisplayName(QString::fromLatin1("%1 (in %2)")
            .arg(buildTargetName, buildEngine->name()));

    SysRootKitAspect::setSysRoot(k, buildTarget.sysRoot);

    DeviceTypeKitAspect::setDeviceTypeId(k, Constants::MER_DEVICE_TYPE);
    k->setMutable(DeviceKitAspect::id(), true);

    ensureDebuggerIsSet(k, buildEngine, buildTargetName);

    ensureCmakeToolIsSet(k, buildEngine, buildTargetName);

    MerSdkKitAspect::setBuildTarget(k, buildEngine, buildTargetName);
}

void MerSdkManager::ensureDebuggerIsSet(Kit *k, const BuildEngine *buildEngine,
        const QString &buildTargetName)
{
    const BuildTargetData buildTarget = buildEngine->buildTarget(buildTargetName);

    QTC_ASSERT(!buildTarget.gdb.isEmpty(), return);

    const QString gdb = HostOsInfo::withExecutableSuffix(buildTarget.gdb.toString());
    QString gdbDir = QCoreApplication::applicationDirPath();
    if (HostOsInfo::isMacHost()) {
        QDir dir = QDir(gdbDir);
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
        gdbDir = dir.path();
    }
    const FilePath gdbFilePath = FilePath::fromString(gdbDir).pathAppended(gdb);

    if (gdbFilePath.exists()) {
        if (const DebuggerItem *existing = DebuggerItemManager::findByCommand(gdbFilePath)) {
            DebuggerKitAspect::setDebugger(k, existing->id());
        } else {
            DebuggerItem debugger;
            debugger.setCommand(gdbFilePath);
            debugger.setEngineType(GdbEngineType);
            debugger.setUnexpandedDisplayName(QObject::tr("GDB (%1)")
                    .arg(buildTarget.gdb.toString()));
            debugger.setAutoDetected(true);
            const int prefixLength = QString(Sfdk::Constants::DEBUGGER_FILENAME_PREFIX).length();
            debugger.setAbi(Abi::abiFromTargetTriplet(buildTarget.gdb.toString().mid(prefixLength)));
            QVariant id = DebuggerItemManager::registerDebugger(debugger);
            DebuggerKitAspect::setDebugger(k, id);
        }
    } else {
        qCWarning(Log::sdks) << "Debugger binary" << buildTarget.gdb << "not found";
        k->setValue(DebuggerKitAspect::id(), QVariant(QString()));
    }
}

void MerSdkManager::ensureCmakeToolIsSet(Kit *k, const BuildEngine *buildEngine,
        const QString &buildTargetName)
{
    const BuildTargetData buildTarget = buildEngine->buildTarget(buildTargetName);

    const FilePath cmakeWrapper = buildTarget.toolsPath.pathAppended(Sfdk::Constants::WRAPPER_CMAKE);

    if (cmakeWrapper.exists()) {
        if (const CMakeTool *existing = CMakeToolManager::findByCommand(cmakeWrapper)) {
            CMakeKitAspect::setCMakeTool(k, existing->id());
        } else {
            auto cmakeTool = std::make_unique<CMakeTool>(CMakeTool::AutoDetectionByPlugin, CMakeTool::createId());
            cmakeTool->setFilePath(cmakeWrapper);
            cmakeTool->setDisplayName(QString::fromLatin1("CMake for %1 in %2").arg(buildTargetName, buildEngine->name()));
            cmakeTool->setAutorun(true);
            cmakeTool->setAutoCreateBuildDirectory(true);
            const Utils::Id id = cmakeTool->id();
            const bool registeredOk = CMakeToolManager::registerCMakeTool(std::move(cmakeTool));
            QTC_ASSERT(registeredOk, return);
            CMakeKitAspect::setCMakeTool(k, id);
            // Qt Creator warns if these are not set or do not match Kit configuration, so we are
            // adding them here and later filtering out in merssh.
            QStringList cmakeConf = { "CMAKE_CXX_COMPILER:STRING=%{Compiler:Executable:Cxx}",
                                      "CMAKE_C_COMPILER:STRING=%{Compiler:Executable:C}",
                                      "CMAKE_PREFIX_PATH:STRING=%{Qt:QT_INSTALL_PREFIX}",
                                      "QT_QMAKE_EXECUTABLE:STRING=%{Qt:qmakeExecutable}" };
            CMakeConfigurationKitAspect::fromStringList(k, cmakeConf);
            for (KitAspect *kitAspect : ProjectExplorer::KitManager::kitAspects())
                if (kitAspect->id() == "CMake.GeneratorKitInformation")
                    kitAspect->setup(k);
        }
    } else {
        qCWarning(Log::sdks) << "CMake wrapper script" << cmakeWrapper.toString() << "not found";
        k->setValue(CMakeKitAspect::id(), QVariant(QString()));
    }
}

std::unique_ptr<MerQtVersion> MerSdkManager::createQtVersion(const BuildEngine *buildEngine,
        const QString &buildTargetName)
{
    const BuildTargetData buildTarget = buildEngine->buildTarget(buildTargetName);

    const FilePath qmake = buildTarget.toolsPath.pathAppended(Sfdk::Constants::WRAPPER_QMAKE);

    BaseQtVersion *const duplicateQtVersion = QtVersionManager::version(
            Utils::equal(&BaseQtVersion::qmakeCommand, qmake));
    QTC_CHECK(!duplicateQtVersion);

    // Hack
    BaseQtVersion *const baseQtVersion =
            QtVersionFactory::createQtVersionFromQMakePath(qmake, true, buildTarget.toolsPath.toString());
    auto const qtVersion = dynamic_cast<MerQtVersion *>(baseQtVersion);
    QTC_ASSERT(qtVersion, delete baseQtVersion; return {});

    qtVersion->setBuildEngineUri(buildEngine->uri());
    qtVersion->setBuildTargetName(buildTargetName);
    qtVersion->setUnexpandedDisplayName(
                QString::fromLatin1("Qt %1 for %2 in %3").arg(qtVersion->qtVersionString(),
                                                              buildTargetName, buildEngine->name()));
    return std::unique_ptr<MerQtVersion>(qtVersion);
}

std::unique_ptr<MerToolChain> MerSdkManager::createToolChain(const BuildEngine *buildEngine,
        const QString &buildTargetName, Utils::Id language)
{
    const BuildTargetData buildTarget = buildEngine->buildTarget(buildTargetName);

    const FilePath gcc = buildTarget.toolsPath.pathAppended(Sfdk::Constants::WRAPPER_GCC);

    QTC_CHECK(!Utils::contains(ToolChainManager::toolChains(),
                Utils::equal(&ToolChain::compilerCommand, gcc)));

    auto mertoolchain = std::make_unique<MerToolChain>();
    mertoolchain->setDisplayName(QString::fromLatin1("GCC (%1 in %2)")
            .arg(buildTargetName, buildEngine->name()));
    mertoolchain->setBuildEngineUri(buildEngine->uri());
    mertoolchain->setBuildTargetName(buildTargetName);
    mertoolchain->setLanguage(language);
    mertoolchain->resetToolChain(gcc);
    return mertoolchain;
}

void MerSdkManager::checkPkgConfigAvailability()
{
    if (HostOsInfo::isAnyUnixHost()) {
        QProcess process;
        process.start("pkg-config", {"--version"});
        if (!process.waitForFinished() || process.error() == QProcess::FailedToStart) {
            MessageManager::write(tr("pkg-config is not available. Ensure it is installed and available from PATH"),
                    MessageManager::Flash);
        }
    }
}

} // Internal
} // Mer
