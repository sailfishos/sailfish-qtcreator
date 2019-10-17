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

#include "sdkmanager.h"

#include <QDir>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <mer/merconstants.h>
#include <mer/mersettings.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include "configuration.h"
#include "dispatch.h"
#include "remoteprocess.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

namespace {
const int CONNECT_TIMEOUT_MS = 60000;
const int LOCK_DOWN_TIMEOUT_MS = 60000;
}

using namespace Mer;
using namespace Mer::Internal;
using namespace QSsh;
using namespace Sfdk;

namespace Sfdk {

class VmConnectionUi : public VirtualMachine::ConnectionUi
{
    Q_OBJECT

public:
    using VirtualMachine::ConnectionUi::ConnectionUi;

    void warn(Warning which) override
    {
        switch (which) {
        case AlreadyConnecting:
            QTC_CHECK(false);
            break;
        case AlreadyDisconnecting:
            QTC_CHECK(false);
            break;
        case UnableToCloseVm:
            qerr() << tr("Timeout waiting for the \"%1\" virtual machine to close.")
                .arg(virtualMachine()->name());
            break;
        case VmNotRegistered:
            qerr() << tr("No virtual machine with the name \"%1\" found. Check your installation.")
                .arg(virtualMachine()->name());
            break;
        }
    }

    void dismissWarning(Warning which) override
    {
        Q_UNUSED(which);
    }

    bool shouldAsk(Question which) const override
    {
        Q_UNUSED(which);
        return false;
    }

    void ask(Question which, std::function<void()> onStatusChanged) override
    {
        switch (which) {
        case StartVm:
            QTC_CHECK(false);
            break;
        case ResetVm:
            QTC_CHECK(false);
            break;
        case CloseVm:
            QTC_CHECK(false);
            break;
        case CancelConnecting:
            QTC_CHECK(!m_cancelConnectingTimer);
            m_cancelConnectingTimer = startTimer(CONNECT_TIMEOUT_MS, onStatusChanged,
                tr("Timeout connecting to the \"%1\" virtual machine.")
                    .arg(virtualMachine()->name()));
            break;
        case CancelLockingDown:
            QTC_CHECK(!m_cancelLockingDownTimer);
            m_cancelLockingDownTimer = startTimer(LOCK_DOWN_TIMEOUT_MS, onStatusChanged,
                tr("Timeout waiting for the \"%1\" virtual machine to close.")
                    .arg(virtualMachine()->name()));
            break;
        }
}

    void dismissQuestion(Question which) override
    {
        switch (which) {
        case StartVm:
        case ResetVm:
        case CloseVm:
            break;
        case CancelConnecting:
            delete m_cancelConnectingTimer;
            break;
        case CancelLockingDown:
            delete m_cancelLockingDownTimer;
            break;
        }
    }

    QuestionStatus status(Question which) const override
    {
        switch (which) {
        case StartVm:
            QTC_CHECK(false);
            return NotAsked;
        case ResetVm:
            QTC_CHECK(false);
            return NotAsked;
        case CloseVm:
            QTC_CHECK(false);
            return NotAsked;
        case CancelConnecting:
            return status(m_cancelConnectingTimer);
        case CancelLockingDown:
            return status(m_cancelLockingDownTimer);
        }

        QTC_CHECK(false);
        return NotAsked;
    }

private:
    QTimer *startTimer(int timeout, std::function<void()> onStatusChanged, const QString &timeoutMessage)
    {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, [=] {
            qerr() << timeoutMessage << endl;
            onStatusChanged();
        });
        timer->setSingleShot(true);
        timer->start(timeout);
        return timer;
    }

    static QuestionStatus status(QTimer *timer)
    {
        if (!timer)
            return NotAsked;
        else if (timer->isActive())
            return Asked;
        else
            return Yes;
    }

private:
    QPointer<QTimer> m_cancelConnectingTimer;
    QPointer<QTimer> m_cancelLockingDownTimer;
};

} // namespace Sfdk

/*!
 * \class SdkManager
 */

SdkManager *SdkManager::s_instance = nullptr;

SdkManager::SdkManager(bool useSystemSettingsOnly)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_enableReversePathMapping =
        qEnvironmentVariableIsEmpty(Constants::DISABLE_REVERSE_PATH_MAPPING_ENV_VAR);

    m_merSettings = std::make_unique<MerSettings>();

    VirtualMachine::registerConnectionUi<VmConnectionUi>();

    Sdk::Options sdkOptions = Sdk::NoOption;
    if (qEnvironmentVariableIsEmpty(Constants::DISABLE_VM_INFO_CACHE_ENV_VAR))
        sdkOptions |= Sdk::CachedVmInfo;
    if (useSystemSettingsOnly) {
        sdkOptions &= ~Sdk::CachedVmInfo;
        sdkOptions |= Sdk::SystemSettingsOnly;
    }
    m_sdk = std::make_unique<Sdk>(sdkOptions);

    QList<BuildEngine *> buildEngines = Sdk::buildEngines();
    if (!buildEngines.isEmpty()) {
        m_buildEngine = buildEngines.first();
        if (buildEngines.count() > 1) {
            qCWarning(sfdk) << "Multiple build engines found. Using"
                << m_buildEngine->uri().toString();
        }
    }
}

SdkManager::~SdkManager()
{
    s_instance = nullptr;
}

bool SdkManager::isValid()
{
    return s_instance->hasEngine();
}

QString SdkManager::installationPath()
{
    return Sdk::installationPath();
}

BuildEngine *SdkManager::engine()
{
    QTC_ASSERT(s_instance->hasEngine(), return nullptr);
    return s_instance->m_buildEngine;
}

bool SdkManager::startEngine()
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return startReliably(s_instance->m_buildEngine->virtualMachine());
}

bool SdkManager::stopEngine()
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return stopReliably(s_instance->m_buildEngine->virtualMachine());
}

bool SdkManager::isEngineRunning()
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return isRunningReliably(s_instance->m_buildEngine->virtualMachine());
}

int SdkManager::runOnEngine(const QString &program, const QStringList &arguments,
        QProcess::InputChannelMode inputChannelMode)
{
    QTC_ASSERT(s_instance->hasEngine(), return SFDK_EXIT_ABNORMAL);

    // Assumption to minimize the time spent here: if the VM is running, we must have been waiting
    // before for the engine to fully start, so no need to wait for connectTo again
    if (!isEngineRunning()) {
        qCInfo(sfdk).noquote() << tr("Starting the build engine…");
        if (!s_instance->m_buildEngine->virtualMachine()->connectTo(VirtualMachine::Block)) {
            qerr() << tr("Failed to start the build engine") << endl;
            return SFDK_EXIT_ABNORMAL;
        }
    }

    QString program_ = program;
    QStringList arguments_ = arguments;
    QString workingDirectory = QDir::current().canonicalPath();
    QProcessEnvironment extraEnvironment = s_instance->environmentToForwardToEngine();
    extraEnvironment.insert(Mer::Constants::SAILFISH_SDK_FRONTEND, Constants::SDK_FRONTEND_ID);

    if (!s_instance->mapEnginePaths(&program_, &arguments_, &workingDirectory, &extraEnvironment))
        return SFDK_EXIT_ABNORMAL;

    RemoteProcess process;
    process.setSshParameters(s_instance->m_buildEngine->virtualMachine()->sshParameters());
    process.setProgram(program_);
    process.setArguments(arguments_);
    process.setWorkingDirectory(workingDirectory);
    process.setExtraEnvironment(extraEnvironment);
    process.setInputChannelMode(inputChannelMode);

    QObject::connect(&process, &RemoteProcess::standardOutput, [&](const QByteArray &data) {
        qout() << s_instance->maybeReverseMapEnginePaths(data) << flush;
    });
    QObject::connect(&process, &RemoteProcess::standardError, [&](const QByteArray &data) {
        qerr() << s_instance->maybeReverseMapEnginePaths(data) << flush;
    });
    QObject::connect(&process, &RemoteProcess::connectionError, [&](const QString &errorString) {
        qerr() << tr("Error connecting to the build engine: ") << errorString << endl;
    });
    QObject::connect(&process, &RemoteProcess::processError, [&](const QString &errorString) {
        qerr() << tr("Error running command on the build engine: ") << errorString << endl;
    });

    return process.exec();
}

void SdkManager::setEnableReversePathMapping(bool enable)
{
    s_instance->m_enableReversePathMapping = enable;
}

Device *SdkManager::configuredDevice(QString *errorMessage)
{
    Q_ASSERT(errorMessage);

    const Option *const deviceOption = Dispatcher::option("device");
    QTC_ASSERT(deviceOption, return nullptr);

    const Utils::optional<OptionEffectiveOccurence> effectiveDeviceOption =
        Configuration::effectiveState(deviceOption);
    if (!effectiveDeviceOption) {
        *errorMessage = tr("No device selected");
        return nullptr;
    }
    QTC_ASSERT(!effectiveDeviceOption->isMasked(), return nullptr);

    return deviceByName(effectiveDeviceOption->argument(), errorMessage);
}

Device *SdkManager::deviceByName(const QString &deviceName, QString *errorMessage)
{
    Q_ASSERT(errorMessage);

    Device *device = nullptr;
    for (Device *const device_ : Sdk::devices()) {
        if (device_->name() == deviceName) {
            if (device) {
                *errorMessage = tr("Ambiguous device name \"%1\". Please fix your configuration.")
                    .arg(deviceName);
                return nullptr;
            }
            device = device_;
        }
    }

    if (!device)
        *errorMessage = deviceName + ": " + tr("No such device");

    return device;
}

int SdkManager::runOnDevice(const Device &device, const QString &program,
        const QStringList &arguments, QProcess::InputChannelMode inputChannelMode)
{
    if (device.machineType() == Device::EmulatorMachine) {
        Emulator *const emulator = static_cast<const EmulatorDevice &>(device).emulator();

        // Assumption to minimize the time spent here: if the VM is running, we must have been waiting
        // before for the emulator to fully start, so no need to wait for connectTo again
        emulator->virtualMachine()->refreshState(VirtualMachine::Synchronous);
        const bool emulatorRunning = !emulator->virtualMachine()->isOff();
        if (!emulatorRunning) {
            qCInfo(sfdk).noquote() << tr("Starting the emulator…");
            if (!emulator->virtualMachine()->connectTo(VirtualMachine::Block)) {
                qerr() << tr("Failed to start the emulator") << endl;
                return SFDK_EXIT_ABNORMAL;
            }
        }
    }

    RemoteProcess process;
    process.setSshParameters(device.sshParameters());
    process.setProgram(program);
    process.setArguments(arguments);
    process.setInputChannelMode(inputChannelMode);

    QObject::connect(&process, &RemoteProcess::standardOutput, [&](const QByteArray &data) {
        qout() << data << flush;
    });
    QObject::connect(&process, &RemoteProcess::standardError, [&](const QByteArray &data) {
        qerr() << data << flush;
    });
    QObject::connect(&process, &RemoteProcess::connectionError, [&](const QString &errorString) {
        if (device.machineType() == Device::EmulatorMachine)
            qerr() << tr("Error connecting to the emulator: ") << errorString << endl;
        else
            qerr() << tr("Error connecting to the device: ") << errorString << endl;
    });
    QObject::connect(&process, &RemoteProcess::processError, [&](const QString &errorString) {
        if (device.machineType() == Device::EmulatorMachine)
            qerr() << tr("Error running command on the emulator: ") << errorString << endl;
        else
            qerr() << tr("Error running command on the device: ") << errorString << endl;
    });

    return process.exec();
}

QList<Emulator *> SdkManager::sortedEmulators()
{
    QList<Emulator *> emulators = Sdk::emulators();
    Utils::sort(emulators, [](Emulator *e1, Emulator *e2) {
            return e1->name() > e2->name();
    });
    return emulators;
}

Emulator *SdkManager::emulatorByName(const QString &emulatorName, QString *errorMessage)
{
    Q_ASSERT(errorMessage);

    Emulator *emulator = nullptr;
    for (Emulator *const emulator_ : Sdk::emulators()) {
        if (emulator_->name() == emulatorName) {
            if (emulator) {
                *errorMessage = tr("Ambiguous emulator name \"%1\". Please fix your configuration.")
                    .arg(emulatorName);
                return nullptr;
            }
            emulator = emulator_;
        }
    }

    if (!emulator)
        *errorMessage = emulatorName + ": " + tr("No such emulator");

    return emulator;
}

bool SdkManager::startEmulator(const Emulator &emulator)
{
    return startReliably(emulator.virtualMachine());
}

bool SdkManager::stopEmulator(const Emulator &emulator)
{
    return stopReliably(emulator.virtualMachine());
}

bool SdkManager::isEmulatorRunning(const Emulator &emulator)
{
    return isRunningReliably(emulator.virtualMachine());
}

int SdkManager::runOnEmulator(const Emulator &emulator, const QString &program,
        const QStringList &arguments, QProcess::InputChannelMode inputChannelMode)
{
    Device *const emulatorDevice = Sdk::device(emulator);
    QTC_ASSERT(emulatorDevice, return SFDK_EXIT_ABNORMAL);
    return runOnDevice(*emulatorDevice, program, arguments, inputChannelMode);
}

bool SdkManager::startReliably(VirtualMachine *virtualMachine)
{
    QTC_ASSERT(virtualMachine, return false);
    virtualMachine->refreshState(VirtualMachine::Synchronous);
    return virtualMachine->connectTo(VirtualMachine::Block);
}

bool SdkManager::stopReliably(VirtualMachine *virtualMachine)
{
    QTC_ASSERT(virtualMachine, return false);
    virtualMachine->refreshState(VirtualMachine::Synchronous);
    return virtualMachine->lockDown(true);
}

bool SdkManager::isRunningReliably(VirtualMachine *virtualMachine)
{
    QTC_ASSERT(virtualMachine, return false);
    virtualMachine->refreshState(VirtualMachine::Synchronous);
    return !virtualMachine->isOff();
}

void SdkManager::saveSettings()
{
    QStringList errorStrings;
    s_instance->m_sdk->saveSettings(&errorStrings);
    for (const QString &errorString : errorStrings)
        qCWarning(sfdk) << "Error saving settings:" << errorString;
}

bool SdkManager::hasEngine() const
{
    return m_buildEngine != nullptr;
}

QString SdkManager::cleanSharedHome() const
{
    return QDir(QDir::cleanPath(m_buildEngine->sharedHomePath().toString())).canonicalPath();
}

QString SdkManager::cleanSharedSrc() const
{
    return QDir(QDir::cleanPath(m_buildEngine->sharedSrcPath().toString())).canonicalPath();
}

bool SdkManager::mapEnginePaths(QString *program, QStringList *arguments, QString *workingDirectory,
        QProcessEnvironment *environment) const
{
    const QString cleanSharedHome = this->cleanSharedHome();
    const QString cleanSharedSrc = this->cleanSharedSrc();

    if (!workingDirectory->startsWith(cleanSharedHome)
            && (cleanSharedSrc.isEmpty() || !workingDirectory->startsWith(cleanSharedSrc))) {
        qCDebug(sfdk) << "cleanSharedHome:" << cleanSharedHome;
        qCDebug(sfdk) << "cleanSharedSrc:" << cleanSharedSrc;
        qerr() << tr("The command needs to be used under build engine's share home or shared "
                "source directory, which are currently configured as \"%1\" and \"%2\"")
            .arg(m_buildEngine->sharedHomePath().toString())
            .arg(m_buildEngine->sharedSrcPath().toString()) << endl;
        return false;
    }

    Qt::CaseSensitivity caseInsensitiveOnWindows = Utils::HostOsInfo::isWindowsHost()
        ? Qt::CaseInsensitive
        : Qt::CaseSensitive;

    struct Mapping {
        QString hostPath;
        QString enginePath;
        Qt::CaseSensitivity cs;
    } const mappings[] = {
#if Q_CC_GNU <= 504 // Let's check if it is still needed with GCC > 5.4
        {cleanSharedHome, QLatin1String(Mer::Constants::MER_SDK_SHARED_HOME_MOUNT_POINT), Qt::CaseSensitive},
        {cleanSharedSrc, QLatin1String(Mer::Constants::MER_SDK_SHARED_SRC_MOUNT_POINT), caseInsensitiveOnWindows}
#else
        {cleanSharedHome, Mer::Constants::MER_SDK_SHARED_HOME_MOUNT_POINT, Qt::CaseSensitive},
        {cleanSharedSrc, Mer::Constants::MER_SDK_SHARED_SRC_MOUNT_POINT, caseInsensitiveOnWindows}
#endif
    };
    for (const Mapping &mapping : mappings) {
        qCDebug(sfdk) << "Mapping" << mapping.hostPath << "as" << mapping.enginePath;
        program->replace(mapping.hostPath, mapping.enginePath, mapping.cs);
        arguments->replaceInStrings(mapping.hostPath, mapping.enginePath, mapping.cs);
        workingDirectory->replace(mapping.hostPath, mapping.enginePath, mapping.cs);
        for (const QString &key : environment->keys()) {
            QString value = environment->value(key);
            value.replace(mapping.hostPath, mapping.enginePath, mapping.cs);
            environment->insert(key, value);
        }
    }

    qCDebug(sfdk) << "Command after mapping engine paths:" << *program << "arguments:" << *arguments
        << "CWD:" << *workingDirectory;

    return true;
}

QByteArray SdkManager::maybeReverseMapEnginePaths(const QByteArray &commandOutput) const
{
    if (!m_enableReversePathMapping)
        return commandOutput;

    const QString cleanSharedHome = this->cleanSharedHome();
    const QString cleanSharedSrc = this->cleanSharedSrc();

    QByteArray retv = commandOutput;

    if (!m_buildEngine->sharedSrcPath().isEmpty())
      retv.replace(Mer::Constants::MER_SDK_SHARED_SRC_MOUNT_POINT, cleanSharedSrc.toUtf8());

    if (!m_buildEngine->sharedHomePath().isEmpty())
      retv.replace(Mer::Constants::MER_SDK_SHARED_HOME_MOUNT_POINT, cleanSharedHome.toUtf8());

    return retv;
}

QProcessEnvironment SdkManager::environmentToForwardToEngine() const
{
    const QStringList patterns = MerSettings::environmentFilter()
        .split(QRegularExpression("[[:space:]]+"), QString::SkipEmptyParts);
    if (patterns.isEmpty())
        return {};

    QStringList regExps;
    for (const QString &pattern : patterns) {
        const QString asRegExp = QRegularExpression::escape(pattern).replace("\\*", ".*");
        regExps.append(asRegExp);
    }
    const QRegularExpression filter("^(" + regExps.join("|") + ")$");

    const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();

    QProcessEnvironment retv;

    QStringList keys = systemEnvironment.keys();
    keys.sort();

    for (const QString key : qAsConst(keys)) {
        if (key == Mer::Constants::SAILFISH_SDK_ENVIRONMENT_FILTER)
            continue;
        if (filter.match(key).hasMatch()) {
            if (Log::sfdk().isDebugEnabled()) {
                if (retv.isEmpty())
                    qCDebug(sfdk) << "Environment to forward to build engine (subject to path mapping):";
                const QString indent = Sfdk::indent(1);
                qCDebug(sfdk).noquote().nospace() << indent << key << '='
                    << Utils::QtcProcess::quoteArgUnix(systemEnvironment.value(key));
            }
            retv.insert(key, systemEnvironment.value(key));
        }
    }

    return retv;
}

#include "sdkmanager.moc"
