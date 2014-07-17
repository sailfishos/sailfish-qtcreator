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
        RemoveOldKeys,
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
        setMaximum(3);
        QString title(tr("Generating new ssh key for %1").arg(user));
        setWindowTitle(title);
        setLabelText(title);
        m_sshDirectoryPath = m_sharedPath + QLatin1Char('/') + m_user+ QLatin1Char('/')
                 + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER);
        QTimer::singleShot(0, this, SLOT(updateState()));
    }

private slots:
    void updateState()
    {
        switch (m_state) {
        case Init:
            m_state = RemoveOldKeys;
            setLabelText(tr("Removing old keys for %1 ...").arg(m_user));
            setCancelButtonText(tr("Close"));
            setValue(0);
            show();
            QTimer::singleShot(1, this, SLOT(updateState()));
            break;
        case RemoveOldKeys:
            m_state = GenerateSsh;
               setLabelText(tr("Generating ssh key for %1 ...").arg(m_user));
            if(QFileInfo(m_privKeyPath).exists()) {
                QFile(m_privKeyPath).remove();
            }

            if(QFileInfo(m_sshDirectoryPath).exists()) {
                QFile(m_sshDirectoryPath).remove();
            }
            setValue(1);
            QTimer::singleShot(0, this, SLOT(updateState()));
            break;
        case GenerateSsh:
            m_state = Deploy;
            m_error.clear();
            setValue(2);
            setLabelText(tr("Deploying key for %1 ...").arg(m_user));

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
            if(m_sharedPath.isEmpty()) {
                m_state = Error;
                m_error.append(tr("SharedPath for emulator not found for this device"));
                QTimer::singleShot(0, this, SLOT(updateState()));
                return;
            }

            if(!MerSdkManager::authorizePublicKey(m_sshDirectoryPath, pubKeyPath, m_error)) {
                 m_state = Error;
            }else{
                setValue(3);
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
    QString m_sshDirectoryPath;
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
    RemoteLinux::LinuxDevice(QString(), Core::Id(Constants::MER_DEVICE_TYPE_I486), Emulator, ManuallyAdded, Core::Id())
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
    return ids;
}

QString MerEmulatorDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID)
        return tr("Regenerate SSH Keys");
    else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID)
        return tr("Start Emulator");
    return QString();
}

void MerEmulatorDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    Q_UNUSED(parent);
    QTC_ASSERT(actionIds().contains(actionId), return);

    if (actionId ==  Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID) {
        generateSshKey(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
        generateSshKey(QLatin1String(Constants::MER_DEVICE_ROOTUSER));
        return;
    } else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID) {
        MerVirtualBoxManager::startVirtualMachine(m_virtualMachine,false);
        return;
    }
}

void MerEmulatorDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_virtualMachine = map.value(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE)).toString();
    m_mac = map.value(QLatin1String(Constants::MER_DEVICE_MAC)).toString();
    m_subnet = map.value(QLatin1String(Constants::MER_DEVICE_SUBNET)).toString();
    m_sharedSshPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_SSH)).toString();
    m_sharedConfigPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG)).toString();
}

QVariantMap MerEmulatorDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE), m_virtualMachine);
    map.insert(QLatin1String(Constants::MER_DEVICE_MAC), m_mac);
    map.insert(QLatin1String(Constants::MER_DEVICE_SUBNET), m_subnet);
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

void MerEmulatorDevice::generateSshKey(const QString& user) const
{
    if(!m_sharedConfigPath.isEmpty()) {
        QString index(QLatin1String("/ssh/private_keys/%1/"));
        //TODO fix me:
        QString privateKeyFile = m_sharedConfigPath +
                index.arg(virtualMachine()).replace(QLatin1String(" "),QLatin1String("_")) + user;
        PublicKeyDeploymentDialog dialog(privateKeyFile, virtualMachine(),
                                         user, sharedSshPath());
        dialog.exec();
    }
}

QSsh::SshConnectionParameters MerEmulatorDevice::sshParametersForUser(const QSsh::SshConnectionParameters &sshParams, const QLatin1String &user) const
{
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    //TODO fix me:
    QString privateKeyFile = sharedConfigPath()  +
            index.arg(virtualMachine()).replace(QLatin1String(" "),QLatin1String("_")) + user;
    QSsh::SshConnectionParameters m_sshParams = sshParams;
    m_sshParams.userName = user;
    m_sshParams.privateKeyFile = privateKeyFile;

    return m_sshParams;
}

#include "meremulatordevice.moc"

}
}
