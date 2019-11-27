/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "meremulatordetailswidget.h"
#include "ui_meremulatordetailswidget.h"

#include "merconstants.h"

#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <ssh/sshconnection.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

using namespace Utils;
using namespace Sfdk;
using namespace Mer::Constants;

namespace Mer {
namespace Internal {

MerEmulatorDetailsWidget::MerEmulatorDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerEmulatorDetailsWidget)
{
    m_ui->setupUi(this);

    m_ui->sshPortInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_ui->sshPortInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    m_ui->qmlLivePortsInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_ui->qmlLivePortsInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    m_ui->freePortsInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_ui->freePortsInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    connect(m_ui->factorySnapshotToolButton, &QAbstractButton::clicked,
            this, &MerEmulatorDetailsWidget::selectFactorySnapshot);

    connect(m_ui->testConnectionPushButton, &QPushButton::clicked,
            this, &MerEmulatorDetailsWidget::testConnectionButtonClicked);
    connect(m_ui->sshPortSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerEmulatorDetailsWidget::sshPortChanged);
    connect(m_ui->sshTimeoutSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerEmulatorDetailsWidget::sshTimeoutChanged);

    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->qmlLivePortsLineEdit->setValidator(portsValidator);
    connect(m_ui->qmlLivePortsLineEdit, &QLineEdit::editingFinished, this, [=]() {
        emit qmlLivePortsChanged(PortList::fromString(m_ui->qmlLivePortsLineEdit->text()));
    });
    m_ui->freePortsLineEdit->setValidator(portsValidator);
    connect(m_ui->freePortsLineEdit, &QLineEdit::editingFinished, this, [=]() {
        emit freePortsChanged(PortList::fromString(m_ui->freePortsLineEdit->text()));
    });

    connect(m_ui->virtualMachineSettingsWidget, &MerVirtualMachineSettingsWidget::memorySizeMbChanged,
            this, &MerEmulatorDetailsWidget::memorySizeMbChanged);
    connect(m_ui->virtualMachineSettingsWidget, &MerVirtualMachineSettingsWidget::cpuCountChanged,
            this, &MerEmulatorDetailsWidget::cpuCountChanged);
    connect(m_ui->virtualMachineSettingsWidget, &MerVirtualMachineSettingsWidget::storageSizeMbChnaged,
            this, &MerEmulatorDetailsWidget::storageSizeMbChnaged);
}

MerEmulatorDetailsWidget::~MerEmulatorDetailsWidget()
{
    delete m_ui;
}

QString MerEmulatorDetailsWidget::searchKeyWordMatchString() const
{
    return m_ui->qmlLivePortsTitle->text();
}

void MerEmulatorDetailsWidget::setEmulator(const Sfdk::Emulator *emulator)
{
    m_emulator = emulator;
    m_ui->nameLabel->setText(emulator->name());
    m_ui->autodetectedLabel->setText(emulator->isAutodetected() ? tr("Yes") : tr("No"));
    m_ui->sshPortSpinBox->setValue(emulator->sshPort());
    m_ui->sshTimeoutSpinBox->setValue(emulator->virtualMachine()->sshParameters().timeout);
    m_ui->userNameLabel->setText(emulator->virtualMachine()->sshParameters().userName());
    m_ui->sshPrivateKeyLabel->setText(emulator->virtualMachine()->sshParameters().privateKeyFile);
    m_ui->qmlLivePortsLineEdit->setText(emulator->qmlLivePorts().toString());
    m_ui->freePortsLineEdit->setText(emulator->freePorts().toString());
    m_ui->sshFolderPathLabel->setText(QDir::toNativeSeparators(emulator->sharedSshPath().toString()));
    m_ui->configFolderPathLabel->setText(QDir::toNativeSeparators(emulator->sharedConfigPath().toString()));
    m_ui->virtualMachineSettingsWidget->setMemorySizeMb(emulator->virtualMachine()->memorySizeMb());
    m_ui->virtualMachineSettingsWidget->setCpuCount(emulator->virtualMachine()->cpuCount());
    m_ui->virtualMachineSettingsWidget->setStorageSizeMb(emulator->virtualMachine()->storageSizeMb());
}

void MerEmulatorDetailsWidget::setTestButtonEnabled(bool enabled)
{
    m_ui->testConnectionPushButton->setEnabled(enabled);
}

void MerEmulatorDetailsWidget::setStatus(const QString &status)
{
    m_ui->statusLabel->setText(status);
}

void MerEmulatorDetailsWidget::setVmOffStatus(bool vmOff)
{
    m_ui->sshPortSpinBox->setEnabled(vmOff);
    m_ui->sshPortInfoLabel->setVisible(!vmOff);
    m_ui->qmlLivePortsLineEdit->setEnabled(vmOff);
    m_ui->qmlLivePortsInfoLabel->setVisible(!vmOff);
    m_ui->freePortsLineEdit->setEnabled(vmOff);
    m_ui->freePortsInfoLabel->setVisible(!vmOff);
    m_ui->virtualMachineSettingsWidget->setVmOff(vmOff);
}

void MerEmulatorDetailsWidget::setFactorySnapshot(const QString &snapshotName)
{
    m_ui->factorySnapshotLineEdit->setText(snapshotName);
}

void MerEmulatorDetailsWidget::setSshTimeout(int timeout)
{
    m_ui->sshTimeoutSpinBox->setValue(timeout);
}

void MerEmulatorDetailsWidget::setSshPort(quint16 port)
{
    m_ui->sshPortSpinBox->setValue(port);
}

void MerEmulatorDetailsWidget::setQmlLivePorts(const Utils::PortList &ports)
{
    m_ui->qmlLivePortsLineEdit->setText(ports.toString());
}

void MerEmulatorDetailsWidget::setFreePorts(const Utils::PortList &ports)
{
    m_ui->freePortsLineEdit->setText(ports.toString());
}

void MerEmulatorDetailsWidget::setMemorySizeMb(int sizeMb)
{
    m_ui->virtualMachineSettingsWidget->setMemorySizeMb(sizeMb);
}

void MerEmulatorDetailsWidget::setCpuCount(int count)
{
    m_ui->virtualMachineSettingsWidget->setCpuCount(count);
}

void MerEmulatorDetailsWidget::setStorageSizeMb(int capacityMb)
{
    m_ui->virtualMachineSettingsWidget->setStorageSizeMb(capacityMb);
}

void MerEmulatorDetailsWidget::selectFactorySnapshot()
{
    QTC_ASSERT(m_emulator, return);

    const QStringList snapshots = m_emulator->virtualMachine()->snapshots();
    if (snapshots.isEmpty()) {
        QMessageBox::warning(this, tr("No snapshot found"),
                tr("No snapshot exists for the '%1' virtual machine.")
                .arg(m_emulator->virtualMachine()->name()));
        return;
    }

    bool ok;
    const bool editable = false;
    QString selected = QInputDialog::getItem(this, tr("Select factory snapshot"),
            tr("Select the virtual machine snapshot to be used as the factory snapshot for '%1'")
            .arg(m_emulator->name()),
            snapshots, snapshots.indexOf(m_emulator->factorySnapshot()), editable, &ok);
    if (!ok)
        return;

    m_ui->factorySnapshotLineEdit->setText(selected);
    emit factorySnapshotChanged(selected);
}

} // Internal
} // Mer
