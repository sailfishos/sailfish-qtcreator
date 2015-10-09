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

#include "meroptionswidget.h"

#include "merconnection.h"
#include "merconnectionmanager.h"
#include "merconstants.h"
#include "mersdkdetailswidget.h"
#include "mersdkmanager.h"
#include "mersdkselectiondialog.h"
#include "mervirtualboxmanager.h"
#include "ui_meroptionswidget.h"

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QUrl>

using namespace Core;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL[] = "http://127.0.0.1:8080/";

MerOptionsWidget::MerOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerOptionsWidget)
    , m_status(tr("Not connected."))
{
    m_ui->setupUi(this);
    connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
            this, &MerOptionsWidget::onSdksUpdated);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::authorizeSshKey,
            this, &MerOptionsWidget::onAuthorizeSshKey);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::generateSshKey,
            this, &MerOptionsWidget::onGenerateSshKey);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::sshKeyChanged,
            this, &MerOptionsWidget::onSshKeyChanged);
    connect(m_ui->sdkComboBox, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
            this, &MerOptionsWidget::onSdkChanged);
    connect(m_ui->addButton, &QPushButton::clicked,
            this, &MerOptionsWidget::onAddButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &MerOptionsWidget::onRemoveButtonClicked);
    connect(m_ui->startVirtualMachineButton, &QPushButton::clicked,
            this, &MerOptionsWidget::onStartVirtualMachineButtonClicked);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::testConnectionButtonClicked,
            this, &MerOptionsWidget::onTestConnectionButtonClicked);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::sshTimeoutChanged,
            this, &MerOptionsWidget::onSshTimeoutChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::headlessCheckBoxToggled,
            this, &MerOptionsWidget::onHeadlessCheckBoxToggled);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::srcFolderApplyButtonClicked,
            this, &MerOptionsWidget::onSrcFolderApplyButtonClicked);
    onSdksUpdated();
}

MerOptionsWidget::~MerOptionsWidget()
{
    delete m_ui;

    // Destroy newly created but not-applied SDKs
    QSet<MerSdk *> currentSdks = MerSdkManager::sdks().toSet();
    foreach (MerSdk *sdk, m_sdks) {
        if (!currentSdks.contains(sdk)) {
            delete sdk;
        }
    }
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
    QList<MerSdk*> currentSdks = MerSdkManager::sdks();

    foreach (MerSdk *sdk, sdks) {
        if (m_sshPrivKeys.contains(sdk))
            sdk->setPrivateKeyFile(m_sshPrivKeys[sdk]);
        if (m_sshTimeout.contains(sdk))
            sdk->setTimeout(m_sshTimeout[sdk]);
        if (m_headless.contains(sdk))
            sdk->setHeadless(m_headless[sdk]);
    }

    foreach (MerSdk *sdk, currentSdks) {
        if (!sdks.contains(sdk->virtualMachineName())) {
            MerSdkManager::removeSdk(sdk);
            delete sdk;
        } else {
            sdks.remove(sdk->virtualMachineName());
        }
    }

    foreach (MerSdk *sdk,sdks)
        MerSdkManager::addSdk(sdk);

    m_sshPrivKeys.clear();
    m_sshTimeout.clear();
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

    MerSdk* sdk = MerSdkManager::createSdk(dialog.selectedSdkName());
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
         MerSdk *removed = m_sdks.take(vmName);
         QList<MerSdk *> currentSdks = MerSdkManager::sdks();
         if (!currentSdks.contains(removed))
             delete removed;
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
    if (!sdk->connection()->isVirtualMachineOff()) {
        SshConnectionParameters params = sdk->connection()->sshParameters();
        if (m_sshPrivKeys.contains(sdk))
            params.privateKeyFile = m_sshPrivKeys[sdk];
        m_ui->sdkDetailsWidget->setStatus(tr("Connecting to machine %1 ...").arg(sdk->virtualMachineName()));
        m_ui->sdkDetailsWidget->setTestButtonEnabled(false);
        m_status = MerConnectionManager::testConnection(params);
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
               + QLatin1Char('/') + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER);
    foreach (const QString &path, authorizedKeysPaths) {
        QString error;
        const bool success = MerSdkManager::authorizePublicKey(path, pubKeyPath, error);
        if (!success)
            QMessageBox::critical(this, tr("Cannot Authorize Keys"), error);
        else
            QMessageBox::information(this, tr("Key Authorized "),
                    tr("Key %1 added to \n %2").arg(pubKeyPath).arg(path));
    }
}

void MerOptionsWidget::onStartVirtualMachineButtonClicked()
{
    const MerSdk *sdk = m_sdks[m_virtualMachine];
    sdk->connection()->connectTo();
}

void MerOptionsWidget::onGenerateSshKey(const QString &privKeyPath)
{
    QString error;
    if (!MerSdkManager::generateSshKey(privKeyPath, error)) {
       QMessageBox::critical(this, tr("Could not generate key."), error);
    } else {
       QMessageBox::information(this, tr("Key generated"),
               tr("Key pair generated \n %1 \n You should authorize key now.").arg(privKeyPath));
       m_ui->sdkDetailsWidget->setPrivateKeyFile(privKeyPath);
    }
}

void MerOptionsWidget::onSdksUpdated()
{
    m_sdks.clear();
    m_virtualMachine.clear();
    foreach (MerSdk *sdk, MerSdkManager::sdks()) {
        m_sdks[sdk->virtualMachineName()] = sdk;
        m_virtualMachine = sdk->virtualMachineName();
    }
    update();
}

void MerOptionsWidget::onSrcFolderApplyButtonClicked(const QString &newFolder)
{
    MerSdk *sdk = m_sdks[m_virtualMachine];

    if (newFolder == sdk->sharedSrcPath()) {
        QMessageBox::information(this, tr("Choose a new folder"),
                                 tr("The given folder (%1) is the current alternative source folder. "
                                    "Please choose another folder if you want to change it.").arg(sdk->sharedSrcPath()));
        return;
    }

    if (!sdk->connection()->isVirtualMachineOff()) {
        QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                tr("Close Virtual Machine"),
                tr("Close the \"%1\" virtual machine?").arg(m_virtualMachine),
                QMessageBox::Yes | QMessageBox::No,
                this);
        questionBox->setInformativeText(
                tr("Virtual machine must be closed before the source folder can be changed."));
        if (questionBox->exec() != QMessageBox::Yes) {
            // reset the path in the chooser
            m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath());
            return;
        }
    }

    if (!sdk->connection()->lockDown(true)) {
        QMessageBox::warning(this, tr("Failed"),
                tr("Alternative source folder not changed"));
        // reset the path in the chooser
        m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath());
        return;
    }

    bool ok = MerVirtualBoxManager::updateSharedFolder(m_virtualMachine,
            QLatin1String("src1"), newFolder);

    sdk->connection()->lockDown(false);

    if (ok) {
        // remember to update this value
        sdk->setSharedSrcPath(newFolder);

        const QMessageBox::StandardButton response =
            QMessageBox::question(this, tr("Success!"),
                                  tr("Alternative source folder for %1 changed to %2.\n\n"
                                     "Do you want to start %1 now?").arg(m_virtualMachine).arg(newFolder),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
        if (response == QMessageBox::Yes)
            sdk->connection()->connectTo();
    }
    else {
        QMessageBox::warning(this, tr("Changing the source folder failed!"),
                             tr("Unable to change the alternative source folder to %1").arg(newFolder));
        // reset the path in the chooser
        m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath());
    }
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

        if (m_sshTimeout.contains(sdk))
            m_ui->sdkDetailsWidget->setSshTimeout(m_sshTimeout[sdk]);
        else
            m_ui->sdkDetailsWidget->setSshTimeout(sdk->timeout());

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

void MerOptionsWidget::onSshTimeoutChanged(int timeout)
{
    //store keys to be saved on save click
    m_sshTimeout[m_sdks[m_virtualMachine]] = timeout;
}

void MerOptionsWidget::onHeadlessCheckBoxToggled(bool checked)
{
    //store keys to be saved on save click
    m_headless[m_sdks[m_virtualMachine]] = checked;
}

} // Internal
} // Mer
