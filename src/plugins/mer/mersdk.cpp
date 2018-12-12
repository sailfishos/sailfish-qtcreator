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

#include "mersdk.h"

#include "merconnection.h"
#include "merconstants.h"
#include "merlogging.h"
#include "merqtversion.h"
#include "mersdkmanager.h"
#include "mertarget.h"
#include "mertargetsxmlparser.h"
#include "mertoolchain.h"

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
using namespace Utils;

namespace Mer {
namespace Internal {

MerSdk::MerSdk(QObject *parent) : QObject(parent)
    , m_autoDetected(false)
    , m_connection(new MerConnection(this))
    , m_wwwPort(-1)
{
    SshConnectionParameters params = m_connection->sshParameters();
    params.timeout = 30;
    m_connection->setSshParameters(params);

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
}

MerSdk::~MerSdk()
{

}

void MerSdk::setHeadless(bool enabled)
{
    if (m_connection->isHeadless() != enabled) {
        m_connection->setHeadless(enabled);
        emit headlessChanged(enabled);
    }
}

bool MerSdk::isHeadless() const
{
    return m_connection->isHeadless();
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
    m_connection->setVirtualMachine(name);
}

QString MerSdk::virtualMachineName() const
{
    return m_connection->virtualMachine();
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
    SshConnectionParameters params = m_connection->sshParameters();
    if (port == params.port())
        return;

    params.setPort(port);
    m_connection->setSshParameters(params);
    emit sshPortChanged(port);
}

quint16 MerSdk::sshPort() const
{
    return m_connection->sshParameters().port();
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

void MerSdk::setPrivateKeyFile(const QString &file)
{
    SshConnectionParameters params = m_connection->sshParameters();
    if (params.privateKeyFile != file) {
        params.privateKeyFile = file;
        m_connection->setSshParameters(params);
        emit privateKeyChanged(file);
    }
}

QString MerSdk::privateKeyFile() const
{
    return m_connection->sshParameters().privateKeyFile;
}

void MerSdk::setHost(const QString &host)
{
    SshConnectionParameters params = m_connection->sshParameters();
    params.setHost(host);
    m_connection->setSshParameters(params);
}

QString MerSdk::host() const
{
    return m_connection->sshParameters().host();
}

void MerSdk::setUserName(const QString &username)
{
    SshConnectionParameters params = m_connection->sshParameters();
    params.setUserName(username);
    m_connection->setSshParameters(params);
}

QString MerSdk::userName() const
{
    return m_connection->sshParameters().userName();
}

void MerSdk::setTimeout(int timeout)
{
    SshConnectionParameters params = m_connection->sshParameters();
    params.timeout = timeout;
    m_connection->setSshParameters(params);
}

int MerSdk::timeout() const
{
    return m_connection->sshParameters().timeout;
}

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

MerConnection *MerSdk::connection() const
{
    return m_connection;
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
             qWarning() << target.name() << "is configured incorrectly...";
        targets << target;
    }

    setTargets(targets);

    return isValid();
}

bool MerSdk::isValid() const
{
    return !m_connection->virtualMachine().isEmpty()
            && !m_sharedHomePath.isEmpty()
            && !m_sharedSshPath.isEmpty()
            && !m_sharedTargetsPath.isEmpty();
}

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
    Kit *kit = target.createKit();
    if (!kit)
        return false;

    ToolChainManager::registerToolChain(toolchain.data());
    ToolChainManager::registerToolChain(toolchain_c.data());
    QtVersionManager::addVersion(version.data());
    QtKitInformation::setQtVersion(kit, version.data());
    ToolChainKitInformation::setToolChain(kit, toolchain.data());
    ToolChainKitInformation::setToolChain(kit, toolchain_c.data());
    KitManager::registerKit(kit);
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
            if (mertoolchain->virtualMachineName() == m_connection->virtualMachine() &&
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

} // Internal
} // Mer
