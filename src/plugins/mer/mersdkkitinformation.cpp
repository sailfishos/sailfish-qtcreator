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

#include "mersdkkitinformation.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "meroptionspage.h"
#include "merdevicefactory.h"
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <QPushButton>
#include <QComboBox>
#include <QDir>

namespace Mer {
namespace Internal {

MerSdkKitInformation::MerSdkKitInformation()
{
}

Core::Id MerSdkKitInformation::dataId() const
{
    return Core::Id(Constants::MER_KIT_INFORMATION);
}

unsigned int MerSdkKitInformation::priority() const
{
    return 24;
}

QVariant MerSdkKitInformation::defaultValue(ProjectExplorer::Kit *kit) const
{
    const MerSdk *sdk = MerSdkKitInformation::sdk(kit);
    if (sdk)
        return sdk->virtualMachineName();
    return QVariant();
}

QList<ProjectExplorer::Task> MerSdkKitInformation::validate(const ProjectExplorer::Kit *kit) const
{
    if (MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(kit))) {
        const QString &vmName = kit->value(Core::Id(Constants::VM_NAME)).toString();
        if (!MerSdkManager::instance()->sdk(vmName)) {
            const QString message = QCoreApplication::translate("MerSdk",
                                                                "No valid MerSdk virtual machine %1 found").arg(vmName);
            return QList<ProjectExplorer::Task>() << ProjectExplorer::Task(ProjectExplorer::Task::Error, message, Utils::FileName(), -1,
                                                                           Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return QList<ProjectExplorer::Task>();
}

MerSdk* MerSdkKitInformation::sdk(const ProjectExplorer::Kit *kit)
{
    if (!kit)
        return 0;
    return MerSdkManager::instance()->sdk(kit->value(Core::Id(Constants::VM_NAME)).toString());
}

ProjectExplorer::KitInformation::ItemList MerSdkKitInformation::toUserOutput(ProjectExplorer::Kit *kit) const
{
    if (MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(kit))) {
        QString vmName;
        MerSdk* sdk = MerSdkKitInformation::sdk(kit);
        if (sdk)
            vmName = sdk->virtualMachineName();
        return ProjectExplorer::KitInformation::ItemList()
                << qMakePair(tr("MerSdk"),vmName);
    }
    return ProjectExplorer::KitInformation::ItemList();
}

ProjectExplorer::KitConfigWidget *MerSdkKitInformation::createConfigWidget(ProjectExplorer::Kit *kit) const
{
    return new MerSdkKitInformationWidget(kit);
}

void MerSdkKitInformation::setSdk(ProjectExplorer::Kit *kit, const MerSdk* sdk)
{
    kit->setValue(Core::Id(Constants::VM_NAME),sdk->virtualMachineName());
}

void MerSdkKitInformation::addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const
{
    const MerSdk *sdk = MerSdkKitInformation::sdk(kit);
    if (sdk) {
	const QString sshPort = QString::number(sdk->sshPort());
	const QString sharedHome = QDir::fromNativeSeparators(sdk->sharedHomePath());
	const QString sharedTarget = QDir::fromNativeSeparators(sdk->sharedTargetsPath());

	env.appendOrSet(QLatin1String(Constants::MER_SSH_USERNAME),
	                QLatin1String(Constants::MER_SDK_DEFAULTUSER));
	env.appendOrSet(QLatin1String(Constants::MER_SSH_PORT), sshPort);
	env.appendOrSet(QLatin1String(Constants::MER_SSH_PRIVATE_KEY), sdk->privateKeyFile());
	env.appendOrSet(QLatin1String(Constants::MER_SSH_SHARED_HOME), sharedHome);
	env.appendOrSet(QLatin1String(Constants::MER_SSH_SHARED_TARGET), sharedTarget);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

MerSdkKitInformationWidget::MerSdkKitInformationWidget(ProjectExplorer::Kit *kit)
    : ProjectExplorer::KitConfigWidget(kit),
      m_combo(new QComboBox),
      m_manageButton(new QPushButton(tr("Manage...")))
{
    m_combo->addItem(tr("None"), -1);
    handleSdksUpdated();
    refresh();
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(handleCurrentIndexChanged()));
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), this, SLOT(handleSdksUpdated()));
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(handleManageClicked()));
}

QString MerSdkKitInformationWidget::displayName() const
{
    return tr("Mer SDK:");
}

QString MerSdkKitInformationWidget::toolTip() const
{
    return tr("Name of virtual machine used as Mer SDK.");
}

void MerSdkKitInformationWidget::makeReadOnly()
{
    m_combo->setEnabled(false);
}

void MerSdkKitInformationWidget::refresh()
{
    m_combo->setCurrentIndex(findMerSdk(MerSdkKitInformation::sdk(m_kit)));
}

bool MerSdkKitInformationWidget::visibleInKit()
{
    return MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(m_kit));
}

QWidget *MerSdkKitInformationWidget::mainWidget() const
{
    return m_combo;
}

QWidget *MerSdkKitInformationWidget::buttonWidget() const
{
    return m_manageButton;
}

int MerSdkKitInformationWidget::findMerSdk(const MerSdk *sdk) const
{
    if (sdk) {
        for (int i = 0; i < m_combo->count(); ++i) {
            if (sdk->virtualMachineName() == m_combo->currentText())
                return i;
        }
    }
    return -1;
}

void MerSdkKitInformationWidget::handleSdksUpdated()
{
    const QList<MerSdk*>& sdks = MerSdkManager::instance()->sdks();
    m_combo->clear();
    foreach (const MerSdk *sdk, sdks)
        m_combo->addItem(sdk->virtualMachineName());
}

void MerSdkKitInformationWidget::handleManageClicked()
{
    MerOptionsPage *page = ExtensionSystem::PluginManager::getObject<MerOptionsPage>();
    if (page)
        page->setSdk(m_combo->currentText());
    Core::ICore::showOptionsDialog(Constants::MER_OPTIONS_CATEGORY,Constants::MER_OPTIONS_ID);
}

void MerSdkKitInformationWidget::handleCurrentIndexChanged()
{
    const MerSdk* sdk = MerSdkManager::instance()->sdk(m_combo->currentText());
    if (sdk)
        MerSdkKitInformation::setSdk(m_kit, sdk);
}

}
}
