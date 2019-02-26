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

#include "mertoolchain.h"

#include "meremulatordevice.h"
#include "mersdkmanager.h"
#include "mersshparser.h"
#include "mertoolchainfactory.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace Mer {
namespace Internal {

using namespace ProjectExplorer;

MerToolChain::MerToolChain(Detection autodetected, Core::Id typeId)
    : GccToolChain(typeId, autodetected)
{

}

void MerToolChain::setVirtualMachine(const QString &name)
{
    m_vmName = name;
}

QString MerToolChain::virtualMachineName() const
{
    return m_vmName;
}

void MerToolChain::setTargetName(const QString &name)
{
    m_targetName = name;
}

QString MerToolChain::targetName() const
{
    return m_targetName;
}

QString MerToolChain::makeCommand(const Environment &environment) const
{
    const QString make = QLatin1String(Constants::MER_WRAPPER_MAKE);
    const QString makePath = environment.searchInPath(make).toString();
    if (!makePath.isEmpty())
        return makePath;

    return make;
}

QList<FileName> MerToolChain::suggestedMkspecList() const
{
    QList<FileName> mkSpecList = GccToolChain::suggestedMkspecList();
    if (mkSpecList.isEmpty())
        mkSpecList << FileName::fromString(QLatin1String("linux-g++"));
    return mkSpecList;
}

ToolChain *MerToolChain::clone() const
{
    return new MerToolChain(*this);
}

IOutputParser *MerToolChain::outputParser() const
{
    IOutputParser *parser = new Internal::MerSshParser;
    parser->appendOutputParser(GccToolChain::outputParser());
    return parser;
}

QVariantMap MerToolChain::toMap() const
{
    QVariantMap data = GccToolChain::toMap();
    data.insert(QLatin1String(Constants::VIRTUAL_MACHINE), m_vmName);
    data.insert(QLatin1String(Constants::SB2_TARGET_NAME), m_targetName);
    return data;
}

bool MerToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_vmName = data.value(QLatin1String(Constants::VIRTUAL_MACHINE)).toString();
    m_targetName = data.value(QLatin1String(Constants::SB2_TARGET_NAME)).toString();
    return !m_vmName.isEmpty() && !m_vmName.isEmpty();
}

QList<Task> MerToolChain::validateKit(const Kit *kit) const
{
    QList<Task> result = GccToolChain::validateKit(kit);
    if (!result.isEmpty())
        return result;

    IDevice::ConstPtr d = DeviceKitInformation::device(kit);
    const MerDevice* device = dynamic_cast<const MerDevice*>(d.data());
    if (device && device->architecture() != targetAbi().architecture()) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "Toolchain \"%1\" can not be used for device with %2 architecture")
                .arg(displayName()).arg(Abi::toString(device->architecture()));
        result << Task(Task::Error, message, FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    BaseQtVersion *version = QtKitInformation::qtVersion(kit);

    if (!version) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "No available Qt version found which can be used with "
                                            "toolchain \"%1\".").arg(displayName());
        result << Task(Task::Error, message, FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    } else if (!Internal::MerSdkManager::validateKit(kit)) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "The toolchain \"%1\" does not match Sailfish OS build engine or Qt version").
                                                                arg(displayName());
        result << Task(Task::Error, message, FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

ProjectExplorer::HeaderPaths MerToolChain::builtInHeaderPaths(const QStringList &cxxflags, const FileName &sysRoot) const
{
    Q_UNUSED(cxxflags)
    if (m_headerPathsOnHost.isEmpty()) {
        m_headerPathsOnHost.append(HeaderPath(sysRoot.toString() + QLatin1String("/usr/local/include"),
                                             HeaderPathType::System));
        m_headerPathsOnHost.append(HeaderPath(sysRoot.toString() + QLatin1String("/usr/include"),
                                             HeaderPathType::System));
    }
    return m_headerPathsOnHost;
}


void MerToolChain::addToEnvironment(Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    env.appendOrSet(QLatin1String(Constants::MER_SSH_TARGET_NAME), m_targetName);
    env.appendOrSet(QLatin1String(Constants::MER_SSH_SDK_TOOLS), compilerCommand().parentDir().toString());
}

MerToolChainFactory::MerToolChainFactory()
{
    setDisplayName(tr("Sailfish OS"));
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
                                            QLatin1String(Constants::MER_WRAPPER_GCC),
                                            QDir::Files);
            if (gcc.count()) {
                MerToolChain *tc = new MerToolChain(true, FileName(gcc.first()));
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

bool MerToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::MER_TOOLCHAIN_ID;
}

ToolChain *MerToolChainFactory::restore(const QVariantMap &data)
{
    MerToolChain *tc = new MerToolChain(ToolChain::AutoDetection); // TODO: unsure
    if (!tc->fromMap(data)) {
        delete tc;
        return 0;
    }
    QFileInfo fi = tc->compilerCommand().toFileInfo();
    if (!fi.exists()) {
        delete tc;
        return 0;
    }

    return tc;
}

QSet<Core::Id> MerToolChainFactory::supportedLanguages() const
{
    return {ProjectExplorer::Constants::CXX_LANGUAGE_ID, ProjectExplorer::Constants::C_LANGUAGE_ID};
}

} // Internal
} // Mer
