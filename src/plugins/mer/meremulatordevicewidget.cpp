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

#include "meremulatordevicewidget.h"
#include "ui_meremulatordevicewidget.h"

#include "merconnection.h"
#include "merconstants.h"
#include "meremulatordevice.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>
#include <utils/fancylineedit.h>
#include <utils/portlist.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerEmulatorDeviceWidget::MerEmulatorDeviceWidget(
        const IDevice::Ptr &deviceConfig, QWidget *parent) :
    IDeviceWidget(deviceConfig, parent),
    m_ui(new Ui::MerEmulatorDeviceWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->userLineEdit, &QLineEdit::editingFinished,
            this, &MerEmulatorDeviceWidget::userNameEditingFinished);
    connect(m_ui->timeoutSpinBox, &QSpinBox::editingFinished,
            this, &MerEmulatorDeviceWidget::timeoutEditingFinished);
    connect(m_ui->timeoutSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerEmulatorDeviceWidget::timeoutEditingFinished);
    connect(m_ui->portsLineEdit, &QLineEdit::editingFinished,
            this, &MerEmulatorDeviceWidget::handleFreePortsChanged);
    connect(m_ui->qmlLivePortsLineEdit, &QLineEdit::editingFinished,
            this, &MerEmulatorDeviceWidget::handleQmlLivePortsChanged);
    initGui();
}

MerEmulatorDeviceWidget::~MerEmulatorDeviceWidget()
{
    delete m_ui;
}

void MerEmulatorDeviceWidget::onVirtualMachineOffChanged(bool vmOff)
{
    m_ui->qmlLivePortsLineEdit->setEnabled(vmOff);
    m_ui->qmlLivePortsInfoLabel->setVisible(!vmOff);
}

void MerEmulatorDeviceWidget::timeoutEditingFinished()
{
    Q_ASSERT(dynamic_cast<MerEmulatorDevice *>(this->device().data()) != 0);
    MerEmulatorDevice* device = static_cast<MerEmulatorDevice*>(this->device().data());

    SshConnectionParameters sshParams = device->sshParameters();
    sshParams.timeout = m_ui->timeoutSpinBox->value();
    device->setSshParameters(sshParams);
    device->updateConnection();
}

void MerEmulatorDeviceWidget::userNameEditingFinished()
{
    Q_ASSERT(dynamic_cast<MerEmulatorDevice *>(this->device().data()) != 0);
    MerEmulatorDevice* device = static_cast<MerEmulatorDevice*>(this->device().data());

    if(!device->sharedConfigPath().isEmpty()) {
        QString index = QLatin1String("/ssh/private_keys/%1/");
        SshConnectionParameters sshParams = device->sshParameters();
        const QString& user = m_ui->userLineEdit->text();
        //TODO fix me:
        const QString privKey = device->sharedConfigPath() +
                index.arg(device->virtualMachine()).replace(QLatin1Char(' '),QLatin1Char('_'))
                + user;

        sshParams.userName = user;
        sshParams.privateKeyFile = privKey;
        m_ui->sshKeyLabelEdit->setText(privKey);
        device->setSshParameters(sshParams);
        device->updateConnection();
    }
}

void MerEmulatorDeviceWidget::handleFreePortsChanged()
{
    device()->setFreePorts(PortList::fromString(m_ui->portsLineEdit->text()));
    updatePortsWarningLabel();
}

void MerEmulatorDeviceWidget::handleQmlLivePortsChanged()
{
    device().staticCast<MerDevice>()->setQmlLivePorts(PortList::fromString(m_ui->qmlLivePortsLineEdit->text()));
    updateQmlLivePortsWarningLabel();
}

void MerEmulatorDeviceWidget::updateDeviceFromUi()
{
    timeoutEditingFinished();
    userNameEditingFinished();
    handleFreePortsChanged();
    handleQmlLivePortsChanged();
}

void MerEmulatorDeviceWidget::updatePortsWarningLabel()
{
    m_ui->portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void MerEmulatorDeviceWidget::updateQmlLivePortsWarningLabel()
{
    const int count = device().staticCast<MerDevice>()->qmlLivePorts().count();
    m_ui->qmlLivePortsWarningLabel->setVisible(count < 1 || count > Constants::MAX_QML_LIVE_PORTS);
}

void MerEmulatorDeviceWidget::initGui()
{
    m_ui->portsWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
                                        + tr("You will need at least two ports for debugging.")
                                        + QLatin1String("</font>"));
    m_ui->qmlLivePortsInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_ui->qmlLivePortsInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop emulator to unlock this field for editing.")
            + QLatin1String("</font>"));
    m_ui->qmlLivePortsWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->qmlLivePortsWarningLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("You will need at least one and at most %1 ports for QmlLive use.").arg(Constants::MAX_QML_LIVE_PORTS)
            + QLatin1String("</font>"));
    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);
    m_ui->qmlLivePortsLineEdit->setValidator(portsValidator);

    Q_ASSERT(dynamic_cast<MerEmulatorDevice *>(this->device().data()) != 0);
    const MerEmulatorDevice* device = static_cast<MerEmulatorDevice*>(this->device().data());
    const SshConnectionParameters &sshParams = device->sshParameters();
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    if (!sshParams.privateKeyFile.isEmpty())
        m_ui->sshKeyLabelEdit->setText(QDir::toNativeSeparators(sshParams.privateKeyFile));
    else
        m_ui->sshKeyLabelEdit->setText(tr("none"));
    if(sshParams.port > 0)
        m_ui->sshPortLabelEdit->setText(QString::number(sshParams.port));
    else
        m_ui->sshPortLabelEdit->setText(tr("none"));
    m_ui->portsLineEdit->setText(device->freePorts().toString());
    m_ui->qmlLivePortsLineEdit->setText(device->qmlLivePorts().toString());
    m_ui->emulatorVmLabelEdit->setText(device->virtualMachine());
    if(!device->sharedConfigPath().isEmpty())
        m_ui->configFolderLabelEdit->setText(QDir::toNativeSeparators(device->sharedConfigPath()));
    else
        m_ui->configFolderLabelEdit->setText(tr("none"));
    if(!device->sharedSshPath().isEmpty())
        m_ui->sshFolderLabelEdit->setText(QDir::toNativeSeparators(device->sharedSshPath()));
    else
        m_ui->sshFolderLabelEdit->setText(tr("none"));

    if(!device->mac().isEmpty())
        m_ui->macLabelEdit->setText(device->mac());
    else
        m_ui->macLabelEdit->setText(tr("none"));
    //block "nemo" user
    m_ui->userLineEdit->setEnabled(false);
    updatePortsWarningLabel();
    updateQmlLivePortsWarningLabel();

    onVirtualMachineOffChanged(device->connection()->isVirtualMachineOff());
    connect(device->connection(), &MerConnection::virtualMachineOffChanged,
            this, &MerEmulatorDeviceWidget::onVirtualMachineOffChanged);
}

} // Internal
} // Mer
