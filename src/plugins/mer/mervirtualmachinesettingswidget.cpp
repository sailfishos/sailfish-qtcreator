#include "mervirtualmachinesettingswidget.h"
#include "mervirtualboxmanager.h"
#include <utils/utilsicons.h>
#include "ui_mervirtualmachinesettingswidget.h"
#include <QFormLayout>

namespace Mer {
namespace Internal {

namespace {
const int DEFAULT_MAX_MEMORY_SIZE_MB = 3584;
const int MAX_VDI_SIZE_INCREMENT_GB = 10;
const int MIN_VIRTUALBOX_MEMORY_SIZE_MB = 512;

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

void MerVirtualMachineSettingsWidget::setMemorySizeMb(int sizeMb)
{
    if (ui->memorySpinBox->maximum() < sizeMb)
        ui->memorySpinBox->setRange(MIN_VIRTUALBOX_MEMORY_SIZE_MB, sizeMb);

    ui->memorySpinBox->setValue(sizeMb);
}

void MerVirtualMachineSettingsWidget::setCpuCount(int count)
{
    ui->cpuCountSpinBox->setValue(count);
}

void MerVirtualMachineSettingsWidget::setVdiCapacityMb(int vdiCapacityMb)
{
    const double vdiCapacityGb = vdiCapacityMb / 1024.0;
    // Prohibit adding more than MAX_VDI_SIZE_INCREMENT_GB in one step, because there is no way to go back
    // the minimum size is the current size because VBoxManager can't reduce storage size
    ui->vdiCapacityGbSpinBox->setRange(vdiCapacityGb, vdiCapacityGb + MAX_VDI_SIZE_INCREMENT_GB);
    ui->vdiCapacityGbSpinBox->setValue(vdiCapacityGb);
}

void MerVirtualMachineSettingsWidget::setVmOff(bool vmOff)
{
    ui->memorySpinBox->setEnabled(vmOff);
    ui->cpuCountSpinBox->setEnabled(vmOff);
    ui->vdiCapacityGbSpinBox->setEnabled(vmOff);

    ui->memoryInfoLabel->setVisible(!vmOff);
    ui->cpuInfoLabel->setVisible(!vmOff);
    ui->vdiCapacityInfoLabel->setVisible(!vmOff);
}

QFormLayout *MerVirtualMachineSettingsWidget::formLayout() const
{
    return ui->formLayout;
}

void MerVirtualMachineSettingsWidget::initGui()
{
    ui->memorySpinBox->setRange(MIN_VIRTUALBOX_MEMORY_SIZE_MB, DEFAULT_MAX_MEMORY_SIZE_MB);
    MerVirtualBoxManager::getHostTotalMemorySizeMb(this, [this] (int sizeMb) {
        int maxMemorySizeMb = ui->memorySpinBox->value() > sizeMb ? ui->memorySpinBox->value() : sizeMb;
        ui->memorySpinBox->setRange(MIN_VIRTUALBOX_MEMORY_SIZE_MB, maxMemorySizeMb);
    });

    const int maxCpuCount = MerVirtualBoxManager::getHostTotalCpuCount();
    ui->cpuCountSpinBox->setRange(1, maxCpuCount);

    connect(ui->memorySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MerVirtualMachineSettingsWidget::memorySizeMbChanged);
    connect(ui->cpuCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MerVirtualMachineSettingsWidget::cpuCountChanged);
    connect(ui->vdiCapacityGbSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
        emit vdiCapacityMbChnaged(static_cast<int>(value * 1024));
    });

    ui->memoryInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->memoryInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    ui->cpuInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->cpuInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    ui->vdiCapacityInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->vdiCapacityInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));
}

} // Internal
} // Mer
