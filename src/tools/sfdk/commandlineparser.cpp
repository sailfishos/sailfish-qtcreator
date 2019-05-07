/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include <QCommandLineParser>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include "command.h"
#include "configuration.h"
#include "dispatch.h"
#include "sfdkconstants.h"
#include "textutils.h"

using namespace Sfdk;

namespace {
using Constants::EXE_NAME;
}

/*!
 * \class CommandLineParser
 */

CommandLineParser::CommandLineParser(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);

    QCommandLineOption hOption("h");
    hOption.setDescription(tr("Display the brief description and exit"));
    QCommandLineOption helpOption("help");
    helpOption.setDescription(tr("Display the generic, introductory description and exit"));
    QCommandLineOption helpAllOption("help-all");
    helpAllOption.setDescription(tr("Display the all-in-one description and exit. This provides the combined view of all the --help-<domain> provided descriptions."));
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

        QString domainHelpOptionName = "--help-" + option->domain()->name;
        QCommandLineOption alias(option->alias);
        alias.setValueName(option->argumentDescription);
        alias.setDescription(tr("This is a shorthand alias for the '%1' configuration option.")
                .arg(option->name));
        m_aliasOptions.append({alias, option.get()});
        parser.addOption(alias);
    }

    QCommandLineOption quietOption("quiet");
    quietOption.setDescription(tr("Suppres informational messages.\n\nThis option only affects generic messages. Subcommands may provide their own equivalents of this option to suppress their informational messages."));
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

    parser.addOptions(m_otherOptions);

    if (!parser.parse(arguments)) {
        badUsage(parser.errorText());
        m_result = BadUsage;
        return;
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
        allDomainsUsage(Pager());
        m_result = Usage;
        return;
    }
    for (const QPair<QCommandLineOption, const Domain *> &pair : qAsConst(m_domainHelpOptions)) {
        if (parser.isSet(pair.first)) {
            domainUsage(Pager(), pair.second);
            m_result = Usage;
            return;
        }
    }
    if (parser.isSet(helpOption)) {
        usage(Pager());
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

    for (const QString &value : parser.values(cOption)) {
        OptionOccurence occurence = OptionOccurence::fromString(value);
        if (occurence.isNull()) {
            badUsage(invalidArgumentToOptionMessage(occurence.errorString(),
                        cOption.names().first()));
            m_result = BadUsage;
            return;
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
        Configuration::push(Configuration::Command, pair.second, argument);
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

    m_commandArguments = positionalArguments;

    // We can just guess here...
    if (m_commandArguments.contains("-h")) {
        commandBriefUsage(qout(), m_command);
        m_result = Usage;
        return;
    }
    if (m_commandArguments.contains("--help")) {
        commandUsage(Pager(), m_command);
        m_result = Usage;
        return;
    }
    if (m_commandArguments.contains("--help-all")) {
        allDomainsUsage(Pager());
        m_result = Usage;
        return;
    }
    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        if (m_commandArguments.contains("--help-" + domain->name)) {
            domainUsage(Pager(), domain.get());
            m_result = Usage;
            return;
        }
    }

    m_result = Dispatch;
}

void CommandLineParser::badUsage(const QString &message) const
{
    qerr() << message << endl;
    briefUsage(qerr());
}

void CommandLineParser::briefUsage(QTextStream &out) const
{
    synopsis(out);
    out << tryLongHelpMessage("--help") << endl;
}

void CommandLineParser::usage(QTextStream &out) const
{
    synopsis(out);
    out << endl;

    out << commandsOverviewHeading() << endl;
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        wrapLine(out, 1, domain->briefDescription());
        out << endl;
        describeBriefly(out, 2, domain->commands());
        out << endl;
        if (domain->name == Constants::GENERAL_DOMAIN_NAME)
            wrapLine(out, 2, tr("The detailed description of these commands follows below."));
        else
            wrapLine(out, 2, tryLongHelpMessage("--help-" + domain->name));
        out << endl;
    }

    const Domain *generalDomain = Dispatcher::domain(Constants::GENERAL_DOMAIN_NAME);
    QTC_ASSERT(generalDomain != nullptr, return);

    for (const Module *module : generalDomain->modules())
        wrapLines(out, 0, {}, {}, module->description);
    out << endl;

    out << commandsHeading() << endl;
    out << endl;
    wrapLine(out, 1, tr("This is the description of the general-usage commands. Use the '--help-<domain>' options to display description of commands specific to particular <domain>."));
    out << endl;

    describe(out, 1, generalDomain->commands());

    out << globalOptionsHeading() << endl;
    out << endl;

    describeGlobalOptions(out, 1, nullptr);

    const Option::ConstList generalDomainOptions = generalDomain->options();
    if (!generalDomainOptions.isEmpty()) {
        out << configurationOptionsHeading() << endl;
        out << endl;
        describe(out, 1, generalDomainOptions);
    }
}

void CommandLineParser::commandBriefUsage(QTextStream &out, const Command *command) const
{
    wrapLines(out, 0, {usageMessage()}, {EXE_NAME, command->name}, command->synopsis);
    out << endl;
    wrapLine(out, 0, tryLongHelpMessage(command->name + " --help"));
}

void CommandLineParser::commandUsage(QTextStream &out, const Command *command) const
{
    wrapLines(out, 0, {usageMessage()}, {EXE_NAME, command->name}, command->synopsis);
    out << endl;
    wrapLines(out, 0, {}, {}, command->description);
    out << endl;
    if (command->module->domain->name == Constants::GENERAL_DOMAIN_NAME)
        wrapLine(out, 0, tryLongHelpMessage("--help"));
    else
        wrapLine(out, 0, tryLongHelpMessage("--help-" + command->module->domain->name));
}

void CommandLineParser::domainUsage(QTextStream &out, const Domain *domain) const
{
    QTC_ASSERT(domain->name != Constants::GENERAL_DOMAIN_NAME, return);

    const Command::ConstList domainCommands = domain->commands();
    const Option::ConstList domainOptions = domain->options();

    wrapLine(out, 0, tr("This manual deals specifically with the \"%1\" aspect of '%3' usage. Try '%2' (without subcommand) for general overview of '%3' usage or '%4' for an all-in-one manual.")
            .arg(domain->briefDescription())
            .arg(QString(EXE_NAME) + " --help")
            .arg(QLatin1String(EXE_NAME))
            .arg(QString(EXE_NAME) + " --help-all"));
    out << endl;

    out << commandsOverviewHeading() << endl;
    out << endl;

    describeBriefly(out, 1, domainCommands);
    out << endl;

    for (const Module *module : domain->modules()) {
        wrapLines(out, 0, {}, {}, module->description);
        out << endl;
        out << endl;
    }

    out << commandsHeading() << endl;
    out << endl;

    describe(out, 1, domainCommands);
    out << endl;

    out << globalOptionsHeading() << endl;
    out << endl;

    describeGlobalOptions(out, 1, domain);
    out << endl;

    if (!domainOptions.isEmpty()) {
        out << configurationOptionsHeading() << endl;
        out << endl;
        describe(out, 1, domainOptions);
    }
}

void CommandLineParser::allDomainsUsage(QTextStream &out) const
{
    synopsis(out);
    out << endl;

    out << commandsOverviewHeading() << endl;
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        wrapLine(out, 1, domain->briefDescription());
        out << endl;
        describeBriefly(out, 2, domain->commands());
        out << endl;
    }

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        for (const Module *module : domain->modules()) {
            wrapLines(out, 0, {}, {}, module->description);
            out << endl;
            out << endl;
        }
    }

    out << commandsHeading() << endl;
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        wrapLine(out, 1, domain->briefDescription());
        describe(out, 2, domain->commands());
        out << endl;
    }

    out << globalOptionsHeading() << endl;
    out << endl;

    describeGlobalOptions(out, 1, nullptr);
    out << endl;

    out << configurationOptionsHeading() << endl;
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        const Option::ConstList domainOptions = domain->options();
        if (domainOptions.isEmpty())
            continue;

        wrapLine(out, 1, domain->briefDescription());
        describe(out, 2, domainOptions);
        out << endl;
    }
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

QString CommandLineParser::usageMessage()
{
    return tr("Usage:");
}

QString CommandLineParser::unrecognizedCommandMessage(const QString &command)
{
    return tr("Unrecognized command '%1'").arg(command);
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

QString CommandLineParser::invalidArgumentToOptionMessage(const QString &problem, const QString
        &option)
{
    return tr("Invalid argument to %1: %2").arg(dashOption(option)).arg(problem);
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
    return tr("Commands Overview").toUpper();
}

QString CommandLineParser::commandsHeading()
{
    return tr("Commands").toUpper();
}

QString CommandLineParser::globalOptionsHeading()
{
    return tr("Global Options").toUpper();
}

QString CommandLineParser::configurationOptionsHeading()
{
    return tr("Configuration Options").toUpper();
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

void CommandLineParser::synopsis(QTextStream &out) const
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

    wrapLines(out, 0, {usageMessage()}, {EXE_NAME}, synopsis.join('\n'));
}

void CommandLineParser::describe(QTextStream &out, int indentLevel, const QList<QCommandLineOption> &options) const
{
    for (const QCommandLineOption &option : options) {
        QString names = Utils::transform(option.names(), dashOption).join(", ");
        wrapLines(out, indentLevel + 0, {}, {names}, option.valueName());
        wrapLines(out, indentLevel + 1, {}, {}, option.description());
        out << endl;
    }
}

void CommandLineParser::describeBriefly(QTextStream &out, int indentLevel, const Command::ConstList &commands) const
{
    for (const Command *command : commands)
        wrapLines(out, indentLevel, {}, {command->name, ""}, command->briefDescription);
}

void CommandLineParser::describe(QTextStream &out, int indentLevel, const Command::ConstList &commands) const
{
    for (const Command *command : commands) {
        wrapLines(out, indentLevel + 0, {}, {command->name}, command->synopsis);
        wrapLines(out, indentLevel + 1, {}, {}, command->description);
        out << endl;
    }
}

void CommandLineParser::describe(QTextStream &out, int indentLevel, const Option::ConstList &options) const
{
    for (const Option *option : options) {
        wrapLines(out, indentLevel + 0, {}, {option->name}, option->argumentDescription);
        wrapLines(out, indentLevel + 1, {}, {}, option->description);
        out << endl;
    }
}

/*!
 * Describe global options. If \a domain is not null, the alias options will be limited to
 * configuration options specific to the given domain. Write to \a out, starting at \a indentLevel.
 */
void CommandLineParser::describeGlobalOptions(QTextStream &out, int indentLevel, const Domain *domain) const
{
    describe(out, indentLevel, m_helpOptions);

    out << indent(indentLevel + 0) << tr("--help-<domain>") << endl;
    QString helpDomainDescription = tr("Display the <domain> specific description. The valid <domain> names are:");
    wrapLine(out, indentLevel + 1, helpDomainDescription);
    out << endl;

    for (const std::unique_ptr<const Domain> &domain : Dispatcher::domains()) {
        if (domain->name == Constants::GENERAL_DOMAIN_NAME)
            continue;
        wrapLines(out, indentLevel + 2, {}, {domain->name, ""}, domain->briefDescription());
    }
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

    describe(out, indentLevel, globalOptions);
}
