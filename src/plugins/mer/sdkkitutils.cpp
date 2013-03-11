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

#include "sdkkitutils.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"

#include <projectexplorer/toolchainmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/qtcassert.h>

namespace Mer {
namespace Internal {

ProjectExplorer::Kit* createKit(const MerSdk &sdk, const QString &target)
{
    ProjectExplorer::Kit *k = new ProjectExplorer::Kit();
    k->setValue(Core::Id(Constants::TYPE), QLatin1String(Constants::MER_SDK));
    k->setDisplayName(QString::fromLocal8Bit("%1-%2").arg(sdk.virtualMachineName(), target));
    k->setIconPath(QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON));

    int qtVersionId = sdk.qtVersions().value(target);
    QtSupport::BaseQtVersion *qtV = QtSupport::QtVersionManager::instance()->version(qtVersionId);
    QTC_ASSERT(qtV, return 0);
    QtSupport::QtKitInformation::setQtVersion(k, qtV);

    QString toolChainId = sdk.toolChains().value(target);
    ProjectExplorer::ToolChain *tc =
            ProjectExplorer::ToolChainManager::instance()->findToolChain(toolChainId);
    QTC_ASSERT(tc, return 0);
    ProjectExplorer::ToolChainKitInformation::setToolChain(k, tc);

    const QString sysroot = sdk.sharedTargetPath() + QLatin1String("/") + target;
    ProjectExplorer::SysRootKitInformation::setSysRoot(k, Utils::FileName::fromString(sysroot));
    ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k, Constants::MER_DEVICE_TYPE);

    return k;
}

void SdkKitUtils::updateKits(const MerSdk &sdk, const QStringList &targets)
{
    QStringList kitsToInstall = targets;
    ProjectExplorer::KitManager *km = ProjectExplorer::KitManager::instance();
    QList<ProjectExplorer::Kit*> kits = km->kits();

    foreach (ProjectExplorer::Kit *kit, kits) {
        if (MerSdkManager::isMerKit(kit)) {
            ProjectExplorer::ToolChain* toolchain =
                    ProjectExplorer::ToolChainKitInformation::toolChain(kit);
            if (!toolchain) {
                km->deregisterKit(kit);
                continue;
            }
            if (toolchain->type()== QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
                MerToolChain* tc = static_cast<MerToolChain*>(toolchain);
                if (tc->virtualMachineName() == sdk.virtualMachineName()) {
                    if (!targets.contains(tc->targetName()))
                        km->deregisterKit(kit);
                    else
                        kitsToInstall.removeAll(tc->targetName());
                }
            }
        }
    }
    // create new kits
    foreach (const QString &t, kitsToInstall) {
        ProjectExplorer::Kit *kit = createKit(sdk, t);
        if (kit)
            km->registerKit(kit);
    }
}

bool SdkKitUtils::isSdkKit(const QString &sdkName, const QString &target)
{
    const QString id = QString::fromLocal8Bit("%1-%2").arg(sdkName, target);
    //TODO::if (ProjectExplorer::Kit *k = ProjectExplorer::KitManager::instance()->find(Core::Id(id.to)))
    //    return k->isAutoDetected();
    return false;
}

} // namespace Internal
} // namespace Mer
