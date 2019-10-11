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
#include <utils/qtcprocess.h>

#include <sfdk/device.h>
#include <sfdk/sdk.h>

#include "commandlineparser.h"
#include "configuration.h"
#include "dispatch.h"
#include "sdkmanager.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
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

}

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
    else if (command->name == "engine")
        return runEngine(arguments, exitCode);
    else if (command->name == "maintain")
        return runMaintain(arguments, exitCode);

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
        *exitCode = Constants::EXIT_ABNORMAL;
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
        qout() << tr("Running: %1").arg(running ? tr("Yes") : tr("No")) << endl;
        *exitCode = EXIT_SUCCESS;
        return NormalExit;
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
