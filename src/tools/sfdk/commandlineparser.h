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

#pragma once

#include "configuration.h"
#include "command.h"

#include <utils/osspecificaspects.h>

#include <QCommandLineOption>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QCommandLineParser;
class QTextStream;
QT_END_NAMESPACE

namespace Sfdk {

class Command;
class Domain;
class Option;

class CommandLineParser
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::CommandLineParser)

public:
    enum Result {
        BadUsage,
        Dispatch,
        Usage,
        Version,
    };

    enum Verbosity {
        Quiet,
        Normal,
        Debug,
    };

    CommandLineParser(const QStringList &arguments);

    Result result() const { return m_result; }
    Verbosity verbosity() const { return m_verbosity; }
    bool noSession() const { return m_noSession; }
    bool useSystemSettingsOnly() const { return m_useSystemSettingsOnly; }
    bool validateCommandScopeConfiguration() const;
    const Command *command() const { return m_command; }
    QStringList commandArguments() const { return m_commandArguments; }

    void badUsage(const QString &message) const;
    void briefUsage(QTextStream &out) const;
    void usage(QTextStream &out) const;
    void commandBriefUsage(QTextStream &out, const Command *command) const;
    void commandUsage(QTextStream &out, const Command *command) const;
    void domainUsage(QTextStream &out, const Domain *domain) const;
    void allDomainsUsage(QTextStream &out) const;

    static bool checkExclusiveOption(const QCommandLineParser &parser,
            const QList<const QCommandLineOption *> &options,
            const QCommandLineOption **out = nullptr);
    static bool checkPositionalArgumentsCount(const QStringList &arguments, int min, int max);
    static int optionCount(const QCommandLineParser &parser, const QCommandLineOption &option);
    static bool splitArgs(const QString &cmd, Utils::OsType osType, QStringList *out);

    static QString summary();
    static QString usageMessage();
    static QString unrecognizedCommandMessage(const QString &command);
    static QString commandNotAvailableMessage(const QString &command);
    static QString commandDeprecatedMessage(const QString &command, const QString &replacement = {});
    static QString optionNotAvailableMessage(const QString &option);
    static QString unexpectedArgumentMessage(const QString &argument);
    static QString missingArgumentMessage();
    static QString invalidArgumentToOptionMessage(const QString &problem, const QString &option,
            const QString &argument);
    static QString invalidPositionalArgumentMessage(const QString &problem,
            const QString &argument);
    static QString unexpectedEmptyArgumentToOptionMessage(const QString &option);
    static QString unrecognizedOptionMessage(const QString &option);
    static QString tryLongHelpMessage(const QString &options);

private:
    static QString commandsOverviewHeading();
    static QString commandsHeading();
    static QString globalOptionsHeading();
    static QString configurationOptionsHeading();
    static QString relatedConfigurationOptionsHeading(const Command *command);
    static QString listRelatedConfigurationOptions(const Command *command);

    static QStringList compactOptions(const QStringList &names);
    static QString dashOption(const QString &option);

    void synopsis(QTextStream &out) const;
    void describe(QTextStream &out, int indentLevel, const QList<QCommandLineOption> &options) const;
    void describeBriefly(QTextStream &out, int indentLevel, const Command::ConstList &commands)
        const;
    void describe(QTextStream &out, int indentLevel, const Command::ConstList &commands) const;
    void describe(QTextStream &out, int indentLevel, const Option::ConstList &options) const;
    void describeGlobalOptions(QTextStream &out, int indentLevel, const Domain *domain) const;
    static void exitStatusSection(QTextStream &out);

private:
    Result m_result;
    QList<QCommandLineOption> m_helpOptions;
    QList<QPair<QCommandLineOption, const Domain *>> m_domainHelpOptions;
    QList<QPair<QCommandLineOption, const Option *>> m_aliasOptions;
    QList<QCommandLineOption> m_otherOptions;
    QList<std::function<bool()>> m_configOptionsValidators;
    Verbosity m_verbosity = Normal;
    bool m_noSession = false;
    bool m_useSystemSettingsOnly = false;
    const Command *m_command = nullptr;
    QStringList m_commandArguments;
    const Domain *m_domain = nullptr;
};

} // namespace Sfdk
