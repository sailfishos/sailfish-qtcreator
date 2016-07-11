/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "meremulatordevice.h"

#include "merconnection.h"
#include "merconstants.h"
#include "meremulatordevicetester.h"
#include "meremulatordevicewidget.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTextStream>
#include <QTimer>

#include <functional>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const int VIDEO_MODE_DEPTH = 32;
const int SCALE_DOWN_FACTOR = 2;
}

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
        QTimer::singleShot(0, this, &PublicKeyDeploymentDialog::updateState);
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
            QTimer::singleShot(1, this, &PublicKeyDeploymentDialog::updateState);
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
            QTimer::singleShot(0, this, &PublicKeyDeploymentDialog::updateState);
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
            QTimer::singleShot(0, this, &PublicKeyDeploymentDialog::updateState);
            break;
        case Deploy: {
            m_state = Idle;
            const QString pubKeyPath = m_privKeyPath + QLatin1String(".pub");
            if(m_sharedPath.isEmpty()) {
                m_state = Error;
                m_error.append(tr("SharedPath for emulator not found for this device"));
                QTimer::singleShot(0, this, &PublicKeyDeploymentDialog::updateState);
                return;
            }

            if(!MerSdkManager::authorizePublicKey(m_sshDirectoryPath, pubKeyPath, m_error)) {
                 m_state = Error;
            }else{
                setValue(3);
                setLabelText(tr("Deployed"));
                setCancelButtonText(tr("Close"));
            }
            QTimer::singleShot(0, this, &PublicKeyDeploymentDialog::updateState);
            break;
        }
        case Error:
            QMessageBox::critical(ICore::dialogParent(), tr("Cannot Authorize Keys"), m_error);
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


IDevice::Ptr MerEmulatorDevice::clone() const
{
    return Ptr(new MerEmulatorDevice(*this));
}

MerEmulatorDevice::MerEmulatorDevice():
    MerDevice(QString(), Emulator, ManuallyAdded, Core::Id())
    , m_connection(new MerConnection(0 /* not bug */))
    , m_orientation(Qt::Vertical)
    , m_viewScaled(false)
{
#if __cplusplus >= 201103L
    m_virtualMachineChangedConnection =
        QObject::connect(m_connection.data(), &MerConnection::virtualMachineChanged,
                         std::bind(&MerEmulatorDevice::updateAvailableDeviceModels, this));
#endif
}

MerEmulatorDevice::MerEmulatorDevice(const MerEmulatorDevice &other):
    MerDevice(other)
    , m_connection(other.m_connection)
    , m_mac(other.m_mac)
    , m_subnet(other.m_subnet)
    , m_sharedConfigPath(other.m_sharedConfigPath)
    , m_deviceModel(other.m_deviceModel)
    , m_availableDeviceModels(other.m_availableDeviceModels)
    , m_orientation(other.m_orientation)
    , m_viewScaled(other.m_viewScaled)
{
#if __cplusplus >= 201103L
    m_virtualMachineChangedConnection =
        QObject::connect(m_connection.data(), &MerConnection::virtualMachineChanged,
                         std::bind(&MerEmulatorDevice::updateAvailableDeviceModels, this));
#endif
}

MerEmulatorDevice::~MerEmulatorDevice()
{
#if __cplusplus >= 201103L
    QObject::disconnect(m_virtualMachineChangedConnection);
#endif
}

IDeviceWidget *MerEmulatorDevice::createWidget()
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

void MerEmulatorDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(parent);
    QTC_ASSERT(actionIds().contains(actionId), return);

    if (actionId ==  Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID) {
        generateSshKey(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
        generateSshKey(QLatin1String(Constants::MER_DEVICE_ROOTUSER));
        return;
    } else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID) {
        m_connection->connectTo();
        return;
    }
}

DeviceTester *MerEmulatorDevice::createDeviceTester() const
{
    return new MerEmulatorDeviceTester;
}

void MerEmulatorDevice::fromMap(const QVariantMap &map)
{
    MerDevice::fromMap(map);
    updateConnection();
    m_connection->setVirtualMachine(
            map.value(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE)).toString());
    m_mac = map.value(QLatin1String(Constants::MER_DEVICE_MAC)).toString();
    m_subnet = map.value(QLatin1String(Constants::MER_DEVICE_SUBNET)).toString();
    m_sharedConfigPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG)).toString();
    m_deviceModel = map.value(QLatin1String(Constants::MER_DEVICE_DEVICE_MODEL)).toString();
    m_orientation = static_cast<Qt::Orientation>(
            map.value(QLatin1String(Constants::MER_DEVICE_ORIENTATION), Qt::Vertical).toInt());
    m_viewScaled = map.value(QLatin1String(Constants::MER_DEVICE_VIEW_SCALED)).toBool();
}

QVariantMap MerEmulatorDevice::toMap() const
{
    QVariantMap map = MerDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE),
            m_connection->virtualMachine());
    map.insert(QLatin1String(Constants::MER_DEVICE_MAC), m_mac);
    map.insert(QLatin1String(Constants::MER_DEVICE_SUBNET), m_subnet);
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG), m_sharedConfigPath);
    map.insert(QLatin1String(Constants::MER_DEVICE_DEVICE_MODEL), m_deviceModel);
    map.insert(QLatin1String(Constants::MER_DEVICE_ORIENTATION), m_orientation);
    map.insert(QLatin1String(Constants::MER_DEVICE_VIEW_SCALED), m_viewScaled);
    return map;
}

Abi::Architecture MerEmulatorDevice::architecture() const
{
    return Abi::X86Architecture;
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
    m_connection->setVirtualMachine(machineName);
}

QString MerEmulatorDevice::virtualMachine() const
{
    return m_connection->virtualMachine();
}

void MerEmulatorDevice::setSharedConfigPath(const QString &configPath)
{
    m_sharedConfigPath = configPath;
}

QString MerEmulatorDevice::sharedConfigPath() const
{
    return m_sharedConfigPath;
}

void MerEmulatorDevice::generateSshKey(const QString& user) const
{
    if(!m_sharedConfigPath.isEmpty()) {
        QString index(QLatin1String("/ssh/private_keys/%1/"));
        //TODO fix me:
        QString privateKeyFile = m_sharedConfigPath +
                index.arg(virtualMachine()).replace(QLatin1Char(' '), QLatin1Char('_')) + user;
        PublicKeyDeploymentDialog dialog(privateKeyFile, virtualMachine(),
                                         user, sharedSshPath(), ICore::dialogParent());

        connection()->setAutoConnectEnabled(false);
        dialog.exec();
        connection()->setAutoConnectEnabled(true);
    }
}

SshConnectionParameters MerEmulatorDevice::sshParametersForUser(const SshConnectionParameters &sshParams, const QLatin1String &user) const
{
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    //TODO fix me:
    QString privateKeyFile = sharedConfigPath()  +
            index.arg(virtualMachine()).replace(QLatin1Char(' '), QLatin1Char('_')) + user;
    SshConnectionParameters m_sshParams = sshParams;
    m_sshParams.userName = user;
    m_sshParams.privateKeyFile = privateKeyFile;

    return m_sshParams;
}

QMap<QString, QMap<QString, QString> > MerEmulatorDevice::availableDeviceModels() const
{
#if __cplusplus < 201103L
    const_cast<MerEmulatorDevice *>(this)->updateAvailableDeviceModels();
#endif
    return m_availableDeviceModels;
}

QSize MerEmulatorDevice::getDeviceModelResolution(const QString &deviceModel) const
{
    if (availableDeviceModels().contains(deviceModel)) {
        const QMap<QString, QString> device = availableDeviceModels().value(deviceModel);
        return QSize(device.value(QStringLiteral("hres")).toInt(), device.value(QStringLiteral("vres")).toInt());
    }

    return QSize();
}

QString MerEmulatorDevice::deviceModel() const
{
    return m_deviceModel;
}

void MerEmulatorDevice::setDeviceModel(const QString &deviceModel)
{
    QTC_CHECK(availableDeviceModels().contains(deviceModel));
    QTC_CHECK(!virtualMachine().isEmpty());

    // Intentionally do not check for equal value - better for synchronization
    // with VB settings

    m_deviceModel = deviceModel;

    setVideoMode();
}

Qt::Orientation MerEmulatorDevice::orientation() const
{
    return m_orientation;
}

void MerEmulatorDevice::setOrientation(Qt::Orientation orientation)
{
    // Intentionally do not check for equal value - better for synchronization
    // with VB settings

    m_orientation = orientation;

    setVideoMode();
}

bool MerEmulatorDevice::isViewScaled() const
{
    return m_viewScaled;
}

void MerEmulatorDevice::setViewScaled(bool viewScaled)
{
    // Intentionally do not check for equal value - better for synchronization
    // with VB settings

    m_viewScaled = viewScaled;

    setVideoMode();
}

MerConnection *MerEmulatorDevice::connection() const
{
    return m_connection.data();
}

void MerEmulatorDevice::updateConnection()
{
    m_connection->setSshParameters(sshParameters());
}

/*
 * The list of available device models is associated with each emulator VM via `vboxmanage
 * setextradata`.
 *
 * Storage format:
 *
 * IT MUST START WITH AN EMPTY LINE! This is to deal with vboxmanage prepending output with
 * (possibly localized) string "Value: ".
 *
 * CSV data, with headline in form '#V<version>'.
 *
 * Columns:
 * - Device model (string)
 * - Horizontal screen resolution, px (number)
 * - Vertical screen resolution, px (number)
 * - Horizontal screen size, mm (number)
 * - Vertical screen size, mm (number)
 *
 * ----example-begin----
 * \n
 * #V2\n
 * #model,hres(px),vres(px),hsize(mm),vsize(mm)\n
 * Jolla Phone,540,960,57,100\n
 * Jolla Tablet,1536,2048,120,160\n
 * ----end----
 */
void MerEmulatorDevice::updateAvailableDeviceModels()
{
    const QString data = MerVirtualBoxManager::getExtraData(virtualMachine(),
            QLatin1String(Constants::MER_DEVICE_MODELS_KEY));
    QStringList lines = data.split(QLatin1Char('\n'));
    if (lines.count() < 3) {
        qWarning() << "Invalid device models configuration - at least three lines expected";
        return;
    }

    // vboxmanage output starts with (possibly localized) string "Value: " :/
    lines.removeFirst();

    const QString header = lines.takeFirst();
    if (header != QStringLiteral("#V2")) {
        if (header.startsWith(QStringLiteral("#V"))) {
            qWarning() << "Using an old version of the device models configuration -" << header;
        } else {
            qWarning() << "Invalid device models configuration - invalid header";
            return;
        }
    }

    foreach (const QString &line, lines) {
        if (line.trimmed().isEmpty() || line.trimmed().startsWith(QStringLiteral("#"))) {
            continue;
        }

        const QStringList fields = line.split(QLatin1Char(','));
        if (fields.count() < 3) {
            qWarning() << "Invalid device models configuration - incorrect fields count";
            return;
        }

        const QString name = fields.at(0);

        QMap<QString, QString> device;
        if (fields.at(1).toInt() && fields.at(2).toInt()) {
            device.insert(QStringLiteral("hres"), fields.at(1));
            device.insert(QStringLiteral("vres"), fields.at(2));
        } else {
            qWarning() << "Invalid device models configuration - invalid data (device screen resolution)";
            return;
        }
        if (fields.count() == 5) {
            if (fields.at(3).toInt() && fields.at(4).toInt()) {
                device.insert(QStringLiteral("hsize"), fields.at(3));
                device.insert(QStringLiteral("vsize"), fields.at(4));
            } else {
                qWarning() << "Invalid device models configuration - invalid data (device screen size)";
            }
        }

        m_availableDeviceModels.insert(name, device);
    }

    if (!m_availableDeviceModels.isEmpty()) {
        if (!m_availableDeviceModels.contains(m_deviceModel)) {
            m_deviceModel = m_availableDeviceModels.keys().first();
        }
    } else {
        m_deviceModel = QString();
    }
}

void MerEmulatorDevice::setVideoMode()
{
    QMap<QString, QString> device = availableDeviceModels().value(m_deviceModel);
    QSize realSize = getDeviceModelResolution(m_deviceModel);
    if (m_orientation == Qt::Horizontal) {
        realSize.transpose();
    }
    QSize virtualSize = realSize;
    if (m_viewScaled) {
        realSize /= SCALE_DOWN_FACTOR;
    }

    MerVirtualBoxManager::setVideoMode(virtualMachine(), realSize, VIDEO_MODE_DEPTH);

    //! \todo Does not support multiple emulators (not supported at other places anyway).
    QTC_ASSERT(!sharedConfigPath().isEmpty(), return);
    const QString file = QDir(sharedConfigPath())
        .absoluteFilePath(QLatin1String(Constants::MER_COMPOSITOR_CONFIG_FILENAME));
    FileSaver saver(file, QIODevice::WriteOnly);

    // Keep environment clean if scaling is disabled - may help in case of errors
    const QString maybeCommentOut = m_viewScaled ? QString() : QString(QStringLiteral("#"));

    QTextStream(saver.file())
        << maybeCommentOut << "QT_QPA_EGLFS_SCALE=" << SCALE_DOWN_FACTOR << endl
        << "QT_QPA_EGLFS_WIDTH=" << virtualSize.width() << endl
        << "QT_QPA_EGLFS_HEIGHT=" << virtualSize.height() << endl;

    if (device.contains(QStringLiteral("hsize")) && device.contains(QStringLiteral("vsize"))) {
        QTextStream(saver.file())
            << "QT_QPA_EGLFS_PHYSICAL_WIDTH=" << device.value(QStringLiteral("hsize")).toInt() << endl
            << "QT_QPA_EGLFS_PHYSICAL_HEIGHT=" << device.value(QStringLiteral("vsize")).toInt() << endl;
    }

    bool ok = saver.finalize();
    QTC_CHECK(ok);
}

#include "meremulatordevice.moc"

}
}
