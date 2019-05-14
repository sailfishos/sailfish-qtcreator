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
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QPushButton>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

MerTargetKitInformation::MerTargetKitInformation()
{
    setId(MerTargetKitInformation::id());
    setPriority(23);
}

QVariant MerTargetKitInformation::defaultValue(const Kit *kit) const
{
    return MerTargetKitInformation::targetName(kit);
}

QList<Task> MerTargetKitInformation::validate(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        const MerSdk *sdk = MerSdkKitInformation::sdk(kit);
        const QString &target = kit->value(MerTargetKitInformation::id()).toString();
        if (sdk && !sdk->targetNames().contains(target)) {
            const QString message = QCoreApplication::translate("MerTarget",
                    "No valid build target found for %1")
                .arg(sdk->virtualMachineName());
            return QList<Task>() << Task(Task::Error, message, FileName(), -1,
                                         Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return QList<Task>();
}

QString MerTargetKitInformation::targetName(const Kit *kit)
{
    QTC_ASSERT(kit, return QString());
    return kit->value(MerTargetKitInformation::id()).toString();
}

MerTarget MerTargetKitInformation::target(const Kit *kit)
{
    QTC_ASSERT(kit, return MerTarget());

    QString targetName = MerTargetKitInformation::targetName(kit);
    if (targetName.isEmpty())
        return MerTarget();

    const MerSdk *const merSdk = MerSdkKitInformation::sdk(kit);
    QTC_ASSERT(merSdk, return MerTarget());

    return merSdk->target(targetName);
}

KitInformation::ItemList MerTargetKitInformation::toUserOutput(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        QString targetName = MerTargetKitInformation::targetName(kit);
        return KitInformation::ItemList()
                << qMakePair(tr("Sailfish OS build target"),targetName);
    }
    return KitInformation::ItemList();
}

KitConfigWidget *MerTargetKitInformation::createConfigWidget(Kit *kit) const
{
    return new MerTargetKitInformationWidget(kit, this);
}

Core::Id MerTargetKitInformation::id()
{
    return "Mer.Target.Kit.Information";
}
void MerTargetKitInformation::setTargetName(Kit *kit, const QString& targetName)
{
    if (kit->value(MerTargetKitInformation::id()) != targetName)
        kit->setValue(MerTargetKitInformation::id(), targetName);
}

void MerTargetKitInformation::addToEnvironment(const Kit *kit, Environment &env) const
{
    const QString targetName = MerTargetKitInformation::targetName(kit);
        env.appendOrSet(QLatin1String(Constants::MER_SSH_TARGET_NAME),targetName);
}

////////////////////////////////////////////////////////////////////////////////////////////

MerTargetKitInformationWidget::MerTargetKitInformationWidget(Kit *kit,
        const MerTargetKitInformation *kitInformation)
    : KitConfigWidget(kit, kitInformation),
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
    return tr("Sailfish OS build target:");
}

QString MerTargetKitInformationWidget::toolTip() const
{
    return tr("Name of an sb2 build target.");
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
    return DeviceTypeKitInformation::deviceTypeId(m_kit) == Constants::MER_DEVICE_TYPE;
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
    MerOptionsPage *page = PluginManager::getObject<MerOptionsPage>();
    if (page) {
        const MerSdk* sdk = MerSdkKitInformation::sdk(m_kit);
        if(sdk)
            page->setSdk(m_combo->currentText());
    }
    ICore::showOptionsDialog(Constants::MER_OPTIONS_ID);
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
