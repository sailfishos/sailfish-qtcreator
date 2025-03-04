/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "mervirtualmachinesettingswidget.h"

#include "ui_mervirtualmachinesettingswidget.h"

#include <sfdk/virtualmachine.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFormLayout>

using namespace Sfdk;

namespace Mer {
namespace Internal {

namespace {
const int DEFAULT_MAX_MEMORY_SIZE_MB = 3584;
const int MAX_STORAGE_SIZE_INCREMENT_GB = 10;
const int MIN_MEMORY_SIZE_MB = 512;

} // namespace anonymous

MerVirtualMachineSettingsWidget::MerVirtualMachineSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MerVirtualMachineSettingsWidget)
{
    ui->setupUi(this);
    initGui();
}

MerVirtualMachineSettingsWidget::~MerVirtualMachineSettingsWidget()
{
    delete ui;
}

void MerVirtualMachineSettingsWidget::setVmFeatures(VirtualMachine::Features features)
{
    m_features = features;

    QString stopVmText = tr("Stop the virtual machine to unlock this field for editing.");

    if (features & VirtualMachine::LimitMemorySize)
        setToolTip(ui->memoryInfoLabel, stopVmText);
    else
        setToolTip(ui->memoryInfoLabel, tr("Virtual Machine does not allow changing memory size"));

    if (features & VirtualMachine::LimitCpuCount)
        setToolTip(ui->cpuInfoLabel, stopVmText);
    else
        setToolTip(ui->cpuInfoLabel, tr("Virtual Machine does not allow changing cpu count"));

    if (features & (VirtualMachine::GrowStorageSize | VirtualMachine::ShrinkStorageSize))
        setToolTip(ui->storageSizeInfoLabel, stopVmText);
    else
        setToolTip(ui->storageSizeInfoLabel, tr("Virtual Machine does not allow changing storage size"));

    if (features & VirtualMachine::SwapMemory) {
        // hack: cannot restore once disabled
        QTC_CHECK(!ui->swapLabel->isHidden());
        setToolTip(ui->swapSizeInfoLabel, stopVmText);
    } else if (ui->swapLabel->isVisibleTo(this)) {
        ui->swapLabel->setVisible(false);
        ui->swapSizeInfoLabel->setVisible(false);
        ui->swapSizeGbSpinBox->setVisible(false);

        // QTBUG-6864
        ui->formLayout->takeRow(ui->swapLabel);
    }
}

void MerVirtualMachineSettingsWidget::setMemorySizeMb(int sizeMb)
{
    if (ui->memorySpinBox->maximum() < sizeMb)
        ui->memorySpinBox->setRange(MIN_MEMORY_SIZE_MB, sizeMb);

    ui->memorySpinBox->setValue(sizeMb);
}

void MerVirtualMachineSettingsWidget::setSwapSizeMb(int sizeMb)
{
    const double sizeGb = sizeMb / 1024.0;
    ui->swapSizeGbSpinBox->setValue(sizeGb);
}

void MerVirtualMachineSettingsWidget::setCpuCount(int count)
{
    ui->cpuCountSpinBox->setValue(count);
}

void MerVirtualMachineSettingsWidget::setStorageSizeMb(int storageSizeMb)
{
    const double storageSizeGb = storageSizeMb / 1024.0;
    ui->storageSizeGbSpinBox->setValue(storageSizeGb);
    setStorageSizeLimits();
}

void MerVirtualMachineSettingsWidget::setVmOff(bool vmOff)
{
    ui->memorySpinBox->setEnabled(vmOff && (m_features & VirtualMachine::LimitMemorySize));
    ui->swapSizeGbSpinBox->setEnabled(vmOff && (m_features & VirtualMachine::SwapMemory));
    ui->cpuCountSpinBox->setEnabled(vmOff && (m_features & VirtualMachine::LimitCpuCount));
    ui->storageSizeGbSpinBox->setEnabled(vmOff && (m_features & (VirtualMachine::GrowStorageSize
                                                                 | VirtualMachine::ShrinkStorageSize)));

    ui->memoryInfoLabel->setVisible(!ui->memorySpinBox->isEnabled());
    ui->swapSizeInfoLabel->setVisible(!ui->swapSizeGbSpinBox->isEnabled()
            && (m_features & VirtualMachine::SwapMemory));
    ui->cpuInfoLabel->setVisible(!ui->cpuCountSpinBox->isEnabled());
    ui->storageSizeInfoLabel->setVisible(!ui->storageSizeGbSpinBox->isEnabled());
}

QFormLayout *MerVirtualMachineSettingsWidget::formLayout() const
{
    return ui->formLayout;
}

void MerVirtualMachineSettingsWidget::initGui()
{
    int maxMemorySizeMb = VirtualMachine::availableMemorySizeMb() > 0
        ? VirtualMachine::availableMemorySizeMb()
        : DEFAULT_MAX_MEMORY_SIZE_MB;
    ui->swapSizeGbSpinBox->setRange(0, 999999); // "unlimited", later limited by storageSizeGb
    ui->memorySpinBox->setRange(MIN_MEMORY_SIZE_MB, maxMemorySizeMb);
    ui->cpuCountSpinBox->setRange(1, VirtualMachine::availableCpuCount());

    connect(ui->memorySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MerVirtualMachineSettingsWidget::memorySizeMbChanged);
    connect(ui->swapSizeGbSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
        emit swapSizeMbChanged(static_cast<int>(value * 1024));
    });
    connect(ui->cpuCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MerVirtualMachineSettingsWidget::cpuCountChanged);
    connect(ui->storageSizeGbSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
        QTC_CHECK(ui->swapSizeGbSpinBox->value() <= value);
        ui->swapSizeGbSpinBox->setMaximum(value);

        emit storageSizeMbChnaged(static_cast<int>(value * 1024));
    });

    ui->memoryInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->swapSizeInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->cpuInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->storageSizeInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
}

void MerVirtualMachineSettingsWidget::setToolTip(QLabel *label, const QString &toolTipText)
{
   label->setToolTip(QLatin1String("<font color=\"red\">")
                     + toolTipText
                     + QLatin1String("</font>"));
}

void MerVirtualMachineSettingsWidget::setStorageSizeLimits()
{
    // Prohibit adding more than MAX_STORAGE_SIZE_INCREMENT_GB in one step, because there is no way to go back
    // with VirtualBox
    const double currentValue = ui->storageSizeGbSpinBox->value();
    const double maximumSizeGb = m_features & VirtualMachine::GrowStorageSize
        ? currentValue + MAX_STORAGE_SIZE_INCREMENT_GB
        : currentValue;
    const double minimumSizeGb = m_features & VirtualMachine::ShrinkStorageSize
        ? 0
        : currentValue;
    ui->storageSizeGbSpinBox->setRange(minimumSizeGb, maximumSizeGb);
}

} // Internal
} // Mer
