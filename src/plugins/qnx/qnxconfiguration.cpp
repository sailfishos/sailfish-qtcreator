/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "qnxconfiguration.h"
#include "qnxqtversion.h"
#include "qnxutils.h"
#include "qnxtoolchain.h"

#include "debugger/debuggeritem.h"

#include <coreplugin/icore.h>

#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QMessageBox>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace Debugger;

namespace Qnx {
namespace Internal {

const QLatin1String QNXEnvFileKey("EnvFile");
const QLatin1String QNXVersionKey("QNXVersion");
// For backward compatibility
const QLatin1String SdpEnvFileKey("NDKEnvFile");

QnxConfiguration::QnxConfiguration() = default;

QnxConfiguration::QnxConfiguration(const FilePath &sdpEnvFile)
{
    setDefaultConfiguration(sdpEnvFile);
    readInformation();
}

QnxConfiguration::QnxConfiguration(const QVariantMap &data)
{
    QString envFilePath = data.value(QNXEnvFileKey).toString();
    if (envFilePath.isEmpty())
        envFilePath = data.value(SdpEnvFileKey).toString();

    m_version = QnxVersionNumber(data.value(QNXVersionKey).toString());

    setDefaultConfiguration(FilePath::fromString(envFilePath));
    readInformation();
}

FilePath QnxConfiguration::envFile() const
{
    return m_envFile;
}

FilePath QnxConfiguration::qnxTarget() const
{
    return m_qnxTarget;
}

FilePath QnxConfiguration::qnxHost() const
{
    return m_qnxHost;
}

FilePath QnxConfiguration::qccCompilerPath() const
{
    return m_qccCompiler;
}

EnvironmentItems QnxConfiguration::qnxEnv() const
{
    return m_qnxEnv;
}

QnxVersionNumber QnxConfiguration::version() const
{
    return m_version;
}

QVariantMap QnxConfiguration::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(QNXEnvFileKey), m_envFile.toString());
    data.insert(QLatin1String(QNXVersionKey), m_version.toString());
    return data;
}

bool QnxConfiguration::isValid() const
{
    return !m_qccCompiler.isEmpty() && !m_targets.isEmpty();
}

QString QnxConfiguration::displayName() const
{
    return m_configName;
}

bool QnxConfiguration::activate()
{
    if (isActive())
        return true;

    if (!isValid()) {
        QString errorMessage
                = QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                              "The following errors occurred while activating the QNX configuration:");
        foreach (const QString &error, validationErrors())
            errorMessage += QLatin1String("\n") + error;

        QMessageBox::warning(Core::ICore::mainWindow(),
                             QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                         "Cannot Set Up QNX Configuration"),
                             errorMessage, QMessageBox::Ok);
        return false;
    }

    foreach (const Target &target, m_targets)
        createTools(target);

    return true;
}

void QnxConfiguration::deactivate()
{
    if (!isActive())
        return;

    QList<DebuggerItem> debuggersToRemove;
    const QList<ToolChain *> toolChainsToRemove
            = ToolChainManager::toolChains(Utils::equal(&ToolChain::compilerCommand, qccCompilerPath()));

    foreach (DebuggerItem debuggerItem,
             DebuggerItemManager::debuggers()) {
        if (findTargetByDebuggerPath(debuggerItem.command()))
            debuggersToRemove.append(debuggerItem);
    }

    foreach (Kit *kit, KitManager::kits()) {
        if (kit->isAutoDetected()
                && DeviceTypeKitAspect::deviceTypeId(kit) == Constants::QNX_QNX_OS_TYPE
                && toolChainsToRemove.contains(ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID)))
            KitManager::deregisterKit(kit);
    }

    foreach (ToolChain *tc, toolChainsToRemove)
        ToolChainManager::deregisterToolChain(tc);

    foreach (DebuggerItem debuggerItem, debuggersToRemove)
        DebuggerItemManager::deregisterDebugger(debuggerItem.id());
}

bool QnxConfiguration::isActive() const
{
    const bool hasToolChain = ToolChainManager::toolChain(Utils::equal(&ToolChain::compilerCommand,
                                                                       qccCompilerPath()));
    const bool hasDebugger = Utils::contains(DebuggerItemManager::debuggers(), [this](const DebuggerItem &di) {
        return findTargetByDebuggerPath(di.command());
    });

    return hasToolChain && hasDebugger;
}

bool QnxConfiguration::canCreateKits() const
{
    if (!isValid())
        return false;

    return Utils::anyOf(m_targets,
                        [this](const Target &target) -> bool { return qnxQtVersion(target); });
}

FilePath QnxConfiguration::sdpPath() const
{
    return envFile().parentDir();
}

QnxQtVersion *QnxConfiguration::qnxQtVersion(const Target &target) const
{
    foreach (BaseQtVersion *version,
             QtVersionManager::instance()->versions(Utils::equal(&BaseQtVersion::type,
                                                                         QString::fromLatin1(Constants::QNX_QNX_QT)))) {
        auto qnxQt = dynamic_cast<QnxQtVersion *>(version);
        if (qnxQt && FilePath::fromString(qnxQt->sdpPath()) == sdpPath()) {
            foreach (const Abi &qtAbi, version->qtAbis()) {
                if ((qtAbi == target.m_abi) && (qnxQt->cpuDir() == target.cpuDir()))
                    return qnxQt;
            }
        }
    }

    return nullptr;
}

QList<ToolChain *> QnxConfiguration::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result;

    foreach (const Target &target, m_targets)
        result += findToolChain(alreadyKnown, target.m_abi);

    return result;
}

void QnxConfiguration::createTools(const Target &target)
{
    QnxToolChain *tc = createToolChain(target);
    QVariant debuggerId = createDebugger(target);
    createKit(target, tc, debuggerId);
}

QVariant QnxConfiguration::createDebugger(const Target &target)
{
    Debugger::DebuggerItem debugger;
    debugger.setCommand(target.m_debuggerPath);
    debugger.reinitializeFromFile();
    debugger.setAutoDetected(true);
    debugger.setUnexpandedDisplayName(
                QCoreApplication::translate(
                    "Qnx::Internal::QnxConfiguration",
                    "Debugger for %1 (%2)")
                .arg(displayName())
                .arg(target.shortDescription()));
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

QnxToolChain *QnxConfiguration::createToolChain(const Target &target)
{
    auto toolChain = new QnxToolChain;
    toolChain->setDetection(ToolChain::AutoDetection);
    toolChain->setLanguage(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    toolChain->setTargetAbi(target.m_abi);
    toolChain->setDisplayName(
                QCoreApplication::translate(
                    "Qnx::Internal::QnxConfiguration",
                    "QCC for %1 (%2)")
                .arg(displayName())
                .arg(target.shortDescription()));
    toolChain->setSdpPath(sdpPath().toString());
    toolChain->setCpuDir(target.cpuDir());
    toolChain->resetToolChain(qccCompilerPath());
    ToolChainManager::registerToolChain(toolChain);
    return toolChain;
}

QList<ToolChain *> QnxConfiguration::findToolChain(const QList<ToolChain *> &alreadyKnown,
                                                   const Abi &abi)
{
    return Utils::filtered(alreadyKnown, [this, abi](ToolChain *tc) {
                                             return tc->typeId() == Constants::QNX_TOOLCHAIN_ID
                                                 && tc->targetAbi() == abi
                                                 && tc->compilerCommand() == m_qccCompiler;
                                         });
}

void QnxConfiguration::createKit(const Target &target, QnxToolChain *toolChain,
                                 const QVariant &debugger)
{
    QnxQtVersion *qnxQt = qnxQtVersion(target);
    // Do not create incomplete kits if no qt qnx version found
    if (!qnxQt)
        return;

    const auto init = [&](Kit *k) {
        QtKitAspect::setQtVersion(k, qnxQt);
        ToolChainKitAspect::setToolChain(k, toolChain);
        ToolChainKitAspect::clearToolChain(k, ProjectExplorer::Constants::C_LANGUAGE_ID);

        if (debugger.isValid())
            DebuggerKitAspect::setDebugger(k, debugger);

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::QNX_QNX_OS_TYPE);
        // TODO: Add sysroot?

        k->setUnexpandedDisplayName(
                    QCoreApplication::translate(
                        "Qnx::Internal::QnxConfiguration",
                        "Kit for %1 (%2)")
                    .arg(displayName())
                    .arg(target.shortDescription()));

        k->setAutoDetected(true);
        k->setAutoDetectionSource(envFile().toString());
        k->setMutable(DeviceKitAspect::id(), true);

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
        k->setSticky(SysRootKitAspect::id(), true);
        k->setSticky(DebuggerKitAspect::id(), true);
        k->setSticky(QmakeProjectManager::Constants::KIT_INFORMATION_ID, true);
    };

    // add kit with device and qt version not sticky
    KitManager::registerKit(init);
}

QStringList QnxConfiguration::validationErrors() const
{
    QStringList errorStrings;
    if (m_qccCompiler.isEmpty())
        errorStrings << QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                    "- No GCC compiler found.");

    if (m_targets.isEmpty())
        errorStrings << QCoreApplication::translate("Qnx::Internal::QnxConfiguration",
                                                    "- No targets found.");

    return errorStrings;
}

void QnxConfiguration::setVersion(const QnxVersionNumber &version)
{
    m_version = version;
}

void QnxConfiguration::readInformation()
{
    const QString qConfigPath = m_qnxConfiguration.pathAppended("qconfig").toString();
    QList <ConfigInstallInformation> installInfoList = QnxUtils::installedConfigs(qConfigPath);
    if (installInfoList.isEmpty())
        return;

    foreach (const ConfigInstallInformation &info, installInfoList) {
        if (m_qnxHost == FilePath::fromString(info.host)
                && m_qnxTarget == FilePath::fromString(info.target)) {
            m_configName = info.name;
            setVersion(QnxVersionNumber(info.version));
            break;
        }
    }
}

void QnxConfiguration::setDefaultConfiguration(const Utils::FilePath &envScript)
{
    QTC_ASSERT(!envScript.isEmpty(), return);
    m_envFile = envScript;
    m_qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(m_envFile.toString());
    foreach (const EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String("QNX_CONFIGURATION"))
            m_qnxConfiguration = FilePath::fromString(item.value);
        else if (item.name == QLatin1String("QNX_TARGET"))
            m_qnxTarget = FilePath::fromString(item.value);
        else if (item.name == QLatin1String("QNX_HOST"))
            m_qnxHost = FilePath::fromString(item.value);
    }

    FilePath qccPath = FilePath::fromString(HostOsInfo::withExecutableSuffix(
                                                m_qnxHost.toString() + QLatin1String("/usr/bin/qcc")));

    if (qccPath.exists())
        m_qccCompiler = qccPath;

    updateTargets();
    assignDebuggersToTargets();

    // Remove debuggerless targets.
    Utils::erase(m_targets, [](const Target &target) {
        if (target.m_debuggerPath.isEmpty())
            qWarning() << "No debugger found for" << target.m_path << "... discarded";
        return target.m_debuggerPath.isEmpty();
    });
}

const QnxConfiguration::Target *QnxConfiguration::findTargetByDebuggerPath(
        const FilePath &path) const
{
    auto it = std::find_if(m_targets.begin(), m_targets.end(),
                           [path](const Target &target) { return target.m_debuggerPath == path; });
    return it == m_targets.end() ? nullptr : &(*it);
}

void QnxConfiguration::updateTargets()
{
    m_targets.clear();
    QList<QnxTarget> targets = QnxUtils::findTargets(m_qnxTarget);
    for (const auto &target : targets)
        m_targets.append(Target(target.m_abi, target.m_path));
}

void QnxConfiguration::assignDebuggersToTargets()
{
    const QDir hostUsrBinDir(m_qnxHost.pathAppended("usr/bin").toString());
    QStringList debuggerNames = hostUsrBinDir.entryList(
                QStringList(HostOsInfo::withExecutableSuffix(QLatin1String("nto*-gdb"))),
                QDir::Files);
    foreach (const QString &debuggerName, debuggerNames) {
        const FilePath debuggerPath = FilePath::fromString(hostUsrBinDir.path())
                .pathAppended(debuggerName);
        DebuggerItem item;
        item.setCommand(debuggerPath);
        item.reinitializeFromFile();
        bool found = false;
        foreach (const Abi &abi, item.abis()) {
            for (Target &target : m_targets) {
                if (target.m_abi.isCompatibleWith(abi)) {
                    found = true;

                    if (target.m_debuggerPath.isEmpty()) {
                        target.m_debuggerPath = debuggerPath;
                    } else {
                        qWarning() << debuggerPath << "has the same ABI as" << target.m_debuggerPath
                                   << "... discarded";
                        break;
                    }
                }
            }
        }
        if (!found)
            qWarning() << "No target found for" << debuggerName << "... discarded";
    }
}

QString QnxConfiguration::Target::shortDescription() const
{
    return QnxUtils::cpuDirShortDescription(cpuDir());
}

QString QnxConfiguration::Target::cpuDir() const
{
    return m_path.fileName();
}

} // namespace Internal
} // namespace Qnx
