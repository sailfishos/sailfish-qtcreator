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

#include "ui_merdeviceconfigwizardcheckpreviouskeysetupcheckpage.h"
#include "ui_merdeviceconfigwizardkeycreationpage.h"
#include "ui_merdeviceconfigwizardreusekeyscheckpage.h"
#include "ui_merdeviceconfigwizardgeneralpage.h"
#include "ui_merdeviceconfigwizarddevicetypepage.h"
#include "merdeviceconfigurationwizardsetuppages.h"
#include "merconstants.h"
#include "merconnectionmanager.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <ssh/sshkeygenerator.h>

#include <QButtonGroup>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;
using namespace QSsh;

namespace Mer {
namespace Internal {

MerDeviceConfigWizardGeneralPage::MerDeviceConfigWizardGeneralPage(const WizardData &wizardData,
                                                                   QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerDeviceConfigWizardGeneralPage)
    , m_wizardData(wizardData)
{
    m_ui->setupUi(this);
    setTitle(tr("General Information"));

    QButtonGroup *buttonGroup1 = new QButtonGroup(this);
    buttonGroup1->setExclusive(true);
    buttonGroup1->addButton(m_ui->passwordRadioButton);
    buttonGroup1->addButton(m_ui->keyRadioButton);
    QButtonGroup *buttonGroup2 = new QButtonGroup(this);
    buttonGroup2->setExclusive(true);
    buttonGroup2->addButton(m_ui->realDeviceRadioButton);
    buttonGroup2->addButton(m_ui->virtualMachineRadioButton);
    connect(buttonGroup1, SIGNAL(buttonClicked(int)), SLOT(authTypeChanged()));
    connect(buttonGroup1, SIGNAL(buttonClicked(int)), SIGNAL(completeChanged()));
    connect(buttonGroup2, SIGNAL(buttonClicked(int)), SLOT(machineTypeChanged()));
    connect(buttonGroup2, SIGNAL(buttonClicked(int)), SIGNAL(completeChanged()));
    m_ui->keyRadioButton->setChecked(true);
    authTypeChanged();

    m_ui->usernameLineEdit->setText(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
    m_ui->sshPortSpinBox->setMinimum(1);
    m_ui->sshPortSpinBox->setMaximum(65535);
    m_ui->sshPortSpinBox->setValue(2223);

    connect(m_ui->deviceNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->hostNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->passwordLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->virtualMachineComboBox, SIGNAL(currentIndexChanged(QString)),
            SLOT(onVirtualMachineChanged(QString)));
}

void MerDeviceConfigWizardGeneralPage::initializePage()
{
    if (IDevice::Hardware == m_wizardData.machineType) {
        m_ui->realDeviceRadioButton->setChecked(true);
    } else {
        m_ui->virtualMachineRadioButton->setChecked(true);
    }
    machineTypeChanged();
}

bool MerDeviceConfigWizardGeneralPage::isComplete() const
{
    return !configName().isEmpty()
            && !hostName().isEmpty()
            && (authType() == SshConnectionParameters::AuthenticationByKey
                || (authType() == SshConnectionParameters::AuthenticationByPassword
                    && !password().isEmpty()));
}

QString MerDeviceConfigWizardGeneralPage::configName() const
{
    return IDevice::Hardware == m_wizardData.machineType ? m_ui->deviceNameLineEdit->text().trimmed()
                                                         : m_ui->virtualMachineComboBox->currentText();
}

QString MerDeviceConfigWizardGeneralPage::hostName() const
{
    return m_ui->hostNameLineEdit->text().trimmed();
}

QString MerDeviceConfigWizardGeneralPage::userName() const
{
    return m_ui->usernameLineEdit->text().trimmed();
}

QString MerDeviceConfigWizardGeneralPage::password() const
{
    return m_ui->passwordLineEdit->text().trimmed();
}

QString MerDeviceConfigWizardGeneralPage::freePorts() const
{
    return m_ui->freePortsLineEdit->text().trimmed();
}

SshConnectionParameters::AuthenticationType MerDeviceConfigWizardGeneralPage::authType() const
{
    return m_ui->passwordRadioButton->isChecked()
            ? SshConnectionParameters::AuthenticationByPassword
            : SshConnectionParameters::AuthenticationByKey;
}

IDevice::MachineType MerDeviceConfigWizardGeneralPage::machineType() const
{
    return m_ui->virtualMachineRadioButton->isChecked()
            ? IDevice::Emulator
            : IDevice::Hardware;
}


int MerDeviceConfigWizardGeneralPage::sshPort() const
{
    return m_ui->sshPortSpinBox->value();
}

void MerDeviceConfigWizardGeneralPage::authTypeChanged()
{
    const bool enable = authType() == SshConnectionParameters::AuthenticationByPassword;
    m_ui->passwordLineEdit->setEnabled(enable);
}

void MerDeviceConfigWizardGeneralPage::machineTypeChanged()
{
    const bool hardware = machineType() == IDevice::Hardware;
    m_ui->deviceNameLineEdit->setVisible(hardware);
    m_ui->virtualMachineComboBox->setVisible(!hardware);
    m_ui->hostNameLineEdit->setReadOnly(!hardware);
    if (!hardware) {
        const QStringList registeredVMs = MerVirtualBoxManager::fetchRegisteredVirtualMachines();
        m_ui->hostNameLineEdit->setText(QLatin1String(Constants::MER_SDK_DEFAULTHOST));
        m_ui->virtualMachineComboBox->clear();
        foreach (const QString &vm, registeredVMs)
            m_ui->virtualMachineComboBox->addItem(vm);
    } else {
        m_ui->deviceNameLineEdit->setText(tr("Mer ARM Device"));
        m_ui->hostNameLineEdit->setText(QString());
    }

    if (m_wizardData.deviceType == Constants::MER_DEVICE_TYPE_ARM) {
        m_ui->architectureLabel->setText(QLatin1String("arm"));
    }

    if (m_wizardData.deviceType == Constants::MER_DEVICE_TYPE_I486) {
        m_ui->architectureLabel->setText(QLatin1String("i486"));
    }
}

void MerDeviceConfigWizardGeneralPage::onVirtualMachineChanged(const QString &vmName)
{
    VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(vmName);
    m_ui->sshPortSpinBox->setValue(info.sshPort);
    QStringList freePorts;
    foreach (quint16 port, info.freePorts)
        freePorts << QString::number(port);
    m_ui->freePortsLineEdit->setText(freePorts.join(QLatin1String(",")));
}

MerDeviceConfigWizardPreviousKeySetupCheckPage::MerDeviceConfigWizardPreviousKeySetupCheckPage(
        QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerDeviceConfigWizardCheckPreviousKeySetupPage)
{
    m_ui->setupUi(this);
    m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
    setTitle(tr("Device Status Check"));
    QButtonGroup * const buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(m_ui->keyWasSetUpButton);
    buttonGroup->addButton(m_ui->keyWasNotSetUpButton);
    connect(buttonGroup, SIGNAL(buttonClicked(int)), SLOT(handleSelectionChanged()));
    connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
}

bool MerDeviceConfigWizardPreviousKeySetupCheckPage::isComplete() const
{
    return !keyBasedLoginWasSetup() || m_ui->privateKeyFilePathChooser->isValid();
}

void MerDeviceConfigWizardPreviousKeySetupCheckPage::initializePage()
{
    m_ui->keyWasNotSetUpButton->setChecked(true);
    m_ui->privateKeyFilePathChooser->setPath(IDevice::defaultPrivateKeyFilePath());
    handleSelectionChanged();
}

bool MerDeviceConfigWizardPreviousKeySetupCheckPage::keyBasedLoginWasSetup() const
{
    return m_ui->keyWasSetUpButton->isChecked();
}

QString MerDeviceConfigWizardPreviousKeySetupCheckPage::privateKeyFilePath() const
{
    return m_ui->privateKeyFilePathChooser->path();
}

void MerDeviceConfigWizardPreviousKeySetupCheckPage::handleSelectionChanged()
{
    m_ui->privateKeyFilePathChooser->setEnabled(keyBasedLoginWasSetup());
    emit completeChanged();
}

MerDeviceConfigWizardReuseKeysCheckPage::MerDeviceConfigWizardReuseKeysCheckPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerDeviceConfigWizardReuseKeysCheckPage)
{
    m_ui->setupUi(this);
    setTitle(tr("Existing Keys Check"));
    m_ui->publicKeyFilePathChooser->setExpectedKind(PathChooser::File);
    m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
    QButtonGroup * const buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(m_ui->reuseButton);
    buttonGroup->addButton(m_ui->dontReuseButton);
    connect(buttonGroup, SIGNAL(buttonClicked(int)), SLOT(handleSelectionChanged()));
    connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    connect(m_ui->publicKeyFilePathChooser, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
}

bool MerDeviceConfigWizardReuseKeysCheckPage::isComplete() const
{
    return (m_ui->publicKeyFilePathChooser->isValid() && m_ui->privateKeyFilePathChooser->isValid())
            || !reuseKeys();
}

void MerDeviceConfigWizardReuseKeysCheckPage::initializePage()
{
    m_ui->dontReuseButton->setChecked(true);
    m_ui->privateKeyFilePathChooser->setPath(IDevice::defaultPrivateKeyFilePath());
    m_ui->publicKeyFilePathChooser->setPath(IDevice::defaultPublicKeyFilePath());
    handleSelectionChanged();
}

bool MerDeviceConfigWizardReuseKeysCheckPage::reuseKeys() const
{
    return m_ui->reuseButton->isChecked();
}

QString MerDeviceConfigWizardReuseKeysCheckPage::privateKeyFilePath() const
{
    return m_ui->privateKeyFilePathChooser->path();
}

QString MerDeviceConfigWizardReuseKeysCheckPage::publicKeyFilePath() const
{
    return m_ui->publicKeyFilePathChooser->path();
}

void MerDeviceConfigWizardReuseKeysCheckPage::handleSelectionChanged()
{
    m_ui->privateKeyFilePathLabel->setEnabled(reuseKeys());
    m_ui->privateKeyFilePathChooser->setEnabled(reuseKeys());
    m_ui->publicKeyFilePathLabel->setEnabled(reuseKeys());
    m_ui->publicKeyFilePathChooser->setEnabled(reuseKeys());
    emit completeChanged();
}

MerDeviceConfigWizardKeyCreationPage::MerDeviceConfigWizardKeyCreationPage(
        const WizardData &wizardData, QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerDeviceConfigWizardKeyCreationPage)
    , m_wizardData(wizardData)
{
    m_ui->setupUi(this);
    setTitle(tr("Key Creation"));
    connect(m_ui->createKeysButton, SIGNAL(clicked()), SLOT(createKeys()));
    connect(m_ui->authorizeKeysPushButton, SIGNAL(clicked()), SLOT(authorizeKeys()));
    m_ui->authorizeKeysPushButton->setEnabled(false);
}

QString MerDeviceConfigWizardKeyCreationPage::privateKeyFilePath() const
{
    return m_ui->keyDirPathChooser->path() + QLatin1String("/qtc_id_rsa");
}

QString MerDeviceConfigWizardKeyCreationPage::publicKeyFilePath() const
{
    return privateKeyFilePath() + QLatin1String(".pub");
}

void MerDeviceConfigWizardKeyCreationPage::initializePage()
{
    m_isComplete = false;
    const QString &dir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
            + QLatin1String("/.ssh");
    m_ui->keyDirPathChooser->setPath(dir);
    enableInput();
}

bool MerDeviceConfigWizardKeyCreationPage::isComplete() const
{
    return m_isComplete;
}

void MerDeviceConfigWizardKeyCreationPage::createKeys()
{
    const QString &dirPath = m_ui->keyDirPathChooser->path();
    QFileInfo fi(dirPath);
    if (fi.exists() && !fi.isDir()) {
        QMessageBox::critical(this, tr("Cannot Create Keys"),
                              tr("The path you have entered is not a directory."));
        return;
    }
    if (!fi.exists() && !QDir::root().mkpath(dirPath)) {
        QMessageBox::critical(this, tr("Cannot Create Keys"),
                              tr("The directory you have entered does not exist and "
                                 "cannot be created."));
        return;
    }

    m_ui->keyDirPathChooser->setEnabled(false);
    m_ui->createKeysButton->setEnabled(false);
    m_ui->statusLabel->setText(tr("Creating keys..."));
    SshKeyGenerator keyGenerator;
    if (!keyGenerator.generateKeys(SshKeyGenerator::Rsa, SshKeyGenerator::Mixed, 2048)) {
        QMessageBox::critical(this, tr("Cannot Create Keys"),
                              tr("Key creation failed: %1").arg(keyGenerator.error()));
        enableInput();
        return;
    }

    if (!saveFile(privateKeyFilePath(), keyGenerator.privateKey())
            || !saveFile(publicKeyFilePath(), keyGenerator.publicKey())) {
        enableInput();
        return;
    }
    QFile::setPermissions(privateKeyFilePath(), QFile::ReadOwner | QFile::WriteOwner);

    m_ui->statusLabel->setText(m_ui->statusLabel->text() + tr("Done."));
    m_ui->authorizeKeysPushButton->setEnabled(true);
}

void MerDeviceConfigWizardKeyCreationPage::authorizeKeys()
{
    m_ui->authorizeKeysPushButton->setEnabled(false);
    m_ui->statusLabel->setText(tr("Authorizing keys..."));
    const QString privKeyPath = m_wizardData.privateKeyFilePath;
    const QString pubKeyPath = privKeyPath + QLatin1String(".pub");
    VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(m_wizardData.configName);
    const QString sshDirectoryPath = info.sharedSsh + QLatin1Char('/');
    const QStringList authorizedKeysPaths = QStringList()
            << sshDirectoryPath + QLatin1String("root/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER)
            << sshDirectoryPath + m_wizardData.userName
               + QLatin1String("/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER);
    QString error;
    bool success = true;
    foreach (const QString &path, authorizedKeysPaths) {
        success &= MerSdkManager::instance()->authorizePublicKey(path, pubKeyPath, error);
        //TODO: error handling
    }

    if (success)
        m_ui->statusLabel->setText(m_ui->statusLabel->text() + tr("Done."));
    m_isComplete = true;
    emit completeChanged();
}

bool MerDeviceConfigWizardKeyCreationPage::saveFile(const QString &filePath, const QByteArray &data)
{
    Utils::FileSaver saver(filePath);
    saver.write(data);
    if (!saver.finalize()) {
        QMessageBox::critical(this, tr("Could Not Save Key File"), saver.errorString());
        return false;
    }
    return true;
}

void MerDeviceConfigWizardKeyCreationPage::enableInput()
{
    m_ui->keyDirPathChooser->setEnabled(true);
    m_ui->createKeysButton->setEnabled(true);
    m_ui->statusLabel->clear();
}

MerDeviceConfigWizardFinalPage::MerDeviceConfigWizardFinalPage(const WizardData &wizardData,
                                                               QWidget *parent)
    : GenericLinuxDeviceConfigurationWizardFinalPage(parent)
    , m_wizardData(wizardData)
{
    QWidget *container = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(container);
    QPushButton * const button = new QPushButton(tr("Start Emulator"));
    hbox->addWidget(button);
    hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    connect(button, SIGNAL(clicked()), this, SLOT(startEmulator()));
    layout()->addWidget(container);
}

void MerDeviceConfigWizardFinalPage::startEmulator()
{
    /*
    SshConnectionParameters params;
    params.host = m_wizardData.hostName;
    params.userName = m_wizardData.userName;
    params.password = m_wizardData.password;
    params.privateKeyFile = m_wizardData.privateKeyFilePath;
    params.timeout = 10;
    params.authenticationType = m_wizardData.authType;
    params.port = m_wizardData.sshPort;
    */
    MerVirtualBoxManager::startVirtualMachine(m_wizardData.configName);
}

QString MerDeviceConfigWizardFinalPage::infoText() const
{
    if (m_wizardData.machineType == IDevice::Emulator)
        return tr("The new device configuration will now be created.");
    return GenericLinuxDeviceConfigurationWizardFinalPage::infoText();
}

} // Internal
} // Mer
