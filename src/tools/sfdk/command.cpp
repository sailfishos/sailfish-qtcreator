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

#include "command.h"

#include <QCommandLineParser>
#include <QDebug>

#include <ssh/sshremoteprocessrunner.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/virtualmachine.h>

#include "commandlineparser.h"
#include "configuration.h"
#include "dispatch.h"
#include "sdkmanager.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "task.h"
#include "textutils.h"

using namespace Sfdk;
using namespace Utils;

namespace {

const char PROGRAM_KEY[] = "program";
const char INITIAL_ARGUMENTS_KEY[] = "initialArguments";
const char OMIT_SUBCOMMAND_KEY[] = "omitSubcommand";
const char DIRECT_TERMINAL_INPUT_KEY[] = "directTerminalInput";
#ifdef Q_OS_MACOS
const char SDK_MAINTENANCE_TOOL[] = "SDKMaintenanceTool.app/Contents/MacOS/SDKMaintenanceTool";
#else
const char SDK_MAINTENANCE_TOOL[] = "SDKMaintenanceTool" QTC_HOST_EXE_SUFFIX;
#endif

const char WWW_PROXY_TYPE[] = "proxy";
const char WWW_PROXY_SERVERS[] = "proxy.servers";
const char WWW_PROXY_EXCLUDES[] = "proxy.excludes";

const char VM_MEMORY_SIZE_MB[] = "vm.memorySize";
const char VM_CPU_COUNT[] = "vm.cpuCount";
// FIXME vdi -> storage
const char VM_VDI_CAPACITY_MB[] = "vm.storageSize";

} // namespace anonymous

namespace Sfdk {

/*!
 * \class PropertiesAccessor
 */

class PropertiesAccessor
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::PropertiesAccessor)

public:
    virtual QMap<QString, QString> get() const = 0;
    virtual bool prepareSet(const QString &name, const QString &value, bool *needsVmOff,
            QString *errorString) = 0;
    virtual bool canSet(QString *errorString) const { Q_UNUSED(errorString); return true; }
    virtual bool set() = 0;

protected:
    static bool parsePositiveInt(int *out, const QString &string, QString *errorString)
    {
        bool ok;
        *out = string.toInt(&ok);
        if (!ok || *out <= 0) {
            *errorString = tr("Positive integer expected: \"%1\"").arg(string);
            return false;
        }

        return true;
    }

    static QString valueTooBigMessage()
    {
        return tr("Value too big");
    }
    static QString valueCannotBeDecreasedMessage()
    {
        return tr("Value cannot be decreased");
    }

    static QString unknownPropertyMessage(const QString &name)
    {
        return tr("Unrecognized property \"%1\"").arg(name);
    }
};

/*!
 * \class SetPropertiesTask
 */

class SetPropertiesTask : public Task
{
    Q_OBJECT

public:
    SetPropertiesTask(std::unique_ptr<PropertiesAccessor> &&accessor,
            VirtualMachine *virtualMachine, const QString &stopVmMessage)
        : m_accessor(std::move(accessor))
        , m_virtualMachine(virtualMachine)
        , m_stopVmMessage(stopVmMessage)
    {
    }

    QMap<QString, QString> get() const
    {
        return m_accessor->get();
    }

    bool prepareSet(const QString &name, const QString &value, QString *errorString)
    {
        bool needsVmOff = false;

        if (!m_accessor->prepareSet(name, value, &needsVmOff, errorString))
            return false;

        m_needsVmOff |= needsVmOff;
        return true;
    }

    bool set(QString *errorString)
    {
        if (!m_accessor->canSet(errorString))
            return false;

        started();

        bool ok = false;
        bool lockDownOk = false;

        if (m_needsVmOff) {
            if (SdkManager::isRunningReliably(m_virtualMachine)) {
                *errorString = m_stopVmMessage;
            } else {
                lockDownOk = m_virtualMachine->lockDown(true);
                QTC_CHECK(lockDownOk);
            }
        }

        if (!m_needsVmOff || lockDownOk) {
            ok = m_accessor->set();
            if (!ok)
                *errorString = tr("Failed to set some of the properties");
        }

        if (lockDownOk)
            m_virtualMachine->lockDown(false);

        exited();

        return ok;
    }

protected:
    void beginTerminate() override
    {
        qerr() << tr("Wait please...");
        endTerminate(true);
    }

    void beginStop() override
    {
        endStop(true);
    }

    void beginContinue() override
    {
        endContinue(true);
    }

private:
    const std::unique_ptr<PropertiesAccessor> m_accessor;
    const QPointer<VirtualMachine> m_virtualMachine;
    const QString m_stopVmMessage;
    bool m_needsVmOff = false;
};

/*!
 * \class VirtualMachinePropertiesAccessor
 */

class VirtualMachinePropertiesAccessor : public PropertiesAccessor
{
public:
    VirtualMachinePropertiesAccessor(VirtualMachine *virtualMachine)
        : m_vm(virtualMachine)
    {
        m_memorySizeMb = m_vm->memorySizeMb();
        m_cpuCount = m_vm->cpuCount();
        m_vdiCapacityMb = m_vm->vdiCapacityMb();
    }

    QMap<QString, QString> get() const override
    {
        QMap<QString, QString> values;
        values.insert(VM_MEMORY_SIZE_MB, QString::number(m_memorySizeMb));
        values.insert(VM_CPU_COUNT, QString::number(m_cpuCount));
        values.insert(VM_VDI_CAPACITY_MB, QString::number(m_vdiCapacityMb));
        return values;
    }

    bool prepareSet(const QString &name, const QString &value, bool *needsVmOff,
            QString *errorString) override
    {
        *needsVmOff = true;

        if (name == VM_MEMORY_SIZE_MB) {
            if (!parsePositiveInt(&m_memorySizeMb, value, errorString))
                return false;
            if (m_memorySizeMb > VirtualMachine::availableMemorySizeMb()) {
                *errorString = valueTooBigMessage();
                return false;
            }
            return true;
        } else if (name == VM_CPU_COUNT) {
            if (!parsePositiveInt(&m_cpuCount, value, errorString))
                return false;
            if (m_cpuCount > VirtualMachine::availableCpuCount()) {
                *errorString = valueTooBigMessage();
                return false;
            }
            return true;
        } else if (name == VM_VDI_CAPACITY_MB) {
            if (!parsePositiveInt(&m_vdiCapacityMb, value, errorString))
                return false;
            if (m_vdiCapacityMb < m_vm->vdiCapacityMb()) {
                *errorString = valueCannotBeDecreasedMessage();
                return false;
            }
            return true;
        } else {
            *errorString = unknownPropertyMessage(name);
            return false;
        }
    }

    bool set() override
    {
        bool ok = true;

        if (m_memorySizeMb != m_vm->memorySizeMb()) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setMemorySizeMb),
                    m_vm.data(), m_memorySizeMb);
            ok &= stepOk;
        }

        if (m_cpuCount != m_vm->cpuCount()) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setCpuCount),
                    m_vm.data(), m_cpuCount);
            ok &= stepOk;
        }

        if (m_vdiCapacityMb != m_vm->vdiCapacityMb()) {
            bool stepOk;
            execAsynchronous(std::tie(stepOk), std::mem_fn(&VirtualMachine::setVdiCapacityMb),
                    m_vm.data(), m_vdiCapacityMb);
            ok &= stepOk;
        }

        return ok;
    }

private:
    QPointer<VirtualMachine> m_vm;
    int m_memorySizeMb = 0;
    int m_cpuCount = 0;
    int m_vdiCapacityMb = 0;
};

/*!
 * \class SdkPropertiesAccessor
 */

class SdkPropertiesAccessor : public PropertiesAccessor
{
public:
    SdkPropertiesAccessor()
    {
        QTC_ASSERT(SdkManager::engine(), return);
        m_wwwProxyType = SdkManager::engine()->wwwProxyType();
        m_wwwProxyServers = SdkManager::engine()->wwwProxyServers();
        m_wwwProxyExcludes = SdkManager::engine()->wwwProxyExcludes();
    }

    QMap<QString, QString> get() const override
    {
        QMap<QString, QString> values;
        values.insert(WWW_PROXY_TYPE, m_wwwProxyType);
        values.insert(WWW_PROXY_SERVERS, m_wwwProxyServers);
        values.insert(WWW_PROXY_EXCLUDES, m_wwwProxyExcludes);
        return values;
    }

    bool prepareSet(const QString &name, const QString &value, bool *needsVmOff,
            QString *errorString) override
    {
        *needsVmOff = false;

        auto validateUrls = [](const QString &urls, QString *errorString) {
            for (const QString &url : urls.split(' ', QString::SkipEmptyParts)) {
                if (!QUrl::fromUserInput(url).isValid()) {
                    *errorString = tr("Not a valid URL: \"%1\"").arg(url);
                    return false;
                }
            }
            return true;
        };

        if (name == WWW_PROXY_TYPE) {
            if (value != Constants::WWW_PROXY_DISABLED
                    && value != Constants::WWW_PROXY_AUTOMATIC
                    && value != Constants::WWW_PROXY_MANUAL) {
                *errorString = tr("Invalid proxy type: \"%1\"").arg(value);
                return false;
            }
            m_wwwProxyType = value;
            return true;
        } else if (name == WWW_PROXY_SERVERS) {
            if (!validateUrls(value, errorString))
                return false;
            m_wwwProxyServers = value.trimmed();
            return true;
        } else if (name == WWW_PROXY_EXCLUDES) {
            if (!validateUrls(value, errorString))
                return false;
            m_wwwProxyExcludes = value.trimmed();
            return true;
        } else {
            *errorString = unknownPropertyMessage(name);
            return false;
        }
    }

    bool canSet(QString *errorString) const override
    {
        if (m_wwwProxyType == Constants::WWW_PROXY_MANUAL
                && m_wwwProxyServers.isEmpty()) {
            *errorString = tr("The value of \"%1\" must not be empty when \"%2\" is set to \"%3\"")
                .arg(WWW_PROXY_SERVERS)
                .arg(WWW_PROXY_TYPE)
                .arg(Constants::WWW_PROXY_MANUAL);
            return false;
        }

        return true;
    }

    bool set() override
    {
        BuildEngine *const engine = SdkManager::engine();

        if (m_wwwProxyType != engine->wwwProxyType()
                || m_wwwProxyServers != engine->wwwProxyServers()
                || m_wwwProxyExcludes != engine->wwwProxyExcludes()) {
            engine->setWwwProxy(m_wwwProxyType, m_wwwProxyServers, m_wwwProxyExcludes);
        }

        return true;
    }

private:
    QString m_wwwProxyType;
    QString m_wwwProxyServers;
    QString m_wwwProxyExcludes;
};

/*!
 * \class EmulatorPropertiesAccessor
 */

class EmulatorPropertiesAccessor : public PropertiesAccessor
{
public:
    EmulatorPropertiesAccessor(Emulator *emulator)
        : m_emulator(emulator)
        , m_vmAccessor(std::make_unique<VirtualMachinePropertiesAccessor>(
                    emulator->virtualMachine()))
    {
    }

    QMap<QString, QString> get() const override
    {
        return m_vmAccessor->get();
    }

    bool prepareSet(const QString &name, const QString &value, bool *needsVmOff,
            QString *errorString) override
    {
        return m_vmAccessor->prepareSet(name, value, needsVmOff, errorString);
    }

    bool set() override
    {
        return m_vmAccessor->set();
    }

private:
    QPointer<Emulator> m_emulator;
    std::unique_ptr<VirtualMachinePropertiesAccessor> m_vmAccessor;
};

/*!
 * \class BuildEnginePropertiesAccessor
 */

class BuildEnginePropertiesAccessor : public PropertiesAccessor
{
public:
    BuildEnginePropertiesAccessor(BuildEngine *engine)
        : m_engine(engine)
        , m_vmAccessor(std::make_unique<VirtualMachinePropertiesAccessor>(
                    engine->virtualMachine()))
    {
    }

    QMap<QString, QString> get() const override
    {
        return m_vmAccessor->get();
    }

    bool prepareSet(const QString &name, const QString &value, bool *needsVmOff,
            QString *errorString) override
    {
        return m_vmAccessor->prepareSet(name, value, needsVmOff, errorString);
    }

    bool set() override
    {
        return m_vmAccessor->set();
    }

private:
    QPointer<BuildEngine> m_engine;
    std::unique_ptr<VirtualMachinePropertiesAccessor> m_vmAccessor;
};

} // namespace Sfdk

/*!
 * \class Command
 */

const Domain *Command::domain() const
{
    return module->domain;
}

/*!
 * \class Worker
 */

QString Worker::crashExitErrorMessage()
{
    return tr("Command exited abnormally");
}

bool Worker::checkVersion(int version, int minSupported, int maxSupported, QString *errorMessage)
{
    if (version < minSupported || version > maxSupported) {
        *errorMessage = tr("Version unsupported: %1").arg(version);
        return false;
    }
    return true;
}

/*!
 * \class BuiltinWorker
 */

Worker::ExitStatus BuiltinWorker::run(const Command *command, const QStringList &arguments,
        int *exitCode) const
{
    Q_ASSERT(exitCode != nullptr);
    *exitCode = EXIT_SUCCESS;

    QStringList arguments0 = arguments;
    arguments0.prepend(command->name);

    if (command->name == "config")
        return runConfig(arguments0);
    else if (command->name == "device")
        return runDevice(arguments, exitCode);
    else if (command->name == "emulator")
        return runEmulator(arguments, exitCode);
    else if (command->name == "engine")
        return runEngine(arguments, exitCode);
    else if (command->name == "maintain")
        return runMaintain(arguments, exitCode);
    else if (command->name == "misc")
        return runMisc(arguments, exitCode);

    qCCritical(sfdk) << "No such builtin:" << command->name << endl;
    return NoSuchCommand;
}

std::unique_ptr<Worker> BuiltinWorker::fromMap(const QVariantMap &data, int version,
        QString *errorString)
{
    if (!checkVersion(version, 1, 1, errorString))
        return {};

    if (!Dispatcher::checkKeys(data, {}, errorString))
        return {};

    auto worker = std::make_unique<BuiltinWorker>();

#ifdef Q_OS_MACOS
    return std::move(worker);
#else
    return worker;
#endif
}

Worker::ExitStatus BuiltinWorker::runConfig(const QStringList &arguments0) const
{
    using P = CommandLineParser;

    if (!Configuration::isLoaded()) {
        qerr() << P::commandNotAvailableMessage(arguments0.first()) << endl;
        return BadUsage;
    }

    QCommandLineParser parser;
    QCommandLineOption showOption("show");
    QCommandLineOption globalOption("global");
    QCommandLineOption sessionOption("session");
    QCommandLineOption pushOption("push");
    QCommandLineOption pushMaskOption("push-mask");
    QCommandLineOption dropOption("drop");
    parser.addOptions({showOption, globalOption, sessionOption, pushOption,
            pushMaskOption, dropOption});
    parser.addPositionalArgument("name", QString(), "[name]");
    parser.addPositionalArgument("value", QString(), "[value]");

    if (!parser.parse(arguments0)) {
        qerr() << parser.errorText() << endl;
        return BadUsage;
    }

    const QCommandLineOption *modeOption;
    if (!P::checkExclusiveOption(parser, {&showOption, &pushOption, &pushMaskOption, &dropOption},
                &modeOption)) {
        return BadUsage;
    }

    if (modeOption == nullptr && parser.positionalArguments().isEmpty())
        modeOption = &showOption;

    if (modeOption == &showOption) {
        if (!P::checkExclusiveOption(parser, {&showOption, &globalOption}))
            return BadUsage;
        if (!P::checkExclusiveOption(parser, {&showOption, &sessionOption}))
            return BadUsage;
        if (!parser.positionalArguments().isEmpty()) {
            qerr() << P::unexpectedArgumentMessage(parser.positionalArguments().first()) << endl;
            return BadUsage;
        }
        qout() << Configuration::print();
        return NormalExit;
    } else {
        Configuration::Scope scope = parser.isSet(globalOption)
            ? Configuration::Global
            : Configuration::Session;

        if (parser.positionalArguments().isEmpty()) {
            qerr() << P::missingArgumentMessage() << endl;
            return BadUsage;
        }

        if (modeOption == nullptr) {
            if (parser.positionalArguments().count() > 1) {
                qerr() << P::unexpectedArgumentMessage(parser.positionalArguments().at(1)) << endl;
                return BadUsage;
            }
            OptionOccurence occurence =
                OptionOccurence::fromString(parser.positionalArguments().first());
            if (occurence.isNull()) {
                qerr() << P::invalidPositionalArgumentMessage(occurence.errorString(),
                        parser.positionalArguments().first()) << endl;
                return BadUsage;
            }

            Configuration::push(scope, occurence);
            return NormalExit;
        }

        const QString name = parser.positionalArguments().first();
        const Option *const option = Dispatcher::option(name);
        if (option == nullptr) {
            qerr() << P::unrecognizedOptionMessage(name) << endl;
            return BadUsage;
        }
        if ((modeOption != &pushOption || option->argumentType == Option::NoArgument)
                && parser.positionalArguments().count() > 1) {
            qerr() << P::unexpectedArgumentMessage(parser.positionalArguments().at(1)) << endl;
            return BadUsage;
        }
        if (modeOption == &pushOption && option->argumentType == Option::MandatoryArgument
                && parser.positionalArguments().count() != 2) {
            if (parser.positionalArguments().count() < 2)
                qerr() << P::missingArgumentMessage() << endl;
            else
                qerr() << P::unexpectedArgumentMessage(parser.positionalArguments().at(2)) << endl;
            return BadUsage;
        }

        if (modeOption == &pushOption) {
            const QString value = parser.positionalArguments().count() == 2
                ? parser.positionalArguments().last()
                : QString();
            Configuration::push(scope, option, value);
        } else if (modeOption == &pushMaskOption) {
            Configuration::pushMask(scope, option);
        } else if (modeOption == &dropOption) {
            Configuration::drop(scope, option);
        } else {
            Q_ASSERT(false);
        }

        return NormalExit;
    }
}

Worker::ExitStatus BuiltinWorker::runDevice(const QStringList &arguments, int *exitCode) const
{
    using P = CommandLineParser;

    if (arguments.count() < 1) {
        qerr() << P::missingArgumentMessage() << endl;
        return BadUsage;
    }

    if (arguments.first() == "list") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        listDevices();
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    QString errorString;
    QString deviceNameOrIndex;
    Device *device;
    if (arguments.count() < 2 || arguments.at(1) == "--") {
        device = SdkManager::configuredDevice(&errorString);
    } else {
        deviceNameOrIndex = arguments.at(1);
        device = deviceForNameOrIndex(deviceNameOrIndex, &errorString);
        // It was not the device name/idx but a command name
        if (!device && (arguments.count() < 3 || arguments.at(2) != "--")) {
            deviceNameOrIndex.clear();
            device = SdkManager::configuredDevice(&errorString);
        }
    }
    if (!device) {
        qerr() << errorString << endl;
        *exitCode = SFDK_EXIT_ABNORMAL;
        return NormalExit;
    }

    if (arguments.first() == "exec") {
        QStringList command = arguments.mid(1);
        if (!command.isEmpty() && command.first() == deviceNameOrIndex)
            command.removeFirst();
        if (!command.isEmpty() && command.first() == "--")
            command.removeFirst();

        QString program;
        QStringList programArguments;
        if (!command.isEmpty()) {
            program = command.first();
            if (command.count() > 1)
                programArguments = command.mid(1);
        } else {
            program = "/bin/bash";
            programArguments << "--login";
        }

        *exitCode = SdkManager::runOnDevice(*device, program, programArguments,
                QProcess::ForwardedInputChannel);
        return NormalExit;
    }

    qerr() << P::unrecognizedCommandMessage(arguments.first()) << endl;
    return BadUsage;
}

Worker::ExitStatus BuiltinWorker::runEmulator(const QStringList &arguments, int *exitCode) const
{
    using P = CommandLineParser;

    if (arguments.count() < 1) {
        qerr() << P::missingArgumentMessage() << endl;
        return BadUsage;
    }

    if (arguments.first() == "list") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        listEmulators();
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    QString errorString;
    QString emulatorNameOrIndex;
    Emulator *emulator;
    if (arguments.count() < 2 || arguments.at(1) == "--") {
        emulator = emulatorForNameOrIndex("0", &errorString);
    } else {
        emulatorNameOrIndex = arguments.at(1);
        emulator = emulatorForNameOrIndex(emulatorNameOrIndex, &errorString);
        // It was not the emulator name/idx but a command name
        if (!emulator && (arguments.first() == "exec" || arguments.first() == "set")
                && (arguments.count() < 3 || arguments.at(2) != "--")) {
            emulatorNameOrIndex.clear();
            emulator = emulatorForNameOrIndex("0", &errorString);
        }
    }
    if (!emulator) {
        qerr() << errorString << endl;
        *exitCode = SFDK_EXIT_ABNORMAL;
        return NormalExit;
    }

    if (arguments.first() == "start") {
        if (arguments.count() > 2) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(2)) << endl;
            return BadUsage;
        }
        *exitCode = SdkManager::startEmulator(*emulator) ? EXIT_SUCCESS : EXIT_FAILURE;
        return NormalExit;
    }

    if (arguments.first() == "stop") {
        if (arguments.count() > 2) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(2)) << endl;
            return BadUsage;
        }
        *exitCode = SdkManager::stopEmulator(*emulator) ? EXIT_SUCCESS : EXIT_FAILURE;
        return NormalExit;
    }

    if (arguments.first() == "status") {
        if (arguments.count() > 2) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(2)) << endl;
            return BadUsage;
        }
        bool running = SdkManager::isEmulatorRunning(*emulator);
        qout() << runningYesNoMessage(running) << endl;
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    if (arguments.first() == "show") {
        if (arguments.count() > 2) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(2)) << endl;
            return BadUsage;
        }
        printProperties(EmulatorPropertiesAccessor(emulator));
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    if (arguments.first() == "set") {
        QStringList assignments = arguments.mid(1);
        if (!assignments.isEmpty() && assignments.first() == emulatorNameOrIndex)
            assignments.removeFirst();
        if (!assignments.isEmpty() && assignments.first() == "--")
            assignments.removeFirst();

        if (assignments.isEmpty()) {
            qerr() << P::missingArgumentMessage() << endl;
            return BadUsage;
        }

        SetPropertiesTask task(
                std::make_unique<EmulatorPropertiesAccessor>(emulator),
                emulator->virtualMachine(),
                tr("Some of the changes cannot be applied while the emulator is running."
                    " Please stop the emulator."));
        return setProperties(&task, assignments, exitCode);
    }

    if (arguments.first() == "exec") {
        QStringList command = arguments.mid(1);
        if (!command.isEmpty() && command.first() == emulatorNameOrIndex)
            command.removeFirst();
        if (!command.isEmpty() && command.first() == "--")
            command.removeFirst();

        QString program;
        QStringList programArguments;
        if (!command.isEmpty()) {
            program = command.first();
            if (command.count() > 1)
                programArguments = command.mid(1);
        } else {
            program = "/bin/bash";
            programArguments << "--login";
        }

        *exitCode = SdkManager::runOnEmulator(*emulator, program, programArguments,
                QProcess::ForwardedInputChannel);
        return NormalExit;
    }

    qerr() << P::unrecognizedCommandMessage(arguments.first()) << endl;
    return BadUsage;
}

Worker::ExitStatus BuiltinWorker::runEngine(const QStringList &arguments, int *exitCode) const
{
    using P = CommandLineParser;

    if (arguments.count() < 1) {
        qerr() << P::missingArgumentMessage() << endl;
        return BadUsage;
    }

    if (arguments.first() == "start") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        *exitCode = SdkManager::startEngine() ? EXIT_SUCCESS : EXIT_FAILURE;
        return NormalExit;
    }

    if (arguments.first() == "stop") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        *exitCode = SdkManager::stopEngine() ? EXIT_SUCCESS : EXIT_FAILURE;
        return NormalExit;
    }

    if (arguments.first() == "status") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        bool running = SdkManager::isEngineRunning();
        qout() << runningYesNoMessage(running) << endl;
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    if (arguments.first() == "show") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        printProperties(BuildEnginePropertiesAccessor(SdkManager::engine()));
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    if (arguments.first() == "set") {
        const QStringList assignments = arguments.mid(1);

        if (assignments.isEmpty()) {
            qerr() << P::missingArgumentMessage() << endl;
            return BadUsage;
        }

        SetPropertiesTask task(
                std::make_unique<BuildEnginePropertiesAccessor>(SdkManager::engine()),
                SdkManager::engine()->virtualMachine(),
                tr("Some of the changes cannot be applied while the build engine is running."
                    " Please stop the build engine."));
        return setProperties(&task, assignments, exitCode);
    }

    if (arguments.first() == "exec") {
        const QStringList command = arguments.mid(1);

        QString program;
        QStringList programArguments;
        if (!command.isEmpty()) {
            program = command.at(0);
            if (command.count() > 1)
                programArguments = command.mid(1);
        } else {
            program = "/bin/bash";
            programArguments << "--login";
        }
        *exitCode = SdkManager::runOnEngine(program, programArguments, QProcess::ForwardedInputChannel);
        return NormalExit;
    }

    qerr() << P::unrecognizedCommandMessage(arguments.first()) << endl;
    return BadUsage;
}

Worker::ExitStatus BuiltinWorker::runMaintain(const QStringList &arguments, int *exitCode) const
{
    using P = CommandLineParser;

    if (!arguments.isEmpty()) {
        qerr() << P::unexpectedArgumentMessage(arguments.first()) << endl;
        return BadUsage;
    }

    QString tool = SdkManager::installationPath() + '/' + SDK_MAINTENANCE_TOOL;
    *exitCode = QProcess::startDetached(tool, {}) ? EXIT_SUCCESS : EXIT_FAILURE;
    return NormalExit;
}

Worker::ExitStatus BuiltinWorker::runMisc(const QStringList &arguments, int *exitCode) const
{
    using P = CommandLineParser;

    if (arguments.count() < 1) {
        qerr() << P::missingArgumentMessage() << endl;
        return BadUsage;
    }

    if (arguments.first() == "stop-vms") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        *exitCode = stopVirtualMachines() ? EXIT_SUCCESS : EXIT_FAILURE;
        return NormalExit;
    }

    if (arguments.first() == "show") {
        if (arguments.count() > 1) {
            qerr() << P::unexpectedArgumentMessage(arguments.at(1)) << endl;
            return BadUsage;
        }
        printProperties(SdkPropertiesAccessor());
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
    }

    if (arguments.first() == "set") {
        const QStringList assignments = arguments.mid(1);

        if (assignments.isEmpty()) {
            qerr() << P::missingArgumentMessage() << endl;
            return BadUsage;
        }

        SetPropertiesTask task(
                std::make_unique<SdkPropertiesAccessor>(),
                SdkManager::engine()->virtualMachine(),
                tr("Some of the changes cannot be applied while the build engine is running."
                    " Please stop the build engine."));
        return setProperties(&task, assignments, exitCode);
    }

    qerr() << P::unrecognizedCommandMessage(arguments.first()) << endl;
    return BadUsage;
}

void BuiltinWorker::listDevices()
{
    auto maxLength = [](const QStringList &strings) {
        const QList<int> lengths = Utils::transform(strings, &QString::length);
        return *std::max_element(lengths.begin(), lengths.end());
    };

    const QString hardwareType = tr("hardware-device");
    const QString emulatorType = tr("emulator");
    const int typeFieldWidth = maxLength({hardwareType, emulatorType});

    const QString autodetected = SdkManager::stateAutodetectedMessage();
    const QString userDefined = SdkManager::stateUserDefinedMessage();
    const int autodetectedFieldWidth = maxLength({autodetected, userDefined});

    int index = 0;
    for (Device *const device : Sdk::devices()) {
        const QString type = device->machineType() == Device::HardwareMachine
            ? hardwareType
            : emulatorType;
        const QString autodetection = device->isAutodetected()
            ? autodetected
            : userDefined;
        const QString privateKeyFile = FileUtils::shortNativePath(
                FileName::fromString(device->sshParameters().privateKeyFile));

        qout() << '#' << index << ' ' << '"' << device->name() << '"' << endl;
        qout() << indent(1) << qSetFieldWidth(typeFieldWidth) << left << type << qSetFieldWidth(0)
            << "  " << qSetFieldWidth(autodetectedFieldWidth) << autodetection << qSetFieldWidth(0)
            << "  " << device->sshParameters().url.authority() << endl;
        qout() << indent(1) << tr("private-key:") << ' ' << privateKeyFile << endl;

        ++index;
    }
}

Device *BuiltinWorker::deviceForNameOrIndex(const QString &deviceNameOrIndex,
        QString *errorString)
{
    bool isInt;
    const int deviceIndex = deviceNameOrIndex.toInt(&isInt);
    if (isInt) {
        if (deviceIndex < 0 || deviceIndex > Sdk::devices().count() - 1) {
            *errorString = tr("Invalid device index: %1").arg(deviceNameOrIndex);
            return nullptr;
        }
        return Sdk::devices().at(deviceIndex);
    } else {
        return SdkManager::deviceByName(deviceNameOrIndex, errorString);
    }
}

void BuiltinWorker::listEmulators()
{
    auto maxLength = [](const QStringList &strings) {
        const QList<int> lengths = Utils::transform(strings, &QString::length);
        return *std::max_element(lengths.begin(), lengths.end());
    };

    const QString autodetected = SdkManager::stateAutodetectedMessage();
    const QString userDefined = SdkManager::stateUserDefinedMessage();
    const int autodetectedFieldWidth = maxLength({autodetected, userDefined});

    int index = 0;
    for (Emulator *const emulator : SdkManager::sortedEmulators()) {
        const QString autodetection = emulator->isAutodetected()
            ? autodetected
            : userDefined;
        const QString privateKeyFile = FileUtils::shortNativePath(
                FileName::fromString(emulator->virtualMachine()->sshParameters().privateKeyFile));

        qout() << '#' << index << ' ' << '"' << emulator->name() << '"' << endl;
        qout() << indent(1) << qSetFieldWidth(autodetectedFieldWidth) << left << autodetection
            << qSetFieldWidth(0) << "  "
            << emulator->virtualMachine()->sshParameters().url.authority() << endl;
        qout() << indent(1) << tr("private-key:") << ' ' << privateKeyFile << endl;

        ++index;
    }
}

Emulator *BuiltinWorker::emulatorForNameOrIndex(const QString &emulatorNameOrIndex,
        QString *errorString)
{
    bool isInt;
    const int emulatorIndex = emulatorNameOrIndex.toInt(&isInt);
    if (isInt) {
        if (emulatorIndex < 0 || emulatorIndex > Sdk::emulators().count() - 1) {
            *errorString = tr("Invalid emulator index: %1").arg(emulatorNameOrIndex);
            return nullptr;
        }
        return SdkManager::sortedEmulators().at(emulatorIndex);
    } else {
        return SdkManager::emulatorByName(emulatorNameOrIndex, errorString);
    }
}

bool BuiltinWorker::stopVirtualMachines()
{
    for (BuildEngine *const engine : Sdk::buildEngines()) {
        if (!SdkManager::stopReliably(engine->virtualMachine())) {
            qerr() << tr("Failed to stop the build engine \"%1\"").arg(engine->name()) << endl;
            return false;
        }
    }

    for (Emulator *const emulator : Sdk::emulators()) {
        if (!SdkManager::stopReliably(emulator->virtualMachine())) {
            qerr() << tr("Failed to stop the emulator \"%1\"").arg(emulator->name()) << endl;
            return false;
        }
    }

    return true;
}

void BuiltinWorker::printProperties(const PropertiesAccessor &accessor)
{
    QMap<QString, QString> properties = accessor.get();
    for (auto it = properties.cbegin(); it != properties.cend(); ++it)
        qout() << it.key() << ':' << ' ' << it.value() << endl;
}

Worker::ExitStatus BuiltinWorker::setProperties(SetPropertiesTask *task,
        const QStringList &assignments, int *exitCode)
{
    for (const QString &assignment : assignments) {
        const int splitAt = assignment.indexOf('=');
        if (splitAt <= 0 || splitAt == assignment.length() - 1) {
            qerr() << tr("Assignment expected: \"%1\"").arg(assignment) << endl;
            return BadUsage;
        }
        const QString property = assignment.left(splitAt);
        const QString value = assignment.mid(splitAt + 1);
        QString errorString;
        if (!task->prepareSet(property, value, &errorString)) {
            *exitCode = EXIT_FAILURE;
            qerr() << errorString << endl;
            return NormalExit;
        }
    }

    QString errorString;
    if (!task->set(&errorString)) {
        *exitCode = EXIT_FAILURE;
        qerr() << errorString << endl;
        return NormalExit;
    }

    *exitCode = EXIT_SUCCESS;
    return NormalExit;
}

QString BuiltinWorker::runningYesNoMessage(bool running)
{
    return tr("Running: %1").arg(running ? tr("Yes") : tr("No"));
}

/*!
 * \class EngineWorker
 */

Worker::ExitStatus EngineWorker::run(const Command *command, const QStringList &arguments,
        int *exitCode) const
{
    Q_ASSERT(exitCode != nullptr);

    QStringList allArguments;
    allArguments += m_initialArguments;
    allArguments += Configuration::toArguments(command->module);
    if (!m_omitSubcommand)
        allArguments += command->name;
    allArguments += arguments;

    qCDebug(sfdk) << "About to run on build engine:" << m_program << "arguments:" << allArguments;

    const QProcess::InputChannelMode inputChannelMode = m_directTerminalInput
        ? QProcess::ForwardedInputChannel
        : QProcess::ManagedInputChannel;

    *exitCode = SdkManager::runOnEngine(m_program, allArguments, inputChannelMode);
    return NormalExit;
}

std::unique_ptr<Worker> EngineWorker::fromMap(const QVariantMap &data, int version,
        QString *errorString)
{
    if (!checkVersion(version, 1, 2, errorString))
        return {};

    if (!Dispatcher::checkKeys(data, {PROGRAM_KEY, INITIAL_ARGUMENTS_KEY, OMIT_SUBCOMMAND_KEY,
                DIRECT_TERMINAL_INPUT_KEY}, errorString)) {
        return {};
    }

    auto worker = std::make_unique<EngineWorker>();

    QVariant program = Dispatcher::value(data, PROGRAM_KEY, QVariant::String, {}, errorString);
    if (!program.isValid())
        return {};
    worker->m_program = program.toString();

    QVariant initialArguments = Dispatcher::value(data, INITIAL_ARGUMENTS_KEY, QVariant::List,
            QStringList(), errorString);
    if (!initialArguments.isValid())
        return {};
    if (!Dispatcher::checkItems(initialArguments.toList(), QVariant::String, errorString))
        return {};
    worker->m_initialArguments = initialArguments.toStringList();

    QVariant omitCommand = Dispatcher::value(data, OMIT_SUBCOMMAND_KEY, QVariant::Bool, false,
            errorString);
    worker->m_omitSubcommand = omitCommand.toBool();

    QVariant directTerminalInput = Dispatcher::value(data, DIRECT_TERMINAL_INPUT_KEY,
            QVariant::Bool, false, errorString);
    worker->m_directTerminalInput = directTerminalInput.toBool();

#ifdef Q_OS_MACOS
    return std::move(worker);
#else
    return worker;
#endif
}

#include "command.moc"
