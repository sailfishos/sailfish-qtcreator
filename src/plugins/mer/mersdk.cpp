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

#include "mersdk.h"

#include "merconstants.h"
#include "merlogging.h"
#include "merqtversion.h"
#include "mersdkmanager.h"
#include "mertarget.h"
#include "mertargetsxmlparser.h"
#include "mertoolchain.h"

#include <sfdk/vboxvirtualmachine_p.h>

#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace QtSupport;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerSdk::MerSdk(QObject *parent) : QObject(parent)
    , m_autoDetected(false)
    , m_vm(std::make_unique<VBoxVirtualMachine>(this))
    , m_wwwPort(-1)
    , m_wwwProxy(MER_SDK_PROXY_DISABLED)
{
    SshConnectionParameters params = m_vm->sshParameters();
    params.timeout = 30;
    params.hostKeyCheckingMode = SshHostKeyCheckingNone;
    params.authenticationType = SshConnectionParameters::AuthenticationTypeSpecificKey;
    m_vm->setSshParameters(params);
#ifdef MER_LIBRARY
    connect(m_vm.get(), &VirtualMachine::stateChanged,
            this, &MerSdk::onVmStateChanged);

    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &MerSdk::handleTargetsFileChanged);

    // Fired from handleTargetsFileChanged(), used to prevent (1) removing and
    // re-adding all targets due to non atomic targets file change
    // and (2) crashing Qt Creator by removing targets during build.
    m_updateTargetsTimer.setInterval(3000);
    m_updateTargetsTimer.setSingleShot(true);
    connect(&m_updateTargetsTimer, &QTimer::timeout, this, [this] {
        if (!BuildManager::isBuilding())
            updateTargets();
        else
            m_updateTargetsTimer.start();
    });
#endif // MER_LIBRARY
}

MerSdk::~MerSdk()
{

}

void MerSdk::setHeadless(bool enabled)
{
    if (m_vm->isHeadless() != enabled) {
        m_vm->setHeadless(enabled);
        emit headlessChanged(enabled);
    }
}

bool MerSdk::isHeadless() const
{
    return m_vm->isHeadless();
}

bool MerSdk::isAutoDetected() const
{
    return m_autoDetected;
}

void MerSdk::setAutodetect(bool autoDetected)
{
    m_autoDetected = autoDetected;
}

void MerSdk::setVirtualMachineName(const QString &name)
{
    m_vm->setName(name);
}

QString MerSdk::virtualMachineName() const
{
    return m_vm->name();
}

void MerSdk::setSharedHomePath(const QString &homePath)
{
    m_sharedHomePath = homePath;
}

QString MerSdk::sharedHomePath() const
{
    return m_sharedHomePath;
}

void MerSdk::setTargets(const QList<MerTarget> &targets)
{
    if (m_targets != targets) {
        m_targets = targets;
        emit targetsChanged(this->targetNames());
    }
}

QStringList MerSdk::targetNames() const
{
    QStringList result;
    foreach (const MerTarget &target, m_targets)
        result << target.name();
    return result;
}

QList<MerTarget> MerSdk::targets() const
{
    return m_targets;
}

MerTarget MerSdk::target(const QString &name) const
{
    foreach (const MerTarget &target, m_targets) {
        if (target.name() == name)
            return target;
    }
    return MerTarget();
}

void MerSdk::setSharedTargetsPath(const QString &targetsPath)
{
    m_sharedTargetsPath = targetsPath;
}

QString MerSdk::sharedTargetsPath() const
{
    return m_sharedTargetsPath;
}

void MerSdk::setSharedConfigPath(const QString &configPath)
{
    m_sharedConfigPath = configPath;
}

QString MerSdk::sharedConfigPath() const
{
    return m_sharedConfigPath;
}

void MerSdk::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerSdk::sharedSshPath() const
{
    return m_sharedSshPath;
}

void MerSdk::setSharedSrcPath(const QString &srcPath)
{
    m_sharedSrcPath = srcPath;
}

QString MerSdk::sharedSrcPath() const
{
    return m_sharedSrcPath;
}

void MerSdk::setSshPort(quint16 port)
{
    SshConnectionParameters params = m_vm->sshParameters();
    if (port == params.port())
        return;

    params.setPort(port);
    m_vm->setSshParameters(params);
    emit sshPortChanged(port);
}

quint16 MerSdk::sshPort() const
{
    return m_vm->sshParameters().port();
}

void MerSdk::setWwwPort(quint16 port)
{
    if (port == m_wwwPort)
        return;

    m_wwwPort = port;
    emit wwwPortChanged(port);
}

quint16 MerSdk::wwwPort() const
{
    return m_wwwPort;
}

void MerSdk::setWwwProxy(const QString &type, const QString &servers, const QString &excludes)
{
    if (type == m_wwwProxy && servers == m_wwwProxyServers && excludes == m_wwwProxyExcludes)
        return;

    m_wwwProxy = type;
    m_wwwProxyServers = servers;
    m_wwwProxyExcludes = excludes;

#ifdef MER_LIBRARY
    if (m_vm->state() == VirtualMachine::Connected)
        syncWwwProxy();
#endif // MER_LIBRARY

    emit wwwProxyChanged(type, servers, excludes);
}

#ifdef MER_LIBRARY
void MerSdk::syncWwwProxy()
{
    QStringList args;
    args.append(Constants::MER_SSH_PORT + QLatin1Char('=') + QString::number(sshPort()));
    args.append(Constants::MER_SSH_PRIVATE_KEY + QLatin1Char('=') + privateKeyFile());
    args.append(Constants::MER_SSH_SHARED_HOME + QLatin1Char('=') + sharedHomePath());
    args.append(Constants::MER_SSH_USERNAME + QLatin1Char('=') + userName());
    args.append(QLatin1String("wwwproxy"));
    args.append(m_wwwProxy);

    if ((m_wwwProxy == MER_SDK_PROXY_AUTOMATIC || m_wwwProxy == MER_SDK_PROXY_MANUAL)
            && !m_wwwProxyServers.isEmpty())
        args.append(m_wwwProxyServers);

    if (m_wwwProxy == MER_SDK_PROXY_MANUAL && !m_wwwProxyExcludes.isEmpty())
        args.append(m_wwwProxyExcludes);

    const QString wrapperBinaryPath = Core::ICore::libexecPath() + QLatin1String("/merssh") + QStringLiteral(QTC_HOST_EXE_SUFFIX);
    m_wwwProxySyncProcess.start(wrapperBinaryPath, args);
}
#endif // MER_LIBRARY

QString MerSdk::wwwProxy() const
{
    return m_wwwProxy;
}

QString MerSdk::wwwProxyServers() const
{
    return m_wwwProxyServers;
}

QString MerSdk::wwwProxyExcludes() const
{
    return m_wwwProxyExcludes;
}

void MerSdk::setPrivateKeyFile(const QString &file)
{
    SshConnectionParameters params = m_vm->sshParameters();
    if (params.privateKeyFile != file) {
        params.privateKeyFile = file;
        m_vm->setSshParameters(params);
        emit privateKeyChanged(file);
    }
}

QString MerSdk::privateKeyFile() const
{
    return m_vm->sshParameters().privateKeyFile;
}

void MerSdk::setHost(const QString &host)
{
    SshConnectionParameters params = m_vm->sshParameters();
    params.setHost(host);
    m_vm->setSshParameters(params);
}

QString MerSdk::host() const
{
    return m_vm->sshParameters().host();
}

void MerSdk::setUserName(const QString &username)
{
    SshConnectionParameters params = m_vm->sshParameters();
    params.setUserName(username);
    m_vm->setSshParameters(params);
}

QString MerSdk::userName() const
{
    return m_vm->sshParameters().userName();
}

void MerSdk::setTimeout(int timeout)
{
    SshConnectionParameters params = m_vm->sshParameters();
    params.timeout = timeout;
    m_vm->setSshParameters(params);
}

int MerSdk::timeout() const
{
    return m_vm->sshParameters().timeout;
}

#ifdef MER_LIBRARY
// call the install all the targets and track the changes
void MerSdk::attach()
{
    if (!m_watcher.files().isEmpty())
            m_watcher.removePaths(m_watcher.files());
    const QString file = sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME);
    QFileInfo fi(file);
    if (fi.exists()) {
        m_watcher.addPath(file);
        updateTargets();
    } else {
        qWarning() << "Targets file not found: " << file;
        qWarning() << "This will remove all installed targets for" << virtualMachineName();
        updateTargets();
    }
}

// call the uninstall all the targets.
void MerSdk::detach()
{
    disconnect(&m_watcher, &QFileSystemWatcher::fileChanged,
               this, &MerSdk::handleTargetsFileChanged);
    if (!m_watcher.files().isEmpty())
            m_watcher.removePaths(m_watcher.files());
    foreach (const MerTarget &target , m_targets) {
        removeTarget(target);
    }
    m_targets.clear();
}
#endif // MER_LIBRARY

Sfdk::VirtualMachine *MerSdk::virtualMachine() const
{
    return m_vm.get();
}

QVariantMap MerSdk::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(VM_NAME), virtualMachineName());
    result.insert(QLatin1String(AUTO_DETECTED), isAutoDetected());
    result.insert(QLatin1String(SHARED_HOME), sharedHomePath());
    result.insert(QLatin1String(SHARED_TARGET), sharedTargetsPath());
    result.insert(QLatin1String(SHARED_CONFIG), sharedConfigPath());
    result.insert(QLatin1String(SHARED_SRC), sharedSrcPath());
    result.insert(QLatin1String(SHARED_SSH), sharedSshPath());
    result.insert(QLatin1String(HOST), host());
    result.insert(QLatin1String(USERNAME), userName());
    result.insert(QLatin1String(PRIVATE_KEY_FILE), privateKeyFile());
    result.insert(QLatin1String(SSH_PORT), QString::number(sshPort()));
    result.insert(QLatin1String(SSH_TIMEOUT), QString::number(timeout()));
    result.insert(QLatin1String(WWW_PORT), QString::number(wwwPort()));
    result.insert(QLatin1String(WWW_PROXY), wwwProxy());
    result.insert(QLatin1String(WWW_PROXY_SERVERS), wwwProxyServers());
    result.insert(QLatin1String(WWW_PROXY_EXCLUDES), wwwProxyExcludes());
    result.insert(QLatin1String(HEADLESS), isHeadless());

    int count = 0;
    foreach (const MerTarget &target, m_targets) {
        if (!target.isValid())
            qWarning() << target.name() << "is configured incorrectly...";
        QVariantMap tmp = target.toMap();
        if (tmp.isEmpty())
            continue;
        result.insert(QString::fromLatin1(MER_TARGET_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    result.insert(QLatin1String(MER_TARGET_COUNT_KEY), count);
    return result;
}

bool MerSdk::fromMap(const QVariantMap &data)
{
    setVirtualMachineName(data.value(QLatin1String(VM_NAME)).toString());
    setAutodetect(data.value(QLatin1String(AUTO_DETECTED)).toBool());
    setSharedHomePath(data.value(QLatin1String(SHARED_HOME)).toString());
    setSharedTargetsPath(data.value(QLatin1String(SHARED_TARGET)).toString());
    setSharedConfigPath(data.value(QLatin1String(SHARED_CONFIG)).toString());
    setSharedSrcPath(data.value(QLatin1String(SHARED_SRC)).toString());
    setSharedSshPath(data.value(QLatin1String(SHARED_SSH)).toString());
    setHost(data.value(QLatin1String(HOST)).toString());
    setUserName(data.value(QLatin1String(USERNAME)).toString());
    setPrivateKeyFile( data.value(QLatin1String(PRIVATE_KEY_FILE)).toString());
    setSshPort(data.value(QLatin1String(SSH_PORT)).toUInt());
    setTimeout(data.value(QLatin1String(SSH_TIMEOUT), timeout()).toUInt());
    setWwwPort(data.value(QLatin1String(WWW_PORT)).toUInt());
    setWwwProxy(data.value(QLatin1String(WWW_PROXY)).toString(),
                data.value(QLatin1String(WWW_PROXY_SERVERS)).toString(),
                data.value(QLatin1String(WWW_PROXY_EXCLUDES)).toString());
    setHeadless(data.value(QLatin1String(HEADLESS)).toBool());

    int count = data.value(QLatin1String(MER_TARGET_COUNT_KEY), 0).toInt();
    QList<MerTarget> targets;
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_TARGET_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;
        const QVariantMap targetMap = data.value(key).toMap();
        MerTarget target(this);
        if (!target.fromMap(targetMap))
#ifdef MER_LIBRARY
             qWarning() << target.name() << "is configured incorrectly...";
#else
             return false;
#endif // MER_LIBRARY
        targets << target;
    }

    setTargets(targets);

    return isValid();
}

bool MerSdk::isValid() const
{
    return !m_vm->name().isEmpty()
            && !m_sharedHomePath.isEmpty()
            && !m_sharedSshPath.isEmpty()
            && !m_sharedTargetsPath.isEmpty();
}

#ifdef MER_LIBRARY
void MerSdk::updateTargets()
{
    QList<MerTarget> sdkTargets = readTargets(FileName::fromString(sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME)));
    QList<MerTarget> targetsToInstall;
    QList<MerTarget> targetsToKeep;
    QList<MerTarget> targetsToRemove = m_targets;

    //sort
    foreach (const MerTarget &sdkTarget, sdkTargets) {
        if (targetsToRemove.contains(sdkTarget)) {
            targetsToRemove.removeAll(sdkTarget);
            targetsToKeep<<sdkTarget;
        } else {
            targetsToInstall << sdkTarget;
        }
    }

    //dispatch
    foreach (const MerTarget &sdkTarget, targetsToRemove)
        removeTarget(sdkTarget);
    foreach (const MerTarget &sdkTarget, targetsToInstall)
        addTarget(sdkTarget);

    setTargets(targetsToInstall<<targetsToKeep);
}

void MerSdk::handleTargetsFileChanged(const QString &file)
{
    QTC_ASSERT(file == sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME), return);
    m_updateTargetsTimer.start();
}

void MerSdk::onVmStateChanged()
{
    if (m_vm->state() == VirtualMachine::Connected)
        syncWwwProxy();
}
#endif // MER_LIBRARY

QList<MerTarget> MerSdk::readTargets(const FileName &fileName)
{
    QList<MerTarget> result;
    MerTargetsXmlReader xmlParser(fileName.toString());
    if (!xmlParser.hasError() && xmlParser.version() > 0) {
        QList<MerTargetData> targetData = xmlParser.targetData();
        foreach (const MerTargetData &data, targetData) {
            MerTarget target(this);
            target.setName(data.name);
            target.setGccDumpMachine(data.gccDumpMachine);
            target.setGccDumpMacros(data.gccDumpMacros);
            target.setGccDumpIncludes(data.gccDumpIncludes);
            target.setQmakeQuery(data.qmakeQuery);
            target.setRpmValidationSuites(data.rpmValidationSuites);

            if (data.gccDumpMachine.contains(QLatin1String(Constants::MER_i486_IDENTIFIER))) {
                target.setDefaultGdb(QLatin1String(Constants::MER_DEBUGGER_i486_FILENAME));
            } else if (data.gccDumpMachine.contains(QLatin1String(Constants::MER_ARM_IDENTIFIER))) {
                target.setDefaultGdb(QLatin1String(Constants::MER_DEBUGGER_ARM_FILENAME));
            }

            result << target;
        }
    }
    return result;
}

#ifdef MER_LIBRARY
bool MerSdk::addTarget(const MerTarget &target)
{
    qCDebug(Log::sdks) << "Installing" << target.name() << "for" << virtualMachineName();
    if (!target.createScripts()) {
        qWarning() << "Failed to create wrapper scripts.";
        return false;
    }

    QScopedPointer<MerToolChain> toolchain(target.createToolChain(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    if (toolchain.isNull())
        return false;
    QScopedPointer<MerToolChain> toolchain_c(target.createToolChain(ProjectExplorer::Constants::C_LANGUAGE_ID));
    if (toolchain_c.isNull())
        return false;
    QScopedPointer<MerQtVersion> version(target.createQtVersion());
    if (version.isNull())
        return false;

    Kit* kitptr = target.kit();
    std::unique_ptr<Kit> kit(nullptr);
    if (!kitptr) {
        kit = std::make_unique<Kit>();
        kitptr = kit.get();
    }

    if (!target.finalizeKitCreation(kitptr))
        return false;

    ToolChainManager::registerToolChain(toolchain.data());
    ToolChainManager::registerToolChain(toolchain_c.data());
    QtVersionManager::addVersion(version.data());
    QtKitInformation::setQtVersion(kitptr, version.data());
    ToolChainKitInformation::setToolChain(kitptr, toolchain.data());
    ToolChainKitInformation::setToolChain(kitptr, toolchain_c.data());
    if (kit)
        KitManager::registerKit(std::move(kit));
    toolchain.take();
    toolchain_c.take();
    version.take();
    return true;
}

bool MerSdk::removeTarget(const MerTarget &target)
{
    qCDebug(Log::sdks) << "Uninstalling" << target.name() << "for" << virtualMachineName();
    //delete kit
    foreach (Kit *kit, KitManager::kits()) {
        if (!kit->isAutoDetected())
            continue;
        ToolChain* tc = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        ToolChain* tc_c = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::C_LANGUAGE_ID);
        if (!tc ) {
            continue;
        }
        if (tc->typeId() == Constants::MER_TOOLCHAIN_ID) {
            MerToolChain *mertoolchain = static_cast<MerToolChain*>(tc);
            if (mertoolchain->virtualMachineName() == m_vm->name() &&
                    mertoolchain->targetName() == target.name()) {
                 BaseQtVersion *v = QtKitInformation::qtVersion(kit);
                 KitManager::deregisterKit(kit);
                 ToolChainManager::deregisterToolChain(tc);
                 if (tc_c)
                     ToolChainManager::deregisterToolChain(tc_c);
                 QTC_ASSERT(v && v->type() == QLatin1String(Constants::MER_QT), continue); //serious bug
                 QtVersionManager::removeVersion(v);
                 target.deleteScripts();
                 return true;
            }
        }
    }
    return false;
}
#endif // MER_LIBRARY

} // Internal
} // Mer
