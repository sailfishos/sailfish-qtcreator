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

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <ssh/sshconnection.h>

#include <QIcon>
#include <QMessageBox>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerConnectionManager *MerConnectionManager::m_instance = 0;
ProjectExplorer::Project *MerConnectionManager::m_project = 0;

MerConnectionManager::MerConnectionManager():
    m_emulatorConnection(new MerRemoteConnection()),
    m_sdkConnection(new MerRemoteConnection())
{
    QIcon emuIcon;
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-run.png"));
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-stop.png"), QSize(), QIcon::Normal,
                    QIcon::On);
    QIcon sdkIcon;
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-run.png"));
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-stop.png"), QSize(), QIcon::Normal, QIcon::On);

    m_emulatorConnection->setName(tr("Emulator"));
    m_emulatorConnection->setIcon(emuIcon);
    m_emulatorConnection->setStartTip(tr("Start Emulator"));
    m_emulatorConnection->setStopTip(tr("Stop Emulator"));
    m_emulatorConnection->initialize();

    m_sdkConnection->setName(tr("MerSdk"));
    m_sdkConnection->setIcon(sdkIcon);
    m_sdkConnection->setStartTip(tr("Start Sdk"));
    m_sdkConnection->setStopTip(tr("Stop Sdk"));
    m_sdkConnection->initialize();

    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            SLOT(handleKitUpdated(ProjectExplorer::Kit*)));

    SessionManager *session = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            SLOT(handleStartupProjectChanged(ProjectExplorer::Project*)));
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), this, SLOT(update()));
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

bool MerConnectionManager::isConnected(const QSsh::SshConnectionParameters &params) const
{
    if (m_emulatorConnection->parameters() == params)
        return m_emulatorConnection->isConnected();
    if (m_sdkConnection->parameters() == params)
        return m_sdkConnection->isConnected();
    return false;
}

QSsh::SshConnectionParameters MerConnectionManager::paramters(const MerSdk *sdk)
{
    QSsh::SshConnectionParameters params;
    params.userName = sdk->userName();
    params.host = sdk->host();
    params.port = sdk->sshPort();
    params.privateKeyFile = sdk->privateKeyFile();
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

void MerConnectionManager::update()
{
    bool sdkRemoteButonEnabled = false;
    bool emulatorRemoteButtonEnabled = false;
    bool sdkRemoteButonVisible = false;
    bool emulatorRemoteButtonVisible = false;

    const Project* const p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (p) {
        const Target* const activeTarget = p->activeTarget();
        const QList<Target *> targets =  p->targets();

        foreach (const Target *t , targets) {
            if (MerSdkManager::isMerKit(t->kit())) {
                sdkRemoteButonVisible = true;
                if (t == activeTarget) {
                    sdkRemoteButonEnabled = true;
                    const QString &sdkName = MerSdkManager::virtualMachineNameForKit(t->kit());
                    if (!sdkName.isEmpty()) {
                        m_sdkConnection->setVirtualMachine(sdkName);
                        m_sdkConnection->setSshParameters(paramters(MerSdkManager::instance()->sdk(sdkName)));
                    }
                }
            }

            if (MerSdkManager::hasMerDevice(t->kit())) {
                emulatorRemoteButtonVisible = true;
                if (t == activeTarget) {
                    const IDevice::ConstPtr device = DeviceKitInformation::device(t->kit());
                    emulatorRemoteButtonEnabled =
                            device && device->machineType() == IDevice::Emulator;
                    const QString &emulatorName = device->id().toString(); //TODO: refactor me
                    m_emulatorConnection->setVirtualMachine(emulatorName);
                    m_emulatorConnection->setSshParameters(device->sshParameters());
                }
            }
        }
    }

    m_emulatorConnection->setEnabled(emulatorRemoteButtonEnabled);
    m_emulatorConnection->setVisible(emulatorRemoteButtonVisible);
    m_sdkConnection->setEnabled(sdkRemoteButonEnabled);
    m_sdkConnection->setVisible(sdkRemoteButonVisible);
    m_emulatorConnection->update();
    m_sdkConnection->update();
}

QString MerConnectionManager::testConnection(const QSsh::SshConnectionParameters &params) const
{
    QSsh::SshConnectionParameters p = params;
    p.timeout = 24; //TODO: magic number
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

}
}
