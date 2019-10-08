/****************************************************************************
**
** Copyright (C) 2012-2015,2017-2019 Jolla Ltd.
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
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
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
    m_sdksUpdatedConnection = connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
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
    connect(m_ui->stopVirtualMachineButton, &QPushButton::clicked,
            this, &MerOptionsWidget::onStopVirtualMachineButtonClicked);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::testConnectionButtonClicked,
            this, &MerOptionsWidget::onTestConnectionButtonClicked);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::sshTimeoutChanged,
            this, &MerOptionsWidget::onSshTimeoutChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::sshPortChanged,
            this, &MerOptionsWidget::onSshPortChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::headlessCheckBoxToggled,
            this, &MerOptionsWidget::onHeadlessCheckBoxToggled);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::srcFolderApplyButtonClicked,
            this, &MerOptionsWidget::onSrcFolderApplyButtonClicked);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::wwwPortChanged,
            this, &MerOptionsWidget::onWwwPortChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::wwwProxyChanged,
            this, &MerOptionsWidget::onWwwProxyChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::memorySizeMbChanged,
            this, &MerOptionsWidget::onMemorySizeMbChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::cpuCountChanged,
            this, &MerOptionsWidget::onCpuCountChanged);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::vdiCapacityMbChnaged,
            this, &MerOptionsWidget::onVdiCapacityMbChnaged);
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
    QProgressDialog progress(this);
    progress.setWindowModality(Qt::WindowModal);
    QPushButton *cancelButton = new QPushButton(tr("Abort"), &progress);
    cancelButton->setDisabled(true);
    progress.setCancelButton(cancelButton);
    progress.setMinimumDuration(2000);
    progress.setMinimum(0);
    progress.setMaximum(0);

    QMap<QString, MerSdk*> sdks = m_sdks;
    QList<MerSdk*> currentSdks = MerSdkManager::sdks();

    bool ok = true;

    disconnect(m_sdksUpdatedConnection);

    QList<MerSdk*> lockedDownSdks;
    ok &= lockDownConnectionsOrCancelChangesThatNeedIt(&lockedDownSdks);

    foreach (MerSdk *sdk, sdks) {
        progress.setLabelText(tr("Applying virtual machine settings: '%1'").arg(sdk->virtualMachineName()));

        if (m_sshPrivKeys.contains(sdk))
            sdk->setPrivateKeyFile(m_sshPrivKeys[sdk]);
        if (m_sshTimeout.contains(sdk))
            sdk->setTimeout(m_sshTimeout[sdk]);
        if (m_sshPort.contains(sdk)) {
            if (MerVirtualBoxManager::updateSdkSshPort(sdk->virtualMachineName(), m_sshPort[sdk])) {
                sdk->setSshPort(m_sshPort[sdk]);
            } else {
                m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());
                m_sshPort.remove(sdk);
                ok = false;
            }
        }
        if (m_headless.contains(sdk))
            sdk->setHeadless(m_headless[sdk]);
        if (m_wwwPort.contains(sdk)) {
            if (MerVirtualBoxManager::updateSdkWwwPort(sdk->virtualMachineName(), m_wwwPort[sdk])) {
                sdk->setWwwPort(m_wwwPort[sdk]);
            } else {
                m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());
                m_wwwPort.remove(sdk);
                ok = false;
            }
        }
        if (m_wwwProxy.contains(sdk))
            sdk->setWwwProxy(m_wwwProxy[sdk], m_wwwProxyServers[sdk], m_wwwProxyExcludes[sdk]);

        if (m_memorySizeMb.contains(sdk)) {
            if (MerVirtualBoxManager::setMemorySizeMb(sdk->virtualMachineName(), m_memorySizeMb[sdk])) {
                sdk->setMemorySizeMb(m_memorySizeMb[sdk]);
            } else {
                m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->memorySizeMb());
                m_memorySizeMb.remove(sdk);
                ok = false;
            }
        }
        if (m_cpuCount.contains(sdk)) {
            if (MerVirtualBoxManager::setCpuCount(sdk->virtualMachineName(), m_cpuCount[sdk])) {
                sdk->setCpuCount(m_cpuCount[sdk]);
            } else {
                m_ui->sdkDetailsWidget->setCpuCount(sdk->cpuCount());
                m_cpuCount.remove(sdk);
                ok = false;
            }
        }
        if (m_vdiCapacityMb.contains(sdk)) {
            const int newVdiCapacityMb = m_vdiCapacityMb[sdk];
            QEventLoop loop;
            MerVirtualBoxManager::setVdiCapacityMb(sdk->virtualMachineName(), newVdiCapacityMb, &loop, [&loop] (bool ok) {
                loop.exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
            });

            if (loop.exec() == EXIT_SUCCESS) {
                sdk->setVdiCapacityMb(newVdiCapacityMb);
            } else {
                m_vdiCapacityMb.remove(sdk);
                ok = false;
            }
            m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->vdiCapacityMb());
        }
    }

    foreach (MerSdk *sdk, lockedDownSdks)
        sdk->connection()->lockDown(false);

    onSdksUpdated();
    m_sdksUpdatedConnection = connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
            this, &MerOptionsWidget::onSdksUpdated);

    if (!ok) {
        progress.cancel();
        QMessageBox::warning(this, tr("Some changes could not be saved!"),
                             tr("Failed to apply some of the changes to virtual machines"));
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
    m_sshPort.clear();
    m_headless.clear();
    m_wwwPort.clear();
    m_memorySizeMb.clear();
    m_cpuCount.clear();
    m_vdiCapacityMb.clear();
}

bool MerOptionsWidget::lockDownConnectionsOrCancelChangesThatNeedIt(QList<MerSdk *> *lockedDownSdks)
{
    QTC_ASSERT(lockedDownSdks, return false);

    QList<MerSdk *> failed;

    for (MerSdk *sdk : qAsConst(m_sdks)) {
        if (m_sshPort.value(sdk) == sdk->sshPort())
            m_sshPort.remove(sdk);
        if (m_wwwPort.value(sdk) == sdk->wwwPort())
            m_wwwPort.remove(sdk);
        if (m_memorySizeMb.value(sdk) == sdk->memorySizeMb())
            m_memorySizeMb.remove(sdk);
        if (m_cpuCount.value(sdk) == sdk->cpuCount())
            m_cpuCount.remove(sdk);
        if (m_vdiCapacityMb.value(sdk) == sdk->vdiCapacityMb())
            m_vdiCapacityMb.remove(sdk);

        if (!m_sshPort.contains(sdk)
                && !m_wwwPort.contains(sdk)
                && !m_memorySizeMb.contains(sdk)
                && !m_cpuCount.contains(sdk)
                && !m_vdiCapacityMb.contains(sdk)) {
            continue;
        }

        if (!sdk->connection()->isVirtualMachineOff()) {
            QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("Close the \"%1\" virtual machine?").arg(m_virtualMachine),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
            questionBox->setInformativeText(
                    tr("Some of the changes require stopping the virtual machine before they can be applied."));
            if (questionBox->exec() != QMessageBox::Yes) {
                failed.append(sdk);
                continue;
            }
        }

        if (!sdk->connection()->lockDown(true)) {
            failed.append(sdk);
            continue;
        }

        lockedDownSdks->append(sdk);
    }

    for (MerSdk *sdk : qAsConst(failed)) {
        m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());
        m_sshPort.remove(sdk);
        m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());
        m_wwwPort.remove(sdk);
        m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->memorySizeMb());
        m_memorySizeMb.remove(sdk);
        m_ui->sdkDetailsWidget->setCpuCount(sdk->cpuCount());
        m_cpuCount.remove(sdk);
        m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->vdiCapacityMb());
        m_vdiCapacityMb.remove(sdk);
    }

    return failed.isEmpty();
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
    dialog.setWindowTitle(tr("Add a Sailfish OS Build Engine"));
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

         m_sshPrivKeys.remove(removed);
         m_sshTimeout.remove(removed);
         m_sshPort.remove(removed);
         m_headless.remove(removed);
         m_wwwPort.remove(removed);
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
        if (m_sshPort.contains(sdk))
            params.setPort(m_sshPort[sdk]);
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

void MerOptionsWidget::onStopVirtualMachineButtonClicked()
{
    const MerSdk *sdk = m_sdks[m_virtualMachine];
    sdk->connection()->disconnectFrom();
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

    disconnect(m_vmOffConnection);

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

        if (m_sshPort.contains(sdk))
            m_ui->sdkDetailsWidget->setSshPort(m_sshPort[sdk]);
        else
            m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());

        if (m_headless.contains(sdk))
            m_ui->sdkDetailsWidget->setHeadless(m_headless[sdk]);
        else
            m_ui->sdkDetailsWidget->setHeadless(sdk->isHeadless());

        if (m_wwwPort.contains(sdk))
            m_ui->sdkDetailsWidget->setWwwPort(m_wwwPort[sdk]);
        else
            m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());

        if (m_wwwProxy.contains(sdk))
            m_ui->sdkDetailsWidget->setWwwProxy(m_wwwProxy[sdk], m_wwwProxyServers[sdk], m_wwwProxyExcludes[sdk]);
        else
            m_ui->sdkDetailsWidget->setWwwProxy(sdk->wwwProxy(), sdk->wwwProxyServers(), sdk->wwwProxyExcludes());

        if (m_memorySizeMb.contains(sdk))
            m_ui->sdkDetailsWidget->setMemorySizeMb(m_memorySizeMb[sdk]);
        else
            m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->memorySizeMb());

        if (m_cpuCount.contains(sdk))
            m_ui->sdkDetailsWidget->setCpuCount(m_cpuCount[sdk]);
        else
            m_ui->sdkDetailsWidget->setCpuCount(sdk->cpuCount());

        if (m_vdiCapacityMb.contains(sdk))
            m_ui->sdkDetailsWidget->setVdiCapacityMb(m_vdiCapacityMb[sdk]);
        else
            m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->vdiCapacityMb());

        onVmOffChanged(sdk->connection()->isVirtualMachineOff());
        m_vmOffConnection = connect(sdk->connection(), &MerConnection::virtualMachineOffChanged,
                this, &MerOptionsWidget::onVmOffChanged);

        int index = m_ui->sdkComboBox->findText(m_virtualMachine);
        m_ui->sdkComboBox->setCurrentIndex(index);
        m_ui->sdkDetailsWidget->setStatus(m_status);
    }

    m_ui->sdkDetailsWidget->setVisible(show);
    m_ui->removeButton->setEnabled(show);
    m_ui->startVirtualMachineButton->setEnabled(show);
    m_ui->stopVirtualMachineButton->setEnabled(show);
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

void MerOptionsWidget::onSshPortChanged(quint16 port)
{
    //store keys to be saved on save click
    m_sshPort[m_sdks[m_virtualMachine]] = port;
}

void MerOptionsWidget::onHeadlessCheckBoxToggled(bool checked)
{
    //store keys to be saved on save click
    m_headless[m_sdks[m_virtualMachine]] = checked;
}

void MerOptionsWidget::onWwwPortChanged(quint16 port)
{
    //store keys to be saved on save click
    m_wwwPort[m_sdks[m_virtualMachine]] = port;
}

void MerOptionsWidget::onWwwProxyChanged(const QString &type, const QString &servers, const QString &excludes)
{
    //store keys to be saved on save click
    m_wwwProxy[m_sdks[m_virtualMachine]] = type;
    m_wwwProxyServers[m_sdks[m_virtualMachine]] = servers;
    m_wwwProxyExcludes[m_sdks[m_virtualMachine]] = excludes;
}

void MerOptionsWidget::onMemorySizeMbChanged(int sizeMb)
{
    m_memorySizeMb[m_sdks[m_virtualMachine]] = sizeMb;
}

void MerOptionsWidget::onCpuCountChanged(int count)
{
    m_cpuCount[m_sdks[m_virtualMachine]] = count;
}

void MerOptionsWidget::onVdiCapacityMbChnaged(int sizeMb)
{
    m_vdiCapacityMb[m_sdks[m_virtualMachine]] = sizeMb;
}

void MerOptionsWidget::onVmOffChanged(bool vmOff)
{
    MerSdk *sdk = m_sdks[m_virtualMachine];

    // If the VM is started, cancel any unsaved changes to SSH/WWW ports to prevent inconsistencies
    if (!vmOff) {
        m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());
        m_sshPort.remove(sdk);
        m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());
        m_wwwPort.remove(sdk);
        m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->memorySizeMb());
        m_memorySizeMb.remove(sdk);
        m_ui->sdkDetailsWidget->setCpuCount(sdk->cpuCount());
        m_cpuCount.remove(sdk);
        m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->vdiCapacityMb());
        m_vdiCapacityMb.remove(sdk);
    }

    m_ui->sdkDetailsWidget->setVmOffStatus(vmOff);
}

} // Internal
} // Mer
