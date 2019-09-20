/****************************************************************************
**
** Copyright (C) 2012-2015,2017-2019 Jolla Ltd.
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

#include "meroptionswidget.h"

#include "merconnectionmanager.h"
#include "merconstants.h"
#include "mersdkdetailswidget.h"
#include "mersdkmanager.h"
#include "mervmselectiondialog.h"
#include "ui_meroptionswidget.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QUrl>

using namespace Core;
using namespace QSsh;
using namespace Sfdk;
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
    connect(Sdk::instance(), &Sdk::buildEngineAdded,
            this, &MerOptionsWidget::onBuildEngineAdded);
    connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
            this, &MerOptionsWidget::onAboutToRemoveBuildEngine);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::authorizeSshKey,
            this, &MerOptionsWidget::onAuthorizeSshKey);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::generateSshKey,
            this, &MerOptionsWidget::onGenerateSshKey);
    connect(m_ui->sdkDetailsWidget, &MerSdkDetailsWidget::sshKeyChanged,
            this, &MerOptionsWidget::onSshKeyChanged);
    connect(m_ui->sdkComboBox, QOverload<int>::of(&QComboBox::activated),
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
    for (int i = 0; i < Sdk::buildEngines().count(); ++i)
        onBuildEngineAdded(i);
    update();
}

MerOptionsWidget::~MerOptionsWidget()
{
    delete m_ui;
}

void MerOptionsWidget::setSdk(const QUrl &uri)
{
    QTC_ASSERT(uri.isValid(), return);
    QTC_ASSERT(m_sdks.contains(uri), return);

    if (m_virtualMachine == uri)
        return;

    m_virtualMachine = uri;
    m_status = tr("Not tested.");
    update();
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

    bool ok = true;

    QList<BuildEngine *> lockedDownSdks;
    ok &= lockDownConnectionsOrCancelChangesThatNeedIt(&lockedDownSdks);

    for (BuildEngine *const sdk : qAsConst(m_sdks)) {
        progress.setLabelText(tr("Applying build engine settings: '%1'").arg(sdk->name()));

        if (m_sshPrivKeys.contains(sdk)) {
            SshConnectionParameters sshParameters = sdk->virtualMachine()->sshParameters();
            sshParameters.privateKeyFile = m_sshPrivKeys[sdk];
            sdk->virtualMachine()->setSshParameters(sshParameters);
        }
        if (m_sshTimeout.contains(sdk)) {
            SshConnectionParameters sshParameters = sdk->virtualMachine()->sshParameters();
            sshParameters.timeout = m_sshTimeout[sdk];
            sdk->virtualMachine()->setSshParameters(sshParameters);
        }
        if (m_sshPort.contains(sdk)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&BuildEngine::setSshPort), sdk,
                    m_sshPort[sdk]);
            if (!stepOk) {
                m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());
                m_sshPort.remove(sdk);
                ok = false;
            }
        }
        if (m_headless.contains(sdk))
            sdk->virtualMachine()->setHeadless(m_headless[sdk]);
        if (m_wwwPort.contains(sdk)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&BuildEngine::setWwwPort), sdk,
                    m_wwwPort[sdk]);
            if (!stepOk) {
                m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());
                m_wwwPort.remove(sdk);
                ok = false;
            }
        }
        if (m_wwwProxy.contains(sdk))
            sdk->setWwwProxy(m_wwwProxy[sdk], m_wwwProxyServers[sdk], m_wwwProxyExcludes[sdk]);

        if (m_memorySizeMb.contains(sdk)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setMemorySizeMb),
                    sdk->virtualMachine(), m_memorySizeMb[sdk]);
            if (!stepOk) {
                m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->virtualMachine()->memorySizeMb());
                m_memorySizeMb.remove(sdk);
                ok = false;
            }
        }
        if (m_cpuCount.contains(sdk)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setCpuCount),
                    sdk->virtualMachine(), m_cpuCount[sdk]);
            if (!stepOk) {
                m_ui->sdkDetailsWidget->setCpuCount(sdk->virtualMachine()->cpuCount());
                m_cpuCount.remove(sdk);
                ok = false;
            }
        }
        if (m_vdiCapacityMb.contains(sdk)) {
            const int newVdiCapacityMb = m_vdiCapacityMb[sdk];
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setVdiCapacityMb),
                    sdk->virtualMachine(), newVdiCapacityMb);
            if (!stepOk) {
                m_vdiCapacityMb.remove(sdk);
                ok = false;
            }
            m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->virtualMachine()->vdiCapacityMb());
        }
    }

    for (BuildEngine *const sdk : lockedDownSdks)
        sdk->virtualMachine()->lockDown(false);

    if (!ok) {
        progress.cancel();
        QMessageBox::warning(this, tr("Some changes could not be saved!"),
                             tr("Failed to apply some of the changes to build engines"));
    }

    for (BuildEngine *const engine : Sdk::buildEngines()) {
        if (!m_sdks.contains(engine->uri()))
            Sdk::removeBuildEngine(engine->uri());
    }
    for (std::unique_ptr<BuildEngine> &newSdk : m_newSdks)
        Sdk::addBuildEngine(std::move(newSdk));
    m_newSdks.clear();

    m_sshPrivKeys.clear();
    m_sshTimeout.clear();
    m_sshPort.clear();
    m_headless.clear();
    m_wwwPort.clear();
    m_memorySizeMb.clear();
    m_cpuCount.clear();
    m_vdiCapacityMb.clear();
}

bool MerOptionsWidget::lockDownConnectionsOrCancelChangesThatNeedIt(QList<BuildEngine *>
        *lockedDownSdks)
{
    QTC_ASSERT(lockedDownSdks, return false);

    QList<BuildEngine *> failed;

    for (BuildEngine *const sdk : qAsConst(m_sdks)) {
        if (m_sshPort.value(sdk) == sdk->sshPort())
            m_sshPort.remove(sdk);
        if (m_wwwPort.value(sdk) == sdk->wwwPort())
            m_wwwPort.remove(sdk);
        if (m_memorySizeMb.value(sdk) == sdk->virtualMachine()->memorySizeMb())
            m_memorySizeMb.remove(sdk);
        if (m_cpuCount.value(sdk) == sdk->virtualMachine()->cpuCount())
            m_cpuCount.remove(sdk);
        if (m_vdiCapacityMb.value(sdk) == sdk->virtualMachine()->vdiCapacityMb())
            m_vdiCapacityMb.remove(sdk);

        if (!m_sshPort.contains(sdk)
                && !m_wwwPort.contains(sdk)
                && !m_memorySizeMb.contains(sdk)
                && !m_cpuCount.contains(sdk)
                && !m_vdiCapacityMb.contains(sdk)) {
            continue;
        }

        if (!sdk->virtualMachine()->isOff()) {
            QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("Close the \"%1\" virtual machine?").arg(sdk->virtualMachine()->name()),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
            questionBox->setInformativeText(
                    tr("Some of the changes require stopping the virtual machine before they can be applied."));
            if (questionBox->exec() != QMessageBox::Yes) {
                failed.append(sdk);
                continue;
            }
        }

        if (!sdk->virtualMachine()->lockDown(true)) {
            failed.append(sdk);
            continue;
        }

        lockedDownSdks->append(sdk);
    }

    for (BuildEngine *const sdk : qAsConst(failed)) {
        m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());
        m_sshPort.remove(sdk);
        m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());
        m_wwwPort.remove(sdk);
        m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->virtualMachine()->memorySizeMb());
        m_memorySizeMb.remove(sdk);
        m_ui->sdkDetailsWidget->setCpuCount(sdk->virtualMachine()->cpuCount());
        m_cpuCount.remove(sdk);
        m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->virtualMachine()->vdiCapacityMb());
        m_vdiCapacityMb.remove(sdk);
    }

    return failed.isEmpty();
}

void MerOptionsWidget::onSdkChanged(int index)
{
    setSdk(m_ui->sdkComboBox->itemData(index).toUrl());
}

void MerOptionsWidget::onAddButtonClicked()
{
    MerVmSelectionDialog dialog(this);
    dialog.setWindowTitle(tr("Add a Sailfish OS Build Engine"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (m_sdks.contains(dialog.selectedVmUri()))
        return;

    std::unique_ptr<BuildEngine> sdk;
    // FIXME overload execAsynchronous to support this
    QEventLoop loop;
    auto whenDone = [&loop, &sdk](std::unique_ptr<BuildEngine> &&newSdk) {
        loop.quit();
        sdk = std::move(newSdk);
    };
    Sdk::createBuildEngine(dialog.selectedVmUri(), &loop, whenDone);
    loop.exec();
    QTC_ASSERT(sdk, return);

    m_sdks[sdk->virtualMachine()->uri()] = sdk.get();
    m_virtualMachine = sdk->virtualMachine()->uri();
    m_newSdks.emplace_back(std::move(sdk));
    update();
}

void MerOptionsWidget::onRemoveButtonClicked()
{
    const QUrl vmUri = m_ui->sdkComboBox->itemData(m_ui->sdkComboBox->currentIndex(),
            Qt::UserRole).toUrl();
    QTC_ASSERT(vmUri.isValid(), return);

    if (m_sdks.contains(vmUri)) {
         BuildEngine *const removed = m_sdks.take(vmUri);
         Utils::erase(m_newSdks, removed);
         if (!m_sdks.isEmpty())
             m_virtualMachine = m_sdks.keys().last();
         else
             m_virtualMachine.clear();

         m_sshPrivKeys.remove(removed);
         m_sshTimeout.remove(removed);
         m_sshPort.remove(removed);
         m_headless.remove(removed);
         m_wwwPort.remove(removed);
         m_wwwProxy.remove(removed);
         m_wwwProxyServers.remove(removed);
         m_wwwProxyExcludes.remove(removed);
         m_vdiCapacityMb.remove(removed);
         m_memorySizeMb.remove(removed);
         m_cpuCount.remove(removed);
    }
    update();
}

void MerOptionsWidget::onTestConnectionButtonClicked()
{
    BuildEngine *const sdk = m_sdks[m_virtualMachine];
    if (!sdk->virtualMachine()->isOff()) {
        SshConnectionParameters params = sdk->virtualMachine()->sshParameters();
        if (m_sshPrivKeys.contains(sdk))
            params.privateKeyFile = m_sshPrivKeys[sdk];
        if (m_sshPort.contains(sdk))
            params.setPort(m_sshPort[sdk]);
        m_ui->sdkDetailsWidget->setStatus(tr("Connecting to machine %1 ...")
                .arg(sdk->virtualMachine()->name()));
        m_ui->sdkDetailsWidget->setTestButtonEnabled(false);
        m_status = MerConnectionManager::testConnection(params);
        m_ui->sdkDetailsWidget->setTestButtonEnabled(true);
        update();
    } else {
        m_ui->sdkDetailsWidget->setStatus(tr("Virtual machine %1 is not running.")
                .arg(sdk->virtualMachine()->name()));
    }
}

void MerOptionsWidget::onAuthorizeSshKey(const QString &file)
{
    BuildEngine *const sdk = m_sdks[m_virtualMachine];
    const QString pubKeyPath = file + QLatin1String(".pub");
    const QString sshDirectoryPath = sdk->sharedSshPath().toString() + QLatin1Char('/');
    const QStringList authorizedKeysPaths = QStringList()
            << sshDirectoryPath + QLatin1String("root/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER)
            << sshDirectoryPath + sdk->virtualMachine()->sshParameters().userName()
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
    BuildEngine *const sdk = m_sdks[m_virtualMachine];
    sdk->virtualMachine()->connectTo();
}

void MerOptionsWidget::onStopVirtualMachineButtonClicked()
{
    BuildEngine *const sdk = m_sdks[m_virtualMachine];
    sdk->virtualMachine()->disconnectFrom();
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

void MerOptionsWidget::onBuildEngineAdded(int index)
{
    BuildEngine *const sdk = Sdk::buildEngines().at(index);

    m_sdks[sdk->uri()] = sdk;
    m_virtualMachine = sdk->uri();

    auto cleaner = [=](auto*... caches) {
        return [=]() {
#if __cplusplus < 201703L
            using helper = int[];
            helper{ (caches->remove(sdk), 0)... };
#else
            (caches->remove(sdk), ...);
#endif
            update();
        };
    };

    connect(sdk->virtualMachine(), &VirtualMachine::sshParametersChanged,
            this, cleaner(&m_sshPort, &m_sshPrivKeys, &m_sshTimeout));
    connect(sdk->virtualMachine(), &VirtualMachine::headlessChanged,
            this, cleaner(&m_headless));
    connect(sdk, &BuildEngine::wwwPortChanged,
            this, cleaner(&m_wwwPort));
    connect(sdk, &BuildEngine::wwwProxyChanged,
            this, cleaner(&m_wwwProxy, &m_wwwProxyServers, &m_wwwProxyExcludes));
    connect(sdk->virtualMachine(), &VirtualMachine::vdiCapacityMbChanged,
            this, cleaner(&m_vdiCapacityMb));
    connect(sdk->virtualMachine(), &VirtualMachine::memorySizeMbChanged,
            this, cleaner(&m_memorySizeMb));
    connect(sdk->virtualMachine(), &VirtualMachine::cpuCountChanged,
            this, cleaner(&m_cpuCount));

    update();
}

void MerOptionsWidget::onAboutToRemoveBuildEngine(int index)
{
    BuildEngine *const sdk = Sdk::buildEngines().at(index);

    m_sdks.remove(sdk->uri());
    if (m_virtualMachine == sdk->uri()) {
        m_virtualMachine.clear();
        if (!m_sdks.isEmpty())
            m_virtualMachine = m_sdks.first()->uri();
    }

    m_sshPrivKeys.remove(sdk);
    m_sshTimeout.remove(sdk);
    m_sshPort.remove(sdk);
    m_headless.remove(sdk);
    m_wwwPort.remove(sdk);
    m_wwwProxy.remove(sdk);
    m_wwwProxyServers.remove(sdk);
    m_wwwProxyExcludes.remove(sdk);
    m_vdiCapacityMb.remove(sdk);
    m_memorySizeMb.remove(sdk);
    m_cpuCount.remove(sdk);

    update();
}

void MerOptionsWidget::onSrcFolderApplyButtonClicked(const QString &newFolder)
{
    BuildEngine *const sdk = m_sdks[m_virtualMachine];

    if (newFolder == sdk->sharedSrcPath().toString()) {
        QMessageBox::information(this, tr("Choose a new folder"),
                                 tr("The given folder (%1) is the current alternative source folder. "
                                    "Please choose another folder if you want to change it.")
                                 .arg(sdk->sharedSrcPath().toString()));
        return;
    }

    if (!sdk->virtualMachine()->isOff()) {
        QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                tr("Close Virtual Machine"),
                tr("Close the \"%1\" virtual machine?").arg(sdk->name()),
                QMessageBox::Yes | QMessageBox::No,
                this);
        questionBox->setInformativeText(
                tr("Virtual machine must be closed before the source folder can be changed."));
        if (questionBox->exec() != QMessageBox::Yes) {
            // reset the path in the chooser
            m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath().toString());
            return;
        }
    }

    if (!sdk->virtualMachine()->lockDown(true)) {
        QMessageBox::warning(this, tr("Failed"),
                tr("Alternative source folder not changed"));
        // reset the path in the chooser
        m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath().toString());
        return;
    }

    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&BuildEngine::setSharedSrcPath), sdk,
            FileName::fromUserInput(newFolder));

    sdk->virtualMachine()->lockDown(false);

    if (ok) {
        const QMessageBox::StandardButton response =
            QMessageBox::question(this, tr("Success!"),
                                  tr("Alternative source folder for %1 changed to %2.\n\n"
                                     "Do you want to start %1 now?").arg(sdk->name()).arg(newFolder),
                                  QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
        if (response == QMessageBox::Yes)
            sdk->virtualMachine()->connectTo();
    }
    else {
        QMessageBox::warning(this, tr("Changing the source folder failed!"),
                             tr("Unable to change the alternative source folder to %1").arg(newFolder));
        // reset the path in the chooser
        m_ui->sdkDetailsWidget->setSrcFolderChooserPath(sdk->sharedSrcPath().toString());
    }
}

void MerOptionsWidget::update()
{
    m_ui->sdkComboBox->clear();
    for (BuildEngine *const sdk : m_sdks)
        m_ui->sdkComboBox->addItem(sdk->name(), sdk->uri());

    bool show = !m_virtualMachine.isEmpty();
    BuildEngine *const sdk = m_sdks.value(m_virtualMachine);
    QTC_ASSERT(!show || sdk, show = false);

    disconnect(m_vmOffConnection);

    if (show) {
        const SshConnectionParameters sshParameters = sdk->virtualMachine()->sshParameters();
        m_ui->sdkDetailsWidget->setSdk(sdk);
        if (m_sshPrivKeys.contains(sdk))
            m_ui->sdkDetailsWidget->setPrivateKeyFile(m_sshPrivKeys[sdk]);
        else
            m_ui->sdkDetailsWidget->setPrivateKeyFile(sshParameters.privateKeyFile);

        if (m_sshTimeout.contains(sdk))
            m_ui->sdkDetailsWidget->setSshTimeout(m_sshTimeout[sdk]);
        else
            m_ui->sdkDetailsWidget->setSshTimeout(sshParameters.timeout);

        if (m_sshPort.contains(sdk))
            m_ui->sdkDetailsWidget->setSshPort(m_sshPort[sdk]);
        else
            m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());

        if (m_headless.contains(sdk))
            m_ui->sdkDetailsWidget->setHeadless(m_headless[sdk]);
        else
            m_ui->sdkDetailsWidget->setHeadless(sdk->virtualMachine()->isHeadless());

        if (m_wwwPort.contains(sdk))
            m_ui->sdkDetailsWidget->setWwwPort(m_wwwPort[sdk]);
        else
            m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());

        if (m_wwwProxy.contains(sdk))
            m_ui->sdkDetailsWidget->setWwwProxy(m_wwwProxy[sdk], m_wwwProxyServers[sdk], m_wwwProxyExcludes[sdk]);
        else
            m_ui->sdkDetailsWidget->setWwwProxy(sdk->wwwProxyType(), sdk->wwwProxyServers(), sdk->wwwProxyExcludes());

        if (m_memorySizeMb.contains(sdk))
            m_ui->sdkDetailsWidget->setMemorySizeMb(m_memorySizeMb[sdk]);
        else
            m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->virtualMachine()->memorySizeMb());

        if (m_cpuCount.contains(sdk))
            m_ui->sdkDetailsWidget->setCpuCount(m_cpuCount[sdk]);
        else
            m_ui->sdkDetailsWidget->setCpuCount(sdk->virtualMachine()->cpuCount());

        if (m_vdiCapacityMb.contains(sdk))
            m_ui->sdkDetailsWidget->setVdiCapacityMb(m_vdiCapacityMb[sdk]);
        else
            m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->virtualMachine()->vdiCapacityMb());

        onVmOffChanged(sdk->virtualMachine()->isOff());
        m_vmOffConnection = connect(sdk->virtualMachine(), &VirtualMachine::virtualMachineOffChanged,
                this, &MerOptionsWidget::onVmOffChanged);

        int index = m_ui->sdkComboBox->findData(m_virtualMachine);
        m_ui->sdkComboBox->setCurrentIndex(index);
        m_ui->sdkDetailsWidget->setStatus(m_status);
    }

    m_ui->sdkDetailsWidget->setVisible(show);
    m_ui->removeButton->setEnabled(show && !sdk->isAutodetected());
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
    BuildEngine *const sdk = m_sdks[m_virtualMachine];

    // If the VM is started, cancel any unsaved changes to values that cannot be changed online to
    // prevent inconsistencies
    if (!vmOff) {
        m_ui->sdkDetailsWidget->setSshPort(sdk->sshPort());
        m_sshPort.remove(sdk);
        m_ui->sdkDetailsWidget->setWwwPort(sdk->wwwPort());
        m_wwwPort.remove(sdk);
        m_ui->sdkDetailsWidget->setMemorySizeMb(sdk->virtualMachine()->memorySizeMb());
        m_memorySizeMb.remove(sdk);
        m_ui->sdkDetailsWidget->setCpuCount(sdk->virtualMachine()->cpuCount());
        m_cpuCount.remove(sdk);
        m_ui->sdkDetailsWidget->setVdiCapacityMb(sdk->virtualMachine()->vdiCapacityMb());
        m_vdiCapacityMb.remove(sdk);
    }

    m_ui->sdkDetailsWidget->setVmOffStatus(vmOff);
}

} // Internal
} // Mer
