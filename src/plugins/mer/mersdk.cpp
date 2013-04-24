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

#include "mersdk.h"
#include "mertarget.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"
#include "merqtversion.h"
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <utils/hostosinfo.h>
#include <QDir>

using namespace Mer::Constants;

static const struct WrapperScript {
    enum Execution {
        ExecutionStandard,
        ExecutionTypeSb2
    };
    const char* name;
    Execution executionType;
} wrapperScripts[] = {
    { MER_WRAPPER_QMAKE, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_MAKE, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_GCC, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_GDB, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_SPECIFY, WrapperScript::ExecutionStandard },
    { MER_WRAPPER_MB, WrapperScript::ExecutionStandard },
    { MER_WRAPPER_RM, WrapperScript::ExecutionStandard },
    { MER_WRAPPER_MV, WrapperScript::ExecutionStandard }
};

namespace Mer {
namespace Internal {

MerSdk::MerSdk(QObject *parent) : QObject(parent)
    , m_autoDetected(false)
{
    connect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(handleTargetsFileChanged(QString)));
}

MerSdk::~MerSdk()
{

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
    m_name = name;
}

QString MerSdk::virtualMachineName() const
{
    return m_name;
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
        emit targetsChanged(this->targets());
    }
}

QStringList MerSdk::targets() const
{
    QStringList result;
    foreach (const MerTarget &target, m_targets)
        result << target.name();
    return result;
}

void MerSdk::setSharedTargetsPath(const QString &targetsPath)
{
    m_sharedTargetsPath = targetsPath;
}

QString MerSdk::sharedTargetsPath() const
{
    return m_sharedTargetsPath;
}

void MerSdk::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerSdk::sharedSshPath() const
{
    return m_sharedSshPath;
}


void MerSdk::setSshPort(quint16 port)
{
    m_sshPort = port;
}

quint16 MerSdk::sshPort() const
{
    return m_sshPort;
}

void MerSdk::setWwwPort(quint16 port)
{
    m_wwwPort = port;
}

quint16 MerSdk::wwwPort() const
{
    return m_wwwPort;
}

void MerSdk::setPrivateKeyFile(const QString &file)
{
    if (m_privateKeyFile != file) {
        m_privateKeyFile = file;
        emit privateKeyChanged(file);
    }
}

QString MerSdk::privateKeyFile() const
{
    return m_privateKeyFile;
}

void MerSdk::setHost(const QString &host)
{
    m_host = host;
}

QString MerSdk::host() const
{
    return m_host;
}

void MerSdk::setUserName(const QString &username)
{
    m_userName = username;
}

QString MerSdk::userName() const
{
    return m_userName;
}

// call the install all the targets and track the changes
void MerSdk::attach()
{
    if (!m_watcher.files().isEmpty())
            m_watcher.removePaths(m_watcher.files());
    QString file = sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME);
    QFileInfo fi(file);
    if (fi.exists()) {
        m_watcher.addPath(sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME));
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
    disconnect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(handleTargetsFileChanged(QString)));
    if (!m_watcher.files().isEmpty())
            m_watcher.removePaths(m_watcher.files());
    foreach (const MerTarget &target , m_targets) {
        removeTarget(target);
    }
    m_targets.clear();
}

QVariantMap MerSdk::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(VM_NAME), virtualMachineName());
    result.insert(QLatin1String(AUTO_DETECTED), isAutoDetected());
    result.insert(QLatin1String(SHARED_HOME), sharedHomePath());
    result.insert(QLatin1String(SHARED_TARGET), sharedTargetsPath());
    result.insert(QLatin1String(SHARED_SSH), sharedSshPath());
    result.insert(QLatin1String(HOST), host());
    result.insert(QLatin1String(USERNAME), userName());
    result.insert(QLatin1String(PRIVATE_KEY_FILE), privateKeyFile());
    result.insert(QLatin1String(SSH_PORT), QString::number(sshPort()));
    result.insert(QLatin1String(WWW_PORT), QString::number(wwwPort()));
    int count = 0;
    foreach (const MerTarget &target, m_targets) {
        if (!target.isValid())
            qWarning() << target.name()<<"is configured incorrectly...";
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
    setSharedSshPath(data.value(QLatin1String(SHARED_SSH)).toString());
    setHost(data.value(QLatin1String(HOST)).toString());
    setUserName(data.value(QLatin1String(USERNAME)).toString());
    setPrivateKeyFile( data.value(QLatin1String(PRIVATE_KEY_FILE)).toString());
    setSshPort(data.value(QLatin1String(SSH_PORT)).toUInt());
    setWwwPort(data.value(QLatin1String(WWW_PORT)).toUInt());

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
    return !m_name.isEmpty() && !m_sharedHomePath.isEmpty() && !m_sharedSshPath.isEmpty() && !m_sharedTargetsPath.isEmpty();
}

void MerSdk::updateTargets()
{
    QList<MerTarget> targetsToInstall;
    QList<MerTarget> targetsToKeep;
    QList<MerTarget> targetsToRemove = m_targets;
    QList<MerTarget> sdkTargets = readTargets(Utils::FileName::fromString(sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME)));

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
    updateTargets();
}

QList<MerTarget> MerSdk::readTargets(const Utils::FileName &fileName)
{
    QList<MerTarget> result;
    Utils::PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();
    int version = data.value(QLatin1String(Constants::MER_TARGET_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return result;

    int count = data.value(QLatin1String(Constants::MER_TARGET_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(Constants::MER_TARGET_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;
        MerTarget target(this);
        if (target.fromMap(data.value(key).toMap()))
            result << target;
    }
    return result;
}

bool MerSdk::addTarget(const MerTarget &target)
{
    qDebug() << "Installing" << target.name() << "for" <<virtualMachineName();
    if (!target.createScripts()) {
        qWarning() << "Failed to create wrapper scripts.";
        return false;
    }

    QScopedPointer<MerToolChain> toolchain(target.createToolChain());
    if (toolchain.isNull())
        return false;
    QScopedPointer<MerQtVersion> version(target.createQtVersion());
    if (version.isNull())
        return false;
    ProjectExplorer::Kit *kit = target.createKit();
    if (!kit)
        return false;

    ProjectExplorer::ToolChainManager::instance()->registerToolChain(toolchain.data());
    QtSupport::QtVersionManager::instance()->addVersion(version.data());
    QtSupport::QtKitInformation::setQtVersion(kit, version.data());
    ProjectExplorer::ToolChainKitInformation::setToolChain(kit, toolchain.data());
    ProjectExplorer::KitManager::instance()->registerKit(kit);
    toolchain.take();
    version.take();
    return true;
}

bool MerSdk::removeTarget(const MerTarget &target)
{
    qDebug() << "Uninstalling" << target.name() << "for" << virtualMachineName();
    //delete kit
    foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits()) {
        if (!kit->isAutoDetected())
            continue;
        ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
        if (!tc ) {
            continue;
        }
        if (tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
            MerToolChain *mertoolchain = static_cast<MerToolChain*>(tc);
            if (mertoolchain->virtualMachineName() == m_name && mertoolchain->targetName() == target.name()) {
                 QtSupport::BaseQtVersion *v = QtSupport::QtKitInformation::qtVersion(kit);
                 ProjectExplorer::KitManager::instance()->deregisterKit(kit);
                 ProjectExplorer::ToolChainManager::instance()->deregisterToolChain(tc);
                 QTC_ASSERT(v && v->type() == QLatin1String(Constants::MER_QT), continue); //serious bug
                 QtSupport::QtVersionManager::instance()->removeVersion(v);
                 target.deleteScripts();
                 return true;
            }
        }
    }
    return false;
}

} // Internal
} // Mer
