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

#include "merconnectionmanager.h"
#include "merconnection.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "mervirtualboxmanager.h"
#include "meremulatordevice.h"
#include "mersdkkitinformation.h"
#include "merconnectionprompt.h"

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/buildmanager.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QIcon>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerConnectionManager *MerConnectionManager::m_instance = 0;
ProjectExplorer::Project *MerConnectionManager::m_project = 0;

MerConnectionManager::MerConnectionManager():
    m_emulatorConnection(new MerConnection()),
    m_sdkConnection(new MerConnection())
{
    QIcon emuIcon;
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-run.png"));
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-stop.png"), QSize(), QIcon::Normal,
                    QIcon::On);
    QIcon sdkIcon;
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-run.png"));
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-stop.png"), QSize(), QIcon::Normal, QIcon::On);

    m_emulatorConnection->setName(tr("Emulator"));
    m_emulatorConnection->setId(Constants::MER_EMULATOR_CONNECTON_ACTION_ID);
    m_emulatorConnection->setIcon(emuIcon);
    m_emulatorConnection->setStartTip(tr("Start Emulator"));
    m_emulatorConnection->setStopTip(tr("Stop Emulator"));
    m_emulatorConnection->setConnectingTip(tr("Connecting..."));
    m_emulatorConnection->setDisconnectingTip(tr("Disconnecting..."));
    m_emulatorConnection->setClosingTip(tr("Closing Emulator"));
    m_emulatorConnection->setStartingTip(tr("Starting Emulator"));
    m_emulatorConnection->setTaskCategory(Core::Id(Constants::MER_TASKHUB_EMULATOR_CATEGORY));
    m_emulatorConnection->setProbeTimeout(1000);
    m_emulatorConnection->initialize();

    m_sdkConnection->setName(tr("MerSdk"));
    m_sdkConnection->setId(Constants::MER_SDK_CONNECTON_ACTION_ID);
    m_sdkConnection->setIcon(sdkIcon);
    m_sdkConnection->setStartTip(tr("Start SDK"));
    m_sdkConnection->setStopTip(tr("Stop SDK"));
    m_sdkConnection->setConnectingTip(tr("Connecting..."));
    m_sdkConnection->setDisconnectingTip(tr("Disconnecting..."));
    m_sdkConnection->setClosingTip(tr("Closing SDK"));
    m_sdkConnection->setStartingTip(tr("Starting SDK"));
    m_sdkConnection->setTaskCategory(Core::Id(Constants::MER_TASKHUB_SDK_CATEGORY));
    m_sdkConnection->setProbeTimeout(1000);
    m_sdkConnection->initialize();

    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            SLOT(handleKitUpdated(ProjectExplorer::Kit*)));

    SessionManager *session = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            SLOT(handleStartupProjectChanged(ProjectExplorer::Project*)));
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), this, SLOT(update()));
    connect(DeviceManager::instance(), SIGNAL(deviceListChanged()), SLOT(update()));

    ProjectExplorerPlugin *projectExplorer = ProjectExplorerPlugin::instance();
    connect(projectExplorer->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(handleBuildStateChanged(ProjectExplorer::Project*)));

    m_instance = this;
}

MerConnectionManager::~MerConnectionManager()
{
    m_instance = 0;
}

MerConnectionManager* MerConnectionManager::instance()
{
    Q_ASSERT(m_instance);
    return m_instance;
}

bool MerConnectionManager::isConnected(const QString &vmName) const
{
    if(m_emulatorConnection->virtualMachine() == vmName) {
        return m_emulatorConnection->isConnected();
    }else if(m_sdkConnection->virtualMachine() == vmName) {
        return m_sdkConnection->isConnected();
    }
    return false;
}

QSsh::SshConnectionParameters MerConnectionManager::parameters(const MerSdk *sdk)
{
    QSsh::SshConnectionParameters params;
    params.userName = sdk->userName();
    params.host = sdk->host();
    params.port = sdk->sshPort();
    params.privateKeyFile = sdk->privateKeyFile();
    params.timeout = sdk->timeout();
    return params;
}

void MerConnectionManager::handleKitUpdated(Kit *kit)
{
    if (MerSdkManager::isMerKit(kit))
        update();
}

void MerConnectionManager::handleStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (project != m_project && project) {
        // handle all target related changes, add, remove, etc...
        connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                SLOT(update()));
    }

    if (m_project) {
        disconnect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        disconnect(m_project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        disconnect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(update()));
    }

    m_project = project;
    update();
    m_emulatorConnection->tryConnectTo();
    m_sdkConnection->tryConnectTo();
}

void MerConnectionManager::handleTargetAdded(Target *target)
{
    if (target && MerSdkManager::isMerKit(target->kit()))
        update();
}

void MerConnectionManager::handleTargetRemoved(Target *target)
{
    if (target && MerSdkManager::isMerKit(target->kit()))
        update();
}

void MerConnectionManager::handleBuildStateChanged(Project* project)
{
     Target* target = project->activeTarget();
     ProjectExplorerPlugin *projectExplorer = ProjectExplorerPlugin::instance();
     if (target && target->kit() && MerSdkManager::isMerKit(target->kit())) {
         if (projectExplorer->buildManager()->isBuilding(project)) {
             if(m_sdkConnection->isConnected()) return;
             const QString vm = m_sdkConnection->virtualMachine();
             QTC_ASSERT(!vm.isEmpty(),return);
             if (!MerVirtualBoxManager::isVirtualMachineRunning(vm)) {
                 MerConnectionPrompt *connectioPrompt = new MerConnectionPrompt(vm);
                 connectioPrompt->prompt(MerConnectionPrompt::Start);
             } else {
                 m_sdkConnection->connectTo();
             }
         }
     }
}

void MerConnectionManager::update()
{
    bool sdkRemoteButtonEnabled = false;
    bool emulatorRemoteButtonEnabled = false;
    bool sdkRemoteButtonVisible = false;
    bool emulatorRemoteButtonVisible = false;

    const Project* const p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (p) {
        const Target* const activeTarget = p->activeTarget();
        const QList<Target *> targets =  p->targets();

        foreach (const Target *t , targets) {
            if (MerSdkManager::isMerKit(t->kit())) {
                sdkRemoteButtonVisible = true;
                if (t == activeTarget) {
                    sdkRemoteButtonEnabled = true;
                    MerSdk* sdk = MerSdkKitInformation::sdk(t->kit());
                    QTC_ASSERT(sdk, continue);
                    QString sdkName = sdk->virtualMachineName();
                    QTC_ASSERT(!sdkName.isEmpty(), continue);
                    m_sdkConnection->setVirtualMachine(sdkName);
                    m_sdkConnection->setHeadless(sdk->isHeadless());
                    m_sdkConnection->setConnectionParameters(parameters(sdk));
                    m_sdkConnection->setupConnection();
                }
            }

            if (MerSdkManager::hasMerDevice(t->kit())) {
                emulatorRemoteButtonVisible = true;
                if (t == activeTarget) {
                    emulatorRemoteButtonEnabled =
                            DeviceTypeKitInformation::deviceTypeId(t->kit()) == Core::Id(Constants::MER_DEVICE_TYPE_I486);
                    if(emulatorRemoteButtonEnabled) {
                          const IDevice::ConstPtr device = DeviceKitInformation::device(t->kit());
                          const MerEmulatorDevice* emu = static_cast<const MerEmulatorDevice*>(device.data());
                          const QString &emulatorName = emu->virtualMachine();
                          m_emulatorConnection->setVirtualMachine(emulatorName);
                          m_emulatorConnection->setConnectionParameters(device->sshParameters());
                          m_emulatorConnection->setupConnection();
                    }
                }
            }
        }
    }

    m_emulatorConnection->setEnabled(emulatorRemoteButtonEnabled);
    m_emulatorConnection->setVisible(emulatorRemoteButtonVisible);
    m_sdkConnection->setEnabled(sdkRemoteButtonEnabled);
    m_sdkConnection->setVisible(sdkRemoteButtonVisible);
    m_emulatorConnection->update();
    m_sdkConnection->update();
}

QString MerConnectionManager::testConnection(const QSsh::SshConnectionParameters &params) const
{
    QSsh::SshConnectionParameters p = params;
    QSsh::SshConnection connection(p);
    QEventLoop loop;
    connect(&connection, SIGNAL(connected()), &loop, SLOT(quit()));
    connect(&connection, SIGNAL(error(QSsh::SshError)), &loop, SLOT(quit()));
    connect(&connection, SIGNAL(disconnected()), &loop, SLOT(quit()));
    connection.connectToHost();
    loop.exec();
    QString result;
    if (connection.errorState() != QSsh::SshNoError)
        result = connection.errorString();
    else
        result = tr("Connected");
    return result;
}

void MerConnectionManager::connectTo(const QString &vmName)
{
    if(m_emulatorConnection->virtualMachine() == vmName) {
        m_emulatorConnection->connectTo();
    }else if(m_sdkConnection->virtualMachine() == vmName) {
        m_sdkConnection->connectTo();
    }
}

void MerConnectionManager::disconnectFrom(const QString &vmName)
{
    if(m_emulatorConnection->virtualMachine() == vmName) {
        m_emulatorConnection->disconnectFrom();
    }else if(m_sdkConnection->virtualMachine() == vmName) {
        m_sdkConnection->disconnectFrom();
    }
}

}
}
