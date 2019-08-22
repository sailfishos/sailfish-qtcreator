/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace Mer {
namespace Internal {

MerQtVersion::MerQtVersion()
    : BaseQtVersion()
{
}

MerQtVersion::MerQtVersion(const FileName &path,
                           bool isAutodetected,
                           const QString &autodetectionSource)
    : BaseQtVersion(path, isAutodetected, autodetectionSource)
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
    return qtAbisFromLibrary(qtCorePaths());
}

QString MerQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Sailfish OS ", "Qt Version is meant for Sailfish OS");
}

QSet<Core::Id> MerQtVersion::targetDeviceTypes() const
{
    return { Constants::MER_DEVICE_TYPE };
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

    BaseQtVersion *version = QtKitInformation::qtVersion(kit);
    QTC_ASSERT(version == this, return result);

    ToolChain *tc = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc) {
        const QString message =
                QCoreApplication::translate("QtVersion", "No available toolchains found to build "
                                            "for Qt version \"%1\".").arg(version->displayName());
        result << Task(Task::Error, message, FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    } else if (!MerSdkManager::validateKit(kit)) {
        const QString message =
                QCoreApplication::translate("QtVersion", "This Qt version \"%1\" does not match Sailfish SDK or toolchain.").
                arg(version->displayName());
        result << Task(Task::Error, message, FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

QList<Task> MerQtVersion::reportIssuesImpl(const QString &proFile,
                                           const QString &buildDir) const
{
    QList<Task> results;

    MerSdk* sdk = MerSdkManager::sdk(m_vmName);

    if(!sdk) {
        Task task(Task::Error,
                  QCoreApplication::translate("QtVersion",
                                              "Qt version \"%1\" is missing Sailfish SDK").arg(displayName()),
                  FileName(), -1, Core::Id());
        results.append(task);
    } 
    else {
        QString proFileClean;
        QString sharedHomeClean;
        QString sharedSrcClean;
    
        QDir proDir(proFile);
    
        // If proDir is empty it means that the pro file has not yet
        // been created (new project wizard) and we must use the values
        // as they are
        if (proDir.canonicalPath().isEmpty()) {
            proFileClean = QDir::cleanPath(proFile);
            sharedHomeClean = QDir::cleanPath(sdk->sharedHomePath());
            sharedSrcClean = QDir::cleanPath(sdk->sharedSrcPath());
        }
        else {
            // proDir is not empty, let's remove any symlinks from paths
            QDir homeDir(sdk->sharedHomePath());
            QDir srcDir(sdk->sharedSrcPath());
            proFileClean = QDir::cleanPath(proDir.canonicalPath());
            sharedHomeClean = QDir::cleanPath(homeDir.canonicalPath());
            sharedSrcClean = QDir::cleanPath(srcDir.canonicalPath());
        }

        if (HostOsInfo::isWindowsHost()) {
            proFileClean = proFileClean.toLower();
            sharedHomeClean = sharedHomeClean.toLower();
            sharedSrcClean = sharedSrcClean.toLower();
        }

        if (!proFileClean.startsWith(sharedHomeClean) && !proFileClean.startsWith(sharedSrcClean)) {
            QString message =  QCoreApplication::translate("QtVersion", "Project is outside of Sailfish SDK workspace");
            if(!sdk->sharedHomePath().isEmpty() && !sdk->sharedSrcPath().isEmpty())
              message = QCoreApplication::translate("QtVersion", "Project is outside of Sailfish SDK shared home \"%1\" and shared src \"%2\"")
                      .arg(QDir::toNativeSeparators(QDir::toNativeSeparators(sdk->sharedHomePath())))
                      .arg(QDir::toNativeSeparators(QDir::toNativeSeparators(sdk->sharedSrcPath())));
            else if(!sdk->sharedHomePath().isEmpty())
              message = QCoreApplication::translate("QtVersion", "Project is outside of shared home \"%1\"")
                      .arg(QDir::toNativeSeparators(QDir::toNativeSeparators(sdk->sharedHomePath())));
            Task task(Task::Error,message,FileName(), -1, Core::Id());
            results.append(task);
        }

    }
    results.append(BaseQtVersion::reportIssuesImpl(proFile, buildDir));
    return results;
}

void MerQtVersion::addToEnvironment(const Kit *k, Environment &env) const
{
    Q_UNUSED(k);
    env.appendOrSet(QLatin1String(Constants::MER_SSH_SDK_TOOLS),qmakeCommand().parentDir().toString());
}


QSet<Core::Id> MerQtVersion::availableFeatures() const
{
    QSet<Core::Id> features = BaseQtVersion::availableFeatures();
    features |= Constants::MER_WIZARD_FEATURE_SAILFISHOS;
    if(!qtAbis().contains(Abi::fromString(QLatin1String("arm-linux-generic-elf-32bit"))))
        features |= Constants::MER_WIZARD_FEATURE_EMULATOR;
    return features;
}

Environment MerQtVersion::qmakeRunEnvironment() const
{
    Environment env = BaseQtVersion::qmakeRunEnvironment();
    env.appendOrSet(QLatin1String(Constants::MER_SSH_TARGET_NAME),m_targetName);
    env.appendOrSet(QLatin1String(Constants::MER_SSH_SDK_TOOLS),qmakeCommand().parentDir().toString());
    return env;
}

} // namespace Interal
} // namespace Mer
