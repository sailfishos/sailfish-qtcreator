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
#include "meroptionswidget.h"
#include "ui_meroptionswidget.h"
#include "mersdkdetailswidget.h"
#include "mersdkmanager.h"
#include "mersdkselectiondialog.h"
#include "mervirtualboxmanager.h"
#include "merconnectionmanager.h"

#include <utils/fileutils.h>
#include <ssh/sshconnection.h>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL[] = "http://127.0.0.1:8080/";

MerOptionsWidget::MerOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerOptionsWidget)
    , m_status(tr("Not connected."))
{
    m_ui->setupUi(this);
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), SLOT(onSdksUpdated()));
    connect(m_ui->sdkDetailsWidget, SIGNAL(authorizeSshKey(QString)), SLOT(onAuthorizeSshKey(QString)));
    connect(m_ui->sdkDetailsWidget, SIGNAL(generateSshKey(QString)), SLOT(onGenerateSshKey(QString)));
    connect(m_ui->sdkDetailsWidget, SIGNAL(sshKeyChanged(QString)), SLOT(onSshKeyChanged(QString)));
    connect(m_ui->sdkComboBox, SIGNAL(activated(QString)), SLOT(onSdkChanged(QString)));
    connect(m_ui->addButton, SIGNAL(clicked()), SLOT(onAddButtonClicked()));
    connect(m_ui->removeButton, SIGNAL(clicked()), SLOT(onRemoveButtonClicked()));
    connect(m_ui->startVirtualMachineButton, SIGNAL(clicked()), SLOT(onStartVirtualMachineButtonClicked()));
    connect(m_ui->sdkDetailsWidget, SIGNAL(testConnectionButtonClicked()), SLOT(onTestConnectionButtonClicked()));
    connect(m_ui->sdkDetailsWidget, SIGNAL(headlessCheckBoxToggled(bool)), SLOT(onHeadlessCheckBoxToggled(bool)));
    connect(m_ui->sdkDetailsWidget, SIGNAL(srcFolderApplyButtonClicked(QString)), SLOT(onSrcFolderApplyButtonClicked(QString)));
    onSdksUpdated();
}

MerOptionsWidget::~MerOptionsWidget()
{
    delete m_ui;
}

void MerOptionsWidget::setSdk(const QString &vmName)
{
    if (m_sdks.contains(vmName))
        onSdkChanged(vmName);
}

QString MerOptionsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    QString keys;
    int count = m_ui->sdkComboBox->count();
    for (int i = 0; i < count; ++i)
        keys += m_ui->sdkComboBox->itemText(i) + blank;
    if (m_ui->sdkDetailsWidget->isVisible())
        keys += m_ui->sdkDetailsWidget->searchKeyWordMatchString();

    return keys.trimmed();
}

void MerOptionsWidget::store()
{
    QMap<QString, MerSdk*> sdks = m_sdks;
    MerSdkManager *sdkManager = MerSdkManager::instance();
    QList<MerSdk*> currentSdks = sdkManager->sdks();

    foreach (MerSdk *sdk, sdks) {
        if (m_sshPrivKeys.contains(sdk))
            sdk->setPrivateKeyFile(m_sshPrivKeys[sdk]);
        if (m_headless.contains(sdk))
            sdk->setHeadless(m_headless[sdk]);
    }

    foreach (MerSdk *sdk, currentSdks) {
        if (!sdks.contains(sdk->virtualMachineName())) {
            sdkManager->removeSdk(sdk);
            delete sdk;
        } else {
            sdks.remove(sdk->virtualMachineName());
        }
    }

    foreach (MerSdk *sdk,sdks)
        sdkManager->addSdk(sdk);

    m_sshPrivKeys.clear();
    m_headless.clear();
}

void MerOptionsWidget::onSdkChanged(const QString &sdkName)
{
   if (m_virtualMachine != sdkName) {
       m_virtualMachine = sdkName;
       m_status = tr("Not tested.");
       update();
   }
}

void MerOptionsWidget::onAddButtonClicked()
{
    MerSdkSelectionDialog dialog(this);
    dialog.setWindowTitle(tr("Add Mer SDK"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (m_sdks.contains(dialog.selectedSdkName()))
        return;

    MerSdk* sdk = MerSdkManager::instance()->createSdk(dialog.selectedSdkName());
    m_sdks[sdk->virtualMachineName()] = sdk;
    m_virtualMachine = sdk->virtualMachineName();
    update();
}

void MerOptionsWidget::onRemoveButtonClicked()
{
    const QString &vmName = m_ui->sdkComboBox->itemData(m_ui->sdkComboBox->currentIndex(), Qt::DisplayRole).toString();
    if (vmName.isEmpty())
        return;

    if (m_sdks.contains(vmName)) {
         m_sdks.remove(vmName);
         if (!m_sdks.isEmpty())
             m_virtualMachine = m_sdks.keys().last();
         else
             m_virtualMachine.clear();
    }
    update();
}

void MerOptionsWidget::onTestConnectionButtonClicked()
{
    MerSdk *sdk = m_sdks[m_virtualMachine];
    if (MerVirtualBoxManager::isVirtualMachineRunning(sdk->virtualMachineName())) {
        QSsh::SshConnectionParameters params = MerConnectionManager::parameters(sdk);
        if (m_sshPrivKeys.contains(sdk))
            params.privateKeyFile = m_sshPrivKeys[sdk];
        else
            params.privateKeyFile = sdk->privateKeyFile();
        m_ui->sdkDetailsWidget->setStatus(tr("Connecting to machine %1 ...").arg(sdk->virtualMachineName()));
        m_ui->sdkDetailsWidget->setTestButtonEnabled(false);
        m_status = MerConnectionManager::instance()->testConnection(params);
        m_ui->sdkDetailsWidget->setTestButtonEnabled(true);
        update();
    } else {
        m_ui->sdkDetailsWidget->setStatus(tr("Virtual machine %1 is not running.").arg(sdk->virtualMachineName()));
    }
}

void MerOptionsWidget::onAuthorizeSshKey(const QString &file)
{
    MerSdk *sdk = m_sdks[m_virtualMachine];
    const QString pubKeyPath = file + QLatin1String(".pub");
    const QString sshDirectoryPath = sdk->sharedSshPath() + QLatin1Char('/');
    const QStringList authorizedKeysPaths = QStringList()
            << sshDirectoryPath + QLatin1String("root/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER)
            << sshDirectoryPath + sdk->userName()
               + QLatin1String("/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER);
    foreach (const QString &path, authorizedKeysPaths) {
        QString error;
        const bool success = MerSdkManager::instance()->authorizePublicKey(path, pubKeyPath, error);
        if (!success)
            QMessageBox::critical(this, tr("Cannot Authorize Keys"), error);
        else
            QMessageBox::information(this, tr("Key Authorized "), tr("Key %1 added to \n %2").arg(pubKeyPath).arg(path));
    }
}

void MerOptionsWidget::onStartVirtualMachineButtonClicked()
{
    const MerSdk *sdk = m_sdks[m_virtualMachine];
    MerVirtualBoxManager::startVirtualMachine(sdk->virtualMachineName(), sdk->isHeadless());
}

void MerOptionsWidget::onGenerateSshKey(const QString &privKeyPath)
{
    QString error;
    if (!MerSdkManager::generateSshKey(privKeyPath, error)) {
       QMessageBox::critical(this, tr("Could not generate key."), error);
    } else {
       QMessageBox::information(this, tr("Key generated"), tr("Key pair generated \n %1 \n You should authorzie key now.").arg(privKeyPath));
       m_ui->sdkDetailsWidget->setPrivateKeyFile(privKeyPath);
    }
}

void MerOptionsWidget::onSdksUpdated()
{
    MerSdkManager *sdkManager = MerSdkManager::instance();
    m_sdks.clear();
    m_virtualMachine.clear();
    foreach (MerSdk *sdk, sdkManager->sdks()) {
        m_sdks[sdk->virtualMachineName()] = sdk;
        m_virtualMachine = sdk->virtualMachineName();
    }
    update();
}

void MerOptionsWidget::onSrcFolderApplyButtonClicked(const QString &newFolder)
{
    MerSdk *sdk = m_sdks[m_virtualMachine];

    if (newFolder == sdk->sharedSrcPath()) {
        QMessageBox::information(0, tr("Choose a new folder"),
                                 tr("The given folder (%1) is the current alternative source folder. "
                                    "Please choose another folder if you want to change it.").arg(sdk->sharedSrcPath()));
        return;
    }

    if (MerVirtualBoxManager::isVirtualMachineRunning(m_virtualMachine)) {
        QMessageBox::information(0, tr("Stop Virtual Machine"),
                                 tr("Virtual Machine %1 is running. "
                                    "It must be stopped before the source folder can be changed.").arg(m_virtualMachine));
    }
    else if (MerVirtualBoxManager::updateSharedFolder(m_virtualMachine, QLatin1String("src1"), newFolder)) {
        // remember to update this value
        sdk->setSharedSrcPath(newFolder);

        const QMessageBox::StandardButton response =
            QMessageBox::question(0, tr("Success!"),
                                  tr("Alternative source folder for %1 changed to %2.\n\n"
                                     "Do you want to start %1 now?").arg(m_virtualMachine).arg(newFolder),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
        if (response == QMessageBox::Yes)
            MerVirtualBoxManager::startVirtualMachine(m_virtualMachine, sdk->isHeadless());
    }
    else {
        QMessageBox::warning(0, tr("Changing the source folder failed!"),
                             tr("Unable to change the alternative source folder to %1").arg(newFolder));
    }

    // update the path in the chooser
    m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath());
}

void MerOptionsWidget::update()
{
    m_ui->sdkComboBox->clear();
    m_ui->sdkComboBox->addItems(m_sdks.keys());
    bool show = !m_virtualMachine.isEmpty();

    if (show && m_sdks.contains(m_virtualMachine)) {
        MerSdk *sdk = m_sdks[m_virtualMachine];
        m_ui->sdkDetailsWidget->setSdk(sdk);
        if (m_sshPrivKeys.contains(sdk))
            m_ui->sdkDetailsWidget->setPrivateKeyFile(m_sshPrivKeys[sdk]);
        else
            m_ui->sdkDetailsWidget->setPrivateKeyFile(sdk->privateKeyFile());

        if (m_headless.contains(sdk))
            m_ui->sdkDetailsWidget->setHeadless(m_headless[sdk]);
        else
            m_ui->sdkDetailsWidget->setHeadless(sdk->isHeadless());

        int index = m_ui->sdkComboBox->findText(m_virtualMachine);
        m_ui->sdkComboBox->setCurrentIndex(index);
        m_ui->sdkDetailsWidget->setStatus(m_status);
    }

    m_ui->sdkDetailsWidget->setVisible(show);
    m_ui->removeButton->setEnabled(show);
    m_ui->startVirtualMachineButton->setEnabled(show);
}

void MerOptionsWidget::onSshKeyChanged(const QString &file)
{
    //store keys to be saved on save click
    m_sshPrivKeys[m_sdks[m_virtualMachine]] = file;
}

void MerOptionsWidget::onHeadlessCheckBoxToggled(bool checked)
{
    //store keys to be saved on save click
    m_headless[m_sdks[m_virtualMachine]] = checked;
}

} // Internal
} // Mer
