/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "meremulatordevice.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "meremulatordevicetester.h"
#include "meremulatoroptionspage.h"
#include "merplugin.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "meremulatormodedialog.h"

#include <sfdk/buildengine.h>
#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <utils/persistentsettings.h>

#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Sfdk;
using namespace Utils;

namespace Mer {

using namespace Internal;
using namespace Constants;

namespace {
const char PRIVATE_KEY_PATH_TEMPLATE[] = "%1/ssh/private_keys/%2/%3";
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
    explicit PublicKeyDeploymentDialog(const QString &privKeyPath, const QString& user, const QString& sharedPath,
                                       QWidget *parent = 0)
        : QProgressDialog(parent)
        , m_state(Init)
        , m_privKeyPath(privKeyPath)
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
    QString m_error;
    QString m_user;
    QString m_sharedPath;
    QString m_sshDirectoryPath;
};

IDevice::Ptr MerEmulatorDevice::clone() const
{
    return Ptr(new MerEmulatorDevice(*this));
}

MerEmulatorDevice::MerEmulatorDevice()
{
    setMachineType(IDevice::Emulator);
    setArchitecture(Abi::X86Architecture);
    init();
}

MerEmulatorDevice::MerEmulatorDevice(const MerEmulatorDevice &other)
    : MerDevice(other)
    , m_emulator(other.m_emulator)
{
}

MerEmulatorDevice::~MerEmulatorDevice()
{
}

IDeviceWidget *MerEmulatorDevice::createWidget()
{
    return nullptr;
}

void MerEmulatorDevice::init()
{
    auto addAction = [this](const QString &displayName,
            const std::function<void(const MerEmulatorDevice::Ptr &, QWidget *)> &handler) {
        auto wrapped = [handler](const IDevice::Ptr &device, QWidget *parent) {
            handler(device.staticCast<MerEmulatorDevice>(), parent);
        };
        addDeviceAction({displayName, wrapped});
    };

    addAction(tr("Start Emulator"), [](const MerEmulatorDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent);
        device->m_emulator->virtualMachine()->connectTo();
    });

    addAction(tr("Stop Emulator"), [](const MerEmulatorDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent);
        device->m_emulator->virtualMachine()->disconnectFrom();
    });

    addAction(tr("Configure Emulator..."), [](const MerEmulatorDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent);
        MerPlugin::emulatorOptionsPage()->setEmulator(device->m_emulator->uri());
        ICore::showOptionsDialog(Constants::MER_EMULATOR_OPTIONS_ID);
    });
}

DeviceTester *MerEmulatorDevice::createDeviceTester() const
{
    return new MerEmulatorDeviceTester;
}

void MerEmulatorDevice::fromMap(const QVariantMap &map)
{
    MerDevice::fromMap(map);

    const QUrl emulatorUri = map.value(Constants::MER_EMULATOR_DEVICE_EMULATOR_URI).toUrl();
    QTC_ASSERT(emulatorUri.isValid(), return);
    m_emulator = Sdk::emulator(emulatorUri);
    QTC_ASSERT(m_emulator, return);
}

QVariantMap MerEmulatorDevice::toMap() const
{
    QVariantMap map = MerDevice::toMap();
    map.insert(Constants::MER_EMULATOR_DEVICE_EMULATOR_URI, m_emulator->uri().toString());
    return map;
}

Sfdk::Emulator *MerEmulatorDevice::emulator() const
{
    return m_emulator;
}

void MerEmulatorDevice::setEmulator(Sfdk::Emulator *emulator)
{
    QTC_CHECK(emulator);
    QTC_CHECK(!m_emulator);
    m_emulator = emulator;
}

void MerEmulatorDevice::generateSshKey(Sfdk::Emulator *emulator, const QString& user)
{
    const QString privateKeyFile = MerEmulatorDevice::privateKeyFile(idFor(*emulator), user);
    QTC_ASSERT(!privateKeyFile.isEmpty(), return);
    PublicKeyDeploymentDialog dialog(privateKeyFile, user, emulator->sharedSshPath().toString(),
            ICore::dialogParent());

    emulator->virtualMachine()->setAutoConnectEnabled(false);
    dialog.exec();
    emulator->virtualMachine()->setAutoConnectEnabled(true);
}

void MerEmulatorDevice::doFactoryReset(Sfdk::Emulator *emulator, QWidget *parent)
{
    QProgressDialog progress(tr("Restoring '%1' to factory state...").arg(emulator->name()),
            tr("Abort"), 0, 0, parent);
    progress.setMinimumDuration(2000);
    progress.setWindowModality(Qt::WindowModal);

    if (!emulator->virtualMachine()->lockDown(true)) {
        progress.cancel();
        QMessageBox::warning(parent, tr("Failed"),
                tr("Failed to close the virtual machine. Factory state cannot be restored."));
        return;
    }

    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&Emulator::restoreFactoryState), emulator);
    if (!ok) {
        emulator->virtualMachine()->lockDown(false);
        progress.cancel();
        QMessageBox::warning(parent, tr("Failed"), tr("Failed to restore factory state."));
        return;
    }

    emulator->virtualMachine()->lockDown(false);

    progress.cancel();

    QMessageBox::information(parent, tr("Factory state restored"),
            tr("Successfully restored '%1' to the factory state").arg(emulator->name()));
}

Core::Id MerEmulatorDevice::idFor(const Sfdk::Emulator &emulator)
{
    // HACK: We know we do not have other than VirtualBox based emulators
    QTC_CHECK(emulator.uri().path() == "VirtualBox");
    return Core::Id::fromString(emulator.name());
}

QString MerEmulatorDevice::privateKeyFile(Core::Id emulatorId, const QString &user)
{
    // FIXME multiple engines
    QTC_ASSERT(Sdk::buildEngines().count() == 1, return {});

    return QString(PRIVATE_KEY_PATH_TEMPLATE)
        .arg(Sdk::buildEngines().first()->sharedConfigPath().toString())
        .arg(emulatorId.toString().replace(' ', '_'))
        .arg(user);
}

/*!
 * \class MerEmulatorDeviceManager
 */

MerEmulatorDeviceManager *MerEmulatorDeviceManager::s_instance = 0;

MerEmulatorDeviceManager::MerEmulatorDeviceManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    for (Emulator *const emulator : Sdk::emulators())
        startWatching(emulator);
    connect(Sdk::instance(), &Sdk::emulatorAdded,
            this, &MerEmulatorDeviceManager::onEmulatorAdded);
    connect(Sdk::instance(), &Sdk::aboutToRemoveEmulator,
            this, &MerEmulatorDeviceManager::onAboutToRemoveEmulator);
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

void MerEmulatorDeviceManager::onEmulatorAdded(int index)
{
    Emulator *const emulator = Sdk::emulators().at(index);

    const MerEmulatorDevice::Ptr device = MerEmulatorDevice::create();
    device->setEmulator(emulator);
    device->setupId(IDevice::AutoDetected, MerEmulatorDevice::idFor(*emulator));
    device->setDisplayName(emulator->name());
    device->setSshParameters(emulator->virtualMachine()->sshParameters());
    device->setFreePorts(emulator->freePorts());
    device->setQmlLivePorts(emulator->qmlLivePorts());
    startWatching(emulator);

    DeviceManager::instance()->addDevice(device);
}

void MerEmulatorDeviceManager::onAboutToRemoveEmulator(int index)
{
    Emulator *const emulator = Sdk::emulators().at(index);
    stopWatching(emulator);
    DeviceManager::instance()->removeDevice(MerEmulatorDevice::idFor(*emulator));
}

void MerEmulatorDeviceManager::startWatching(Emulator *emulator)
{
    auto withDevice = [=](const std::function<void(MerEmulatorDevice *device)> &function) {
        return [=]() {
            const auto manager = DeviceManager::instance();
            const IDevice::ConstPtr device = manager->find(MerEmulatorDevice::idFor(*emulator));
            QTC_ASSERT(device, return);
            QTC_ASSERT(device.dynamicCast<const MerEmulatorDevice>(), return);
            const IDevice::Ptr clone = device->clone();

            function(static_cast<MerEmulatorDevice *>(clone.data()));

            manager->addDevice(clone);
        };
    };

    connect(emulator->virtualMachine(), &VirtualMachine::sshParametersChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setSshParameters(device->emulator()->virtualMachine()->sshParameters());
    }));
    connect(emulator, &Emulator::freePortsChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setFreePorts(device->emulator()->freePorts());
    }));
    connect(emulator, &Emulator::qmlLivePortsChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setQmlLivePorts(device->emulator()->qmlLivePorts());
    }));
}

void MerEmulatorDeviceManager::stopWatching(Emulator *emulator)
{
    emulator->disconnect(this);
    emulator->virtualMachine()->disconnect(this);
}

#include "meremulatordevice.moc"

}
