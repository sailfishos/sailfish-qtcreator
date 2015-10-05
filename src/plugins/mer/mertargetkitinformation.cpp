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

#include "mertargetkitinformation.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "meroptionspage.h"
#include "mersdk.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QComboBox>
#include <QPushButton>

namespace Mer {
namespace Internal {

MerTargetKitInformation::MerTargetKitInformation()
{
    setId(MerTargetKitInformation::id());
    setPriority(23);
}

QVariant MerTargetKitInformation::defaultValue(ProjectExplorer::Kit *kit) const
{
    return MerTargetKitInformation::targetName(kit);
}

QList<ProjectExplorer::Task> MerTargetKitInformation::validate(const ProjectExplorer::Kit *kit) const
{
    if (MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(kit))) {
        const MerSdk *sdk = MerSdkKitInformation::sdk(kit);
        const QString &target = kit->value(Core::Id(Constants::TARGET)).toString();
        if (sdk && !sdk->targetNames().contains(target)) {
            const QString message = QCoreApplication::translate("MerTarget",
                                                                "No valid MerTarget for %1 sdk found").arg(sdk->virtualMachineName());
            return QList<ProjectExplorer::Task>() << ProjectExplorer::Task(ProjectExplorer::Task::Error, message, Utils::FileName(), -1,
                                                                           Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return QList<ProjectExplorer::Task>();
}

QString MerTargetKitInformation::targetName(const ProjectExplorer::Kit *kit)
{
    if (!kit)
        return QString();
    return kit->value(Core::Id(Constants::TARGET)).toString();
}

ProjectExplorer::KitInformation::ItemList MerTargetKitInformation::toUserOutput(const ProjectExplorer::Kit *kit) const
{
    if (MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(kit))) {
        QString targetName = MerTargetKitInformation::targetName(kit);
        return ProjectExplorer::KitInformation::ItemList()
                << qMakePair(tr("MerTarget"),targetName);
    }
    return ProjectExplorer::KitInformation::ItemList();
}

ProjectExplorer::KitConfigWidget *MerTargetKitInformation::createConfigWidget(ProjectExplorer::Kit *kit) const
{
    return new MerTargetKitInformationWidget(kit, this);
}

Core::Id MerTargetKitInformation::id()
{
    return "Mer.Target.Kit.Information";
}
void MerTargetKitInformation::setTargetName(ProjectExplorer::Kit *kit, const QString& targetName)
{
    if(kit->value(Core::Id(Constants::TARGET)) != targetName)
        kit->setValue(Core::Id(Constants::TARGET),targetName);
}

void MerTargetKitInformation::addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const
{
    const QString targetName = MerTargetKitInformation::targetName(kit);
        env.appendOrSet(QLatin1String(Constants::MER_SSH_TARGET_NAME),targetName);
}

////////////////////////////////////////////////////////////////////////////////////////////

MerTargetKitInformationWidget::MerTargetKitInformationWidget(ProjectExplorer::Kit *kit,
        const MerTargetKitInformation *kitInformation)
    : ProjectExplorer::KitConfigWidget(kit, kitInformation),
      m_combo(new QComboBox),
      m_manageButton(new QPushButton(tr("Manage...")))
{
    refresh();
    connect(m_combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MerTargetKitInformationWidget::handleCurrentIndexChanged);
    connect(m_manageButton, &QPushButton::clicked,
            this, &MerTargetKitInformationWidget::handleManageClicked);
    connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
            this, &MerTargetKitInformationWidget::handleSdksUpdated);
}

QString MerTargetKitInformationWidget::displayName() const
{
    return tr("Mer Target:");
}

QString MerTargetKitInformationWidget::toolTip() const
{
    return tr("Name of target used by sb2.");
}

void MerTargetKitInformationWidget::makeReadOnly()
{
    m_combo->setEnabled(false);
}

void MerTargetKitInformationWidget::refresh()
{
    const MerSdk* sdk = MerSdkKitInformation::sdk(m_kit);
    QString targetName = MerTargetKitInformation::targetName(m_kit);
    int i = -1;
    m_combo->blockSignals(true);
    m_combo->clear();
    if (sdk && !targetName.isEmpty()) {
        foreach (const QString& targetName, sdk->targetNames()) {
            m_combo->addItem(targetName);        
        }
    }

    for (i = m_combo->count() - 1; i >= 0; --i) {
        if (targetName == m_combo->itemText(i))
            break;
    }

    if(m_combo->count() == 0) {
        m_combo->addItem(tr("None"));
        i=0;
    }

    m_combo->blockSignals(false);
    m_combo->setCurrentIndex(i);
}

bool MerTargetKitInformationWidget::visibleInKit()
{
    return MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(m_kit));
}

QWidget *MerTargetKitInformationWidget::mainWidget() const
{
    return m_combo;
}

QWidget *MerTargetKitInformationWidget::buttonWidget() const
{
    return m_manageButton;
}

void MerTargetKitInformationWidget::handleManageClicked()
{
    MerOptionsPage *page = ExtensionSystem::PluginManager::getObject<MerOptionsPage>();
    if (page) {
        const MerSdk* sdk = MerSdkKitInformation::sdk(m_kit);
        if(sdk)
            page->setSdk(m_combo->currentText());
    }
    Core::ICore::showOptionsDialog(Constants::MER_OPTIONS_ID);
}

void MerTargetKitInformationWidget::handleCurrentIndexChanged()
{
    const MerSdk* sdk = MerSdkKitInformation::sdk(m_kit);
    if (sdk && sdk->targetNames().contains(m_combo->currentText()))
        MerTargetKitInformation::setTargetName(m_kit,m_combo->currentText());
}

void MerTargetKitInformationWidget::handleSdksUpdated()
{
    refresh();
}

}
}
