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
#include "mersettings.h"
#include "mervirtualboxmanager.h"
#include "meremulatormodedialog.h"

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/persistentsettings.h>

#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

using namespace Constants;

namespace {
const int VIDEO_MODE_DEPTH = 32;
const int SCALE_DOWN_FACTOR = 2;
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
            close();
            break;
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
    , m_memorySizeMb(0)
    , m_cpuCount(0)
    , m_vdiCapacityMb(0)
{
}

MerEmulatorDevice::MerEmulatorDevice(const MerEmulatorDevice &other):
    MerDevice(other)
    , m_connection(other.m_connection)
    , m_factorySnapshot(other.m_factorySnapshot)
    , m_mac(other.m_mac)
    , m_subnet(other.m_subnet)
    , m_sharedConfigPath(other.m_sharedConfigPath)
    , m_deviceModel(other.m_deviceModel)
    , m_orientation(other.m_orientation)
    , m_viewScaled(other.m_viewScaled)
    , m_memorySizeMb(other.m_memorySizeMb)
    , m_cpuCount(other.m_cpuCount)
    , m_vdiCapacityMb(other.m_vdiCapacityMb)
{
}

MerEmulatorDevice::~MerEmulatorDevice()
{
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
    ids << Core::Id(Constants::MER_EMULATOR_CHANGE_MODE_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_START_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_STOP_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_DEPLOYKEY_ACTION_ID);
    ids << Core::Id(Constants::MER_EMULATOR_FACTORY_RESET_ACTION_ID);
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
    else if (actionId == Constants::MER_EMULATOR_CHANGE_MODE_ACTION_ID)
        return tr(Constants::MER_EMULATOR_MODE_ACTION_NAME);
    else if (actionId == Constants::MER_EMULATOR_FACTORY_RESET_ACTION_ID)
        return tr("Factory Reset...");
    return QString();
}

void MerEmulatorDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(parent);
    QTC_ASSERT(actionIds().contains(actionId), return);

    // Cancel any unsaved changes to SSH/QmlLive ports, memory size, cpu count and vdi size or it will blow up
    MerEmulatorDeviceManager::restoreSystemSettings(sharedFromThis().staticCast<MerEmulatorDevice>());

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
    } else if (actionId == Constants::MER_EMULATOR_CHANGE_MODE_ACTION_ID) {
        const IDevice::ConstPtr device = DeviceManager::instance()->find(id());
        const MerEmulatorDevice::Ptr merDevice = device
                ? device.dynamicCast<const MerEmulatorDevice>().constCast<MerEmulatorDevice>()
                : sharedFromThis().dynamicCast<MerEmulatorDevice>();

        QTC_ASSERT(merDevice, return);

        MerEmulatorModeDialog dialog(merDevice);
        if (!dialog.exec())
            return;
        // Set device model in copy of MerEmulatorDevice (in case of Apply pressed)
        m_deviceModel = merDevice->deviceModel();
        return;
    } else if (actionId == Constants::MER_EMULATOR_FACTORY_RESET_ACTION_ID) {
        if (factorySnapshot().isEmpty()) {
            QMessageBox::warning(parent, tr("No factory snapshot set"),
                    tr("No factory snapshot is configured. Cannot reset to the factory state"));
            return;
        }
        QMessageBox::StandardButton button = QMessageBox::question(parent,
                tr("Reset emulator?"),
                tr("Do you really want to reset '%1' to the factory state?").arg(virtualMachine()),
                QMessageBox::Yes | QMessageBox::No);
        if (button == QMessageBox::Yes)
            doFactoryReset(parent);
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
    m_factorySnapshot = map.value(QLatin1String(Constants::MER_DEVICE_FACTORY_SNAPSHOT)).toString();
    m_mac = map.value(QLatin1String(Constants::MER_DEVICE_MAC)).toString();
    m_subnet = map.value(QLatin1String(Constants::MER_DEVICE_SUBNET)).toString();
    m_sharedConfigPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG)).toString();
    m_deviceModel = map.value(QLatin1String(Constants::MER_DEVICE_DEVICE_MODEL)).toString();
    m_orientation = static_cast<Qt::Orientation>(
            map.value(QLatin1String(Constants::MER_DEVICE_ORIENTATION), Qt::Vertical).toInt());
    m_viewScaled = map.value(QLatin1String(Constants::MER_DEVICE_VIEW_SCALED)).toBool();
    m_memorySizeMb = map.value(QLatin1String(MEMORY_SIZE_MB)).toInt();
    m_cpuCount = map.value(QLatin1String(CPU_COUNT)).toInt();
    m_vdiCapacityMb = map.value(QLatin1String(VDI_CAPACITY_MB)).toInt();
}

QVariantMap MerEmulatorDevice::toMap() const
{
    QVariantMap map = MerDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_VIRTUAL_MACHINE),
            m_connection->virtualMachine());
    map.insert(QLatin1String(Constants::MER_DEVICE_FACTORY_SNAPSHOT), m_factorySnapshot);
    map.insert(QLatin1String(Constants::MER_DEVICE_MAC), m_mac);
    map.insert(QLatin1String(Constants::MER_DEVICE_SUBNET), m_subnet);
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_CONFIG), m_sharedConfigPath);
    map.insert(QLatin1String(Constants::MER_DEVICE_DEVICE_MODEL), m_deviceModel);
    map.insert(QLatin1String(Constants::MER_DEVICE_ORIENTATION), m_orientation);
    map.insert(QLatin1String(Constants::MER_DEVICE_VIEW_SCALED), m_viewScaled);
    map.insert(QLatin1String(Constants::MEMORY_SIZE_MB), m_memorySizeMb);
    map.insert(QLatin1String(Constants::CPU_COUNT), m_cpuCount);
    map.insert(QLatin1String(Constants::VDI_CAPACITY_MB), m_vdiCapacityMb);
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

void MerEmulatorDevice::setMemorySizeMb(int sizeMb)
{
    m_memorySizeMb = sizeMb;
}

int MerEmulatorDevice::memorySizeMb() const
{
    return m_memorySizeMb;
}

void MerEmulatorDevice::setVdiCapacityMb(int valueMb)
{
    m_vdiCapacityMb = valueMb;
}

int MerEmulatorDevice::vdiCapacityMb() const
{
    return m_vdiCapacityMb;
}

void MerEmulatorDevice::setCpuCount(int count)
{
    m_cpuCount = count;
}

int MerEmulatorDevice::cpuCount() const
{
    return m_cpuCount;
}

void MerEmulatorDevice::setVirtualMachine(const QString& machineName)
{
    m_connection->setVirtualMachine(machineName);
}

QString MerEmulatorDevice::virtualMachine() const
{
    return m_connection->virtualMachine();
}

void MerEmulatorDevice::setFactorySnapshot(const QString &snapshotName)
{
    m_factorySnapshot = snapshotName;
}

QString MerEmulatorDevice::factorySnapshot() const
{
    return m_factorySnapshot;
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

QString MerEmulatorDevice::deviceModel() const
{
    return m_deviceModel;
}

void MerEmulatorDevice::setDeviceModel(const QString &deviceModel)
{
    QTC_CHECK(MerSettings::deviceModels().contains(deviceModel));
    QTC_CHECK(!virtualMachine().isEmpty());

    // Intentionally do not check for equal value - better for synchronization
    // with VB settings

    m_deviceModel = deviceModel;

    updateDconfDb();
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

void MerEmulatorDevice::doFactoryReset(QWidget *parent)
{
    QProgressDialog progress(tr("Restoring '%1' to factory state...").arg(virtualMachine()),
            tr("Abort"), 0, 0, parent);
    progress.setMinimumDuration(2000);
    progress.setWindowModality(Qt::WindowModal);

    if (!m_connection->lockDown(true)) {
        progress.cancel();
        QMessageBox::warning(parent, tr("Failed"),
                tr("Failed to close the virtual machine. Factory state cannot be restored."));
        return;
    }

    QEventLoop loop;
    MerVirtualBoxManager::restoreSnapshot(virtualMachine(), factorySnapshot(), &loop, [&loop](bool ok) {
        loop.exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
    });
    if (loop.exec() != EXIT_SUCCESS) {
        progress.cancel();
        QMessageBox::warning(parent, tr("Failed"), tr("Failed to restore factory state."));
        return;
    }

    m_connection->lockDown(false);

    progress.cancel();

    QMessageBox::information(parent, tr("Factory state restored"),
            tr("Successfully restored '%1' to the factory state").arg(virtualMachine()));
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
    const MerEmulatorDeviceModel deviceModel = MerSettings::deviceModels().value(m_deviceModel);
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

void MerEmulatorDevice::updateDconfDb()
{
    const MerEmulatorDeviceModel deviceModel = MerSettings::deviceModels().value(m_deviceModel);
    QTC_ASSERT(!deviceModel.isNull(), return);

    if (deviceModel.dconf().isEmpty()) // allow chaining
        return;

    //! \todo Does not support multiple emulators (not supported at other places anyway).
    QTC_ASSERT(!sharedConfigPath().isEmpty(), return);
    const QString file = QDir(sharedConfigPath())
        .absoluteFilePath(QLatin1String(Constants::MER_EMULATOR_DCONF_DB_FILENAME));
    FileSaver saver(file, QIODevice::WriteOnly);

    QTextStream(saver.file()) << deviceModel.dconf();

    bool ok = saver.finalize();
    QTC_CHECK(ok);
}

MerEmulatorDeviceModel::MerEmulatorDeviceModel(const QString &name, const QSize &displayResolution, const QSize &displaySize,
                                               const QString &dconf, bool isSdkProvided)
    : d(new Data)
{
    d->name = name;
    d->displayResolution = displayResolution;
    d->displaySize = displaySize;
    d->dconf = dconf;
    d->isSdkProvided = isSdkProvided;
}

MerEmulatorDeviceModel::MerEmulatorDeviceModel(bool isSdkProvided)
    : d(new Data)
{
    d->isSdkProvided = isSdkProvided;
}

void MerEmulatorDeviceModel::fromMap(const QVariantMap &map)
{
    d->name = map.value(QLatin1String(MER_DEVICE_MODEL_NAME)).toString();
    d->displayResolution = map.value(QLatin1String(MER_DEVICE_MODEL_DISPLAY_RESOLUTION)).toRect().size();
    d->displaySize = map.value(QLatin1String(MER_DEVICE_MODEL_DISPLAY_SIZE)).toRect().size();
    d->dconf = map.value(QLatin1String(MER_DEVICE_MODEL_DCONF_DB)).toString();
}

QVariantMap MerEmulatorDeviceModel::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(MER_DEVICE_MODEL_NAME), d->name);
    map.insert(QLatin1String(MER_DEVICE_MODEL_DISPLAY_RESOLUTION), QRect(QPoint(0, 0), d->displayResolution));
    map.insert(QLatin1String(MER_DEVICE_MODEL_DISPLAY_SIZE), QRect(QPoint(0, 0), d->displaySize));
    map.insert(QLatin1String(MER_DEVICE_MODEL_DCONF_DB), d->dconf);

    return map;
}

bool MerEmulatorDeviceModel::operator==(const MerEmulatorDeviceModel &other) const
{
    return name() == other.name()
            && displayResolution() == other.displayResolution()
            && displaySize() == other.displaySize()
            && dconf() == other.dconf()
            && isSdkProvided() == other.isSdkProvided();
}

bool MerEmulatorDeviceModel::operator!=(const MerEmulatorDeviceModel &other) const
{
    return !(operator==(other));
}

QString MerEmulatorDeviceModel::uniqueName(const QString &baseName, const QSet<QString> &existingNames)
{
    if (!existingNames.contains(baseName))
        return baseName;

    const QString pattern = QStringLiteral("%1 (%2)");
    int num = 1;
    while (existingNames.contains(pattern.arg(baseName).arg(num)))
        ++num;

    return pattern.arg(baseName).arg(num);
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
    connect(MerSettings::instance(), &MerSettings::deviceModelsChanged,
            this, [this](const QSet<QString> &deviceModelNames) {
        m_editedDeviceModels = deviceModelNames;
        onDeviceModelsChanged(m_editedDeviceModels);
    });
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
    if (!s_instance->m_deviceMemorySizeCache.contains(device->id()))
        return false;
    if (!s_instance->m_deviceCpuCountCache.contains(device->id()))
        return false;
    if (!s_instance->m_vdiCapacityCache.contains(device->id()))
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
bool MerEmulatorDeviceManager::restoreSystemSettings(const MerEmulatorDevice::Ptr &device)
{
    Q_ASSERT(device);

    if (!isStored(device))
        return false;

    int savedMemoryMb = s_instance->m_deviceMemorySizeCache.value(device->id());
    device->setMemorySizeMb(savedMemoryMb);
    int savedCpu = s_instance->m_deviceCpuCountCache.value(device->id());
    device->setCpuCount(savedCpu);
    int savedVdiCapacityMb = s_instance->m_vdiCapacityCache.value(device->id());
    device->setVdiCapacityMb(savedVdiCapacityMb);

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
    QTC_CHECK(!m_deviceMemorySizeCache.contains(id));
    m_deviceMemorySizeCache.insert(id, merEmulator->memorySizeMb());
    QTC_CHECK(!m_deviceCpuCountCache.contains(id));
    m_deviceCpuCountCache.insert(id, merEmulator->cpuCount());
    QTC_CHECK(!m_vdiCapacityCache.contains(id));
    m_vdiCapacityCache.insert(id, merEmulator->vdiCapacityMb());

    emit storedDevicesChanged();
}

void MerEmulatorDeviceManager::onDeviceRemoved(Core::Id id)
{
    m_deviceSshPortCache.remove(id);
    m_deviceQmlLivePortsCache.remove(id);
    m_deviceMemorySizeCache.remove(id);
    m_deviceCpuCountCache.remove(id);
    m_vdiCapacityCache.remove(id);

    emit storedDevicesChanged();
}

void MerEmulatorDeviceManager::onDeviceModelsChanged(const QSet<QString> &deviceModelNames)
{
    const int deviceCount = DeviceManager::instance()->deviceCount();
    const MerEmulatorDeviceModel::Map deviceModels = MerSettings::deviceModels();
    for (int i = 0; i < deviceCount; ++i) {
        const auto merEmulator = DeviceManager::instance()->deviceAt(i).dynamicCast<const MerEmulatorDevice>();
        if (!merEmulator
                || !deviceModelNames.contains(merEmulator->deviceModel()))
            continue;

        const QString name = deviceModels.contains(merEmulator->deviceModel())
                ? merEmulator->deviceModel() // device model was changed
                : deviceModels.firstKey(); // device model was removed

        merEmulator.constCast<MerEmulatorDevice>()->setDeviceModel(name);
    }
}

void MerEmulatorDeviceManager::onDeviceListReplaced()
{
    QProgressDialog progress(ICore::dialogParent());
    progress.setWindowModality(Qt::WindowModal);
    QPushButton *cancelButton = new QPushButton(tr("Abort"), &progress);
    cancelButton->setDisabled(true);
    progress.setCancelButton(cancelButton);
    progress.setMinimumDuration(2000);
    progress.setMinimum(0);
    progress.setMaximum(0);

    const auto oldSshPortsCache = m_deviceSshPortCache;
    m_deviceSshPortCache.clear();
    const auto oldQmlLivePortsCache = m_deviceQmlLivePortsCache;
    m_deviceQmlLivePortsCache.clear();
    const auto oldMemorySizeCache = m_deviceMemorySizeCache;
    m_deviceMemorySizeCache.clear();
    const auto oldCpuCountCache = m_deviceCpuCountCache;
    m_deviceCpuCountCache.clear();
    const auto oldVdiInfoCache = m_vdiCapacityCache;
    m_vdiCapacityCache.clear();

    bool ok = true;

    const int deviceCount = DeviceManager::instance()->deviceCount();
    for (int i = 0; i < deviceCount; ++i) {
        auto merEmulator = DeviceManager::instance()->deviceAt(i).dynamicCast<const MerEmulatorDevice>();
        if (!merEmulator)
            continue;

        progress.setLabelText(tr("Applying virtual machine settings: '%1'").arg(merEmulator->virtualMachine()));

        quint16 nowSshPort = merEmulator->sshParameters().port();
        if (nowSshPort != oldSshPortsCache.value(merEmulator->id())) {
            if (MerVirtualBoxManager::updateEmulatorSshPort(merEmulator->virtualMachine(), nowSshPort)) {
                merEmulator->updateConnection();
            } else {
                nowSshPort = oldSshPortsCache.value(merEmulator->id());
                QSsh::SshConnectionParameters nowShhParameters = merEmulator->sshParameters();
                nowShhParameters.setPort(nowSshPort);
                merEmulator.constCast<MerEmulatorDevice>()->setSshParameters(nowShhParameters);
                ok = false;
            }
        }
        m_deviceSshPortCache.insert(merEmulator->id(), nowSshPort);


        auto isPortListEqual = [] (Utils::PortList l1, Utils::PortList l2) {
            if (l1.count() != l2.count())
                return false;

            while (l1.hasMore()) {
                const Port current = l1.getNext();
                if (!l2.contains(current))
                    return false;
            }
            return true;
        };

        Utils::PortList nowQmlLivePorts = merEmulator->qmlLivePorts();
        if (!isPortListEqual(nowQmlLivePorts, oldQmlLivePortsCache.value(merEmulator->id()))) {
            Utils::PortList savedPorts = MerVirtualBoxManager::updateEmulatorQmlLivePorts(merEmulator->virtualMachine(),
                    merEmulator->qmlLivePortsList());

            if (!isPortListEqual(savedPorts, nowQmlLivePorts)) {
                nowQmlLivePorts = savedPorts;
                merEmulator.constCast<MerEmulatorDevice>()->setQmlLivePorts(nowQmlLivePorts);
                ok = false;
            }
        }
        m_deviceQmlLivePortsCache.insert(merEmulator->id(), nowQmlLivePorts);

        int nowMemorySize = merEmulator->memorySizeMb();
        if (nowMemorySize != oldMemorySizeCache.value(merEmulator->id())) {
            if (!MerVirtualBoxManager::setMemorySizeMb(merEmulator->virtualMachine(), nowMemorySize)) {
                nowMemorySize = oldMemorySizeCache.value(merEmulator->id());
                merEmulator.constCast<MerEmulatorDevice>()->setMemorySizeMb(nowMemorySize);
                ok = false;
            }
        }
        m_deviceMemorySizeCache.insert(merEmulator->id(), nowMemorySize);

        int nowCpuCount = merEmulator->cpuCount();
        if (nowCpuCount != oldCpuCountCache.value(merEmulator->id())) {
            if (!MerVirtualBoxManager::setCpuCount(merEmulator->virtualMachine(), nowCpuCount)) {
                  nowCpuCount = oldCpuCountCache.value(merEmulator->id());
                merEmulator.constCast<MerEmulatorDevice>()->setCpuCount(nowCpuCount);
                ok = false;
            }
        }
        m_deviceCpuCountCache.insert(merEmulator->id(), nowCpuCount);


        int nowVdiCapacityMb = merEmulator->vdiCapacityMb();
        if (nowVdiCapacityMb != oldVdiInfoCache.value(merEmulator->id())) {
            QEventLoop loop;
            MerVirtualBoxManager::setVdiCapacityMb(merEmulator->virtualMachine(), nowVdiCapacityMb, &loop, [&loop] (bool ok) {
                loop.exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
            });

            if (loop.exec() != EXIT_SUCCESS) {
                nowVdiCapacityMb = oldVdiInfoCache.value(merEmulator->id());
                merEmulator.constCast<MerEmulatorDevice>()->setVdiCapacityMb(nowVdiCapacityMb);
                ok = false;
            }
        }
        m_vdiCapacityCache.insert(merEmulator->id(), nowVdiCapacityMb);
    }

    if (!ok) {
        progress.cancel();
        QMessageBox::warning(ICore::dialogParent(), tr("Some changes could not be saved!"),
                             tr("Failed to apply some of the changes to virtual machines"));
    }

    onDeviceModelsChanged(m_editedDeviceModels);
    m_editedDeviceModels.clear();

    emit storedDevicesChanged();
}

#include "meremulatordevice.moc"

}
}
