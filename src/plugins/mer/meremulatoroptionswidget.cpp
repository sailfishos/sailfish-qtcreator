/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "meremulatoroptionswidget.h"

#include "merconnectionmanager.h"
#include "merconstants.h"
#include "meremulatordetailswidget.h"
#include "meremulatordevice.h"
#include "meremulatormodedialog.h"
#include "mervmselectiondialog.h"
#include "ui_meremulatoroptionswidget.h"

#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>
#include <utils/pointeralgorithm.h>
#include <utils/portlist.h>
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

MerEmulatorOptionsWidget::MerEmulatorOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerEmulatorOptionsWidget)
    , m_status(tr("Not connected."))
{
    m_ui->setupUi(this);
    connect(Sdk::instance(), &Sdk::emulatorAdded,
            this, &MerEmulatorOptionsWidget::onEmulatorAdded);
    connect(Sdk::instance(), &Sdk::aboutToRemoveEmulator,
            this, &MerEmulatorOptionsWidget::onAboutToRemoveEmulator);

    connect(m_ui->emulatorComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &MerEmulatorOptionsWidget::onEmulatorChanged);

    connect(m_ui->addButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onAddButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onRemoveButtonClicked);

    connect(m_ui->startVirtualMachineButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onStartVirtualMachineButtonClicked);
    connect(m_ui->stopVirtualMachineButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onStopVirtualMachineButtonClicked);
    connect(m_ui->regenerateSshKeysButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onRegenerateSshKeysButtonClicked);
    connect(m_ui->emulatorModeButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onEmulatorModeButtonClicked);
    connect(m_ui->factoryResetButton, &QPushButton::clicked,
            this, &MerEmulatorOptionsWidget::onFactoryResetButtonClicked);
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::testConnectionButtonClicked,
            this, &MerEmulatorOptionsWidget::onTestConnectionButtonClicked);

    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::factorySnapshotChanged,
            this, [=](const QString &factorySnapshot) {
        m_factorySnapshot[m_emulators[m_virtualMachine]] = factorySnapshot;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::sshTimeoutChanged,
            this, [=](int timeout) {
        m_sshTimeout[m_emulators[m_virtualMachine]] = timeout;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::sshPortChanged,
            this, [=](quint16 port) {
        m_sshPort[m_emulators[m_virtualMachine]] = port;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::qmlLivePortsChanged,
            this, [=](const PortList &ports) {
        m_qmlLivePorts[m_emulators[m_virtualMachine]] = ports;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::freePortsChanged,
            this, [=](const PortList &ports) {
        m_freePorts[m_emulators[m_virtualMachine]] = ports;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::memorySizeMbChanged,
            this, [=](int sizeMb) {
        m_memorySizeMb[m_emulators[m_virtualMachine]] = sizeMb;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::cpuCountChanged,
            this, [=](int count) {
        m_cpuCount[m_emulators[m_virtualMachine]] = count;
    });
    connect(m_ui->emulatorDetailsWidget, &MerEmulatorDetailsWidget::vdiCapacityMbChnaged,
            this, [=](int sizeMb) {
        m_vdiCapacityMb[m_emulators[m_virtualMachine]] = sizeMb;
    });

    for (int i = 0; i < Sdk::emulators().count(); ++i)
        onEmulatorAdded(i);
    update();
}

MerEmulatorOptionsWidget::~MerEmulatorOptionsWidget()
{
    delete m_ui;
}

void MerEmulatorOptionsWidget::setEmulator(const QUrl &uri)
{
    QTC_ASSERT(uri.isValid(), return);
    QTC_ASSERT(m_emulators.contains(uri), return);

    if (m_virtualMachine == uri)
        return;

    m_virtualMachine = uri;
    m_status = tr("Not tested.");
    update();
}

QString MerEmulatorOptionsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    QString keys;
    int count = m_ui->emulatorComboBox->count();
    for (int i = 0; i < count; ++i)
        keys += m_ui->emulatorComboBox->itemText(i) + blank;
    if (m_ui->emulatorDetailsWidget->isVisible())
        keys += m_ui->emulatorDetailsWidget->searchKeyWordMatchString();

    return keys.trimmed();
}

void MerEmulatorOptionsWidget::store()
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

    QList<Emulator *> lockedDownEmulators;
    ok &= lockDownConnectionsOrCancelChangesThatNeedIt(&lockedDownEmulators);

    for (Emulator *const emulator : qAsConst(m_emulators)) {
        progress.setLabelText(tr("Applying emulator settings: '%1'").arg(emulator->name()));

        if (m_factorySnapshot.contains(emulator))
            emulator->setFactorySnapshot(m_factorySnapshot[emulator]);
        if (m_sshTimeout.contains(emulator)) {
            SshConnectionParameters sshParameters = emulator->virtualMachine()->sshParameters();
            sshParameters.timeout = m_sshTimeout[emulator];
            emulator->virtualMachine()->setSshParameters(sshParameters);
        }
        if (m_sshPort.contains(emulator)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&Emulator::setSshPort), emulator,
                    m_sshPort[emulator]);
            if (!stepOk) {
                m_ui->emulatorDetailsWidget->setSshPort(emulator->sshPort());
                m_sshPort.remove(emulator);
                ok = false;
            }
        }
        if (m_qmlLivePorts.contains(emulator)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&Emulator::setQmlLivePorts), emulator,
                    m_qmlLivePorts[emulator]);
            if (!stepOk) {
                m_ui->emulatorDetailsWidget->setQmlLivePorts(emulator->qmlLivePorts());
                m_qmlLivePorts.remove(emulator);
                ok = false;
            }
        }
        if (m_freePorts.contains(emulator)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&Emulator::setFreePorts), emulator,
                    m_freePorts[emulator]);
            if (!stepOk) {
                m_ui->emulatorDetailsWidget->setFreePorts(emulator->freePorts());
                m_freePorts.remove(emulator);
                ok = false;
            }
        }
        if (m_memorySizeMb.contains(emulator)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setMemorySizeMb),
                    emulator->virtualMachine(), m_memorySizeMb[emulator]);
            if (!stepOk) {
                m_ui->emulatorDetailsWidget->setMemorySizeMb(emulator->virtualMachine()->memorySizeMb());
                m_memorySizeMb.remove(emulator);
                ok = false;
            }
        }
        if (m_cpuCount.contains(emulator)) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setCpuCount),
                    emulator->virtualMachine(), m_cpuCount[emulator]);
            if (!stepOk) {
                m_ui->emulatorDetailsWidget->setCpuCount(emulator->virtualMachine()->cpuCount());
                m_cpuCount.remove(emulator);
                ok = false;
            }
        }
        if (m_vdiCapacityMb.contains(emulator)) {
            const int newVdiCapacityMb = m_vdiCapacityMb[emulator];
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setVdiCapacityMb),
                    emulator->virtualMachine(), newVdiCapacityMb);
            if (!stepOk) {
                m_vdiCapacityMb.remove(emulator);
                ok = false;
            }
            m_ui->emulatorDetailsWidget->setVdiCapacityMb(emulator->virtualMachine()->vdiCapacityMb());
        }
    }

    for (Emulator *const emulator : lockedDownEmulators)
        emulator->virtualMachine()->lockDown(false);

    if (!ok) {
        progress.cancel();
        QMessageBox::warning(this, tr("Some changes could not be saved!"),
                             tr("Failed to apply some of the changes to emulators"));
    }

    for (Emulator *const emulator : Sdk::emulators()) {
        if (!m_emulators.contains(emulator->uri()))
            Sdk::removeEmulator(emulator->uri());
    }
    for (std::unique_ptr<Emulator> &newEmulator : m_newEmulators)
        Sdk::addEmulator(std::move(newEmulator));
    m_newEmulators.clear();

    m_factorySnapshot.clear();
    m_sshTimeout.clear();
    m_sshPort.clear();
    m_qmlLivePorts.clear();
    m_freePorts.clear();
    m_memorySizeMb.clear();
    m_cpuCount.clear();
    m_vdiCapacityMb.clear();
}

bool MerEmulatorOptionsWidget::lockDownConnectionsOrCancelChangesThatNeedIt(QList<Emulator *>
        *lockedDownEmulators)
{
    QTC_ASSERT(lockedDownEmulators, return false);

    QList<Emulator *> failed;

    for (Emulator *const emulator : qAsConst(m_emulators)) {
        if (m_sshPort.value(emulator) == emulator->sshPort())
            m_sshPort.remove(emulator);
        if (m_qmlLivePorts.value(emulator).toString() == emulator->qmlLivePorts().toString())
            m_qmlLivePorts.remove(emulator);
        if (m_freePorts.value(emulator).toString() == emulator->freePorts().toString())
            m_freePorts.remove(emulator);
        if (m_memorySizeMb.value(emulator) == emulator->virtualMachine()->memorySizeMb())
            m_memorySizeMb.remove(emulator);
        if (m_cpuCount.value(emulator) == emulator->virtualMachine()->cpuCount())
            m_cpuCount.remove(emulator);
        if (m_vdiCapacityMb.value(emulator) == emulator->virtualMachine()->vdiCapacityMb())
            m_vdiCapacityMb.remove(emulator);

        if (!m_sshPort.contains(emulator)
                && !m_qmlLivePorts.contains(emulator)
                && !m_freePorts.contains(emulator)
                && !m_memorySizeMb.contains(emulator)
                && !m_cpuCount.contains(emulator)
                && !m_vdiCapacityMb.contains(emulator)) {
            continue;
        }

        if (!emulator->virtualMachine()->isOff()) {
            QPointer<QMessageBox> questionBox = new QMessageBox(QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("Close the \"%1\" virtual machine?").arg(emulator->virtualMachine()->name()),
                    QMessageBox::Yes | QMessageBox::No,
                    this);
            questionBox->setInformativeText(
                    tr("Some of the changes require stopping the virtual machine before they can be applied."));
            if (questionBox->exec() != QMessageBox::Yes) {
                failed.append(emulator);
                continue;
            }
        }

        if (!emulator->virtualMachine()->lockDown(true)) {
            failed.append(emulator);
            continue;
        }

        lockedDownEmulators->append(emulator);
    }

    for (Emulator *const emulator : qAsConst(failed)) {
        m_ui->emulatorDetailsWidget->setSshPort(emulator->sshPort());
        m_sshPort.remove(emulator);
        m_ui->emulatorDetailsWidget->setQmlLivePorts(emulator->qmlLivePorts());
        m_qmlLivePorts.remove(emulator);
        m_ui->emulatorDetailsWidget->setFreePorts(emulator->freePorts());
        m_freePorts.remove(emulator);
        m_ui->emulatorDetailsWidget->setMemorySizeMb(emulator->virtualMachine()->memorySizeMb());
        m_memorySizeMb.remove(emulator);
        m_ui->emulatorDetailsWidget->setCpuCount(emulator->virtualMachine()->cpuCount());
        m_cpuCount.remove(emulator);
        m_ui->emulatorDetailsWidget->setVdiCapacityMb(emulator->virtualMachine()->vdiCapacityMb());
        m_vdiCapacityMb.remove(emulator);
    }

    return failed.isEmpty();
}

void MerEmulatorOptionsWidget::onEmulatorChanged(int index)
{
    setEmulator(m_ui->emulatorComboBox->itemData(index).toUrl());
}

void MerEmulatorOptionsWidget::onAddButtonClicked()
{
    MerVmSelectionDialog dialog(this);
    dialog.setWindowTitle(tr("Add a %1 Emulator").arg(QCoreApplication::translate("Mer", Mer::Constants::MER_OS_NAME)));
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (m_emulators.contains(dialog.selectedVmUri()))
        return;

    std::unique_ptr<Emulator> emulator;
    // FIXME overload execAsynchronous to support this
    QEventLoop loop;
    auto whenDone = [&loop, &emulator](std::unique_ptr<Emulator> &&newEmulator) {
        loop.quit();
        emulator = std::move(newEmulator);
    };
    Sdk::createEmulator(dialog.selectedVmUri(), &loop, whenDone);
    loop.exec();
    QTC_ASSERT(emulator, return);

    // FIXME do this on Sfdk side
    SshConnectionParameters sshParameters = emulator->virtualMachine()->sshParameters();
    sshParameters.privateKeyFile =
        MerEmulatorDevice::privateKeyFile(MerEmulatorDevice::idFor(*emulator),
                Constants::MER_DEVICE_DEFAULTUSER);
    emulator->virtualMachine()->setSshParameters(sshParameters);

    // FIXME detect the device model and fallback to some default one on Sfdk side
    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&Emulator::setDisplayProperties), emulator.get(),
            Sdk::deviceModels().first(), emulator->orientation(), emulator->isViewScaled());
    QTC_CHECK(ok);

    m_emulators[emulator->virtualMachine()->uri()] = emulator.get();
    m_virtualMachine = emulator->virtualMachine()->uri();
    m_newEmulators.emplace_back(std::move(emulator));
    // FIXME originally this was done on user request, now we should only do it if needed, and it
    // should be implemented on Sfdk side
    onRegenerateSshKeysButtonClicked();
    update();
}

void MerEmulatorOptionsWidget::onRemoveButtonClicked()
{
    const QUrl vmUri = m_ui->emulatorComboBox->itemData(m_ui->emulatorComboBox->currentIndex(),
            Qt::UserRole).toUrl();
    QTC_ASSERT(vmUri.isValid(), return);

    if (m_emulators.contains(vmUri)) {
        Emulator *const removed = m_emulators.take(vmUri);
        Utils::erase(m_newEmulators, removed);
        if (!m_emulators.isEmpty())
            m_virtualMachine = m_emulators.keys().last();
        else
            m_virtualMachine.clear();

        m_factorySnapshot.remove(removed);
        m_sshTimeout.remove(removed);
        m_sshPort.remove(removed);
        m_qmlLivePorts.remove(removed);
        m_freePorts.remove(removed);
        m_vdiCapacityMb.remove(removed);
        m_memorySizeMb.remove(removed);
        m_cpuCount.remove(removed);
    }
    update();
}

void MerEmulatorOptionsWidget::onTestConnectionButtonClicked()
{
    Emulator *const emulator = m_emulators[m_virtualMachine];
    if (!emulator->virtualMachine()->isOff()) {
        SshConnectionParameters params = emulator->virtualMachine()->sshParameters();
        if (m_sshPort.contains(emulator))
            params.setPort(m_sshPort[emulator]);
        m_ui->emulatorDetailsWidget->setStatus(tr("Connecting…"));
        m_ui->emulatorDetailsWidget->setTestButtonEnabled(false);
        m_status = MerConnectionManager::testConnection(params);
        m_ui->emulatorDetailsWidget->setTestButtonEnabled(true);
        update();
    } else {
        m_ui->emulatorDetailsWidget->setStatus(tr("Virtual machine is not running."));
    }
}

void MerEmulatorOptionsWidget::onStartVirtualMachineButtonClicked()
{
    Emulator *const emulator = m_emulators[m_virtualMachine];
    emulator->virtualMachine()->connectTo();
}

void MerEmulatorOptionsWidget::onStopVirtualMachineButtonClicked()
{
    Emulator *const emulator = m_emulators[m_virtualMachine];
    emulator->virtualMachine()->disconnectFrom();
}

void MerEmulatorOptionsWidget::onRegenerateSshKeysButtonClicked()
{
    Emulator *const emulator = m_emulators[m_virtualMachine];
    MerEmulatorDevice::generateSshKey(emulator, QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
    MerEmulatorDevice::generateSshKey(emulator, QLatin1String(Constants::MER_DEVICE_ROOTUSER));
}

void MerEmulatorOptionsWidget::onEmulatorModeButtonClicked()
{
    Emulator *const emulator = m_emulators[m_virtualMachine];
    MerEmulatorModeDialog dialog(emulator, ICore::dialogParent());
    dialog.exec();
}

void MerEmulatorOptionsWidget::onFactoryResetButtonClicked()
{
    Emulator *const emulator = m_emulators[m_virtualMachine];
    if (emulator->factorySnapshot().isEmpty()) {
        QMessageBox::warning(ICore::dialogParent(), tr("No factory snapshot set"),
                tr("No factory snapshot is configured. Cannot reset to the factory state"));
        return;
    }
    QMessageBox::StandardButton button = QMessageBox::question(ICore::dialogParent(),
            tr("Reset emulator?"),
            tr("Do you really want to reset '%1' to the factory state?")
                .arg(emulator->virtualMachine()->name()),
            QMessageBox::Yes | QMessageBox::No);
    if (button == QMessageBox::Yes)
        MerEmulatorDevice::doFactoryReset(emulator, ICore::dialogParent());
}

void MerEmulatorOptionsWidget::onEmulatorAdded(int index)
{
    Emulator *const emulator = Sdk::emulators().at(index);

    m_emulators[emulator->uri()] = emulator;
    m_virtualMachine = emulator->uri();

    auto cleaner = [=](auto*... caches) {
        return [=]() {
#if __cplusplus < 201703L
            using helper = int[];
            helper{ (caches->remove(emulator), 0)... };
#else
            (caches->remove(emulator), ...);
#endif
            update();
        };
    };

    connect(emulator, &Emulator::factorySnapshotChanged,
            this, cleaner(&m_factorySnapshot));
    connect(emulator->virtualMachine(), &VirtualMachine::sshParametersChanged,
            this, cleaner(&m_sshPort, &m_sshTimeout));
    connect(emulator, &Emulator::qmlLivePortsChanged,
            this, cleaner(&m_qmlLivePorts));
    connect(emulator, &Emulator::freePortsChanged,
            this, cleaner(&m_freePorts));
    connect(emulator->virtualMachine(), &VirtualMachine::vdiCapacityMbChanged,
            this, cleaner(&m_vdiCapacityMb));
    connect(emulator->virtualMachine(), &VirtualMachine::memorySizeMbChanged,
            this, cleaner(&m_memorySizeMb));
    connect(emulator->virtualMachine(), &VirtualMachine::cpuCountChanged,
            this, cleaner(&m_cpuCount));

    update();
}

void MerEmulatorOptionsWidget::onAboutToRemoveEmulator(int index)
{
    Emulator *const emulator = Sdk::emulators().at(index);

    m_emulators.remove(emulator->uri());
    if (m_virtualMachine == emulator->uri()) {
        m_virtualMachine.clear();
        if (!m_emulators.isEmpty())
            m_virtualMachine = m_emulators.first()->uri();
    }

    m_factorySnapshot.remove(emulator);
    m_sshTimeout.remove(emulator);
    m_sshPort.remove(emulator);
    m_qmlLivePorts.remove(emulator);
    m_freePorts.remove(emulator);
    m_vdiCapacityMb.remove(emulator);
    m_memorySizeMb.remove(emulator);
    m_cpuCount.remove(emulator);

    update();
}

void MerEmulatorOptionsWidget::update()
{
    m_ui->emulatorComboBox->clear();
    for (Emulator *const emulator : m_emulators)
        m_ui->emulatorComboBox->addItem(emulator->name(), emulator->uri());

    bool show = !m_virtualMachine.isEmpty();
    Emulator *const emulator = m_emulators.value(m_virtualMachine);
    QTC_ASSERT(!show || emulator, show = false);

    disconnect(m_vmOffConnection);

    if (show) {
        const SshConnectionParameters sshParameters = emulator->virtualMachine()->sshParameters();
        m_ui->emulatorDetailsWidget->setEmulator(emulator);
        if (m_factorySnapshot.contains(emulator))
            m_ui->emulatorDetailsWidget->setFactorySnapshot(m_factorySnapshot[emulator]);
        else
            m_ui->emulatorDetailsWidget->setFactorySnapshot(emulator->factorySnapshot());

        if (m_sshTimeout.contains(emulator))
            m_ui->emulatorDetailsWidget->setSshTimeout(m_sshTimeout[emulator]);
        else
            m_ui->emulatorDetailsWidget->setSshTimeout(sshParameters.timeout);

        if (m_sshPort.contains(emulator))
            m_ui->emulatorDetailsWidget->setSshPort(m_sshPort[emulator]);
        else
            m_ui->emulatorDetailsWidget->setSshPort(emulator->sshPort());

        if (m_qmlLivePorts.contains(emulator))
            m_ui->emulatorDetailsWidget->setQmlLivePorts(m_qmlLivePorts[emulator]);
        else
            m_ui->emulatorDetailsWidget->setQmlLivePorts(emulator->qmlLivePorts());

        if (m_freePorts.contains(emulator))
            m_ui->emulatorDetailsWidget->setFreePorts(m_freePorts[emulator]);
        else
            m_ui->emulatorDetailsWidget->setFreePorts(emulator->freePorts());

        if (m_memorySizeMb.contains(emulator))
            m_ui->emulatorDetailsWidget->setMemorySizeMb(m_memorySizeMb[emulator]);
        else
            m_ui->emulatorDetailsWidget->setMemorySizeMb(emulator->virtualMachine()->memorySizeMb());

        if (m_cpuCount.contains(emulator))
            m_ui->emulatorDetailsWidget->setCpuCount(m_cpuCount[emulator]);
        else
            m_ui->emulatorDetailsWidget->setCpuCount(emulator->virtualMachine()->cpuCount());

        if (m_vdiCapacityMb.contains(emulator))
            m_ui->emulatorDetailsWidget->setVdiCapacityMb(m_vdiCapacityMb[emulator]);
        else
            m_ui->emulatorDetailsWidget->setVdiCapacityMb(emulator->virtualMachine()->vdiCapacityMb());

        onVmOffChanged(emulator->virtualMachine()->isOff());
        m_vmOffConnection = connect(emulator->virtualMachine(), &VirtualMachine::virtualMachineOffChanged,
                this, &MerEmulatorOptionsWidget::onVmOffChanged);

        int index = m_ui->emulatorComboBox->findData(m_virtualMachine);
        m_ui->emulatorComboBox->setCurrentIndex(index);
        m_ui->emulatorDetailsWidget->setStatus(m_status);
    }

    m_ui->emulatorDetailsWidget->setVisible(show);
    m_ui->removeButton->setEnabled(show && !emulator->isAutodetected());
    m_ui->startVirtualMachineButton->setEnabled(show);
    m_ui->stopVirtualMachineButton->setEnabled(show);
    m_ui->regenerateSshKeysButton->setEnabled(show);
    m_ui->emulatorModeButton->setEnabled(show);
    m_ui->factoryResetButton->setEnabled(show);
}

void MerEmulatorOptionsWidget::onVmOffChanged(bool vmOff)
{
    Emulator *const emulator = m_emulators[m_virtualMachine];

    // If the VM is started, cancel any unsaved changes to values that cannot be changed online to
    // prevent inconsistencies
    if (!vmOff) {
        m_ui->emulatorDetailsWidget->setSshPort(emulator->sshPort());
        m_sshPort.remove(emulator);
        m_ui->emulatorDetailsWidget->setQmlLivePorts(emulator->qmlLivePorts());
        m_qmlLivePorts.remove(emulator);
        m_ui->emulatorDetailsWidget->setFreePorts(emulator->freePorts());
        m_freePorts.remove(emulator);
        m_ui->emulatorDetailsWidget->setMemorySizeMb(emulator->virtualMachine()->memorySizeMb());
        m_memorySizeMb.remove(emulator);
        m_ui->emulatorDetailsWidget->setCpuCount(emulator->virtualMachine()->cpuCount());
        m_cpuCount.remove(emulator);
        m_ui->emulatorDetailsWidget->setVdiCapacityMb(emulator->virtualMachine()->vdiCapacityMb());
        m_vdiCapacityMb.remove(emulator);
    }

    m_ui->emulatorDetailsWidget->setVmOffStatus(vmOff);
}

} // Internal
} // Mer
