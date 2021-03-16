/****************************************************************************
**
** Copyright (C) 2012-2018 Jolla Ltd.
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

#include "mertoolchain.h"

#include "meremulatordevice.h"
#include "mersdkmanager.h"
#include "mersshparser.h"

#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace Sfdk;
using namespace QtSupport;
using namespace Utils;

namespace Mer {
namespace Internal {

using namespace ProjectExplorer;

MerToolChain::MerToolChain(Utils::Id typeId)
    : GccToolChain(typeId)
{

}

void MerToolChain::setBuildEngineUri(const QUrl &uri)
{
    m_buildEngineUri = uri;
}

QUrl MerToolChain::buildEngineUri() const
{
    return m_buildEngineUri;
}

void MerToolChain::setBuildTargetName(const QString &name)
{
    m_buildTargetName = name;
}

QString MerToolChain::buildTargetName() const
{
    return m_buildTargetName;
}

Utils::FilePath MerToolChain::makeCommand(const Environment &environment) const
{
    const QString make = QLatin1String(Sfdk::Constants::WRAPPER_MAKE);
    const FilePath makePath = environment.searchInPath(make);
    if (!makePath.isEmpty())
        return makePath;

    return FilePath::fromString(make);
}

QStringList MerToolChain::suggestedMkspecList() const
{
    QStringList mkSpecList = GccToolChain::suggestedMkspecList();
    if (mkSpecList.isEmpty())
        mkSpecList << "linux-g++";
    return mkSpecList;
}

QList<OutputTaskParser *> MerToolChain::createOutputParsers() const
{
    auto parsers = GccToolChain::createOutputParsers();
    parsers.prepend(new Internal::MerSshParser);
    return parsers;
}

QVariantMap MerToolChain::toMap() const
{
    QVariantMap data = GccToolChain::toMap();
    data.insert(QLatin1String(Constants::BUILD_ENGINE_URI), m_buildEngineUri);
    data.insert(QLatin1String(Constants::BUILD_TARGET_NAME), m_buildTargetName);
    return data;
}

bool MerToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_buildEngineUri = data.value(QLatin1String(Constants::BUILD_ENGINE_URI)).toUrl();
    m_buildTargetName = data.value(QLatin1String(Constants::BUILD_TARGET_NAME)).toString();
    return !m_buildEngineUri.isEmpty() && !m_buildTargetName.isEmpty();
}

Tasks MerToolChain::validateKit(const Kit *kit) const
{
    Tasks result = GccToolChain::validateKit(kit);
    if (!result.isEmpty())
        return result;

    IDevice::ConstPtr d = DeviceKitAspect::device(kit);
    const MerDevice* device = dynamic_cast<const MerDevice*>(d.data());
    if (device && device->architecture() != targetAbi().architecture()) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "Toolchain \"%1\" can not be used for device with %2 architecture")
                .arg(displayName()).arg(Abi::toString(device->architecture()));
        result << Task(Task::Error, message, FilePath(), -1,
                       Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    BaseQtVersion *version = QtKitAspect::qtVersion(kit);

    if (!version) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "No available Qt version found which can be used with "
                                            "toolchain \"%1\".").arg(displayName());
        result << Task(Task::Error, message, FilePath(), -1,
                       Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    } else if (!Internal::MerSdkManager::validateKit(kit)) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "The toolchain \"%1\" does not match %2 build engine or Qt version").
                                                                arg(displayName()).arg(Sdk::osVariant());
        result << Task(Task::Error, message, FilePath(), -1,
                       Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

void MerToolChain::addToEnvironment(Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_TARGET_NAME), m_buildTargetName);
    env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SDK_TOOLS),
            compilerCommand().parentDir().toString());
}

MerToolChainFactory::MerToolChainFactory()
{
    setDisplayName(Sdk::osVariant());
    setSupportedToolChainType(Constants::MER_TOOLCHAIN_ID);
    setToolchainConstructor([] { return new MerToolChain; });
    setSupportedLanguages({ProjectExplorer::Constants::CXX_LANGUAGE_ID,
        ProjectExplorer::Constants::C_LANGUAGE_ID});
}

/*
QList<ToolChain *> MerToolChainFactory::autoDetect()
{
    QList<ToolChain *> result;

    const QFileInfoList sdks =
            QDir(MerSdkManager::sdkToolsDirectory()).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
    foreach (QFileInfo sdk, sdks) {
        QDir toolsDir(sdk.absoluteFilePath());
        const QFileInfoList targets = toolsDir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
        const QString sdkName = sdk.baseName();
        foreach (const QFileInfo &target, targets) {
            const QDir targetDir(target.absoluteFilePath());
            const QFileInfoList gcc =
                    targetDir.entryInfoList(QStringList() <<
                                            QLatin1String(Sfdk::Constants::WRAPPER_GCC),
                                            QDir::Files);
            if (gcc.count()) {
                MerToolChain *tc = new MerToolChain(true, FilePath(gcc.first()));
                tc->setDisplayName(QString::fromLocal8Bit("GCC (%1 %2)").arg(sdkName,
                                                                             target.baseName()));
                result.append(tc);
            }
        }
    }
    return result;
}
*/

QList<ToolChain *> MerToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    // Only "confirm" these are autodetected so that they do not get demoted as manually added
    return Utils::filtered(alreadyKnown, [](const ToolChain *tc) {
        return tc->typeId() == Constants::MER_TOOLCHAIN_ID;
    });
}

} // Internal
} // Mer
