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

#include "merqtversion.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "virtualboxmanager.h"

#include <utils/environment.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerQtVersion::MerQtVersion()
    : QtSupport::BaseQtVersion()
{
}

MerQtVersion::MerQtVersion(const Utils::FileName &path,
                           bool isAutodetected,
                           const QString &autodetectionSource)
    : QtSupport::BaseQtVersion(path, isAutodetected, autodetectionSource)
{
}

MerQtVersion::~MerQtVersion()
{
}

void MerQtVersion::setVirtualMachineName(const QString &name)
{
    m_vmName = name;
}

QString MerQtVersion::virtualMachineName() const
{
    return m_vmName;
}

void MerQtVersion::setTargetName(const QString &name)
{
    m_targetName = name;
}

QString MerQtVersion::targetName() const
{
    return m_targetName;
}

QString MerQtVersion::type() const
{
    return QLatin1String(Constants::MER_QT);
}

MerQtVersion *MerQtVersion::clone() const
{
    return new MerQtVersion(*this);
}

QList<Abi> MerQtVersion::detectQtAbis() const
{
    return qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
}

QString MerQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Mer ", "Qt Version is meant for Mer");
}

bool MerQtVersion::supportsShadowBuilds() const
{
    return false;
}

QString MerQtVersion::platformName() const
{
    return QLatin1String(Constants::MER_PLATFORM);
}

QString MerQtVersion::platformDisplayName() const
{
    return QLatin1String(Constants::MER_PLATFORM_TR);
}

QVariantMap MerQtVersion::toMap() const
{
    QVariantMap data = BaseQtVersion::toMap();
    data.insert(QLatin1String(Constants::VIRTUAL_MACHINE), m_vmName);
    data.insert(QLatin1String(Constants::SB2_TARGET_NAME), m_targetName);
    return data;
}

void MerQtVersion::fromMap(const QVariantMap &data)
{
    BaseQtVersion::fromMap(data);
    m_vmName = data.value(QLatin1String(Constants::VIRTUAL_MACHINE)).toString();
    m_targetName = data.value(QLatin1String(Constants::SB2_TARGET_NAME)).toString();
}

QList<Task> MerQtVersion::validateKit(const Kit *kit)
{
    QList<Task> result = BaseQtVersion::validateKit(kit);
    if (!result.isEmpty())
        return result;

    BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    QTC_ASSERT(version == this, return result);

    ToolChain *tc = ToolChainKitInformation::toolChain(kit);

    if (!tc) {
        const QString message =
                QCoreApplication::translate("QtVersion", "No available toolchains found to build "
                                            "for Qt version '%1'.").arg(version->displayName());
        result << Task(Task::Error, message, Utils::FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    } else if (!MerSdkManager::validateKit(kit)) {
        const QString message =
                QCoreApplication::translate("QtVersion", "This Qt version '%1' can not be used "
                                            "with toolchain '%2'.").arg(version->displayName(),
                                                                        tc->displayName());
        result << Task(Task::Error, message, Utils::FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

void MerQtVersion::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    BaseQtVersion::addToEnvironment(k, env);
    // Add Mer SDK ssh connection params
    MerSdkManager::addToEnvironment(MerSdkManager::virtualMachineNameForKit(k), env);
}

QList<ProjectExplorer::Task> MerQtVersion::reportIssuesImpl(const QString &proFile,
                                                            const QString &buildDir) const
{
    QList<ProjectExplorer::Task> results;

    // Exctracting the VM name out of the qmake wrapper path
    const QString merSDKVMName = qmakeCommand().parentDir().parentDir().toFileInfo().baseName();

    const VirtualMachineInfo vmInfo = VirtualBoxManager::fetchVirtualMachineInfo(merSDKVMName);
    if (vmInfo.sharedHome.isEmpty())
        return results;
    const QString proFileClean = QDir::cleanPath(proFile);
    const QString sharedHomeClean = QDir::cleanPath(vmInfo.sharedHome);
    if (!proFileClean.startsWith(sharedHomeClean)) {
        ProjectExplorer::Task task(ProjectExplorer::Task::Error,
                                   QCoreApplication::translate("QtVersion",
                                                               "Project is outside of shared "
                                                               "home '%1'")
                                   .arg(QDir::toNativeSeparators(vmInfo.sharedHome)),
                                   Utils::FileName(), -1, Core::Id());
        results.append(task);
    }
    results.append(BaseQtVersion::reportIssuesImpl(proFile, buildDir));
    return results;
}

Core::FeatureSet MerQtVersion::availableFeatures() const
{
    Core::FeatureSet features = BaseQtVersion::availableFeatures();
    features |= Core::FeatureSet(Constants::SAILFISHOS_FEATURE);
    return features;
}

} // namespace Interal
} // namespace Mer
