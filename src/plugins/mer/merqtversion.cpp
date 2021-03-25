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

#include "merqtversion.h"

#include "merconstants.h"
#include "mersdkkitaspect.h"
#include "mersdkmanager.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>

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
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerQtVersion::MerQtVersion()
    : BaseQtVersion()
{
}

MerQtVersion::~MerQtVersion()
{
}

void MerQtVersion::setBuildEngineUri(const QUrl &uri)
{
    m_buildEngineUri = uri;
}

QUrl MerQtVersion::buildEngineUri() const
{
    return m_buildEngineUri;
}

void MerQtVersion::setBuildTargetName(const QString &name)
{
    m_buildTargetName = name;
}

QString MerQtVersion::buildTargetName() const
{
    return m_buildTargetName;
}

QString MerQtVersion::description() const
{
    return Sdk::osVariant();
}

QSet<Utils::Id> MerQtVersion::targetDeviceTypes() const
{
    return { Constants::MER_DEVICE_TYPE };
}

QVariantMap MerQtVersion::toMap() const
{
    QVariantMap data = BaseQtVersion::toMap();
    data.insert(QLatin1String(Constants::BUILD_ENGINE_URI), m_buildEngineUri);
    data.insert(QLatin1String(Constants::BUILD_TARGET_NAME), m_buildTargetName);
    return data;
}

void MerQtVersion::fromMap(const QVariantMap &data)
{
    BaseQtVersion::fromMap(data);
    m_buildEngineUri = data.value(QLatin1String(Constants::BUILD_ENGINE_URI)).toUrl();
    m_buildTargetName = data.value(QLatin1String(Constants::BUILD_TARGET_NAME)).toString();
}

Tasks MerQtVersion::validateKit(const Kit *kit)
{
    Tasks result = BaseQtVersion::validateKit(kit);
    if (!result.isEmpty())
        return result;

    BaseQtVersion *version = QtKitAspect::qtVersion(kit);
    QTC_ASSERT(version == this, return result);

    ToolChain *tc = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc) {
        const QString message =
                QCoreApplication::translate("QtVersion", "No available toolchains found to build "
                                            "for Qt version \"%1\".").arg(version->displayName());
        result << Task(Task::Error, message, FilePath(), -1,
                       Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    } else if (!MerSdkManager::validateKit(kit)) {
        const QString message =
                QCoreApplication::translate("QtVersion", "This Qt version \"%1\" does not match %2 or toolchain.").
                arg(version->displayName()).arg(Sdk::sdkVariant());
        result << Task(Task::Error, message, FilePath(), -1,
                       Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

Tasks MerQtVersion::reportIssuesImpl(const QString &proFile,
                                           const QString &buildDir) const
{
    Tasks results;

    BuildEngine* buildEngine = Sdk::buildEngine(m_buildEngineUri);

    if(!buildEngine) {
        Task task(Task::Error,
                  QCoreApplication::translate("QtVersion",
                                              "Qt version \"%1\" is missing build engine").arg(displayName()),
                  FilePath(), -1, Utils::Id());
        results.append(task);
    } 
    else {
        QString proFileClean;
        QString sharedSrcClean;
    
        QDir proDir(proFile);
    
        // If proDir is empty it means that the pro file has not yet
        // been created (new project wizard) and we must use the values
        // as they are
        if (proDir.canonicalPath().isEmpty()) {
            proFileClean = QDir::cleanPath(proFile);
            sharedSrcClean = QDir::cleanPath(buildEngine->sharedSrcPath().toString());
        }
        else {
            // proDir is not empty, let's remove any symlinks from paths
            QDir srcDir(buildEngine->sharedSrcPath().toString());
            proFileClean = QDir::cleanPath(proDir.canonicalPath());
            sharedSrcClean = QDir::cleanPath(srcDir.canonicalPath());
        }

        if (HostOsInfo::isWindowsHost()) {
            proFileClean = proFileClean.toLower();
            sharedSrcClean = sharedSrcClean.toLower();
        }

        if (!proFileClean.startsWith(sharedSrcClean)) {
            const QString message = QCoreApplication::translate("QtVersion", "Project is outside of workspace \"%1\"")
                    .arg(QDir::toNativeSeparators(buildEngine->sharedSrcPath().toString()));
            Task task(Task::Error,message,FilePath(), -1, Utils::Id());
            results.append(task);
        }

    }
    results.append(BaseQtVersion::reportIssuesImpl(proFile, buildDir));
    return results;
}

QSet<Utils::Id> MerQtVersion::availableFeatures() const
{
    QSet<Utils::Id> features = BaseQtVersion::availableFeatures();
    features |= Constants::MER_WIZARD_FEATURE_SAILFISHOS;
    if(!qtAbis().contains(Abi::fromString(QLatin1String("arm-linux-generic-elf-32bit"))))
        features |= Constants::MER_WIZARD_FEATURE_EMULATOR;
    return features;
}

bool MerQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;

    if (!qmakeCommand().toString().contains(BuildTargetData::toolsPathCommonPrefix().toString()))
        return false;

    return true;
}

QString MerQtVersion::invalidReason() const
{
    if (!BaseQtVersion::isValid())
        return BaseQtVersion::invalidReason();

    if (!qmakeCommand().toString().contains(BuildTargetData::toolsPathCommonPrefix().toString()))
        return tr("qmake not recognized as provided by %1").arg(Sdk::sdkVariant());

    return {};
}

/*!
 * \class MerQtVersionFactory
 * \internal
 */

MerQtVersionFactory::MerQtVersionFactory()
{
    setSupportedType(Constants::MER_QT);
    setPriority(50);
    setQtVersionCreator([]() { return new MerQtVersion; });
    setRestrictionChecker([](const SetupData &setup) {
        // sdk-manage adds that during target synchronization
        return setup.platforms.contains("sailfishos");
    });
}

MerQtVersionFactory::~MerQtVersionFactory()
{
}

} // namespace Interal
} // namespace Mer
