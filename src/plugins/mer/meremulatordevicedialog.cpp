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
#include "merconstants.h"
#include "mersdkmanager.h"
#include "meremulatordevicedialog.h"
#include "ui_meremulatordevicedialog.h"
#include "mervirtualboxmanager.h"

namespace Mer {
namespace Internal {

MerEmulatorDeviceDialog::MerEmulatorDeviceDialog(QWidget *parent): QDialog(parent),
    m_ui(new Ui::MerEmulatorDeviceDialog),
    m_index(0)
{
    m_ui->setupUi(this);
    setWindowTitle(tr("Sailfish Emulator Information"));
    m_ui->configNameLineEdit->setText(tr("SailfishOS Emulator"));
    m_ui->userLineEdit->setText(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
    m_ui->sshPortSpinBox->setMinimum(1);
    m_ui->sshPortSpinBox->setMaximum(65535);
    m_ui->sshPortSpinBox->setValue(2223);
    m_ui->timeoutSpinBox->setMinimum(1);
    m_ui->timeoutSpinBox->setMaximum(65535);
    m_ui->timeoutSpinBox->setValue(10);

    m_index = MerSdkManager::generateDeviceId();

    QList<MerSdk*> sdks = MerSdkManager::instance()->sdks();
    QStringList vmNames;
    foreach (const MerSdk *s, sdks) {
        vmNames << s->virtualMachineName();
        m_ui->merVmComboBox->addItem(s->virtualMachineName());
    }

    static QRegExp regExp(tr("Emulator"));

    const QStringList registeredVMs = MerVirtualBoxManager::fetchRegisteredVirtualMachines();

    foreach (const QString &vm, registeredVMs) {
        //add remove machine wich are sdks
        if(!vmNames.contains(vm)) {
            m_ui->emulatorComboBox->addItem(vm);
            if (regExp.indexIn(vm) != -1) {
                //preselect emulator
                m_ui->emulatorComboBox->setCurrentIndex(m_ui->emulatorComboBox->count()-1);
            }
        }
    }

    connect(m_ui->merVmComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(handleKeyChanged()));
    connect(m_ui->emulatorComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(handleEmulatorVmChanged(QString)));
    connect(m_ui->userLineEdit, SIGNAL(textChanged(QString)), this, SLOT(handleKeyChanged()));
    handleKeyChanged();
    handleEmulatorVmChanged(m_ui->emulatorComboBox->currentText());
}

MerEmulatorDeviceDialog::~MerEmulatorDeviceDialog()
{

}

QString MerEmulatorDeviceDialog::configName() const
{
    return m_ui->configNameLineEdit->text();
}

QString MerEmulatorDeviceDialog::userName() const
{
    return m_ui->userLineEdit->text();
}

int MerEmulatorDeviceDialog::sshPort() const
{
    return m_ui->sshPortSpinBox->value();
}

int MerEmulatorDeviceDialog::timeout() const
{
    return m_ui->timeoutSpinBox->value();
}

QString MerEmulatorDeviceDialog::privateKey() const
{
    return m_ui->sshKeyLabelEdit->text();
}

QString MerEmulatorDeviceDialog::emulatorVm() const
{
    return m_ui->emulatorComboBox->currentText();
}

QString MerEmulatorDeviceDialog::sdkVm() const
{
    return m_ui->merVmComboBox->currentText();
}

QString MerEmulatorDeviceDialog::freePorts() const
{
    return m_ui->portsLineEdit->text();
}

void MerEmulatorDeviceDialog::handleKeyChanged()
{
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    const MerSdk* sdk = MerSdkManager::instance()->sdk(sdkVm());
    if(!sdk) return;
    m_ui->sshKeyLabelEdit->setText(sdk->sharedConfigPath() + index.arg(m_index)  + userName());
}

void MerEmulatorDeviceDialog::handleEmulatorVmChanged(const QString &vmName)
{
    VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(vmName);
    if (info.sshPort == 0)
        m_ui->sshPortSpinBox->setValue(22);
    else
        m_ui->sshPortSpinBox->setValue(info.sshPort);
    QStringList freePorts;
    foreach (quint16 port, info.freePorts)
        freePorts << QString::number(port);
    m_ui->portsLineEdit->setText(freePorts.join(QLatin1String(",")));
}

int MerEmulatorDeviceDialog::index() const
{
    return m_index;
}

}
}
