/****************************************************************************
**
** Copyright (C) 2012 - 2018 Jolla Ltd.
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
#include "merdevicefactory.h"
#include "meremulatordevicetester.h"
#include "meremulatordevicewidget.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTextStream>
#include <QTimer>

#include <functional>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

using namespace Constants;

namespace {
const int VIDEO_MODE_DEPTH = 32;
const int SCALE_DOWN_FACTOR = 2;

FileName globalSettingsFileName()
{
    QSettings *globalSettings = PluginManager::globalSettings();
    return FileName::fromString(QFileInfo(globalSettings->fileName()).absolutePath()
                                + QLatin1String(MER_DEVICE_MODELS_FILENAME));
}

} // namespace anonymous

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


MerEmulatorDevice::Ptr MerEmulatorDevice::create(Core::Id id)
{
    return Ptr(new MerEmulatorDevice(id));
}


IDevice::Ptr MerEmulatorDevice::clone() const
{
    return Ptr(new MerEmulatorDevice(*this));
}

MerEmulatorDevice::MerEmulatorDevice(Core::Id id)
    : MerDevice(QString(), Emulator, ManuallyAdded, id)
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

    if (m_setVideoModeTimer && m_setVideoModeTimer->isActive())
        setVideoMode();

    delete m_setVideoModeTimer;
}

IDeviceWidget *MerEmulatorDevice::createWidget()
{
    return new MerEmulatorDeviceWidget(sharedFromThis());
}

QList<Core::Id> MerEmulatorDevice::actionIds() const
{
    QList<Core::Id> ids;
    ids << Core::Id(Constants::MER_EMULATOR_START_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_STOP_ACTION_ID);
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
    else if (actionId == Constants::MER_EMULATOR_STOP_ACTION_ID)
        return tr("Stop Emulator");
    return QString();
}

void MerEmulatorDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(parent);
    QTC_ASSERT(actionIds().contains(actionId), return);

    // Cancel any unsaved changes to SSH/QmlLive ports or it will blow up
    MerEmulatorDeviceManager::restorePorts(sharedFromThis().staticCast<MerEmulatorDevice>());

    if (actionId ==  Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID) {
        generateSshKey(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
        generateSshKey(QLatin1String(Constants::MER_DEVICE_ROOTUSER));
        return;
    } else if (actionId == Constants::MER_EMULATOR_START_ACTION_ID) {
        m_connection->connectTo();
        return;
    } else if (actionId == Constants::MER_EMULATOR_STOP_ACTION_ID) {
        m_connection->disconnectFrom();
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
                index.arg(id().toString()).replace(QLatin1Char(' '), QLatin1Char('_')) + user;
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
            index.arg(id().toString()).replace(QLatin1Char(' '), QLatin1Char('_')) + user;
    SshConnectionParameters m_sshParams = sshParams;
    m_sshParams.setUserName(user);
    m_sshParams.privateKeyFile = privateKeyFile;

    return m_sshParams;
}

QMap<QString, MerEmulatorDeviceModel> MerEmulatorDevice::availableDeviceModels() const
{
#if __cplusplus < 201103L
    const_cast<MerEmulatorDevice *>(this)->updateAvailableDeviceModels();
#endif
    return m_availableDeviceModels;
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

    QVariantMap fullDeviceModelData = readFullDeviceModelData();
    updateDconfDb(fullDeviceModelData);
    scheduleSetVideoMode();
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

    scheduleSetVideoMode();
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

    scheduleSetVideoMode();
}

MerConnection *MerEmulatorDevice::connection() const
{
    return m_connection.data();
}

// Hack, all clones share the connection
void MerEmulatorDevice::updateConnection() const
{
    m_connection->setSshParameters(sshParameters());
}

void MerEmulatorDevice::updateAvailableDeviceModels()
{
    //! \todo Does not support multiple (different) emulators (not supported at other places anyway).
    PersistentSettingsReader reader;
    if (!reader.load(globalSettingsFileName()))
        return;

    QVariantMap data = reader.restoreValues();

    int version = data.value(QLatin1String(MER_DEVICE_MODELS_FILE_VERSION_KEY), 0).toInt();
    if (version < 1) {
        qWarning() << "Invalid configuration version: " << version;
        return;
    }

    int count = data.value(QLatin1String(MER_DEVICE_MODELS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_DEVICE_MODELS_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap deviceModelData = data.value(key).toMap();
        MerEmulatorDeviceModel deviceModel;
        deviceModel.fromMap(deviceModelData);

        m_availableDeviceModels.insert(deviceModel.name(), deviceModel);
    }

    if (!m_availableDeviceModels.isEmpty()) {
        if (!m_availableDeviceModels.contains(m_deviceModel)) {
            m_deviceModel = m_availableDeviceModels.keys().first();
        }
    } else {
        m_deviceModel = QString();
    }
}

void MerEmulatorDevice::scheduleSetVideoMode()
{
    // this is not a QObject
    if (!m_setVideoModeTimer) {
        m_setVideoModeTimer = new QTimer;
        m_setVideoModeTimer->setSingleShot(true);
        QObject::connect(m_setVideoModeTimer.data(), &QTimer::timeout, [this]() {
            setVideoMode();
            m_setVideoModeTimer->deleteLater();
        });
    }
    m_setVideoModeTimer->start(0);
}

void MerEmulatorDevice::setVideoMode()
{
    MerEmulatorDeviceModel deviceModel = availableDeviceModels().value(m_deviceModel);
    QTC_ASSERT(!deviceModel.isNull(), return);
    QSize realResolution = deviceModel.displayResolution();
    if (m_orientation == Qt::Horizontal) {
        realResolution.transpose();
    }
    QSize virtualResolution = realResolution;
    if (m_viewScaled) {
        realResolution /= SCALE_DOWN_FACTOR;
    }

    MerVirtualBoxManager::setVideoMode(virtualMachine(), realResolution, VIDEO_MODE_DEPTH);

    //! \todo Does not support multiple emulators (not supported at other places anyway).
    QTC_ASSERT(!sharedConfigPath().isEmpty(), return);
    const QString file = QDir(sharedConfigPath())
        .absoluteFilePath(QLatin1String(Constants::MER_COMPOSITOR_CONFIG_FILENAME));
    FileSaver saver(file, QIODevice::WriteOnly);

    // Keep environment clean if scaling is disabled - may help in case of errors
    const QString maybeCommentOut = m_viewScaled ? QString() : QString(QStringLiteral("#"));

    QTextStream(saver.file())
        << maybeCommentOut << "QT_QPA_EGLFS_SCALE=" << SCALE_DOWN_FACTOR << endl
        << "QT_QPA_EGLFS_WIDTH=" << virtualResolution.width() << endl
        << "QT_QPA_EGLFS_HEIGHT=" << virtualResolution.height() << endl
        << "QT_QPA_EGLFS_PHYSICAL_WIDTH=" << deviceModel.displaySize().width() << endl
        << "QT_QPA_EGLFS_PHYSICAL_HEIGHT=" << deviceModel.displaySize().height() << endl;

    bool ok = saver.finalize();
    QTC_CHECK(ok);
}

void MerEmulatorDevice::updateDconfDb(const QVariantMap &fullDeviceModelData)
{
    if (fullDeviceModelData.isEmpty()) // allow chaining
        return;

    //! \todo Does not support multiple emulators (not supported at other places anyway).
    QTC_ASSERT(!sharedConfigPath().isEmpty(), return);
    const QString file = QDir(sharedConfigPath())
        .absoluteFilePath(QLatin1String(Constants::MER_EMULATOR_DCONF_DB_FILENAME));
    FileSaver saver(file, QIODevice::WriteOnly);

    QTextStream(saver.file())
        << fullDeviceModelData.value(QLatin1String(Constants::MER_DEVICE_MODEL_DCONF_DB)).toString();

    bool ok = saver.finalize();
    QTC_CHECK(ok);
}

QVariantMap MerEmulatorDevice::readFullDeviceModelData() const
{
    PersistentSettingsReader reader;
    if (!reader.load(globalSettingsFileName()))
        return QVariantMap();

    QVariantMap data = reader.restoreValues();

    int version = data.value(QLatin1String(MER_DEVICE_MODELS_FILE_VERSION_KEY), 0).toInt();
    if (version < 1) {
        qWarning() << "Invalid configuration version: " << version;
        return QVariantMap();
    }

    QString contents;

    int count = data.value(QLatin1String(MER_DEVICE_MODELS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_DEVICE_MODELS_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap deviceModelData = data.value(key).toMap();

        if (deviceModelData.value(QLatin1String(MER_DEVICE_MODEL_NAME)).toString() == m_deviceModel)
            return deviceModelData;
    }

    qWarning() << "Device model data not found for" << m_deviceModel;
    return QVariantMap();
}

void MerEmulatorDeviceModel::fromMap(const QVariantMap &map)
{
    d->name = map.value(QLatin1String(MER_DEVICE_MODEL_NAME)).toString();
    d->displayResolution = map.value(QLatin1String(MER_DEVICE_MODEL_DISPLAY_RESOLUTION)).toRect().size();
    d->displaySize = map.value(QLatin1String(MER_DEVICE_MODEL_DISPLAY_SIZE)).toRect().size();
}

QVariantMap MerEmulatorDeviceModel::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(MER_DEVICE_MODEL_NAME), d->name);
    map.insert(QLatin1String(MER_DEVICE_MODEL_DISPLAY_RESOLUTION), QRect(QPoint(0, 0), d->displayResolution));
    map.insert(QLatin1String(MER_DEVICE_MODEL_DISPLAY_SIZE), QRect(QPoint(0, 0), d->displaySize));

    return map;
}

/*!
 * \class MerEmulatorDeviceManager
 *
 * The original purpose of this class was just to deal with the fact that IDevice is not a QObject
 * and no substitute for property notification signals exist in DeviceManager API; it notifies just
 * by emitting deviceListReplaced when changes are applied in options. More issues have been
 * identified later with comments elsewhere.
 */

MerEmulatorDeviceManager *MerEmulatorDeviceManager::s_instance = 0;

MerEmulatorDeviceManager::MerEmulatorDeviceManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    const int deviceCount = DeviceManager::instance()->deviceCount();
    for (int i = 0; i < deviceCount; ++i)
        onDeviceAdded(DeviceManager::instance()->deviceAt(i)->id());
    connect(MerDeviceFactory::instance(), &MerDeviceFactory::deviceCreated,
            this, &MerEmulatorDeviceManager::onDeviceCreated);
    connect(DeviceManager::instance(), &DeviceManager::deviceAdded,
            this, &MerEmulatorDeviceManager::onDeviceAdded);
    connect(DeviceManager::instance(), &DeviceManager::deviceRemoved,
            this, &MerEmulatorDeviceManager::onDeviceRemoved);
    connect(DeviceManager::instance(), &DeviceManager::deviceListReplaced,
            this, &MerEmulatorDeviceManager::onDeviceListReplaced);
}

MerEmulatorDeviceManager *MerEmulatorDeviceManager::instance()
{
    QTC_CHECK(s_instance);
    return s_instance;
}

MerEmulatorDeviceManager::~MerEmulatorDeviceManager()
{
    s_instance = 0;
}

bool MerEmulatorDeviceManager::isStored(const MerEmulatorDevice::ConstPtr &device)
{
    Q_ASSERT(device);

    if (!s_instance->m_deviceSshPortCache.contains(device->id()))
        return false;
    if (!s_instance->m_deviceQmlLivePortsCache.contains(device->id()))
        return false;

    return true;
}

/*
 * DeviceManager implements device modification via options by cloning the list of devices, letting
 * IDeviceWidget instances work on the cloned IDevice instances and then replacing the original
 * devices with the modified clones.  Unfortunately for some reason (bug) on subsequent apply (while
 * the options dialog remains open) the original instance and the instance used by the IDeviceWidget
 * happen to be the same object, so it is no more possible to restore original values by reading
 * properties of the original instance. That's why this function exists.
 */
bool MerEmulatorDeviceManager::restorePorts(const MerEmulatorDevice::Ptr &device)
{
    Q_ASSERT(device);

    if (!isStored(device))
        return false;

    quint16 savedSshPort = s_instance->m_deviceSshPortCache.value(device->id());
    Utils::PortList savedQmlLivePorts = s_instance->m_deviceQmlLivePortsCache.value(device->id());

    auto sshParameters = device->sshParameters();
    sshParameters.setPort(savedSshPort);
    device->setSshParameters(sshParameters);

    device->setQmlLivePorts(savedQmlLivePorts);

    return true;
}

void MerEmulatorDeviceManager::onDeviceCreated(const ProjectExplorer::IDevice::Ptr &device)
{
    auto merEmulator = device.dynamicCast<MerEmulatorDevice>();
    if (!merEmulator)
        return;

    merEmulator->updateConnection();
}

void MerEmulatorDeviceManager::onDeviceAdded(Core::Id id)
{
    IDevice::ConstPtr device = DeviceManager::instance()->find(id);
    auto merEmulator = device.dynamicCast<const MerEmulatorDevice>();
    if (!merEmulator)
        return;

    merEmulator->updateConnection();

    QTC_CHECK(!m_deviceSshPortCache.contains(id));
    m_deviceSshPortCache.insert(id, merEmulator->sshParameters().port());
    QTC_CHECK(!m_deviceQmlLivePortsCache.contains(id));
    m_deviceQmlLivePortsCache.insert(id, merEmulator->qmlLivePorts());

    emit storedDevicesChanged();
}

void MerEmulatorDeviceManager::onDeviceRemoved(Core::Id id)
{
    m_deviceSshPortCache.remove(id);
    m_deviceQmlLivePortsCache.remove(id);

    emit storedDevicesChanged();
}

void MerEmulatorDeviceManager::onDeviceListReplaced()
{
    const auto oldSshPortsCache = m_deviceSshPortCache;
    m_deviceSshPortCache.clear();
    const auto oldQmlLivePortsCache = m_deviceQmlLivePortsCache;
    m_deviceQmlLivePortsCache.clear();

    const int deviceCount = DeviceManager::instance()->deviceCount();
    for (int i = 0; i < deviceCount; ++i) {
        auto merEmulator = DeviceManager::instance()->deviceAt(i).dynamicCast<const MerEmulatorDevice>();
        if (!merEmulator)
            continue;

        const quint16 nowSshPort = merEmulator->sshParameters().port();
        if (nowSshPort != oldSshPortsCache.value(merEmulator->id())) {
            MerVirtualBoxManager::updateEmulatorSshPort(merEmulator->virtualMachine(), nowSshPort);
            merEmulator->updateConnection();
        }
        m_deviceSshPortCache.insert(merEmulator->id(), nowSshPort);

        const Utils::PortList nowQmlLivePorts = merEmulator->qmlLivePorts();
        if (nowQmlLivePorts.toString() != oldQmlLivePortsCache.value(merEmulator->id()).toString())
            MerVirtualBoxManager::updateEmulatorQmlLivePorts(merEmulator->virtualMachine(),
                    merEmulator->qmlLivePortsList());
        m_deviceQmlLivePortsCache.insert(merEmulator->id(), nowQmlLivePorts);
    }

    emit storedDevicesChanged();
}

#include "meremulatordevice.moc"

}
}
