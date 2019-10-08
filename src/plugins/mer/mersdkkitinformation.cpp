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

#include "mersdkkitinformation.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "meroptionspage.h"
#include "mersdkmanager.h"
#include "mersettings.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QComboBox>
#include <QDir>
#include <QPushButton>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

MerSdkKitInformation::MerSdkKitInformation()
{
    setId(MerSdkKitInformation::id());
    setPriority(24);

    connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
            this, &MerSdkKitInformation::onUpdated);
    connect(MerSettings::instance(), &MerSettings::environmentFilterChanged,
            this, &MerSdkKitInformation::onUpdated);
}

QVariant MerSdkKitInformation::defaultValue(const Kit *kit) const
{
    const MerSdk *sdk = MerSdkKitInformation::sdk(kit);
    if (sdk)
        return sdk->virtualMachineName();
    return QString();
}

QList<Task> MerSdkKitInformation::validate(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        const QString &vmName = kit->value(MerSdkKitInformation::id()).toString();
        if (!MerSdkManager::sdk(vmName)) {
            const QString message = QCoreApplication::translate("MerSdk",
                                                                "No valid Sailfish OS build engine \"%1\" found").arg(vmName);
            return QList<Task>() << Task(Task::Error, message, FileName(), -1,
                                         Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return QList<Task>();
}

MerSdk* MerSdkKitInformation::sdk(const Kit *kit)
{
    if (!kit)
        return 0;
    return MerSdkManager::sdk(kit->value(MerSdkKitInformation::id()).toString());
}

KitInformation::ItemList MerSdkKitInformation::toUserOutput(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        QString vmName;
        MerSdk* sdk = MerSdkKitInformation::sdk(kit);
        if (sdk)
            vmName = sdk->virtualMachineName();
        return KitInformation::ItemList()
                << qMakePair(tr("Sailfish OS build engine"), vmName);
    }
    return KitInformation::ItemList();
}

KitConfigWidget *MerSdkKitInformation::createConfigWidget(Kit *kit) const
{
    return new MerSdkKitInformationWidget(kit, this);
}

Core::Id MerSdkKitInformation::id()
{
    return "Mer.Sdk.Kit.Information";
}

void MerSdkKitInformation::setSdk(Kit *kit, const MerSdk* sdk)
{
    if(kit->value(MerSdkKitInformation::id()) != sdk->virtualMachineName())
        kit->setValue(MerSdkKitInformation::id(), sdk->virtualMachineName());
}

void MerSdkKitInformation::addToEnvironment(const Kit *kit, Environment &env) const
{
    const MerSdk *sdk = MerSdkKitInformation::sdk(kit);
    if (sdk) {
        const QString sshPort = QString::number(sdk->sshPort());
        const QString sharedHome = QDir::fromNativeSeparators(sdk->sharedHomePath());
        const QString sharedTarget = QDir::fromNativeSeparators(sdk->sharedTargetsPath());
        const QString sharedSrc = QDir::fromNativeSeparators(sdk->sharedSrcPath());

        env.appendOrSet(QLatin1String(Constants::MER_SSH_USERNAME),
                        QLatin1String(Constants::MER_SDK_DEFAULTUSER));
        env.appendOrSet(QLatin1String(Constants::MER_SSH_PORT), sshPort);
        env.appendOrSet(QLatin1String(Constants::MER_SSH_PRIVATE_KEY), sdk->privateKeyFile());
        env.appendOrSet(QLatin1String(Constants::MER_SSH_SHARED_HOME), sharedHome);
        env.appendOrSet(QLatin1String(Constants::MER_SSH_SHARED_TARGET), sharedTarget);
        if (!sharedSrc.isEmpty())
            env.appendOrSet(QLatin1String(Constants::MER_SSH_SHARED_SRC), sharedSrc);
        if (!MerSettings::isEnvironmentFilterFromEnvironment() &&
                !MerSettings::environmentFilter().isEmpty()) {
            env.appendOrSet(QLatin1String(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER),
                    MerSettings::environmentFilter());
        }
    }
}

void MerSdkKitInformation::onUpdated()
{
    for (Kit *k : KitManager::kits([](const Kit *k) { return k->hasValue(MerSdkKitInformation::id()); }))
        notifyAboutUpdate(k);
}

////////////////////////////////////////////////////////////////////////////////////////////

MerSdkKitInformationWidget::MerSdkKitInformationWidget(Kit *kit,
        const MerSdkKitInformation *kitInformation)
    : KitConfigWidget(kit, kitInformation),
      m_combo(new QComboBox),
      m_manageButton(new QPushButton(tr("Manage...")))
{
    handleSdksUpdated();
    connect(m_combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MerSdkKitInformationWidget::handleCurrentIndexChanged);
    connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
            this, &MerSdkKitInformationWidget::handleSdksUpdated);
    connect(m_manageButton, &QPushButton::clicked,
            this, &MerSdkKitInformationWidget::handleManageClicked);
}

QString MerSdkKitInformationWidget::displayName() const
{
    return tr("Sailfish OS build engine:");
}

QString MerSdkKitInformationWidget::toolTip() const
{
    return tr("Name of the virtual machine used as a Sailfish OS build engine.");
}

void MerSdkKitInformationWidget::makeReadOnly()
{
    m_combo->setEnabled(false);
}

void MerSdkKitInformationWidget::refresh()
{
    const MerSdk* sdk = MerSdkKitInformation::sdk(m_kit);
    int i;
    if (sdk) {
        for (i = m_combo->count() - 1; i >= 0; --i) {
            if (sdk->virtualMachineName() == m_combo->itemText(i))
                break;
        }
    } else {
        i = 0;
    }
    m_combo->setCurrentIndex(i);
}

bool MerSdkKitInformationWidget::visibleInKit()
{
    return DeviceTypeKitInformation::deviceTypeId(m_kit) == Constants::MER_DEVICE_TYPE;
}

QWidget *MerSdkKitInformationWidget::mainWidget() const
{
    return m_combo;
}

QWidget *MerSdkKitInformationWidget::buttonWidget() const
{
    return m_manageButton;
}

void MerSdkKitInformationWidget::handleSdksUpdated()
{
    const QList<MerSdk*>& sdks = MerSdkManager::sdks();
    m_combo->blockSignals(true);
    m_combo->clear();
    if(sdks.isEmpty()) {
        m_combo->addItem(tr("None"));
    } else {
        foreach (const MerSdk *sdk, sdks) {
            m_combo->addItem(sdk->virtualMachineName());
        }
    }
    m_combo->blockSignals(false);
    refresh();
}

void MerSdkKitInformationWidget::handleManageClicked()
{
    MerOptionsPage *page = PluginManager::getObject<MerOptionsPage>();
    if (page)
        page->setSdk(m_combo->currentText());
    ICore::showOptionsDialog(Constants::MER_OPTIONS_ID);
}

void MerSdkKitInformationWidget::handleCurrentIndexChanged()
{
    const MerSdk* sdk = MerSdkManager::sdk(m_combo->currentText());
    if (sdk)
        MerSdkKitInformation::setSdk(m_kit, sdk);

}

}
}
