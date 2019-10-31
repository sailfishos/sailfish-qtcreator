/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "merhardwaredevicewidget.h"
#include "ui_merhardwaredevicewidget.h"
#include "ui_merhardwaredevicewidgetauthorizedialog.h"

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
    connect(m_ui->privateKeyAuthorizeButton, &QAbstractButton::clicked,
            this, &MerHardwareDeviceWidget::authorizePrivateKey);
    initGui();
}

MerHardwareDeviceWidget::~MerHardwareDeviceWidget()
{
    delete m_ui;
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

void MerHardwareDeviceWidget::authorizePrivateKey()
{
    QPointer<QDialog> dialog = new QDialog(ICore::dialogParent());
    auto *ui = new Ui::MerHardwareDeviceWidget_AuthorizeDialog;
    ui->setupUi(dialog);
    ui->deviceLineEdit->setText(device()->displayName());
    ui->userNameLineEdit->setText(device()->sshParameters().userName());
    ui->progressBar->setVisible(false);
    ui->progressLabel->setText(QString());

    QPushButton *authorizeButton =
        ui->buttonBox->addButton(tr("Authorize"), QDialogButtonBox::ActionRole);

    auto generateKey = [this, ui]() -> bool {
        QString privateKeyFile = device()->sshParameters().privateKeyFile;
        QFileInfo info(privateKeyFile);
        if (info.exists())
            return true;

        ui->progressLabel->setText(tr("Generating key…"));

        QString error;
        if (!MerSdkManager::generateSshKey(privateKeyFile, error)) {
            QMessageBox::warning(ICore::dialogParent(), tr("Failed"),
                                 tr("Failed to generate key: %1").arg(error));
            return false;
        }
        return true;
    };

    auto deployKey = [this, ui]() -> bool {
        ui->progressLabel->setText(tr("Deploying key…"));
        QEventLoop loop;
        SshKeyDeployer deployer;
        connect(&deployer, &SshKeyDeployer::error, [&loop](const QString &errorString) {
            QMessageBox::warning(ICore::dialogParent(), tr("Failed"),
                                 tr("Failed to deploy SSH key: %1").arg(errorString));
            loop.exit(EXIT_FAILURE);
        });
        connect(&deployer, &SshKeyDeployer::finishedSuccessfully, [&loop]() {
            loop.exit(EXIT_SUCCESS);
        });

        SshConnectionParameters sshParameters = device()->sshParameters();
        sshParameters.authenticationType = SshConnectionParameters::AuthenticationTypeAll;
        QString publicKeyPath = sshParameters.privateKeyFile + QLatin1String(".pub");

        deployer.deployPublicKey(sshParameters, publicKeyPath);

        return loop.exec() == EXIT_SUCCESS;
    };

    connect(authorizeButton, &QAbstractButton::clicked, [&] {
        ui->formWidget->setEnabled(false);
        ui->buttonBox->setEnabled(false);
        ui->progressBar->setVisible(true);

        bool ok = generateKey() && deployKey();

        ui->progressBar->setVisible(false);
        ui->progressLabel->setText(QString());
        ui->buttonBox->setEnabled(true);
        ui->formWidget->setEnabled(true);

        updatePrivateKeyWarningLabel();

        if (ok) {
            QMessageBox::information(ICore::dialogParent(), tr("Authorized"),
                tr("Successfully authorized SSH key"));
            dialog->accept();
        }
    });

    dialog->exec();
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
    m_ui->qmlLivePortsWarningLabel->setVisible(count < 1
            || count > Sfdk::Constants::MAX_PORT_LIST_PORTS);
}

void MerHardwareDeviceWidget::updatePrivateKeyWarningLabel()
{
    QFileInfo info(device()->sshParameters().privateKeyFile);
    m_ui->privateKeyWarningLabel->setVisible(!info.exists());
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
    m_ui->privateKeyWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->privateKeyWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
                                             + tr("File does not exist.")
                                             + QLatin1String("</font>"));

    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);
    m_ui->qmlLivePortsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    m_ui->autheticationTypeLabelEdit->setText(tr("SSH Key"));
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setText(sshParams.host());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName());
    m_ui->privateKeyLabelEdit->setText(QDir::toNativeSeparators(sshParams.privateKeyFile));
    m_ui->sshPortSpinBox->setValue(sshParams.port());
    m_ui->portsLineEdit->setText(device()->freePorts().toString());
    m_ui->qmlLivePortsLineEdit->setText(device().staticCast<MerDevice>()->qmlLivePorts().toString());
    updatePortsWarningLabel();
    updateQmlLivePortsWarningLabel();
    updatePrivateKeyWarningLabel();
}

} // Internal
} // Mer
