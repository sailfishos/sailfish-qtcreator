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

#include "merhardwaredevicewidget.h"
#include "ui_merhardwaredevicewidget.h"

#include "merconstants.h"
#include "merhardwaredevice.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/coreicons.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>
#include <utils/fancylineedit.h>
#include <utils/portlist.h>

#include <QDir>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerHardwareDeviceWidget::MerHardwareDeviceWidget(
        const IDevice::Ptr &deviceConfig, QWidget *parent) :
    IDeviceWidget(deviceConfig, parent),
    m_ui(new Ui::MerHardwareDeviceWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->hostLineEdit, &QLineEdit::editingFinished,
            this, &MerHardwareDeviceWidget::hostNameEditingFinished);
    connect(m_ui->userLineEdit, &QLineEdit::editingFinished,
            this, &MerHardwareDeviceWidget::userNameEditingFinished);
    connect(m_ui->timeoutSpinBox, &QSpinBox::editingFinished,
            this, &MerHardwareDeviceWidget::timeoutEditingFinished);
    connect(m_ui->timeoutSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerHardwareDeviceWidget::timeoutEditingFinished);
    connect(m_ui->sshPortSpinBox, &QSpinBox::editingFinished,
            this, &MerHardwareDeviceWidget::sshPortEditingFinished);
    connect(m_ui->sshPortSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerHardwareDeviceWidget::sshPortEditingFinished);
    connect(m_ui->portsLineEdit, &QLineEdit::editingFinished,
            this, &MerHardwareDeviceWidget::handleFreePortsChanged);
    connect(m_ui->qmlLivePortsLineEdit, &QLineEdit::editingFinished,
            this, &MerHardwareDeviceWidget::handleQmlLivePortsChanged);
    initGui();
}

MerHardwareDeviceWidget::~MerHardwareDeviceWidget()
{
    delete m_ui;
}

void MerHardwareDeviceWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.host = m_ui->hostLineEdit->text().trimmed();
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.port = m_ui->sshPortSpinBox->value();
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::timeoutEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.timeout = m_ui->timeoutSpinBox->value();
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::userNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.userName = m_ui->userLineEdit->text();
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::handleFreePortsChanged()
{
    device()->setFreePorts(PortList::fromString(m_ui->portsLineEdit->text()));
    updatePortsWarningLabel();
}

void MerHardwareDeviceWidget::handleQmlLivePortsChanged()
{
    device().staticCast<MerDevice>()->setQmlLivePorts(PortList::fromString(m_ui->qmlLivePortsLineEdit->text()));
    updateQmlLivePortsWarningLabel();
}

void MerHardwareDeviceWidget::updateDeviceFromUi()
{
    hostNameEditingFinished();
    sshPortEditingFinished();
    timeoutEditingFinished();
    userNameEditingFinished();
    handleFreePortsChanged();
    handleQmlLivePortsChanged();
}

void MerHardwareDeviceWidget::updatePortsWarningLabel()
{
    m_ui->portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void MerHardwareDeviceWidget::updateQmlLivePortsWarningLabel()
{
    const int count = device().staticCast<MerDevice>()->qmlLivePorts().count();
    m_ui->qmlLivePortsWarningLabel->setVisible(count < 1 || count > Constants::MAX_QML_LIVE_PORTS);
}

void MerHardwareDeviceWidget::initGui()
{
    m_ui->portsWarningLabel->setPixmap(Core::Icons::WARNING.pixmap());
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
                                        + tr("You will need at least two ports for debugging.")
                                        + QLatin1String("</font>"));
    m_ui->qmlLivePortsWarningLabel->setPixmap(Core::Icons::WARNING.pixmap());
    m_ui->qmlLivePortsWarningLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("You will need at least one and at most %1 ports for QmlLive use.").arg(Constants::MAX_QML_LIVE_PORTS)
            + QLatin1String("</font>"));
    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);
    m_ui->qmlLivePortsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    m_ui->autheticationTypeLabelEdit->setText(tr("SSH Key"));
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setText(sshParams.host);
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    m_ui->privateKeyLabelEdit->setText(QDir::toNativeSeparators(sshParams.privateKeyFile));
    m_ui->sshPortSpinBox->setValue(sshParams.port);
    m_ui->portsLineEdit->setText(device()->freePorts().toString());
    m_ui->qmlLivePortsLineEdit->setText(device().staticCast<MerDevice>()->qmlLivePorts().toString());
    updatePortsWarningLabel();
    updateQmlLivePortsWarningLabel();
}

} // Internal
} // Mer
