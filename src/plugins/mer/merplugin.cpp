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

#include "merplugin.h"

#include "merbuildconfiguration.h"
#include "merbuildsteps.h"
#include "merconnectionmanager.h"
#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merdeploysteps.h"
#include "merdevicedebugsupport.h"
#include "merdevicefactory.h"
#include "meremulatordevice.h"
#include "meremulatormodedialog.h"
#include "mergeneraloptionspage.h"
#include "mermode.h"
#include "meroptionspage.h"
#include "merqmllivebenchmanager.h"
#include "merqmlrunconfigurationfactory.h"
#include "merqtversionfactory.h"
#include "merrunconfigurationaspect.h"
#include "merrunconfigurationfactory.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "mertoolchainfactory.h"
#include "mervmconnectionui.h"
#include "meremulatormodeoptionspage.h"

#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/kitmanager.h>
#include <remotelinux/remotelinuxcustomrunconfiguration.h>
#include <remotelinux/remotelinuxqmltoolingsupport.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <utils/checkablemessagebox.h>

#include <QCheckBox>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QtPlugin>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Sfdk;
using Utils::CheckableMessageBox;

namespace Mer {
namespace Internal {

namespace {
const char *VM_NAME_PROPERTY = "merVmName";
}

class MerPluginPrivate
{
public:
    MerPluginPrivate()
        : sdk(Sdk::VersionedSettings)
    {
    }

    Sdk sdk;
    MerSdkManager sdkManager;
    MerConnectionManager connectionManager;
    MerOptionsPage optionsPage;
    MerEmulatorModeOptionsPage emulatorModeOptionsPage;
    MerGeneralOptionsPage generalOptionsPage;
    MerDeviceFactory deviceFactory;
    MerEmulatorDeviceManager emulatorDeviceManager;
    MerQtVersionFactory qtVersionFactory;
    MerToolChainFactory toolChainFactory;
    MerMb2RpmBuildConfigurationFactory mb2RpmBuildConfigurationFactory;
    MerRsyncDeployConfigurationFactory rsyncDeployConfigurationFactory;
    MerRpmDeployConfigurationFactory rpmDeployConfigurationFactory;
    MerAddRemoveSpecialDeployStepsProjectListener addRemoveSpecialDeployStepsProjectListener;
    MerRunConfigurationFactory runConfigurationFactory;
    MerQmlRunConfigurationFactory qmlRunConfigurationFactory;
    MerBuildStepFactory<MerSdkStartStep> sdkStartStepFactory;
    MerDeployStepFactory<MerPrepareTargetStep> prepareTargetStepFactory;
    MerDeployStepFactory<MerMb2RsyncDeployStep> mb2RsyncDeployStepFactory;
    MerDeployStepFactory<MerMb2RpmDeployStep> mb2RpmDeployStepFactory;
    MerDeployStepFactory<MerMb2RpmBuildStep> mb2RpmBuildStepFactory;
    MerDeployStepFactory<MerRpmValidationStep> rpmValidationStepFactory;
    MerDeployStepFactory<MerLocalRsyncDeployStep> localRsyncDeployStepFactory;
    MerDeployStepFactory<MerResetAmbienceDeployStep> resetAmbienceDeployStepFactory;
    MerQmlLiveBenchManager qmlLiveBenchManager;
    MerMode mode;
    MerBuildConfigurationFactory buildConfigurationFactory;
};

static MerPluginPrivate *dd = nullptr;

MerPlugin::MerPlugin()
{
}

MerPlugin::~MerPlugin()
{
    delete dd, dd = nullptr;
}

bool MerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    using namespace ProjectExplorer::Constants;

    new MerSettings(this);

    RunConfiguration::registerAspect<MerRunConfigurationAspect>();

    auto constraint = [](RunConfiguration *runConfig) {
        const Core::Id id = runConfig->id();
        return id == Constants::MER_QMLRUNCONFIGURATION
            || id.name().startsWith(Constants::MER_RUNCONFIGURATION_PREFIX);
    };

    RunControl::registerWorker<SimpleTargetRunner>(NORMAL_RUN_MODE, constraint);
    RunControl::registerWorker<MerDeviceDebugSupport>(DEBUG_RUN_MODE, constraint);
    RunControl::registerWorker<RemoteLinuxQmlProfilerSupport>(QML_PROFILER_RUN_MODE, constraint);
    //RunControl::registerWorker<RemoteLinuxPerfSupport>(PERFPROFILER_RUN_MODE, constraint);

    VirtualMachine::registerConnectionUi<MerVmConnectionUi>();

    dd = new MerPluginPrivate;

    Command *emulatorConnectionCommand =
        ActionManager::command(Constants::MER_EMULATOR_CONNECTON_ACTION_ID);
    ModeManager::addAction(emulatorConnectionCommand->action(), 1);

    Command *sdkConnectionCommand =
        ActionManager::command(Constants::MER_SDK_CONNECTON_ACTION_ID);
    ModeManager::addAction(sdkConnectionCommand->action(), 1);

    ActionContainer *toolsMenu =
        ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *menu = ActionManager::createMenu(Constants::MER_TOOLS_MENU);
    menu->menu()->setTitle(tr("&Sailfish OS"));
    toolsMenu->addMenu(menu);

    MerEmulatorModeDialog *emulatorModeDialog = new MerEmulatorModeDialog(this);
    Command *emulatorModeCommand =
        ActionManager::registerAction(emulatorModeDialog->action(),
                                            Constants::MER_EMULATOR_MODE_ACTION_ID,
                                            Context(Core::Constants::C_GLOBAL));
    menu->addAction(emulatorModeCommand);

    QAction *startQmlLiveBenchAction = new QAction(tr("Start &QmlLive Bench..."), this);
    Command *startQmlLiveBenchCommand =
        ActionManager::registerAction(startQmlLiveBenchAction,
                                      Constants::MER_START_QML_LIVE_BENCH_ACTION_ID,
                                      Context(Core::Constants::C_GLOBAL));
    connect(startQmlLiveBenchAction, &QAction::triggered,
            MerQmlLiveBenchManager::startBench);
    menu->addAction(startQmlLiveBenchCommand);

    return true;
}

void MerPlugin::extensionsInitialized()
{
    // Do not enable updates untils devices and kits are loaded. We know devices
    // are loaded prior to kits.
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            Sdk::instance(), Sdk::enableUpdates);

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &MerPlugin::saveSettings);
}

IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    m_stopList.clear();
    for (BuildEngine *const engine : Sdk::buildEngines()) {
        VirtualMachine *const virtualMachine = engine->virtualMachine();
        bool headless = false;
        if (!virtualMachine->isOff(&headless) && headless) {
            QMessageBox *prompt = new QMessageBox(
                    QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("The headless virtual machine \"%1\" is still running.\n\n"
                        "Close the virtual machine now?").arg(virtualMachine->name()),
                    QMessageBox::Yes | QMessageBox::No,
                    ICore::mainWindow());
            prompt->setCheckBox(new QCheckBox(CheckableMessageBox::msgDoNotAskAgain()));
            prompt->setProperty(VM_NAME_PROPERTY, virtualMachine->name());
            connect(prompt, &QMessageBox::finished,
                    this, &MerPlugin::handlePromptClosed);
            m_stopList.insert(virtualMachine->name(), virtualMachine);
            if (MerSettings::isAskBeforeClosingVmEnabled()) {
                prompt->open();
            } else {
                QTimer::singleShot(0, prompt, [prompt] { prompt->done(QMessageBox::Yes); });
            }
        }
    }
    if(m_stopList.isEmpty())
        return SynchronousShutdown;
    else
        return AsynchronousShutdown;
}

void MerPlugin::saveSettings()
{
    QStringList errorStrings;
    Sdk::saveSettings(&errorStrings);
    for (const QString &errorString : errorStrings) {
        // See Utils::PersistentSettingsWriter::save()
        QMessageBox::critical(ICore::dialogParent(),
                QCoreApplication::translate("Utils::FileSaverBase", "File Error"),
                errorString);
    }
}

void MerPlugin::handlePromptClosed(int result)
{
    QMessageBox *prompt = qobject_cast<QMessageBox *>(sender());
    prompt->deleteLater();

    QString vm = prompt->property(VM_NAME_PROPERTY).toString();

    if (prompt->checkBox() && prompt->checkBox()->isChecked())
        MerSettings::setAskBeforeClosingVmEnabled(false);

    if (result == QMessageBox::Yes) {
        VirtualMachine *virtualMachine = m_stopList.value(vm);
        connect(virtualMachine, &VirtualMachine::stateChanged,
                this, &MerPlugin::handleConnectionStateChanged);
        connect(virtualMachine, &VirtualMachine::lockDownFailed,
                this, &MerPlugin::handleLockDownFailed);
        virtualMachine->lockDown(true);
    } else {
        m_stopList.remove(vm);
    }

    if(m_stopList.isEmpty()) {
        emit asynchronousShutdownFinished();
    }
}

void MerPlugin::handleConnectionStateChanged()
{
    VirtualMachine *virtualMachine = qobject_cast<VirtualMachine *>(sender());

    if (virtualMachine->state() == VirtualMachine::Disconnected) {
        m_stopList.remove(virtualMachine->name());

        if (m_stopList.isEmpty()) {
            emit asynchronousShutdownFinished();
        }
    }
}

void MerPlugin::handleLockDownFailed()
{
    VirtualMachine *virtualMachine = qobject_cast<VirtualMachine *>(sender());

    m_stopList.remove(virtualMachine->name());

    if (m_stopList.isEmpty()) {
        emit asynchronousShutdownFinished();
    }
}

} // Internal
} // Mer
