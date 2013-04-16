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

#include "merdevice.h"
#include "merdeviceconfigurationwidget.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <remotelinux/remotelinux_constants.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <ssh/sshconnection.h>

#include <QDir>
#include <QTimer>
#include <QProgressDialog>

namespace Mer {
namespace Internal {

class PublicKeyDeploymentDialog : public QProgressDialog
{
    Q_OBJECT
    enum State {
        Idle,
        Deploy
    };

public:
    explicit PublicKeyDeploymentDialog(const QString &vmName,
                                       const QSsh::SshConnectionParameters &params,
                                       QWidget *parent = 0)
        : QProgressDialog(parent)
        , m_state(Idle)
        , m_params(params)
        , m_vmName(vmName)
    {
        setAutoReset(false);
        setAutoClose(false);
        setMinimumDuration(0);
        setMaximum(1);
        setLabelText(tr("Deploying..."));
        setValue(0);

        QTimer::singleShot(0, this, SLOT(updateState()));
    }

private slots:
    void updateState()
    {
        if (m_state == Idle) {
            m_state = Deploy;
            deployKey();
            setValue(1);
            setLabelText(tr("Deployed"));
            setCancelButtonText(tr("Close"));
            QTimer::singleShot(0, this, SLOT(updateState()));
        } else {
            m_state = Idle;
        }
    }

private:
    void deployKey()
    {
        const QString privKeyPath = m_params.privateKeyFile;
        const QString pubKeyPath = privKeyPath + QLatin1String(".pub");
        QString error;
        VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(m_vmName);
        const QString sshDirectoryPath = info.sharedSsh + QLatin1Char('/');
        const QStringList authorizedKeysPaths = QStringList()
                << sshDirectoryPath + QLatin1String("root/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER)
                << sshDirectoryPath + m_params.userName
                   + QLatin1String("/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER);
        foreach (const QString &path, authorizedKeysPaths) {
            const bool success = MerSdkManager::instance()->authorizePublicKey(path, pubKeyPath, error);
            Q_UNUSED(success)
            //TODO: error handling
        }
    }

private:
    QTimer m_timer;
    int m_state;
    const QSsh::SshConnectionParameters m_params;
    const QString m_vmName;
};

MerDevice::Ptr MerDevice::create()
{
    return Ptr(new MerDevice);
}

MerDevice::Ptr MerDevice::create(const QString &name,
                                 Core::Id type,
                                 MachineType machineType,
                                 Origin origin,
                                 Core::Id id)
{
    return Ptr(new MerDevice(name, type, machineType, origin, id));
}

QString MerDevice::displayType() const
{
    return tr("Mer");
}

ProjectExplorer::IDeviceWidget *MerDevice::createWidget()
{
    return new MerDeviceConfigurationWidget(sharedFromThis());
}

MerDevice::MerDevice(const QString &name,
                     Core::Id type,
                     MachineType machineType,
                     Origin origin,
                     Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

MerDevice::MerDevice(const MerDevice &other)
    : RemoteLinux::LinuxDevice(other)
{
}

ProjectExplorer::IDevice::Ptr MerDevice::clone() const
{
    return Ptr(new MerDevice(*this));
}

MerDevice::MerDevice()
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

QList<Core::Id> MerDevice::actionIds() const
{
    QList<Core::Id> ids = LinuxDevice::actionIds();
    if (machineType() == Emulator)
        ids << Core::Id(Constants::MER_EMULATOR_START_ACTION_ID);
    return ids;
}

QString MerDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == RemoteLinux::Constants::GenericDeployKeyToDeviceActionId)
        return tr("Deploy Public Key");
    else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID)
        return tr("Start Emulator");
    return LinuxDevice::displayNameForActionId(actionId);
}

void MerDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    if (machineType() == Emulator) {
        if (actionId == RemoteLinux::Constants::GenericDeployKeyToDeviceActionId) {
            PublicKeyDeploymentDialog dialog(id().toString(), sshParameters(), parent);
            dialog.exec();
            return;
        } else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID) {
            MerVirtualBoxManager::shutVirtualMachine(id().toString());
            return;
        }
    }

    LinuxDevice::executeAction(actionId, parent);
}

#include "merdevice.moc"

} // Internal
} // Mer
