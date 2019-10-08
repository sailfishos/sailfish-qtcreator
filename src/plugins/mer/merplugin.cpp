/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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
#include "merconnection.h"
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
#include "merrpmpackagingstep.h"
#include "merrunconfigurationaspect.h"
#include "merrunconfigurationfactory.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "mertoolchainfactory.h"
#include "meruploadandinstallrpmsteps.h"
#include "mervirtualboxmanager.h"
#include "meremulatormodeoptionspage.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
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
using Utils::CheckableMessageBox;

namespace Mer {
namespace Internal {

namespace {
const char *VM_NAME_PROPERTY = "merVmName";
}

class MerPluginPrivate
{
public:
    MerSdkManager sdkManager;
    MerVirtualBoxManager virtualBoxManager;
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
    MerDeployStepFactory<MerRpmPackagingStep> rpmPackagingStepFactory;
    MerDeployStepFactory<MerRpmValidationStep> rpmValidationStepFactory;
    MerDeployStepFactory<MerUploadAndInstallRpmStep> uploadAndInstallRpmStepFactory;
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

    MerConnection::registerUi<MerConnectionWidgetUi>();

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
}

IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    m_stopList.clear();
    QList<MerSdk*> sdks = MerSdkManager::sdks();
    foreach(const MerSdk* sdk, sdks) {
        MerConnection *connection = sdk->connection();
        bool headless = false;
        if (!connection->isVirtualMachineOff(&headless) && headless) {
            QMessageBox *prompt = new QMessageBox(
                    QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("The headless virtual machine \"%1\" is still running.\n\n"
                        "Close the virtual machine now?").arg(connection->virtualMachine()),
                    QMessageBox::Yes | QMessageBox::No,
                    ICore::mainWindow());
            prompt->setCheckBox(new QCheckBox(CheckableMessageBox::msgDoNotAskAgain()));
            prompt->setProperty(VM_NAME_PROPERTY, connection->virtualMachine());
            connect(prompt, &QMessageBox::finished,
                    this, &MerPlugin::handlePromptClosed);
            m_stopList.insert(connection->virtualMachine(), connection);
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

void MerPlugin::handlePromptClosed(int result)
{
    QMessageBox *prompt = qobject_cast<QMessageBox *>(sender());
    prompt->deleteLater();

    QString vm = prompt->property(VM_NAME_PROPERTY).toString();

    if (prompt->checkBox() && prompt->checkBox()->isChecked())
        MerSettings::setAskBeforeClosingVmEnabled(false);

    if (result == QMessageBox::Yes) {
        MerConnection *connection = m_stopList.value(vm);
        connect(connection, &MerConnection::stateChanged,
                this, &MerPlugin::handleConnectionStateChanged);
        connect(connection, &MerConnection::lockDownFailed,
                this, &MerPlugin::handleLockDownFailed);
        connection->lockDown(true);
    } else {
        m_stopList.remove(vm);
    }

    if(m_stopList.isEmpty()) {
        emit asynchronousShutdownFinished();
    }
}

void MerPlugin::handleConnectionStateChanged()
{
    MerConnection *connection = qobject_cast<MerConnection *>(sender());

    if (connection->state() == MerConnection::Disconnected) {
        m_stopList.remove(connection->virtualMachine());

        if (m_stopList.isEmpty()) {
            emit asynchronousShutdownFinished();
        }
    }
}

void MerPlugin::handleLockDownFailed()
{
    MerConnection *connection = qobject_cast<MerConnection *>(sender());

    m_stopList.remove(connection->virtualMachine());

    if (m_stopList.isEmpty()) {
        emit asynchronousShutdownFinished();
    }
}

} // Internal
} // Mer
