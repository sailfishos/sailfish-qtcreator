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

#include "commandlineparser.h"

#include "command.h"
#include "configuration.h"
#include "dispatch.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>

#include <utils/algorithm.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QCommandLineParser>
#include <QRegularExpression>
#include <QSettings>

using namespace Sfdk;
using namespace Utils;

namespace {
using Constants::EXE_NAME;
}

/*!
 * \class CommandLineParser
 */

CommandLineParser::CommandLineParser(const QStringList &arguments)
{
    bool ok;

    QCommandLineParser parser;
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);

    QCommandLineOption hOption("h");
    hOption.setDescription(tr("Display the brief description and exit"));
    QCommandLineOption helpOption("help");
    helpOption.setDescription(tr("Display the generic, introductory manual and exit"));
    QCommandLineOption helpAllOption("help-all");
    helpAllOption.setDescription(tr("Display the all-in-one manual and exit. This provides the combined view of all the --help-<domain> provided manuals."));
    m_helpOptions = {hOption, helpOption, helpAllOption};
    parser.addOptions(m_helpOptions);

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        if (domain->name == Constants::GENERAL_DOMAIN_NAME)
            continue;
        QCommandLineOption domainHelpOption("help-" + domain->name);
        m_domainHelpOptions.append({domainHelpOption, domain.get()});
        parser.addOption(domainHelpOption);
    }

    for (const std::unique_ptr<const Option> &option : Dispatcher::options()) {
        QTC_ASSERT(option->alias.isNull() || option->argumentType == Option::MandatoryArgument,
                continue);
        if (option->alias.isNull())
            continue;

        QCommandLineOption alias(option->alias);
        alias.setValueName(option->argumentDescription);
        alias.setDescription(tr("This is a shorthand alias for the '%1' configuration option.")
                .arg(option->name));
        m_aliasOptions.append({alias, option.get()});
        parser.addOption(alias);
    }

    QCommandLineOption quietOption("quiet");
    quietOption.setDescription(tr("Suppress informational messages.\n\nThis option only affects generic messages. Subcommands may provide their own equivalents of this option to suppress their informational messages."));
    m_otherOptions.append(quietOption);

    QCommandLineOption debugOption("debug");
    debugOption.setDescription(tr("Enable diagnostic messages and disable reverse path mapping in command output.\n\nWhen a command is executed inside the build engine, certain paths from the host file system are available to the command through shared locations. Normally reverse mapping is done on shared paths the command prints on it standard output and/or error stream to turn them to valid host file system paths. When debug mode is activated, this function is suppressed in favor of greater clarity.\n\nThis option only affects generic diagnostic messages. Subcommands may provide their own equivalents of this option to enable their specific diagnostic messages."));
    m_otherOptions.append(debugOption);

    QCommandLineOption versionOption("version");
    versionOption.setDescription(tr("Report the version information and exit"));
    m_otherOptions.append(versionOption);

    QCommandLineOption noPagerOption("no-pager");
    noPagerOption.setDescription(tr("Do not paginate output"));
    m_otherOptions.append(noPagerOption);

    QCommandLineOption cOption("c");
    cOption.setValueName(tr("<name>[=[<value>]]"));
    cOption.setDescription(tr("Push the configuration option <name>. Omitting just <value> masks the option (see the 'config' subcommand). Omitting both <value> and '=' sets the option using the default value for its optional argument if any.\n\nSee the 'config' command for more details about configuration."));
    m_otherOptions.append(cOption);

    QCommandLineOption COption("C");
    COption.setValueName(tr("<path>"));
    COption.setDescription(tr("Run as if the current working directory was changed to <path> first. Each subsequent occurrence with a relative <path> is applied relatively to the preceding path. When <path> is empty, the current working directory is left unchanged.\n\nWhen \"%1\" is passed (literally), the current working directory will be changed to the %2 workspace directory.")
            .arg(QLatin1String(Constants::WORKSPACE_PSEUDO_VARIABLE))
            .arg(Sdk::sdkVariant()));
    m_otherOptions.append(COption);

    QCommandLineOption noSessionOption("no-session");
    noSessionOption.setDescription(tr("Do not try to read or write session-scope configuration. Alternatively, the same effect can be achieved by setting the '%1' environment variable.\n\nSee the 'config' command for more details about configuration.")
            .arg(QLatin1String(Constants::NO_SESSION_ENV_VAR)));
    m_otherOptions.append(noSessionOption);

    QCommandLineOption systemConfigOnlyOption("system-config-only");
    systemConfigOnlyOption.setDescription(tr("Enable the special purpose mode in which just the system-scope configuration is read and only read. The '-c' option handling is not affected by this mode. Implies '--no-session'. You want to enable this mode when invoking %1 during SDK installation or maintenance.")
            .arg(QLatin1String(EXE_NAME)));
    m_otherOptions.append(systemConfigOnlyOption);

    parser.addOptions(m_otherOptions);

    QStringList allArguments = arguments;

    const QStringList argumentsFromEnvironment =
        environmentVariableAsArguments(Constants::OPTIONS_ENV_VAR, &ok);
    if (!ok) {
        qerr() << tr("Malformed content of the \"%1\" environment variable")
            .arg(Constants::OPTIONS_ENV_VAR) << endl;
        m_result = BadUsage;
        return;
    }

    if (!argumentsFromEnvironment.isEmpty()) {
        if (!parser.parse(QStringList(arguments.first()) + argumentsFromEnvironment)) {
            qerr() << tr("%1 (Arguments received via the \"%2\" environment variable)")
                .arg(parser.errorText())
                .arg(Constants::OPTIONS_ENV_VAR)
                << endl;
            m_result = BadUsage;
            return;
        }

        if (!parser.positionalArguments().isEmpty()) {
            qerr() << tr("Unexpected positional argument received via the \"%1\" environment variable: \"%2\"")
                .arg(Constants::OPTIONS_ENV_VAR)
                .arg(parser.positionalArguments().first())
                << endl;
            m_result = BadUsage;
            return;
        }

        qCInfo(sfdk).noquote() << tr("Options from environment: %1")
            .arg(qEnvironmentVariable(Constants::OPTIONS_ENV_VAR));

        QStringList combined = argumentsFromEnvironment;
        combined.prepend(allArguments.first());
        combined.append(allArguments.mid(1));
        allArguments = combined;
    }

    if (!parser.parse(allArguments)) {
        badUsage(parser.errorText());
        m_result = BadUsage;
        return;
    }

    for (const QString &value : parser.values(COption)) {
        // setCurrentWorkingDirectory() cannot be used before SdkManager is instantiated
        m_cwdChanges += [=]() -> bool {
            QString errorString;
            if (!setCurrentWorkingDirectory(value, &errorString)) {
                badUsage(invalidArgumentToOptionMessage(errorString, COption.names().first(), value));
                return false;
            }
            return true;
        };
    }

    if (parser.isSet(noPagerOption))
        Pager::setEnabled(false);

    if (!checkExclusiveOption(parser, {&quietOption, &debugOption})) {
        m_result = BadUsage;
        return;
    }
    if (parser.isSet(quietOption))
        m_verbosity = Quiet;
    else if (parser.isSet(debugOption))
        m_verbosity = Debug;

    if (parser.isSet(helpAllOption)) {
        allDomainsUsage();
        m_result = Usage;
        return;
    }
    for (const QPair<QCommandLineOption, const Domain *> &pair : qAsConst(m_domainHelpOptions)) {
        if (parser.isSet(pair.first)) {
            domainUsage(pair.second);
            m_result = Usage;
            return;
        }
    }
    if (parser.isSet(helpOption) && parser.positionalArguments().isEmpty()) {
        usage();
        m_result = Usage;
        return;
    }
    if (parser.isSet(hOption)) {
        briefUsage(qout());
        m_result = Usage;
        return;
    }
    if (parser.isSet(versionOption)) {
        m_result = Version;
        return;
    }

    if (parser.isSet(noSessionOption)
            || qEnvironmentVariableIntValue(Constants::NO_SESSION_ENV_VAR)) {
        m_noSession = true;
    }

    // "config" is more in line with the UI, "settings" more with the code...
    if (parser.isSet(systemConfigOnlyOption)) {
        m_useSystemSettingsOnly = true;
        m_noSession = true;
    }

    for (const QString &value : parser.values(cOption)) {
        OptionOccurence occurence = OptionOccurence::fromString(value);
        if (occurence.isNull()) {
            badUsage(invalidArgumentToOptionMessage(occurence.errorString(),
                        cOption.names().first(), value));
            m_result = BadUsage;
            return;
        }

        if (occurence.type() == OptionOccurence::Push && !occurence.argument().isEmpty()) {
            // Argument validation needs to be delayed until SdkManager is instantiated
            m_configOptionsValidators += [=]() -> bool {
                QString errorString;
                if (!occurence.isArgumentValid(&errorString)) {
                    badUsage(invalidArgumentToOptionMessage(errorString, cOption.names().first(),
                                value));
                    return false;
                }
                return true;
            };
        }

        Configuration::push(Configuration::Command, occurence);
    }

    for (const QPair<QCommandLineOption, const Option *> &pair : qAsConst(m_aliasOptions)) {
        if (!parser.isSet(pair.first))
            continue;

        QString argument = parser.values(pair.first).last();
        if (argument.isEmpty()) {
            badUsage(unexpectedEmptyArgumentToOptionMessage(pair.first.names().first()));
            m_result = BadUsage;
            return;
        }

        const OptionOccurence occurence(pair.second, OptionOccurence::Push, argument);

        // Argument validation needs to be delayed until SdkManager is instantiated
        m_configOptionsValidators += [=]() -> bool {
            QString errorString;
            if (!occurence.isArgumentValid(&errorString)) {
                badUsage(invalidArgumentToOptionMessage(errorString, pair.first.names().first(),
                            argument));
                return false;
            }
            return true;
        };

        Configuration::push(Configuration::Command, occurence);
    }

    QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        badUsage(tr("Command name expected"));
        m_result = BadUsage;
        return;
    }

    QString commandName = positionalArguments.takeFirst();
    m_command = Dispatcher::command(commandName);
    if (m_command == nullptr) {
        badUsage(unrecognizedCommandMessage(commandName));
        m_result = BadUsage;
        return;
    }

    if (parser.isSet(helpOption)) {
        commandUsage(m_command);
        m_result = Usage;
        return;
    }

    m_commandArguments = positionalArguments;

    // When checking for the help-request options, only check up to the
    // first non-option argument to a dynamic (sub)command to allow dynamic
    // subcommands handle these themselves.
    QStringList commandArgumentsWithoutDynamicSubcommands;
    if (m_command->dynamic) {
        for (const QString &argument : m_commandArguments) {
            if (!argument.startsWith('-'))
                break;
            commandArgumentsWithoutDynamicSubcommands.append(argument);
        }
    } else if (!m_command->dynamicSubcommands.isEmpty()) {
        bool underDynamic = false;
        for (const QString &argument : m_commandArguments) {
            if (m_command->dynamicSubcommands.contains(argument))
                underDynamic = true;
            else if (underDynamic && !argument.startsWith('-'))
                break;
            commandArgumentsWithoutDynamicSubcommands.append(argument);
        }
    } else {
        commandArgumentsWithoutDynamicSubcommands = m_commandArguments;
    }

    // We can just guess here...
    if (commandArgumentsWithoutDynamicSubcommands.contains("-h")) {
        commandBriefUsage(qout(), m_command);
        m_result = Usage;
        return;
    }
    if (commandArgumentsWithoutDynamicSubcommands.contains("--help")) {
        commandUsage(m_command);
        m_result = Usage;
        return;
    }
    if (commandArgumentsWithoutDynamicSubcommands.contains("--help-all")) {
        allDomainsUsage();
        m_result = Usage;
        return;
    }
    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        if (commandArgumentsWithoutDynamicSubcommands.contains("--help-" + domain->name)) {
            domainUsage(domain.get());
            m_result = Usage;
            return;
        }
    }

    m_result = Dispatch;
}

bool CommandLineParser::setCurrentWorkingDirectory() const
{
    for (const std::function<bool()> &cd : m_cwdChanges) {
        if (!cd())
            return false;
    }
    return true;
}

bool CommandLineParser::validateCommandScopeConfiguration() const
{
    for (const std::function<bool()> &validator : m_configOptionsValidators) {
        if (!validator())
            return false;
    }
    return true;
}

void CommandLineParser::badUsage(const QString &message) const
{
    qerr() << message << endl;
    briefUsage(qerr());
}

void CommandLineParser::briefUsage(QTextStream &out) const
{
    wrapLines(out, 0, {usageMessage()}, {EXE_NAME}, synopsis());
    wrapLine(out, 0, summary());
    out << tryLongHelpMessage("--help") << endl;
}

void CommandLineParser::usage() const
{
    const QString title = QString(EXE_NAME);
    showManual(title, [=](QTextStream &out) { usage(out, title); });
}

void CommandLineParser::usage(QTextStream &out, const QString &title) const
{
    const Domain *generalDomain = Dispatcher::domain(Constants::GENERAL_DOMAIN_NAME);
    QTC_ASSERT(generalDomain != nullptr, return);

    AsciiMan::header(out, title);
    out << endl;

    AsciiMan::nameSection(out);
    out << endl;
    out << introManPageSummary(title) << endl;
    out << endl;

    AsciiMan::synopsisSection(out);
    out << endl;
    synopsis(out);
    out << endl;

    AsciiMan::subsection(out, commandsOverviewHeading());
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        out << "==== " << domain->briefDescription() << endl;
        out << endl;
        describeBriefly(out, domain->commands());
        out << endl;
        if (domain->name == Constants::GENERAL_DOMAIN_NAME) {
            out << tr("More information follows below.") << endl;
        } else {
            out << tryLongHelpMessage("--help-" + domain->name) << endl;
        }
        out << endl;
    }

    AsciiMan::section(out, descriptionHeading());
    out << endl;

    for (const Module *module : generalDomain->modules()) {
        if (!module->description.isEmpty()) {
            out << module->description << endl;
            out << endl;
            out << endl;
        }
    }

    AsciiMan::subsection(out, tr("Further Reading"));
    out << endl;

    out << tr("Use the '--help-<domain>' options to read more on the particular <domain> or the '--help-all' option for the all-in-one manual.") << endl;
    out << endl;

    AsciiMan::section(out, globalOptionsHeading());
    out << endl;

    describeGlobalOptions(out, nullptr);

    const Option::ConstList generalDomainOptions = generalDomain->options();
    out << endl;
    AsciiMan::section(out, configurationOptionsHeading());
    out << endl;
    if (generalDomainOptions.isEmpty()) {
        out << tr("None of the general-usage commands supports any configuration options. Use the '--help-<domain>' options to learn about configuration options supported by commands specific to the particular <domain>.") << endl;
        out << endl;
    } else {
        out << tr("This is the description of the general-usage configuration options. Use the '--help-<domain>' options to display description of configuration options specific to the particular <domain>.") << endl;
        out << endl;
        describe(out, generalDomainOptions);
    }

    const Hook::ConstList generalDomainHooks = generalDomain->hooks();
    out << endl;
    AsciiMan::section(out, hooksHeading());
    out << endl;
    if (generalDomainHooks.isEmpty()) {
        out << tr("None of the general-usage commands supports any hooks. Use the '--help-<domain>' options to learn about hooks supported by commands specific to the particular <domain>.") << endl;
        out << endl;
    } else {
        out << tr("This is the description of the general-usage hooks. Use the '--help-<domain>' options to display description of hooks specific to the particular <domain>.") << endl;
        out << endl;
        describe(out, generalDomainHooks);
    }

    out << endl;
    bottomSections(out);
}

void CommandLineParser::commandBriefUsage(QTextStream &out, const Command *command) const
{
    wrapLines(out, 0, {usageMessage()}, {EXE_NAME, command->name}, command->synopsis);
    out << endl;
    wrapLine(out, 0, command->briefDescription);
    out << endl;
    wrapLine(out, 0, tryLongHelpMessage(command->name + " --help"));
}

void CommandLineParser::commandUsage(const Command *command) const
{
    const QString title = QString(EXE_NAME) + '-' + command->name;
    showManual(title, [=](QTextStream &out) { commandUsage(out, command, title); });
}

void CommandLineParser::commandUsage(QTextStream &out, const Command *command, const QString &title) const
{
    AsciiMan::header(out, title);
    out << endl;

    AsciiMan::nameSection(out);
    out << endl;
    out << EXE_NAME << '-' << command->name << " - " << command->briefDescription << endl;
    out << endl;

    AsciiMan::synopsisSection(out);
    out << endl;
    AsciiMan::verseBegin(out);
    wrapLines(out, 0, {}, {AsciiMan::boldLeadingWords(QString(EXE_NAME) + ' ' + command->name)},
            command->synopsis, AsciiMan::BOLD_INDENT_CORRECTION);
    AsciiMan::verseEnd(out);
    out << endl;

    AsciiMan::section(out, descriptionHeading());
    out << endl;
    out << command->description << endl;
    out << endl;

    if (!command->configOptions.isEmpty()) {
        AsciiMan::section(out, configurationOptionsHeading());
        out << endl;
        out << relatedConfigurationOptionsHeading(command) << endl;
        out << endl;
        out << listRelatedConfigurationOptions(command) << '.' << endl;
        out << endl;
    }

    if (!command->hooks.isEmpty()) {
        AsciiMan::section(out, hooksHeading());
        out << endl;
        out << relatedHooksHeading(command) << endl;
        out << endl;
        out << listRelatedHooks(command) << '.' << endl;
        out << endl;
    }

    AsciiMan::section(out, seeAlsoHeading());
    out << endl;

    if (command->module->domain->name == Constants::GENERAL_DOMAIN_NAME)
        out << tryLongHelpMessage("--help") << endl;
    else
        out << tryLongHelpMessage("--help-" + command->module->domain->name) << endl;
}

void CommandLineParser::domainUsage(const Domain *domain) const
{
    const QString title = QString(EXE_NAME) + "manual-" + domain->name;
    showManual(title, [=](QTextStream &out) { domainUsage(out, domain, title); });
}

void CommandLineParser::domainUsage(QTextStream &out, const Domain *domain, const QString &title) const
{
    QTC_ASSERT(domain->name != Constants::GENERAL_DOMAIN_NAME, return);

    const Command::ConstList domainCommands = domain->commands();
    const Option::ConstList domainOptions = domain->options();
    const Hook::ConstList domainHooks = domain->hooks();

    AsciiMan::header(out, title);
    out << endl;

    AsciiMan::nameSection(out);
    out << endl;
    out << manPageSummary(title) << endl;
    out << endl;

    AsciiMan::synopsisSection(out);
    out << endl;
    synopsis(out);
    out << endl;

    AsciiMan::subsection(out, commandsOverviewHeading());
    out << endl;

    out << tr("This manual deals specifically with the \"%1\" aspect of '%3' usage. Try '%2' (without subcommand) for general overview of '%3' usage or '%4' for an all-in-one manual.")
            .arg(domain->briefDescription())
            .arg(QString(EXE_NAME) + " --help")
            .arg(QLatin1String(EXE_NAME))
            .arg(QString(EXE_NAME) + " --help-all") << endl;
    out << endl;

    out << tr("Related commands:") << endl;
    out << endl;

    describeBriefly(out, domainCommands);
    out << endl;
    out << endl;

    out << tr("Try '%1 <command> --help' for the detailed description of each command.")
        .arg(EXE_NAME) << endl;

    out << endl;

    AsciiMan::section(out, descriptionHeading());
    out << endl;

    for (const Module *module : domain->modules()) {
        if (!module->description.isEmpty()) {
            out << module->description << endl;
            out << endl;
            out << endl;
        }
    }

    AsciiMan::section(out, globalOptionsHeading());
    out << endl;

    describeGlobalOptions(out, domain);

    if (!domainOptions.isEmpty()) {
        out << endl;
        AsciiMan::section(out, configurationOptionsHeading());
        out << endl;
        describe(out, domainOptions);
    }

    out << endl;
    AsciiMan::section(out, hooksHeading());
    out << endl;
    if (domainHooks.isEmpty()) {
        out << tr("None of the commands under this domain supports any hooks.") << endl;
        out << endl;
    } else {
        describe(out, domainHooks);
    }

    out << endl;
    bottomSections(out);
}

void CommandLineParser::allDomainsUsage() const
{
    const QString title = QString(EXE_NAME) + "manual";
    showManual(title, [=](QTextStream &out) { allDomainsUsage(out, title); });
}

void CommandLineParser::allDomainsUsage(QTextStream &out, const QString &title) const
{
    AsciiMan::header(out, title);
    out << endl;

    AsciiMan::nameSection(out);
    out << endl;
    out << manPageSummary(title) << endl;
    out << endl;

    AsciiMan::synopsisSection(out);
    out << endl;
    synopsis(out);
    out << endl;

    AsciiMan::subsection(out, commandsOverviewHeading());
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        out << domain->briefDescription() << endl;
        out << endl;
        describeBriefly(out, domain->commands());
        out << endl;
    }

    AsciiMan::section(out, descriptionHeading());
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        for (const Module *module : domain->modules()) {
            if (!module->description.isEmpty()) {
                out << module->description << endl;
                out << endl;
                out << endl;
            }
        }
    }

    AsciiMan::section(out, globalOptionsHeading());
    out << endl;

    describeGlobalOptions(out, nullptr);
    out << endl;

    AsciiMan::section(out, configurationOptionsHeading());
    out << endl;

    describe(out, Utils::toRawPointer<QList>(Dispatcher::options()));
    out << endl;

    AsciiMan::section(out, hooksHeading());
    out << endl;

    describe(out, Utils::toRawPointer<QList>(Dispatcher::hooks()));
    out << endl;

    bottomSections(out);
}

bool CommandLineParser::checkExclusiveOption(const QCommandLineParser &parser,
        const QList<const QCommandLineOption *> &options, const QCommandLineOption **out)
{
    const QCommandLineOption *tmp;
    if (out == nullptr)
        out = &tmp;
    *out = nullptr;

    for (const QCommandLineOption *option : options) {
        if (!parser.isSet(*option))
            continue;
        if (*out != nullptr) {
            qerr() << tr("Cannot combine '%1' and '%2' options")
                .arg((*out)->names().first())
                .arg(option->names().first()) << endl;
            *out = nullptr;
            return false;
        }
        *out = option;
    }
    return true;
}

bool CommandLineParser::checkPositionalArgumentsCount(const QStringList &arguments, int min,
        int max)
{
    if (arguments.count() < min) {
        qerr() << missingArgumentMessage() << endl;
        return false;
    }
    if (max >= 0 && arguments.count() > max) {
        qerr() << unexpectedArgumentMessage(arguments.at(max)) << endl;
        return false;
    }

    return true;
}

int CommandLineParser::optionCount(const QCommandLineParser &parser, const QCommandLineOption &option)
{
    int retv = 0;
    for (const QString &name : option.names())
        retv += parser.optionNames().count(name);
    return retv;
}

bool CommandLineParser::splitArgs(const QString &args, Utils::OsType osType, QStringList *out)
{
    QtcProcess::SplitError error;
    const bool abortOnMeta = true;
    *out = QtcProcess::splitArgs(args, osType, abortOnMeta, &error);

    switch (error) {
    case QtcProcess::SplitOk:
        return true;
    case QtcProcess::BadQuoting:
        qerr() << tr("Argument contains quoting errors: %1").arg(args) << endl;
        return false;
    case QtcProcess::FoundMeta:
        qerr() << tr("Argument contains complex shell constructs: %1").arg(args) << endl;
        return false;
    }

    QTC_CHECK(false);
    return false;
}

QString CommandLineParser::introManPageSummary(const QString &name)
{
    return tr("%1 - the command line frontend of the %2").arg(name).arg(Sdk::sdkVariant());
}

QString CommandLineParser::manPageSummary(const QString &name)
{
    return name + " - " + summary();
}

QString CommandLineParser::summary()
{
    return tr("%1 is the command line frontend of the %2.").arg(EXE_NAME).arg(Sdk::sdkVariant());
}

QString CommandLineParser::usageMessage()
{
    return tr("Usage:");
}

QString CommandLineParser::unrecognizedCommandMessage(const QString &command)
{
    return tr("Unrecognized command '%1'").arg(command);
}

QString CommandLineParser::commandNotAvailableMessage(const QString &command)
{
    return tr("The command '%1' is not available in this mode").arg(command);
}

QString CommandLineParser::commandDeprecatedMessage(const QString &command,
        const QString &replacement)
{
    return replacement.isEmpty()
        ? tr("The command '%1' is deprecated and will be removed")
            .arg(command)
        : tr("The command '%1' is deprecated in favor of '%2' and will be removed")
            .arg(command).arg(replacement);
}

QString CommandLineParser::optionNotAvailableMessage(const QString &option)
{
    return tr("The option '%1' is not available in this mode").arg(option);
}

QString CommandLineParser::unexpectedArgumentMessage(const QString &argument)
{
    return tr("Unexpected argument: '%1'").arg(argument);
}

QString CommandLineParser::missingArgumentMessage()
{
    // Do not include the argument name - it would have to be localized as well to be correct
    return tr("Argument expected");
}

QString CommandLineParser::invalidArgumentToOptionMessage(const QString &problem,
        const QString &option, const QString &argument)
{
    return tr("Invalid argument to %1: '%2': %3")
        .arg(dashOption(option)).arg(argument).arg(problem);
}

QString CommandLineParser::invalidPositionalArgumentMessage(const QString &problem, const QString
        &argument)
{
    return tr("Invalid argument '%1': %2").arg(argument).arg(problem);
}

QString CommandLineParser::unexpectedEmptyArgumentToOptionMessage(const QString &option)
{
    return tr("Unexpected empty argument to %1").arg(dashOption(option));
}

QString CommandLineParser::unrecognizedOptionMessage(const QString &option)
{
    return tr("Unrecognized option: '%1'").arg(option);
}

QString CommandLineParser::tryLongHelpMessage(const QString &options)
{
    return tr("Try '%1' for more information.").arg(QString(EXE_NAME) + ' ' + options);
}

QString CommandLineParser::commandsOverviewHeading()
{
    return tr("Commands Overview");
}

QString CommandLineParser::descriptionHeading()
{
    return tr("Description").toUpper();
}

QString CommandLineParser::globalOptionsHeading()
{
    return tr("Global Options").toUpper();
}

QString CommandLineParser::configurationOptionsHeading()
{
    return tr("Configuration Options").toUpper();
}

QString CommandLineParser::hooksHeading()
{
    return tr("Hooks").toUpper();
}

QString CommandLineParser::relatedConfigurationOptionsHeading(const Command *command)
{
    return tr("The '%1' command obeys the following configuration options:")
        .arg(command->name);
}

QString CommandLineParser::relatedHooksHeading(const Command *command)
{
    return tr("The '%1' command supports the following hooks:")
        .arg(command->name);
}

QString CommandLineParser::seeAlsoHeading()
{
    return tr("See Also").toUpper();
}

/*
 * List options, using '[no-]option' syntax for when opposite option exists
 */
QString CommandLineParser::listRelatedConfigurationOptions(const Command *command)
{
    const QStringList names = Utils::transform(command->configOptions, &Option::name);

    QStringList compacted = compactOptions(names);

    // Ignore '[.*]' prefixes
    Utils::sort(compacted, [](const QString &s1, const QString &s2) {
        const QString s1Base = s1.mid(s1.indexOf(']') + 1);
        const QString s2Base = s2.mid(s2.indexOf(']') + 1);
        return s1Base < s2Base;
    });

    const QStringList quoted = Utils::transform(compacted, [](const QString &s) -> QString {
        return '\'' + s + '\'';
    });

    return quoted.join(", ");
}

/*
 * List hooks, using '[pre-|post-]command' syntax where these variants exists
 */
QString CommandLineParser::listRelatedHooks(const Command *command)
{
    const QStringList names = Utils::transform(command->hooks, &Hook::name);

    QStringList compacted = compactHooks(names);

    // Ignore '[.*]' prefixes
    Utils::sort(compacted, [](const QString &s1, const QString &s2) {
        const QString s1Base = s1.mid(s1.indexOf(']') + 1);
        const QString s2Base = s2.mid(s2.indexOf(']') + 1);
        return s1Base < s2Base;
    });

    const QStringList quoted = Utils::transform(compacted, [](const QString &s) -> QString {
        return '\'' + s + '\'';
    });

    return quoted.join(", ");
}

QString CommandLineParser::environmentVariablesHeading()
{
    return tr("Environment Variables").toUpper();
}

QString CommandLineParser::exitStatusHeading()
{
    return tr("Exit Status").toUpper();
}

bool CommandLineParser::setCurrentWorkingDirectory(const QString &path, QString *errorString)
{
    if (path.isEmpty())
        return true;

    if (path == Constants::WORKSPACE_PSEUDO_VARIABLE) {
        QTC_ASSERT(SdkManager::hasEngine(), return false);
        const QString workspacePath = SdkManager::engine()->sharedSrcPath().toString();
        if (!QDir::setCurrent(workspacePath)) {
            *errorString = tr("Could not change working directory (path after expansion: \"%1\")")
                .arg(workspacePath);
            return false;
        }
        return true;
    }

    if (!QDir::setCurrent(path)) {
        *errorString = tr("Could not change working directory");
        return false;
    }

    return true;
}

/*
 * Replace pairs of opposite options with compact notation "[no-]foo".
 */
QStringList CommandLineParser::compactOptions(const QStringList &names)
{
    const QRegularExpression oppositeCandidateRe("\\bno-");
    QStringList oppositeCandidates = names.filter(oppositeCandidateRe);
    QSet<QString> compactedOpposites;

    QStringList compacted;

    for (const QString &name : names) {
        if (oppositeCandidates.contains(name))
            continue;
        if (compactedOpposites.contains(name))
            continue;

        const QRegularExpression oppositeNameRe(".*\\bno-" + QRegularExpression::escape(name) + "$");
        const int oppositeIndex = oppositeCandidates.indexOf(oppositeNameRe);
        if (oppositeIndex < 0) {
            compacted << name;
        } else {
            const QString opposite = oppositeCandidates.takeAt(oppositeIndex);
            const QString oppositePrefix = opposite.chopped(name.length());
            compacted << "[" + oppositePrefix + "]" + name;
            compactedOpposites << opposite;
        }
    }

    compacted += oppositeCandidates;

    return compacted;
}

/*
 * Replace related hooks with compact notation "[pre-|post-]foo".
 */
QStringList CommandLineParser::compactHooks(const QStringList &names)
{
    const QRegularExpression prePostCandidateRe("^(pre|post)-");
    QStringList prePostCandidates = names.filter(prePostCandidateRe);
    QSet<QString> compactedPrePosts;

    QStringList compacted;

    for (const QString &name : names) {
        if (prePostCandidates.contains(name))
            continue;
        if (compactedPrePosts.contains(name))
            continue;

        const QRegularExpression prePostNameRe("^(pre|post)-" + QRegularExpression::escape(name) + "$");
        const QStringList prePosts = prePostCandidates.filter(prePostNameRe);
        if (prePosts.isEmpty()) {
            compacted << name;
        } else {
            QStringList prePostPrefixes;
            for (const QString &prePost : prePosts) {
                prePostCandidates.removeAll(prePost);
                prePostPrefixes << prePost.chopped(name.length());
            }
            compacted << "[" + prePostPrefixes.join('|') + "]" + name;
            compactedPrePosts |= prePosts.toSet();
        }
    }

    compacted += prePostCandidates;

    return compacted;
}

QString CommandLineParser::dashOption(const QString &option)
{
    if (option.startsWith('-'))
        return option;
    else if (option.length() == 1)
        return '-' + option;
    else
        return "--" + option;
}

QStringList CommandLineParser::environmentVariableAsArguments(const char *name, bool *ok)
{
    *ok = true;

    if (qEnvironmentVariableIsEmpty(name))
        return {};

    const QString value = qEnvironmentVariable(name);
    const bool abortOnMeta = true;
    QtcProcess::SplitError error;
    const QStringList arguments = QtcProcess::splitArgs(value, Utils::OsTypeLinux,
            abortOnMeta, &error);
    if (error != QtcProcess::SplitOk) {
        *ok = false;
        return {};
    }

    return arguments;
}

QString CommandLineParser::synopsis() const
{
    QStringList synopsis;
    synopsis.append(tr("[global-options] <command> [command-options]"));
    synopsis.append("{--help |");
    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        if (domain->name != Constants::GENERAL_DOMAIN_NAME)
            synopsis.last().append(" --help-" + domain->name + " |");
    }
    synopsis.last().append(" --help-all}");
    synopsis.append("--version");

    return synopsis.join('\n');
}

void CommandLineParser::synopsis(QTextStream &out) const
{
    AsciiMan::verseBegin(out);
    wrapLines(out, 0, {}, {AsciiMan::boldLeadingWords(EXE_NAME)}, synopsis(),
            AsciiMan::BOLD_INDENT_CORRECTION);
    AsciiMan::verseEnd(out);
}

void CommandLineParser::describe(QTextStream &out, const QList<QCommandLineOption> &options) const
{
    for (const QCommandLineOption &option : options) {
        QString names = Utils::transform(option.names(), dashOption).join(", ");
        AsciiMan::labeledListItemBegin(out, names + ' ' + option.valueName());
        out << option.description() << endl;
        AsciiMan::labeledListItemEnd(out);
        out << endl;
    }
}

void CommandLineParser::describeBriefly(QTextStream &out, const Command::ConstList &commands) const
{
    AsciiMan::verseBegin(out);
    for (const Command *command : commands) {
        wrapLines(out, 0, {}, {AsciiMan::boldLeadingWords(command->name), ""},
                command->briefDescription, AsciiMan::BOLD_INDENT_CORRECTION);
    }
    AsciiMan::verseEnd(out);
}

void CommandLineParser::describe(QTextStream &out, const Option::ConstList &options) const
{
    for (const Option *option : options) {
        AsciiMan::labeledListItemBegin(out, option->name + ' ' + option->argumentDescription);
        out << option->description << endl;
        AsciiMan::labeledListItemEnd(out);
        out << endl;
    }
}

void CommandLineParser::describe(QTextStream &out, const Hook::ConstList &hooks) const
{
    for (const Hook *hook : hooks) {
        AsciiMan::labeledListItemBegin(out, hook->name + ' ' + hook->synopsis);
        out << hook->description << endl;
        AsciiMan::labeledListItemEnd(out);
        out << endl;
    }
}

/*!
 * Describe global options. If \a domain is not null, the alias options will be limited to
 * configuration options specific to the given domain. Write to \a out, starting at \a indentLevel.
 */
void CommandLineParser::describeGlobalOptions(QTextStream &out, const Domain *domain) const
{
    describe(out, m_helpOptions);

    AsciiMan::labeledListItemBegin(out, tr("--help-<domain>"));
    QString helpDomainDescription = tr("Display the <domain> specific manual and exit. The valid <domain> names are:");
    out << helpDomainDescription << endl;
    out << endl;

    AsciiMan::verseBegin(out);
    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        if (domain->name == Constants::GENERAL_DOMAIN_NAME)
            continue;
        wrapLines(out, 1, {}, {AsciiMan::boldLeadingWords(domain->name), ""},
                domain->briefDescription(), AsciiMan::BOLD_INDENT_CORRECTION);
    }
    AsciiMan::verseEnd(out);
    AsciiMan::labeledListItemEnd(out);
    out << endl;

    QList<QCommandLineOption> globalOptions;

    globalOptions.append(m_otherOptions);

    using AliasOptionPair = QPair<QCommandLineOption, const Option *>;
    auto filteredOrAnnotatedAliasOptions = m_aliasOptions;
    if (domain != nullptr) {
        filteredOrAnnotatedAliasOptions = Utils::filtered(filteredOrAnnotatedAliasOptions,
                [domain](const AliasOptionPair &pair) {
            return pair.second->module->domain == domain;
        });
    } else {
        filteredOrAnnotatedAliasOptions = Utils::transform(filteredOrAnnotatedAliasOptions,
                [domain](const AliasOptionPair &pair) {
            QString annotation = pair.second->module->domain->name == Constants::GENERAL_DOMAIN_NAME
                ? tryLongHelpMessage("--help")
                : tryLongHelpMessage("--help-" + pair.second->module->domain->name);
            QCommandLineOption annotated = pair.first;
            annotated.setDescription(annotated.description() + ' ' + annotation);
            return AliasOptionPair(annotated, pair.second);
        });
    }
    globalOptions.append(Utils::transform(filteredOrAnnotatedAliasOptions, &AliasOptionPair::first));

    Utils::sort(globalOptions, [](const QCommandLineOption &o1, const QCommandLineOption &o2) {
        return o1.names().first() < o2.names().first();
    });

    describe(out, globalOptions);
}

void CommandLineParser::bottomSections(QTextStream &out)
{
    QString body;

    AsciiMan::section(out, environmentVariablesHeading());
    out << endl;

    AsciiMan::labeledListItemBegin(out, Constants::EXIT_ABNORMAL_ENV_VAR);
    out << tr("See the %1 section.").arg(exitStatusHeading()) << endl;
    AsciiMan::labeledListItemEnd(out);
    out << endl;

    AsciiMan::labeledListItemBegin(out, Constants::NO_SESSION_ENV_VAR);
    out << tr("See the '--no-session' option.") << endl;
    AsciiMan::labeledListItemEnd(out);
    out << endl;

    AsciiMan::labeledListItemBegin(out, Constants::OPTIONS_ENV_VAR);
    out << tr("Setting this variable has the same effect as passing the value on the command line before any other options.") << endl;
    AsciiMan::labeledListItemEnd(out);
    out << endl;

    out << endl;

    AsciiMan::section(out, exitStatusHeading());
    out << endl;
    out << tr("%1 exits with zero exit code on success, command-specific nonzero exit code on command failure, or the reserved exit code of %2 to indicate bad usage, internal error, (remote) command dispatching error and suchlike conditions, that either prevented command starting or resulted in premature or otherwise abnormal command termination (different exit code may be designated for this purpose through the '%3' environment variable).")
        .arg(Constants::EXE_NAME)
        .arg(Constants::EXIT_ABNORMAL_DEFAULT_CODE)
        .arg(Constants::EXIT_ABNORMAL_ENV_VAR)
        << endl;
}

void CommandLineParser::showManual(const QString &title,
        std::function<void(QTextStream &out)> generator) const
{
    if (qEnvironmentVariableIntValue(Constants::GENDOC_RUN_ENV_VAR)) {
        generator(Pager());
        return;
    }

    if (!qEnvironmentVariableIntValue(Constants::NO_MAN_ENV_VAR)) {
        if (showManualPage(title))
            return;
    }

    generator(AsciiMan(Pager()));
}

bool CommandLineParser::showManualPage(const QString &title)
{
    const FilePath manPath = FilePath::fromString(QCoreApplication::applicationDirPath())
        .resolvePath(QSettings().value(Constants::DOC_PATH_SETTINGS_KEY, RELATIVE_DOC_PATH)
                .toString())
        .pathAppended("man/man1")
        .canonicalPath()
        .resolvePath(title + ".1.gz");
    if (!manPath.exists()) {
        qCDebug(sfdk) << "Manual page does not exist:" << manPath.toString();
        return false;
    }

    QStringList arguments;
    if (!Pager::isEnabled()) {
        if (HostOsInfo::isMacHost())
            arguments << "-P" << "cat";
        else
            arguments << "--pager" << "cat";
    }

    if (!HostOsInfo::isMacHost())
        arguments << "--local-file";

    arguments << manPath.toString();

    QProcess viewer;
    viewer.setProcessChannelMode(QProcess::ForwardedChannels);
    viewer.start("man", arguments);
    if (!viewer.waitForStarted() || !viewer.waitForFinished(-1)) {
        qCDebug(sfdk) << "'man' could not be started";
        return false;
    }

    // Intentionally ignore exit status and code
    return true;
}
