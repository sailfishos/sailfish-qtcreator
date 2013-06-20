#include "meremulatordevice.h"

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

#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"
#include "merconstants.h"
#include "meremulatordevicewidget.h"
#include <utils/qtcassert.h>
#include <remotelinux/linuxdevicetestdialog.h>

#include <QProgressDialog>
#include <QMessageBox>
#include <QTimer>
#include <QFileInfo>

namespace Mer {
namespace Internal {

class PublicKeyDeploymentDialog : public QProgressDialog
{
    Q_OBJECT
    enum State {
        Init,
        GenerateSsh,
        Deploy,
        Error,
        Idle
    };

public:
    explicit PublicKeyDeploymentDialog(const QString &privKeyPath, const QString& vmName, const QString& user, const QString& sharedPath,
                                       QWidget *parent = 0)
        : QProgressDialog(parent)
        , m_state(Init)
        , m_privKeyPath(privKeyPath)
        , m_vmName(vmName)
        , m_user(user)
        , m_sharedPath(sharedPath)
    {
        setAutoReset(false);
        setAutoClose(false);
        setMinimumDuration(0);
        setMaximum(2);
        QTimer::singleShot(0, this, SLOT(updateState()));
    }

private slots:
    void updateState()
    {
        switch (m_state) {
        case Init:
            m_state = GenerateSsh;
            setLabelText(tr("Regenerating ssh keys..."));
            setCancelButtonText(tr("Close"));
            setValue(0);
            show();
            QTimer::singleShot(0, this, SLOT(updateState()));
            break;
        case GenerateSsh:
            m_state = Deploy;
            m_error.clear();
            setValue(1);
            setLabelText(tr("Generate key..."));
            setCancelButtonText(tr("Close"));

            if (!QFileInfo(m_privKeyPath).exists()) {
                if (!MerSdkManager::generateSshKey(m_privKeyPath, m_error)) {
                m_state = Error;
                }
            }
            QTimer::singleShot(0, this, SLOT(updateState()));
            break;
        case Deploy: {
            m_state = Idle;
            const QString pubKeyPath = m_privKeyPath + QLatin1String(".pub");
            setLabelText(tr("Deploying.."));


            if(m_sharedPath.isEmpty()) {
                m_state = Error;
                m_error.append(tr("SharedPath for emulator not found for this device"));
                QTimer::singleShot(0, this, SLOT(updateState()));
                return;
            }
            const QString sshDirectoryPath = m_sharedPath + QLatin1Char('/') + m_user+ QLatin1Char('/')
                     + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER);

            if(!MerSdkManager::authorizePublicKey(sshDirectoryPath, pubKeyPath, m_error)) {
                 m_state = Error;
            }else{
                setValue(2);
                setLabelText(tr("Deployed"));
                setCancelButtonText(tr("Close"));
            }
            QTimer::singleShot(0, this, SLOT(updateState()));
            break;
        }
        case Error:
            QMessageBox::critical(this, tr("Cannot Authorize Keys"), m_error);
            setValue(0);
            setLabelText(tr("Error occured"));
            setCancelButtonText(tr("Close"));
        case Idle:
            close();
        default:
            break;
        }
    }

private:
    QTimer m_timer;
    int m_state;
    QString m_privKeyPath;
    QString m_vmName;
    QString m_error;
    QString m_user;
    QString m_sharedPath;
};


MerEmulatorDevice::Ptr MerEmulatorDevice::create()
{
    return Ptr(new MerEmulatorDevice());
}


ProjectExplorer::IDevice::Ptr MerEmulatorDevice::clone() const
{
    return Ptr(new MerEmulatorDevice(*this));
}

MerEmulatorDevice::MerEmulatorDevice():
    IDevice(Constants::MER_DEVICE_TYPE_I486, IDevice::ManuallyAdded, IDevice::Emulator)
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

QString MerEmulatorDevice::displayType() const
{
    return tr("Mer Emulator");
}

ProjectExplorer::IDeviceWidget *MerEmulatorDevice::createWidget()
{
    return new MerEmulatorDeviceWidget(sharedFromThis());
}

QList<Core::Id> MerEmulatorDevice::actionIds() const
{
    QList<Core::Id> ids;
    ids << Core::Id(Constants::MER_EMULATOR_START_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_TEST_ID);
    return ids;
}

QString MerEmulatorDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID)
        return tr("Regenerate SshKeys");
    else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID)
        return tr("Start Emulator");
    else if (actionId == Constants::MER_EMULATOR_TEST_ID)
        return tr("Test Emulator");
    return QString();
}

void MerEmulatorDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    QTC_ASSERT(actionIds().contains(actionId), return);

    if (actionId ==  Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID) {
        PublicKeyDeploymentDialog dialog(sshParameters().privateKeyFile, virtualMachine(),
                                         sshParameters().userName, sharedSshPath(),parent);
        dialog.exec();
        return;
    } else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID) {
        MerVirtualBoxManager::shutVirtualMachine(m_virtualMachine);
        return;
    } else if (actionId == Constants::MER_EMULATOR_TEST_ID) {
        RemoteLinux::LinuxDeviceTestDialog dialog(sharedFromThis(), new RemoteLinux::GenericLinuxDeviceTester, parent);
        dialog.exec();
        return;
    }
}

void MerEmulatorDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_virtualMachine = map.value(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE)).toString();
    m_sdkName = map.value(QLatin1String(Constants::MER_DEVICE_MER_SDK)).toString();
    m_mac = map.value(QLatin1String(Constants::MER_DEVICE_MAC)).toString();
    m_subnet = map.value(QLatin1String(Constants::MER_DEVICE_SUBNET)).toString();
    m_index = map.value(QLatin1String(Constants::MER_DEVICE_INDEX)).toInt();
    m_sharedSshPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_SSH)).toString();
    m_sharedConfigPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG)).toString();
}

QVariantMap MerEmulatorDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE), m_virtualMachine);
    map.insert(QLatin1String(Constants::MER_DEVICE_MER_SDK), m_sdkName);
    map.insert(QLatin1String(Constants::MER_DEVICE_MAC), m_mac);
    map.insert(QLatin1String(Constants::MER_DEVICE_SUBNET), m_subnet);
    map.insert(QLatin1String(Constants::MER_DEVICE_INDEX), m_index);
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_SSH), m_sharedSshPath);
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG), m_sharedConfigPath);
    return map;
}

void MerEmulatorDevice::setMac(const QString& mac)
{
    m_mac = mac;
}

QString MerEmulatorDevice::mac() const
{
    return m_mac;
}

void MerEmulatorDevice::setSubnet(const QString& subnet)
{
    m_subnet = subnet;
}

QString MerEmulatorDevice::subnet() const
{
    return m_subnet;
}

void MerEmulatorDevice::setVirtualMachine(const QString& machineName)
{
    m_virtualMachine = machineName;
}

QString MerEmulatorDevice::virtualMachine() const
{
    return m_virtualMachine;
}

void MerEmulatorDevice::setSdkName(const QString& sdkName)
{
    m_sdkName = sdkName;
}

QString MerEmulatorDevice::sdkName() const
{
    return m_sdkName;
}

void MerEmulatorDevice::setIndex(int index)
{
     m_index = index;
}

int MerEmulatorDevice::index() const
{
    return m_index;
}

void MerEmulatorDevice::setSharedConfigPath(const QString &configPath)
{
    m_sharedConfigPath = configPath;
}

QString MerEmulatorDevice::sharedConfigPath() const
{
    return m_sharedConfigPath;
}

void MerEmulatorDevice::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerEmulatorDevice::sharedSshPath() const
{
    return m_sharedSshPath;
}

void MerEmulatorDevice::generteSshKey(const QString& user)
{
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    QString privateKeyFile = m_sharedConfigPath + index.arg(m_index) + user;
    PublicKeyDeploymentDialog dialog(privateKeyFile, virtualMachine(),
                                     user, sharedSshPath());
    dialog.exec();
}


#include "meremulatordevice.moc"

}
}
