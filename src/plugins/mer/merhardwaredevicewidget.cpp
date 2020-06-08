/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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
#include "mersdkmanager.h"

#include <sfdk/sfdkconstants.h>

#include <coreplugin/icore.h>
#include <remotelinux/sshkeydeployer.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>
#include <utils/fancylineedit.h>
#include <utils/portlist.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QDialog>
#include <QMessageBox>
#include <QPointer>
#include <QTextStream>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace RemoteLinux;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerHardwareDeviceWidget::MerHardwareDeviceWidget(const IDevice::Ptr &deviceConfig)
    : IDeviceWidget(deviceConfig)
    , m_ui(new Ui::MerHardwareDeviceWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->hostLineEdit, &QLineEdit::editingFinished,
            this, &MerHardwareDeviceWidget::hostNameEditingFinished);
    connect(m_ui->userLineEdit, &QLineEdit::editingFinished,
            this, &MerHardwareDeviceWidget::userNameEditingFinished);
    connect(m_ui->keyFileLineEdit, &PathChooser::editingFinished,
            this, &MerHardwareDeviceWidget::keyFileEditingFinished);
    connect(m_ui->keyFileLineEdit, &PathChooser::browsingFinished,
            this, &MerHardwareDeviceWidget::keyFileEditingFinished);
    connect(m_ui->keyButton, &QAbstractButton::toggled,
            this, &MerHardwareDeviceWidget::authenticationTypeChanged);
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
    connect(m_ui->createKeyButton, &QAbstractButton::clicked,
            this, &MerHardwareDeviceWidget::createNewKey);
    initGui();
}

MerHardwareDeviceWidget::~MerHardwareDeviceWidget()
{
    delete m_ui;
}

void MerHardwareDeviceWidget::authenticationTypeChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    const bool useKeyFile = m_ui->keyButton->isChecked();
    sshParams.authenticationType = useKeyFile
            ? SshConnectionParameters::AuthenticationTypeSpecificKey
            : SshConnectionParameters::AuthenticationTypeAll;
    device()->setSshParameters(sshParams);
    m_ui->keyFileLineEdit->setEnabled(useKeyFile);
    m_ui->keyLabel->setEnabled(useKeyFile);
}

void MerHardwareDeviceWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.setHost(m_ui->hostLineEdit->text().trimmed());
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.setPort(m_ui->sshPortSpinBox->value());
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
    sshParams.setUserName(m_ui->userLineEdit->text());
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::keyFileEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.privateKeyFile = m_ui->keyFileLineEdit->path();
    device()->setSshParameters(sshParams);
}

void MerHardwareDeviceWidget::handleFreePortsChanged()
{
    device()->setFreePorts(PortList::fromString(m_ui->portsLineEdit->text()));
    updatePortsWarningLabel();
}

void MerHardwareDeviceWidget::handleQmlLivePortsChanged()
{
    device().staticCast<MerHardwareDevice>()
        ->setQmlLivePorts(PortList::fromString(m_ui->qmlLivePortsLineEdit->text()));
    updateQmlLivePortsWarningLabel();
}

void MerHardwareDeviceWidget::setPrivateKey(const QString &path)
{
    m_ui->keyFileLineEdit->setPath(path);
    keyFileEditingFinished();
}

void MerHardwareDeviceWidget::createNewKey()
{
    SshKeyCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        setPrivateKey(dialog.privateKeyFilePath());
}

void MerHardwareDeviceWidget::updateDeviceFromUi()
{
    hostNameEditingFinished();
    sshPortEditingFinished();
    timeoutEditingFinished();
    userNameEditingFinished();
    keyFileEditingFinished();
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
    m_ui->qmlLivePortsWarningLabel->setVisible(count < 1
            || count > Sfdk::Constants::MAX_PORT_LIST_PORTS);
}

void MerHardwareDeviceWidget::initGui()
{
    m_ui->portsWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
                                        + tr("You will need at least two ports for debugging.")
                                        + QLatin1String("</font>"));
    m_ui->qmlLivePortsWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->qmlLivePortsWarningLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("You will need at least one and at most %1 ports for QmlLive use.")
                .arg(Sfdk::Constants::MAX_PORT_LIST_PORTS)
            + QLatin1String("</font>"));

    m_ui->keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_ui->keyFileLineEdit->setHistoryCompleter(QLatin1String("Ssh.KeyFile.History"));
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);

    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);
    m_ui->qmlLivePortsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setText(sshParams.host());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName());
    m_ui->sshPortSpinBox->setValue(sshParams.port());
    m_ui->portsLineEdit->setText(device()->freePorts().toString());
    m_ui->qmlLivePortsLineEdit->setText(device().staticCast<MerDevice>()->qmlLivePorts().toString());

    switch (sshParams.authenticationType) {
    case SshConnectionParameters::AuthenticationTypeSpecificKey:
        m_ui->keyButton->setChecked(true);
        m_ui->keyFileLineEdit->setEnabled(true);
        m_ui->keyLabel->setEnabled(true);
        break;
    case SshConnectionParameters::AuthenticationTypeAll:
        m_ui->defaultAuthButton->setChecked(true);
        m_ui->keyFileLineEdit->setEnabled(false);
        m_ui->keyLabel->setEnabled(false);
        break;
    }

    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);

    updatePortsWarningLabel();
    updateQmlLivePortsWarningLabel();
}

} // Internal
} // Mer
