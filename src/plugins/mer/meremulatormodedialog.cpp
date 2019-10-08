/****************************************************************************
**
** Copyright (C) 2015-2016,2018-2019 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "meremulatormodedialog.h"
#include "ui_meremulatormodedialog.h"

#include "merconnection.h"
#include "mersettings.h"
#include "merconstants.h"
#include "meremulatordevice.h"

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDesktopWidget>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;

using namespace Mer;
using namespace Mer::Internal;
using namespace Mer::Constants;

MerEmulatorModeDialog::MerEmulatorModeDialog(QObject *parent)
    : QObject(parent),
      m_action(new QAction(this)),
      m_ui(0),
      m_kit(0)
{
    m_action->setEnabled(false);
    m_action->setText(tr(MER_EMULATOR_MODE_ACTION_NAME));

    connect(m_action, &QAction::triggered,
            this, &MerEmulatorModeDialog::execDialog);

    onStartupProjectChanged(SessionManager::startupProject());
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &MerEmulatorModeDialog::onStartupProjectChanged);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &MerEmulatorModeDialog::onKitUpdated);

    connect(DeviceManager::instance(), &DeviceManager::deviceListReplaced,
            this, &MerEmulatorModeDialog::onDeviceListReplaced);
}

MerEmulatorModeDialog::MerEmulatorModeDialog(const MerEmulatorDevice::Ptr &emulator, QObject *parent)
    : QObject(parent),
      m_action(new QAction(tr(MER_EMULATOR_MODE_ACTION_NAME), this)),
      m_ui(nullptr),
      m_kit(nullptr)
{
    setEmulator(emulator);

    connect(m_action, &QAction::triggered, this, &MerEmulatorModeDialog::execDialog);
}

MerEmulatorModeDialog::~MerEmulatorModeDialog()
{
}

QAction *MerEmulatorModeDialog::action() const
{
    return m_action;
}

MerEmulatorDevice::Ptr MerEmulatorModeDialog::emulator() const
{
    return m_emulator.toStrongRef();
}

bool MerEmulatorModeDialog::exec()
{
    QTC_ASSERT(m_emulator != nullptr, return false);
    return execDialog();
}

void MerEmulatorModeDialog::setEmulator(const MerEmulatorDevice::Ptr &emulator)
{
    m_emulator = emulator;
    m_action->setEnabled(m_emulator != 0);
}

void MerEmulatorModeDialog::onStartupProjectChanged(Project *project)
{
    if (m_project != 0) {
        m_project->disconnect(this);
        onActiveTargetChanged(0);
    }

    m_project = project;

    if (m_project != 0) {
        onActiveTargetChanged(m_project->activeTarget());
        connect(m_project.data(), &Project::activeTargetChanged,
                this, &MerEmulatorModeDialog::onActiveTargetChanged);
    }
}

void MerEmulatorModeDialog::onActiveTargetChanged(Target *target)
{
    if (m_target != 0) {
        m_target->disconnect(this);
        setEmulator(MerEmulatorDevice::Ptr());
    }

    m_target = target;

    if (m_target != 0) {
        onTargetKitChanged();
        connect(m_target.data(), &Target::kitChanged,
                this, &MerEmulatorModeDialog::onTargetKitChanged);
    }
}

void MerEmulatorModeDialog::onTargetKitChanged()
{
    Q_ASSERT(m_target != 0);

    if (m_kit != 0) {
        setEmulator(MerEmulatorDevice::Ptr());
    }

    m_kit = m_target->kit();

    if (m_kit != 0) {
        onKitUpdated(m_kit);
    }
}

void MerEmulatorModeDialog::onKitUpdated(Kit *kit)
{
    Q_ASSERT(kit != 0);

    if (kit != m_kit) {
        return;
    }

    auto device = DeviceKitInformation::device(m_kit);
    setEmulator(device.dynamicCast<const MerEmulatorDevice>().constCast<MerEmulatorDevice>());
}

void MerEmulatorModeDialog::onDeviceListReplaced()
{
    if (m_kit == nullptr)
        return;

    auto device = DeviceKitInformation::device(m_kit);
    setEmulator(device.dynamicCast<const MerEmulatorDevice>().constCast<MerEmulatorDevice>());
}

bool MerEmulatorModeDialog::execDialog()
{
    QTC_ASSERT(m_emulator != 0, return false);
    QTC_ASSERT(m_ui == 0, return false); // possible, but little issue..

    m_dialog = new QDialog(ICore::dialogParent());
    m_ui = new Ui::MerEmulatorModeDialog;
    m_ui->setupUi(m_dialog);
    m_ui->deviceNameLabel->setText(m_emulator.data()->displayName());

    m_ui->deviceModelComboBox->setDeviceModels(MerSettings::deviceModels().values());
    m_ui->deviceModelComboBox->setCurrentDeviceModel(m_emulator.data()->deviceModel());
    const bool supportsMultipleModels = m_ui->deviceModelComboBox->count();

    QRadioButton *orientationRadioButton = m_emulator.data()->orientation() == Qt::Vertical
        ? m_ui->portraitRadioButton
        : m_ui->landscapeRadioButton;
    orientationRadioButton->setChecked(true);

    QRadioButton *viewModeRadioButton = m_emulator.data()->isViewScaled()
        ? m_ui->scaledViewModeRadioButton
        : m_ui->originalViewModeRadioButton;
    viewModeRadioButton->setChecked(true);

    connect(m_ui->deviceModelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerEmulatorModeDialog::guessOptimalViewMode);
    connect(m_ui->portraitRadioButton, &QRadioButton::toggled,
            this, &MerEmulatorModeDialog::guessOptimalViewMode);

    m_ui->unsupportedLabel->setVisible(!supportsMultipleModels);
    m_ui->contentWrapper->setEnabled(supportsMultipleModels);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(supportsMultipleModels);

    if (m_emulator.data()->connection()->isVirtualMachineOff() || !supportsMultipleModels) {
        m_ui->restartEmulatorCheckBox->setChecked(false);
        m_ui->restartEmulatorCheckBox->setEnabled(false);
    }

    const bool result = (m_dialog->exec() == QDialog::Accepted);
    if (!result)
        goto end;

    if (!m_emulator.data()->connection()->isVirtualMachineOff()
        && m_ui->restartEmulatorCheckBox->isChecked()) {
        m_emulator.data()->connection()->lockDown(true);
    }

    m_emulator.data()->setDeviceModel(m_ui->deviceModelComboBox->currentDeviceModel());
    m_emulator.data()->setOrientation(m_ui->portraitRadioButton->isChecked()
                               ? Qt::Vertical
                               : Qt::Horizontal);
    m_emulator.data()->setViewScaled(m_ui->scaledViewModeRadioButton->isChecked());

    if (m_emulator.data()->connection()->isVirtualMachineOff()
            && m_ui->restartEmulatorCheckBox->isChecked()) {
        m_emulator.data()->connection()->lockDown(false);
        m_emulator.data()->connection()->connectTo();
    }

end:
    delete m_ui, m_ui = 0;
    delete m_dialog;

    return result;
}

void MerEmulatorModeDialog::guessOptimalViewMode()
{
    Q_ASSERT(m_emulator != 0);

    // TODO use the word resolution instead of size
    const QSize desktopSize = qApp->desktop()->availableGeometry().size();

    const QMap<QString, MerEmulatorDeviceModel> models = MerSettings::deviceModels();
    auto selectedModel = models.value(m_ui->deviceModelComboBox->currentDeviceModel());
    QTC_ASSERT(!selectedModel.isNull(), return);

    QSize selectedSize = selectedModel.displayResolution();
    if (m_ui->landscapeRadioButton->isChecked()) {
        selectedSize.transpose();
    }

    const bool tooBig = selectedSize.boundedTo(desktopSize) != selectedSize;

    QRadioButton *viewModeRadioButton = tooBig
        ? m_ui->scaledViewModeRadioButton
        : m_ui->originalViewModeRadioButton;
    viewModeRadioButton->setChecked(true);
}
