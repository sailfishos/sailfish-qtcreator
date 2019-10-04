/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "emulator_p.h"

#include "asynchronous_p.h"
#include "sfdkconstants.h"
#include "sdk_p.h"
#include "usersettings_p.h"
#include "utils_p.h"
#include "virtualmachine_p.h"

#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/persistentsettings.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QTimer>

using namespace QSsh;
using namespace Utils;

namespace Sfdk {

namespace {
const int SCALE_DOWN_FACTOR = 2;
const int VIDEO_MODE_DEPTH = 32;
}

/*!
 * \class DeviceModelData
 */

bool DeviceModelData::isNull() const
{
    return name.isEmpty();
}

bool DeviceModelData::isValid() const
{
    return !name.isEmpty() && !displayResolution.isEmpty() && !displaySize.isEmpty();
}

bool DeviceModelData::operator==(const DeviceModelData &other) const
{
    return name == other.name
            && displayResolution == other.displayResolution
            && displaySize == other.displaySize
            && dconf == other.dconf
            && autodetected == other.autodetected;
}

bool DeviceModelData::operator!=(const DeviceModelData &other) const
{
    return !operator==(other);
}

/*!
 * \class Emulator
 */

struct Emulator::PrivateConstructorTag {};

Emulator::Emulator(QObject *parent, const PrivateConstructorTag &)
    : QObject(parent)
    , d_ptr(std::make_unique<EmulatorPrivate>(this))
{
}

Emulator::~Emulator() = default;

QUrl Emulator::uri() const
{
    return d_func()->virtualMachine->uri();
}

QString Emulator::name() const
{
    return d_func()->virtualMachine->name();
}

VirtualMachine *Emulator::virtualMachine() const
{
    return d_func()->virtualMachine.get();
}

bool Emulator::isAutodetected() const
{
    return d_func()->autodetected;
}

Utils::FileName Emulator::sharedConfigPath() const
{
    return d_func()->sharedConfigPath;
}

Utils::FileName Emulator::sharedSshPath() const
{
    return d_func()->sharedSshPath;
}

quint16 Emulator::sshPort() const
{
    return d_func()->virtualMachine->sshParameters().port();
}

void Emulator::setSshPort(quint16 sshPort, const QObject *context, const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};
    VirtualMachinePrivate::get(virtualMachine())->setReservedPortForwarding(
            VirtualMachinePrivate::SshPort, sshPort, this, [=](bool ok) {
        Q_D(Emulator);
        if (ok) {
            SshConnectionParameters sshParameters = d->virtualMachine->sshParameters();
            sshParameters.setPort(sshPort);
            d->setSshParameters(sshParameters);
        }
        if (context_)
            functor(ok);
    });
}

Utils::PortList Emulator::freePorts() const
{
    return d_func()->freePorts;
}

void Emulator::setFreePorts(const Utils::PortList &freePorts, const QObject *context,
        const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};
    VirtualMachinePrivate::get(virtualMachine())->setReservedPortListForwarding(
            VirtualMachinePrivate::FreePortList, toList(freePorts), this,
            [=](const QList<Port> &savedPorts, bool ok) {
        QTC_CHECK(ok);
        d_func()->setFreePorts(toPortList(savedPorts));
        if (context_)
            functor(ok);
    });
}

Utils::PortList Emulator::qmlLivePorts() const
{
    return d_func()->qmlLivePorts;
}

void Emulator::setQmlLivePorts(const Utils::PortList &qmlLivePorts, const QObject *context,
        const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};
    VirtualMachinePrivate::get(virtualMachine())->setReservedPortListForwarding(
            VirtualMachinePrivate::QmlLivePortList, toList(qmlLivePorts), this,
            [=](const QList<Port> &savedPorts, bool ok) {
        QTC_CHECK(ok);
        d_func()->setQmlLivePorts(toPortList(savedPorts));
        if (context_)
            functor(ok);
    });
}

QString Emulator::factorySnapshot() const
{
    return d_func()->factorySnapshot;
}

void Emulator::setFactorySnapshot(const QString &snapshotName)
{
    Q_D(Emulator);
    if (d->factorySnapshot == snapshotName)
        return;
    d->factorySnapshot = snapshotName;
    emit factorySnapshotChanged(d->factorySnapshot);
}

void Emulator::restoreFactoryState(const QObject *context, const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};
    VirtualMachinePrivate::get(virtualMachine())->restoreSnapshot(factorySnapshot(), this,
            [=](bool restoredOk) {
        QTC_CHECK(restoredOk);
        d_func()->updateVmProperties(context_.data(), [=](bool updatedOk) {
            QTC_CHECK(updatedOk);
            functor(restoredOk && updatedOk);
        });
    });
}

DeviceModelData Emulator::deviceModel() const
{
    return d_func()->deviceModel;
}

Qt::Orientation Emulator::orientation() const
{
    return d_func()->orientation;
}

bool Emulator::isViewScaled() const
{
    return d_func()->viewScaled;
}

void Emulator::setDisplayProperties(const DeviceModelData &deviceModel, Qt::Orientation orientation,
        bool viewScaled, const QObject *context, const Functor<bool> &functor)
{
    Q_D(Emulator);

    const QPointer<const QObject> context_{context};

    d->setDisplayProperties(deviceModel, orientation, viewScaled);

    bool allOk = true;

    QSize realResolution = d->deviceModel.displayResolution;
    if (d->orientation == Qt::Horizontal)
        realResolution.transpose();
    QSize virtualResolution = realResolution;
    if (d->viewScaled)
        realResolution /= SCALE_DOWN_FACTOR;

    const QString dconfDbFile = FileName(d->sharedConfigPath)
        .appendPath(Constants::EMULATOR_DCONF_DB_FILENAME).toString();
    allOk &= d->updateDconfDb(dconfDbFile, d->deviceModel.dconf);

    const QString compositorConfigFile = FileName(d->sharedConfigPath)
        .appendPath(Constants::EMULATOR_COMPOSITOR_CONFIG_FILENAME).toString();
    allOk &= d->updateCompositorConfig(compositorConfigFile, d->deviceModel.displaySize,
            virtualResolution, d->viewScaled);

    VirtualMachinePrivate::get(d->virtualMachine.get())
        ->setVideoMode(realResolution, VIDEO_MODE_DEPTH, context, [=](bool ok) {
        functor(allOk && ok);
    });
}

/*!
 * \class EmulatorPrivate
 */

EmulatorPrivate::EmulatorPrivate(Emulator *q)
    : q_ptr(q)
{
}

EmulatorPrivate::~EmulatorPrivate() = default;

QVariantMap EmulatorPrivate::toMap() const
{
    QVariantMap data;

    data.insert(Constants::EMULATOR_VM_URI, virtualMachine->uri());
    data.insert(Constants::EMULATOR_AUTODETECTED, autodetected);

    data.insert(Constants::EMULATOR_SHARED_CONFIG, sharedConfigPath.toString());
    data.insert(Constants::EMULATOR_SHARED_SSH, sharedSshPath.toString());

    const SshConnectionParameters sshParameters = virtualMachine->sshParameters();
    data.insert(Constants::EMULATOR_HOST, sshParameters.host());
    data.insert(Constants::EMULATOR_USER_NAME, sshParameters.userName());
    data.insert(Constants::EMULATOR_PRIVATE_KEY_FILE, sshParameters.privateKeyFile);
    data.insert(Constants::EMULATOR_SSH_PORT, sshParameters.port());
    data.insert(Constants::EMULATOR_SSH_TIMEOUT, sshParameters.timeout);

    data.insert(Constants::EMULATOR_FREE_PORTS, freePorts.toString());
    data.insert(Constants::EMULATOR_QML_LIVE_PORTS, qmlLivePorts.toString());

    data.insert(Constants::EMULATOR_FACTORY_SNAPSHOT, factorySnapshot);

    data.insert(Constants::EMULATOR_MAC, mac_);
    data.insert(Constants::EMULATOR_SUBNET, subnet_);

    const QVariantMap deviceModelData = EmulatorManager::toMap(deviceModel);
    data.insert(Constants::EMULATOR_DEVICE_MODEL, deviceModelData);
    data.insert(Constants::EMULATOR_ORIENTATION, orientation);
    data.insert(Constants::EMULATOR_VIEW_SCALED, viewScaled);

    return data;
}

bool EmulatorPrivate::fromMap(const QVariantMap &data)
{
    Q_Q(Emulator);

    const QUrl vmUri = data.value(Constants::EMULATOR_VM_URI).toUrl();
    QTC_ASSERT(vmUri.isValid(), return false);
    QTC_ASSERT(!virtualMachine || virtualMachine->uri() == vmUri, return false);

    if (!virtualMachine) {
        if (!initVirtualMachine(vmUri))
            return false;
    }

    autodetected = data.value(Constants::EMULATOR_AUTODETECTED).toBool();

    auto toFileName = [](const QVariant &v) { return FileName::fromString(v.toString()); };
    setSharedConfigPath(toFileName(data.value(Constants::EMULATOR_SHARED_CONFIG)));
    setSharedSshPath(toFileName(data.value(Constants::EMULATOR_SHARED_SSH)));

    SshConnectionParameters sshParameters = virtualMachine->sshParameters();
    sshParameters.setHost(data.value(Constants::EMULATOR_HOST).toString());
    sshParameters.setUserName(data.value(Constants::EMULATOR_USER_NAME).toString());
    sshParameters.privateKeyFile = data.value(Constants::EMULATOR_PRIVATE_KEY_FILE).toString();
    sshParameters.setPort(data.value(Constants::EMULATOR_SSH_PORT).toUInt());
    sshParameters.timeout = data.value(Constants::EMULATOR_SSH_TIMEOUT).toInt();
    setSshParameters(sshParameters);

    setFreePorts(PortList::fromString(data.value(Constants::EMULATOR_FREE_PORTS).toString()));
    setQmlLivePorts(PortList::fromString(data.value(Constants::EMULATOR_QML_LIVE_PORTS).toString()));

    q->setFactorySnapshot(data.value(Constants::EMULATOR_FACTORY_SNAPSHOT).toString());

    // FIXME update build engine's devices.xml when these change
    mac_ = data.value(Constants::EMULATOR_MAC).toString();
    subnet_ = data.value(Constants::EMULATOR_SUBNET).toString();

    DeviceModelData deviceModel;
    EmulatorManager::fromMap(&deviceModel, data.value(Constants::EMULATOR_DEVICE_MODEL).toMap());
    setDisplayProperties(deviceModel,
            static_cast<Qt::Orientation>(data.value(Constants::EMULATOR_ORIENTATION).toInt()),
            data.value(Constants::EMULATOR_VIEW_SCALED).toBool());

    return true;
}

bool EmulatorPrivate::initVirtualMachine(const QUrl &vmUri)
{
    Q_ASSERT(!virtualMachine);

    virtualMachine = VirtualMachineFactory::create(vmUri);
    QTC_ASSERT(virtualMachine, return false);

    SshConnectionParameters sshParameters;
    sshParameters.setUserName(Constants::EMULATOR_DEFAULT_USER_NAME);
    sshParameters.setHost(Constants::EMULATOR_DEFAULT_HOST);
    sshParameters.timeout = Constants::EMULATOR_DEFAULT_SSH_TIMEOUT;
    sshParameters.hostKeyCheckingMode = SshHostKeyCheckingNone;
    sshParameters.authenticationType = SshConnectionParameters::AuthenticationTypeSpecificKey;
    virtualMachine->setSshParameters(sshParameters);

    // Do not attempt to auto-connect until the settings from UserScope is applied
    virtualMachine->setAutoConnectEnabled(false);

    return true;
}

void EmulatorPrivate::enableUpdates()
{
    Q_Q(Emulator);
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);

    virtualMachine->setAutoConnectEnabled(true);

    updateVmProperties(q, [](bool ok) { Q_UNUSED(ok) });
}

void EmulatorPrivate::updateOnce()
{
    QTC_ASSERT(!SdkPrivate::isVersionedSettingsEnabled(), return);

    virtualMachine->setAutoConnectEnabled(true);

    // FIXME Not ideal
    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&EmulatorPrivate::updateVmProperties), this);
    QTC_CHECK(ok);
}

void EmulatorPrivate::updateVmProperties(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(Emulator);

    const QPointer<const QObject> context_{context};

    VirtualMachinePrivate::get(virtualMachine.get())->fetchInfo(VirtualMachineInfo::SnapshotInfo, q,
            [=](const VirtualMachineInfo &info, bool ok) {
        if (!ok) {
            if (context_)
                functor(false);
            return;
        }

        if (factorySnapshot.isEmpty() && !info.snapshots.isEmpty())
            q->setFactorySnapshot(info.snapshots.first());

        // FIXME if sharedConfig changes for a build engine, at least privateKeyFile path needs to
        // be updated - what should be done for an emulator?
        setSharedConfigPath(FileName::fromString(info.sharedConfig));
        setSharedSshPath(FileName::fromString(info.sharedSsh));

        SshConnectionParameters sshParameters = virtualMachine->sshParameters();
        sshParameters.setPort(info.sshPort);
        setSshParameters(sshParameters);

        setQmlLivePorts(toPortList(info.qmlLivePorts.values()));
        setFreePorts(toPortList(info.freePorts.values()));

        QTC_CHECK(info.macs.count() > 1);
        if (info.macs.count() > 1)
            mac_ = info.macs.at(1); // FIXME

        subnet_ = Constants::VIRTUAL_BOX_VIRTUAL_SUBNET;

        if (context_)
            functor(true);
    });
}

void EmulatorPrivate::setSharedConfigPath(const Utils::FileName &sharedConfigPath)
{
    if (this->sharedConfigPath == sharedConfigPath)
        return;
    this->sharedConfigPath = sharedConfigPath;
    emit q_func()->sharedConfigPathChanged(sharedConfigPath);
}

void EmulatorPrivate::setSharedSshPath(const Utils::FileName &sharedSshPath)
{
    if (this->sharedSshPath == sharedSshPath)
        return;
    this->sharedSshPath = sharedSshPath;
    emit q_func()->sharedSshPathChanged(sharedSshPath);
}

void EmulatorPrivate::setSshParameters(const QSsh::SshConnectionParameters &sshParameters)
{
    Q_Q(Emulator);

    const SshConnectionParameters old = virtualMachine->sshParameters();
    virtualMachine->setSshParameters(sshParameters);
    if (sshParameters.port() != old.port())
        emit q->sshPortChanged(sshParameters.port());
}

void EmulatorPrivate::setFreePorts(const Utils::PortList &freePorts)
{
    if (this->freePorts.toString() == freePorts.toString())
        return;
    this->freePorts = freePorts;
    emit q_func()->freePortsChanged();
}

void EmulatorPrivate::setQmlLivePorts(const Utils::PortList &qmlLivePorts)
{
    if (this->qmlLivePorts.toString() == qmlLivePorts.toString())
        return;
    this->qmlLivePorts = qmlLivePorts;
    emit q_func()->qmlLivePortsChanged();
}

void EmulatorPrivate::setDisplayProperties(const DeviceModelData &deviceModel,
        Qt::Orientation orientation, bool viewScaled)
{
    Q_Q(Emulator);

    const bool deviceModelDiffers = this->deviceModel != deviceModel;
    const bool deviceModelNameDiffers = this->deviceModel.name != deviceModel.name;
    const bool orientationDiffers = this->orientation != orientation;
    const bool viewScaledDiffers = this->viewScaled != viewScaled;

    this->deviceModel = deviceModel;
    this->orientation = orientation;
    this->viewScaled = viewScaled;

    if (deviceModelDiffers)
        emit q->deviceModelChanged();
    if (deviceModelNameDiffers)
        emit q->deviceModelNameChanged(deviceModel.name);
    if (orientationDiffers)
        emit q->orientationChanged(orientation);
    if (viewScaledDiffers)
        emit q->viewScaledChanged(viewScaled);
}

bool EmulatorPrivate::updateCompositorConfig(const QString &file, const QSize &displaySize,
        const QSize &displayResolution, bool viewScaled)
{
    FileSaver saver(file, QIODevice::WriteOnly);

    // Keep environment clean if scaling is disabled - may help in case of errors
    const QString maybeCommentOut = viewScaled ? QString() : QString("#");

    QString config;
    QTextStream(&config)
        << maybeCommentOut << "QT_QPA_EGLFS_SCALE=" << SCALE_DOWN_FACTOR << endl
        << "QT_QPA_EGLFS_WIDTH=" << displayResolution.width() << endl
        << "QT_QPA_EGLFS_HEIGHT=" << displayResolution.height() << endl
        << "QT_QPA_EGLFS_PHYSICAL_WIDTH=" << displaySize.width() << endl
        << "QT_QPA_EGLFS_PHYSICAL_HEIGHT=" << displaySize.height() << endl;
    saver.write(config.toUtf8());

    const bool ok = saver.finalize();
    QTC_ASSERT(ok, qCCritical(Log::emulator) << saver.errorString());

    return ok;
}

bool EmulatorPrivate::updateDconfDb(const QString &file, const QString &dconf)
{
    FileSaver saver(file, QIODevice::WriteOnly);

    saver.write(dconf.toUtf8());
    if (!dconf.endsWith('\n'))
        saver.write("\n");

    const bool ok = saver.finalize();
    QTC_ASSERT(ok, qCCritical(Log::emulator) << saver.errorString());

    return ok;
}

/*!
 * \class EmulatorManager
 */

EmulatorManager *EmulatorManager::s_instance = nullptr;

EmulatorManager::EmulatorManager(QObject *parent)
    : QObject(parent)
    , m_userSettings(std::make_unique<UserSettings>(
                Constants::EMULATORS_SETTINGS_FILE_NAME,
                Constants::EMULATORS_SETTINGS_DOC_TYPE, this))
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    if (!SdkPrivate::useSystemSettingsOnly()) {
        // FIXME ugly
        const optional<QVariantMap> userData = m_userSettings->load();
        if (userData)
            fromMap(userData.value());
    }

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        connect(SdkPrivate::instance(), &SdkPrivate::enableUpdatesRequested,
                this, &EmulatorManager::enableUpdates);
    } else {
        connect(SdkPrivate::instance(), &SdkPrivate::updateOnceRequested,
                this, &EmulatorManager::updateOnce);
    }

    connect(SdkPrivate::instance(), &SdkPrivate::saveSettingsRequested,
            this, &EmulatorManager::saveSettings);
}

EmulatorManager::~EmulatorManager()
{
    s_instance = nullptr;
}

EmulatorManager *EmulatorManager::instance()
{
    return s_instance;
}

QList<Emulator *> EmulatorManager::emulators()
{
    return Utils::toRawPointer<QList>(s_instance->m_emulators);
}

Emulator *EmulatorManager::emulator(const QUrl &uri)
{
    return Utils::findOrDefault(s_instance->m_emulators, Utils::equal(&Emulator::uri, uri));
}

void EmulatorManager::createEmulator(const QUrl &virtualMachineUri, const QObject *context,
        const Functor<std::unique_ptr<Emulator> &&> &functor)
{
    // Needs to be captured by multiple lambdas and their copies
    auto emulator = std::make_shared<std::unique_ptr<Emulator>>(
            std::make_unique<Emulator>(nullptr, Emulator::PrivateConstructorTag{}));

    auto emulator_d = EmulatorPrivate::get(emulator->get());
    if (!emulator_d->initVirtualMachine(virtualMachineUri)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0) // just guessing, not sure when it was fixed
        QTimer::singleShot(0, context, std::bind(functor, nullptr));
#else
        QTimer::singleShot(0, const_cast<QObject *>(context), std::bind(functor, nullptr));
#endif
        return;
    }
    // FIXME the VM info is fetched twice unnecessary
    emulator_d->updateVmProperties(context, [=](bool ok) {
        QTC_CHECK(ok);
        if (!ok) {
            functor({});
            return;
        }

        emulator->get()->virtualMachine()->refreshConfiguration(context, [=](bool ok) {
            QTC_CHECK(ok);
            if (!ok) {
                functor({});
                return;
            }

            functor(std::move(*emulator));
        });
    });
}

int EmulatorManager::addEmulator(std::unique_ptr<Emulator> &&emulator)
{
    QTC_ASSERT(emulator, return -1);

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        QTC_ASSERT(SdkPrivate::isUpdatesEnabled(), return -1);
        EmulatorPrivate::get(emulator.get())->enableUpdates();
    } else {
        EmulatorPrivate::get(emulator.get())->updateOnce();
    }

    s_instance->m_emulators.emplace_back(std::move(emulator));
    const int index = s_instance->m_emulators.size() - 1;
    emit s_instance->emulatorAdded(index);
    return index;
}

void EmulatorManager::removeEmulator(const QUrl &uri)
{
    int index = Utils::indexOf(s_instance->m_emulators, Utils::equal(&Emulator::uri, uri));
    QTC_ASSERT(index >= 0, return);

    emit s_instance->aboutToRemoveEmulator(index);
    s_instance->m_emulators.erase(s_instance->m_emulators.cbegin() + index);
}

QList<DeviceModelData> EmulatorManager::deviceModels()
{
    return s_instance->m_deviceModels.values();
}

DeviceModelData EmulatorManager::deviceModel(const QString &name)
{
    return s_instance->m_deviceModels.value(name);
}

void EmulatorManager::setDeviceModels(const QList<DeviceModelData> &deviceModels,
        const QObject *context, const Functor<bool> &functor)
{
    QMap<QString, DeviceModelData> newDeviceModels;
    for (const DeviceModelData &newDeviceModel : deviceModels) {
        // Reject changes to autodetected models
        QTC_ASSERT(!newDeviceModel.autodetected
                || s_instance->m_deviceModels.value(newDeviceModel.name) == newDeviceModel,
                continue);
        newDeviceModels.insert(newDeviceModel.name, newDeviceModel);
    }
    for (const DeviceModelData &oldDeviceModel : s_instance->m_deviceModels) {
        // Reject removals of autodetected models
        QTC_ASSERT(!oldDeviceModel.autodetected
                || newDeviceModels.value(oldDeviceModel.name) == oldDeviceModel,
                newDeviceModels.insert(oldDeviceModel.name, oldDeviceModel));
    }

    s_instance->m_deviceModels = newDeviceModels;
    s_instance->fixDeviceModelsInUse(context, functor);
    emit s_instance->deviceModelsChanged();
}

QVariantMap EmulatorManager::toMap(const DeviceModelData &deviceModel)
{
    QVariantMap data;

    data.insert(Constants::DEVICE_MODEL_NAME, deviceModel.name);
    data.insert(Constants::DEVICE_MODEL_AUTODETECTED, deviceModel.autodetected);
    data.insert(Constants::DEVICE_MODEL_DISPLAY_RESOLUTION,
            QRect(QPoint(0, 0), deviceModel.displayResolution));
    data.insert(Constants::DEVICE_MODEL_DISPLAY_SIZE,
            QRect(QPoint(0, 0), deviceModel.displaySize));
    data.insert(Constants::DEVICE_MODEL_DCONF, deviceModel.dconf);

    return data;
}

void EmulatorManager::fromMap(DeviceModelData *deviceModel, const QVariantMap &data)
{
    Q_ASSERT(deviceModel);

    deviceModel->name = data.value(Constants::DEVICE_MODEL_NAME).toString();
    deviceModel->autodetected = data.value(Constants::DEVICE_MODEL_AUTODETECTED).toBool();
    deviceModel->displayResolution =
        data.value(Constants::DEVICE_MODEL_DISPLAY_RESOLUTION).toRect().size();
    deviceModel->displaySize = data.value(Constants::DEVICE_MODEL_DISPLAY_SIZE).toRect().size();
    deviceModel->dconf = data.value(Constants::DEVICE_MODEL_DCONF).toString();
}

QVariantMap EmulatorManager::toMap() const
{
    QVariantMap data;
    data.insert(Constants::EMULATORS_VERSION_KEY, m_version);
    data.insert(Constants::EMULATORS_INSTALL_DIR_KEY, m_installDir);

    int count = 0;
    for (const auto &emulator : m_emulators) {
        const QVariantMap emulatorData = EmulatorPrivate::get(emulator.get())->toMap();
        QTC_ASSERT(!emulatorData.isEmpty(), return {});
        data.insert(QString::fromLatin1(Constants::EMULATORS_DATA_KEY_PREFIX)
                + QString::number(count),
                emulatorData);
        ++count;
    }
    data.insert(Constants::EMULATORS_COUNT_KEY, count);

    count = 0;
    for (const auto &deviceModel : m_deviceModels) {
        const QVariantMap deviceModelData = toMap(deviceModel);
        QTC_ASSERT(!deviceModelData.isEmpty(), return {});
        data.insert(QString::fromLatin1(Constants::DEVICE_MODELS_DATA_KEY_PREFIX)
                + QString::number(count),
                deviceModelData);
        ++count;
    }
    data.insert(Constants::DEVICE_MODELS_COUNT_KEY, count);

    return data;
}

void EmulatorManager::fromMap(const QVariantMap &data, bool merge)
{
    m_version = data.value(Constants::EMULATORS_VERSION_KEY).toInt();
    QTC_ASSERT(m_version > 0, return);

    m_installDir = data.value(Constants::EMULATORS_INSTALL_DIR_KEY).toString();
    QTC_ASSERT(!m_installDir.isEmpty(), return);

    const int newCount = data.value(Constants::EMULATORS_COUNT_KEY, 0).toInt();
    QMap<QUrl, QVariantMap> newEmulatorsData;
    for (int i = 0; i < newCount; ++i) {
        const QString key = QString::fromLatin1(Constants::EMULATORS_DATA_KEY_PREFIX)
            + QString::number(i);
        QTC_ASSERT(data.contains(key), return);

        const QVariantMap emulatorData = data.value(key).toMap();
        const QUrl vmUri = emulatorData.value(Constants::EMULATOR_VM_URI).toUrl();
        QTC_ASSERT(!vmUri.isEmpty(), return);

        newEmulatorsData.insert(vmUri, emulatorData);
    }

    // Unles 'merge' is true, remove emulators missing from the updated configuration.
    // Index the preserved ones by VM URI
    QMap<QUrl, Emulator *> existingEmulators;
    for (auto it = m_emulators.cbegin(); it != m_emulators.cend(); ) {
        if (!merge && !newEmulatorsData.contains(it->get()->virtualMachine()->uri())) {
            emit aboutToRemoveEmulator(it - m_emulators.cbegin());
            it = m_emulators.erase(it);
        } else {
            // we know 'merge' is only used to preserve manually added
            QTC_CHECK(!merge || !it->get()->isAutodetected());
            existingEmulators.insert(it->get()->virtualMachine()->uri(), it->get());
            ++it;
        }
    }

    // Update existing/add new emulators
    for (const QUrl &vmUri : newEmulatorsData.keys()) {
        const QVariantMap emulatorData = newEmulatorsData.value(vmUri);
        Emulator *emulator = existingEmulators.value(vmUri);
        std::unique_ptr<Emulator> newEmulator;
        if (!emulator) {
            newEmulator = std::make_unique<Emulator>(this, Emulator::PrivateConstructorTag{});
            emulator = newEmulator.get();
        }

        const bool ok = EmulatorPrivate::get(emulator)->fromMap(emulatorData);
        QTC_ASSERT(ok, return);

        if (newEmulator) {
            m_emulators.emplace_back(std::move(newEmulator));
            emit emulatorAdded(m_emulators.size() - 1);
        }
    }

    // Update device models
    const int newDeviceModelsCount = data.value(Constants::DEVICE_MODELS_COUNT_KEY, 0).toInt();
    QMap<QString, DeviceModelData> newDeviceModels;
    for (int i = 0; i < newDeviceModelsCount; ++i) {
        const QString key = QString::fromLatin1(Constants::DEVICE_MODELS_DATA_KEY_PREFIX)
            + QString::number(i);
        QTC_ASSERT(data.contains(key), return);

        DeviceModelData deviceModel;
        fromMap(&deviceModel, data.value(key).toMap());
        QTC_ASSERT(deviceModel.isValid(), return);
        newDeviceModels.insert(deviceModel.name, deviceModel);
    }

    bool deviceModelsChanged = false;
    if (!merge) {
        deviceModelsChanged = m_deviceModels != newDeviceModels;
        m_deviceModels = newDeviceModels;
    } else {
        deviceModelsChanged = true; // Do not take it too seriously
        for (auto it = newDeviceModels.cbegin(); it != newDeviceModels.cend(); ++it) {
            QTC_CHECK(it.value().autodetected);
            DeviceModelData existing = m_deviceModels.value(it.key());
            if (!existing.isNull()) {
                QTC_CHECK(!existing.autodetected);
                const QString renameAs = Utils::makeUniquelyNumbered(it.key(),
                        m_deviceModels.keys());
                m_deviceModels.insert(renameAs, existing);
                // FIXME issue a message in the general messages pane
            }
            m_deviceModels.insert(it.key(), it.value());
        }
    }

    QTC_CHECK(!m_deviceModels.isEmpty());

    if (deviceModelsChanged) {
        // FIXME?
        fixDeviceModelsInUse(this, [=](bool ok) { Q_UNUSED(ok) });
        emit this->deviceModelsChanged();
    }
}

void EmulatorManager::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);

    qCDebug(Log::emulator) << "Enabling updates";

    connect(m_userSettings.get(), &UserSettings::updated,
            this, [=](const QVariantMap &data) { fromMap(data); });
    m_userSettings->enableUpdates();

    checkSystemSettings();

    for (const std::unique_ptr<Emulator> &emulator : m_emulators)
        EmulatorPrivate::get(emulator.get())->enableUpdates();
}

void EmulatorManager::updateOnce()
{
    QTC_ASSERT(!SdkPrivate::isVersionedSettingsEnabled(), return);

    checkSystemSettings();

    for (const std::unique_ptr<Emulator> &emulator : m_emulators)
        EmulatorPrivate::get(emulator.get())->updateOnce();
}

void EmulatorManager::checkSystemSettings()
{
    qCDebug(Log::emulator) << "Checking system-wide configuration file"
        << systemSettingsFile();

    PersistentSettingsReader systemReader;
    if (!systemReader.load(systemSettingsFile())) {
        qCCritical(Log::emulator) << "Failed to load system-wide emulators configuration";
        return;
    }

    const QVariantMap systemData = systemReader.restoreValues();

    if (m_version == 0)
        qCDebug(Log::emulator) << "No user configuration => using system-wide configuration";
    else if (m_version != systemData.value(Constants::EMULATORS_VERSION_KEY).toInt()) {
        qCDebug(Log::emulator) << "Version changed => forget user configuration";
    } else if (m_installDir != systemData.value(Constants::EMULATORS_INSTALL_DIR_KEY)) {
        qCDebug(Log::emulator) << "Install dir changed => forget user configuration";
    } else {
        qCDebug(Log::emulator) << "Using user configuration";
        return;
    }

    for (auto it = m_emulators.cbegin(); it != m_emulators.cend(); ) {
        if (it->get()->isAutodetected()) {
            emit aboutToRemoveEmulator(it - m_emulators.cbegin());
            it = m_emulators.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_deviceModels.begin(); it != m_deviceModels.end(); ) {
        if (it->autodetected) {
            // deviceModelsChanged will be emitted later from fromMap()
            it = m_deviceModels.erase(it);
        } else {
            ++it;
        }
    }

    const bool merge = true;
    fromMap(systemData, merge);
}

void EmulatorManager::saveSettings(QStringList *errorStrings) const
{
    QString errorString;
    const bool ok = m_userSettings->save(toMap(), &errorString);
    if (!ok)
        errorStrings->append(errorString);
}

void EmulatorManager::fixDeviceModelsInUse(const QObject *context, const Functor<bool> &functor)
{
    auto allOk = std::make_shared<bool>(true);

    for (const std::unique_ptr<Emulator> &emulator : qAsConst(m_emulators)) {
        QTC_ASSERT(!m_deviceModels.isEmpty(), break);
        DeviceModelData newDeviceModel;
        if (!m_deviceModels.contains(emulator->deviceModel().name))
            newDeviceModel = m_deviceModels.first();
        else if (m_deviceModels.value(emulator->deviceModel().name) != emulator->deviceModel())
            newDeviceModel = m_deviceModels.value(emulator->deviceModel().name);
        else
            continue;

        emulator->setDisplayProperties(newDeviceModel, emulator->orientation(),
                emulator->isViewScaled(), context, [=](bool ok) {
            *allOk &= ok;
        });
    }
    SdkPrivate::commandQueue()->enqueueCheckPoint(context, [=]() { functor(*allOk); });
}

FileName EmulatorManager::systemSettingsFile()
{
    return SdkPrivate::settingsFile(SdkPrivate::SystemScope,
            Constants::EMULATORS_SETTINGS_FILE_NAME);
}

} // namespace Sfdk
