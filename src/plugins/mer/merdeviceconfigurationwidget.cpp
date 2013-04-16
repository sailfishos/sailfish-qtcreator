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

#include "merdeviceconfigurationwidget.h"
#include "ui_merdeviceconfigurationwidget.h"
#include "mervirtualboxmanager.h"

#include <utils/portlist.h>
#include <utils/fancylineedit.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>

#include <QTextStream>
#include <QDir>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerDeviceConfigurationWidget::MerDeviceConfigurationWidget(
        const IDevice::Ptr &deviceConfig, QWidget *parent) :
    IDeviceWidget(deviceConfig, parent),
    m_ui(new Ui::MerDeviceConfigurationWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->hostLineEdit, SIGNAL(editingFinished()), SLOT(hostNameEditingFinished()));
    connect(m_ui->userLineEdit, SIGNAL(editingFinished()), SLOT(userNameEditingFinished()));
    connect(m_ui->pwdLineEdit, SIGNAL(editingFinished()), SLOT(passwordEditingFinished()));
    connect(m_ui->passwordButton, SIGNAL(toggled(bool)), SLOT(authenticationTypeChanged()));
    connect(m_ui->keyFileLineEdit, SIGNAL(editingFinished()), SLOT(keyFileEditingFinished()));
    connect(m_ui->keyFileLineEdit, SIGNAL(browsingFinished()), SLOT(keyFileEditingFinished()));
    connect(m_ui->keyButton, SIGNAL(toggled(bool)), SLOT(authenticationTypeChanged()));
    connect(m_ui->timeoutSpinBox, SIGNAL(editingFinished()), SLOT(timeoutEditingFinished()));
    connect(m_ui->timeoutSpinBox, SIGNAL(valueChanged(int)), SLOT(timeoutEditingFinished()));
    connect(m_ui->sshPortSpinBox, SIGNAL(editingFinished()), SLOT(sshPortEditingFinished()));
    connect(m_ui->sshPortSpinBox, SIGNAL(valueChanged(int)), SLOT(sshPortEditingFinished()));
    connect(m_ui->showPasswordCheckBox, SIGNAL(toggled(bool)), SLOT(showPassword(bool)));
    connect(m_ui->portsLineEdit, SIGNAL(editingFinished()), SLOT(handleFreePortsChanged()));
    connect(m_ui->createKeyButton, SIGNAL(clicked()), SLOT(createNewKey()));

    initGui();
}

MerDeviceConfigurationWidget::~MerDeviceConfigurationWidget()
{
    delete m_ui;
}

void MerDeviceConfigurationWidget::authenticationTypeChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    const bool usePassword = m_ui->passwordButton->isChecked();
    sshParams.authenticationType = usePassword
            ? SshConnectionParameters::AuthenticationByPassword
            : SshConnectionParameters::AuthenticationByKey;
    device()->setSshParameters(sshParams);
    m_ui->pwdLineEdit->setEnabled(usePassword);
    m_ui->passwordLabel->setEnabled(usePassword);
    m_ui->keyFileLineEdit->setEnabled(!usePassword);
    m_ui->keyLabel->setEnabled(!usePassword);
    m_ui->createKeyButton->setEnabled(!usePassword);
}

void MerDeviceConfigurationWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.host = m_ui->hostLineEdit->text().trimmed();
    device()->setSshParameters(sshParams);
}

void MerDeviceConfigurationWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.port = m_ui->sshPortSpinBox->value();
    device()->setSshParameters(sshParams);
}

void MerDeviceConfigurationWidget::timeoutEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.timeout = m_ui->timeoutSpinBox->value();
    device()->setSshParameters(sshParams);
}

void MerDeviceConfigurationWidget::userNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.userName = m_ui->userLineEdit->text();
    device()->setSshParameters(sshParams);
}

void MerDeviceConfigurationWidget::passwordEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.password = m_ui->pwdLineEdit->text();
    device()->setSshParameters(sshParams);
}

void MerDeviceConfigurationWidget::keyFileEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.privateKeyFile = m_ui->keyFileLineEdit->path();
    device()->setSshParameters(sshParams);
}

void MerDeviceConfigurationWidget::handleFreePortsChanged()
{
    device()->setFreePorts(PortList::fromString(m_ui->portsLineEdit->text()));
    updatePortsWarningLabel();
}

void MerDeviceConfigurationWidget::showPassword(bool showClearText)
{
    m_ui->pwdLineEdit->setEchoMode(showClearText ? QLineEdit::Normal : QLineEdit::Password);
}

void MerDeviceConfigurationWidget::setPrivateKey(const QString &path)
{
    m_ui->keyFileLineEdit->setPath(path);
    keyFileEditingFinished();
}

void MerDeviceConfigurationWidget::createNewKey()
{
    SshKeyCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        setPrivateKey(dialog.privateKeyFilePath());
}

void MerDeviceConfigurationWidget::updateDeviceFromUi()
{
    VirtualMachineInfo info =
            MerVirtualBoxManager::fetchVirtualMachineInfo(device()->id().toString());
    int sshPort = info.sshPort ? info.sshPort : device()->sshParameters().port;
    m_ui->sshPortSpinBox->setValue(sshPort);
    if (info.freePorts.count()) {
        QStringList freePorts;
        foreach (quint16 port, info.freePorts)
            freePorts << QString::number(port);
        m_ui->portsLineEdit->setText(freePorts.join(QLatin1String(",")));
    } else {
        m_ui->portsLineEdit->setText(device()->freePorts().toString());
    }

    hostNameEditingFinished();
    sshPortEditingFinished();
    timeoutEditingFinished();
    userNameEditingFinished();
    passwordEditingFinished();
    keyFileEditingFinished();
    handleFreePortsChanged();
}

void MerDeviceConfigurationWidget::updatePortsWarningLabel()
{
    m_ui->portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void MerDeviceConfigurationWidget::initGui()
{
    if (device()->machineType() == IDevice::Hardware) {
        m_ui->machineTypeValueLabel->setText(tr("Physical Device"));
        m_ui->hostLineEdit->setReadOnly(false);
    } else {
        m_ui->machineTypeValueLabel->setText(tr("Emulator"));
        m_ui->hostLineEdit->setReadOnly(true);
    }
    m_ui->portsWarningLabel->setPixmap(QPixmap(QLatin1String(":/mer/images/warning.png")));
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
                                        + tr("You will need at least two ports for debugging.")
                                        + QLatin1String("</font>"));
    m_ui->keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    SshConnectionParameters::AuthenticationByPassword == sshParams.authenticationType
            ? m_ui->passwordButton->setChecked(true)
            : m_ui->keyButton->setChecked(true);
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);

    m_ui->hostLineEdit->setText(sshParams.host);
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    m_ui->pwdLineEdit->setText(sshParams.password);
    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    m_ui->showPasswordCheckBox->setChecked(false);
    VirtualMachineInfo info =
            MerVirtualBoxManager::fetchVirtualMachineInfo(device()->id().toString());
    int sshPort = info.sshPort ? info.sshPort : sshParams.port;
    m_ui->sshPortSpinBox->setValue(sshPort);
    if (info.freePorts.count()) {
        QStringList freePorts;
        foreach (quint16 port, info.freePorts)
            freePorts << QString::number(port);
        m_ui->portsLineEdit->setText(freePorts.join(QLatin1String(",")));
    } else {
        m_ui->portsLineEdit->setText(device()->freePorts().toString());
    }
    sshPortEditingFinished();
    handleFreePortsChanged();
    updatePortsWarningLabel();
}

} // Internal
} // Mer
