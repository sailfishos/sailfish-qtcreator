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

#include "mertoolchain.h"
#include "mertoolchainfactory.h"
#include "mersdkmanager.h"
#include "mersshparser.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDir>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

using namespace ProjectExplorer;

MerToolChain::MerToolChain(bool autodetected,const QString &id)
    : GccToolChain(id, autodetected)
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

QString MerToolChain::type() const
{
    return QLatin1String(Constants::MER_TOOLCHAIN_TYPE);
}

QString MerToolChain::makeCommand(const Utils::Environment &environment) const
{
    const QString make = QLatin1String(Constants::MER_WRAPPER_MAKE);
    const QString makePath = environment.searchInPath(make);
    if (!makePath.isEmpty())
        return makePath;

    return make;
}

QList<Utils::FileName> MerToolChain::suggestedMkspecList() const
{
    QList<Utils::FileName> mkSpecList = GccToolChain::suggestedMkspecList();
    if (mkSpecList.isEmpty())
        mkSpecList << Utils::FileName::fromString(QLatin1String("linux-g++"));
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

    Core::Id type = DeviceTypeKitInformation::deviceTypeId(kit);

    if (type == Constants::MER_DEVICE_TYPE_I486 && targetAbi().architecture() != ProjectExplorer::Abi::X86Architecture) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "MerToolChain '%1' can not be used for device with i486 architecture").arg(displayName());
        result << Task(Task::Error, message, Utils::FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    if (type == Constants::MER_DEVICE_TYPE_ARM && targetAbi().architecture() != ProjectExplorer::Abi::ArmArchitecture) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "MerToolChain '%1' can not be used for device with arm architecture").arg(displayName());
        result << Task(Task::Error, message, Utils::FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);

    if (!version) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "No available Qt version found which can be used with "
                                            "toolchain '%1'.").arg(displayName());
        result << Task(Task::Error, message, Utils::FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));

    } else if (!Internal::MerSdkManager::validateKit(kit)) {
        const QString message =
                QCoreApplication::translate("ProjectExplorer::MerToolChain",
                                            "The toolchain '%1' does not match mersdk or qt version").
                                                                arg(displayName());
        result << Task(Task::Error, message, Utils::FileName(), -1,
                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

QList<HeaderPath> MerToolChain::systemHeaderPaths(const QStringList &cxxflags, const Utils::FileName &sysRoot) const
{
    Q_UNUSED(cxxflags)
    if (m_headerPathsOnHost.isEmpty()) {
        m_headerPathsOnHost.append(HeaderPath(sysRoot.toString() + QLatin1String("/usr/local/include"),
                                             HeaderPath::GlobalHeaderPath));
        m_headerPathsOnHost.append(HeaderPath(sysRoot.toString() + QLatin1String("/usr/include"),
                                             HeaderPath::GlobalHeaderPath));
    }
    return m_headerPathsOnHost;
}


void MerToolChain::addToEnvironment(Utils::Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    env.appendOrSet(QLatin1String(Constants::MER_SSH_TARGET_NAME),m_targetName);
    env.appendOrSet(QLatin1String(Constants::MER_SSH_SDK_TOOLS),compilerCommand().parentDir().toString());
}

QString MerToolChainFactory::displayName() const
{
    return tr("Mer");
}

QString MerToolChainFactory::id() const
{
    return QLatin1String(Constants::MER_TOOLCHAIN_ID);
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
                MerToolChain *tc = new MerToolChain(true, Utils::FileName(gcc.first()));
                tc->setDisplayName(QString::fromLocal8Bit("GCC (%1 %2)").arg(sdkName,
                                                                             target.baseName()));
                result.append(tc);
            }
        }
    }
    return result;
}
*/
bool MerToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::MER_TOOLCHAIN_ID)
                                      + QLatin1Char(':'));
}

ToolChain *MerToolChainFactory::restore(const QVariantMap &data)
{
    MerToolChain *tc = new MerToolChain(true);
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

} // Internal
} // Mer
