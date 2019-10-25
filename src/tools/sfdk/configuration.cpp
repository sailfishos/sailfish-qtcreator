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

#include "configuration.h"

#include "dispatch.h"
#include "session.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

using namespace Sfdk;

namespace {
const char CONFIGURATION_SUFFIX[] = ".conf";
const char CONFIGURATION_MASK[] = "*.conf";
}

namespace Sfdk {

class ConfigurationScope
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::ConfigurationScope)

public:
    ConfigurationScope(const QString &name,
            const ConfigurationScope *parent = nullptr);

    QString name() const { return m_name; }

    void push(const OptionOccurence &occurence);
    void drop(const Option *option);

    QList<OptionEffectiveOccurence> effectiveState() const;

    bool load(const QString &fileName);
    bool load(const QString &configuration, const QString &fileName);
    bool save(const QString &fileName) const;
    QString save() const;

private:
    QString m_name;
    const ConfigurationScope *m_parent = nullptr;
    QList<OptionOccurence> m_occurences;
};

} // namespace Sfdk

/*!
 * \class Option
 */

const Domain *Option::domain() const
{
    return module->domain;
}

/*!
 * \class OptionOccurence
 */

QString OptionOccurence::toString() const
{
    Q_ASSERT(!isNull());

    if (m_type == Mask)
        return m_option->name + '=';
    else if (m_argument.isEmpty())
        return m_option->name;
    else
        return m_option->name + '=' + m_argument;
}

OptionOccurence OptionOccurence::fromString(const QString &string)
{
    OptionOccurence retv;

    int equalPosition = string.indexOf('=');

    QString name = string.left(equalPosition);

    retv.m_option = Dispatcher::option(name);
    if (retv.m_option == nullptr) {
        retv.m_errorString = tr("Unrecognized option: '%1'").arg(name);
        return retv;
    }

    retv.m_type = (equalPosition == string.length() - 1) ? Mask : Push;

    if (equalPosition != -1)
        retv.m_argument = string.mid(equalPosition + 1);

    if (retv.m_type == Push && retv.m_option->argumentType == Option::MandatoryArgument
            && retv.m_argument.isEmpty()) {
        retv.m_errorString = tr("Option requires an argument: '%1'").arg(retv.m_option->name);
        retv.m_option = nullptr;
    }

    return retv;
}

/*!
 * \class ConfigurationScope
 */

ConfigurationScope::ConfigurationScope(const QString &name,
        const ConfigurationScope *parent)
    : m_name(name)
    , m_parent(parent)
{
}

void ConfigurationScope::push(const OptionOccurence &occurence)
{
    drop(occurence.option());
    m_occurences.append(occurence);
}

void ConfigurationScope::drop(const Option *option)
{
    (void)Utils::take(m_occurences, Utils::equal(&OptionOccurence::option, option));
}

QList<OptionEffectiveOccurence> ConfigurationScope::effectiveState() const
{
    QList<OptionEffectiveOccurence> retv;

    if (m_parent)
        retv = m_parent->effectiveState();

    for (const OptionOccurence &occurence : m_occurences) {
        for (auto it = retv.rbegin(); it != retv.rend(); ++it) {
            if (it->option() == occurence.option()) {
                it->setMaskedBy(this);
                break;
            }
        }
        if (occurence.type() == OptionOccurence::Push) {
            retv.append(OptionEffectiveOccurence(occurence.option(), occurence.argument(),
                        this));
        }
    }

    return retv;
}

bool ConfigurationScope::load(const QString &fileName)
{
    Utils::FileReader reader;
    QString errorString;
    if (!reader.fetch(fileName, &errorString)) {
        qCCritical(sfdk) << "Error reading configuration file:" << qPrintable(errorString);
        return false;
    }

    QTextStream in(reader.data());
    return load(in.readAll(), fileName);
}

bool ConfigurationScope::load(const QString &configuration, const QString &fileName)
{
    QStringList splitted;
    QRegularExpression blankLine("^[[:space:]]*(#.*)?$");
    QRegularExpression continuationLine("^[[:space:]]*=(.*)");
    for (const QString &line : configuration.split('\n')) {
        if (blankLine.match(line).hasMatch())
            continue;
        QRegularExpressionMatch match = continuationLine.match(line);
        if (match.hasMatch())
            splitted.last() += '\n' + match.captured(1);
        else
            splitted.append(line);
    }

    for (const QString &savedOccurence : qAsConst(splitted)) {
        OptionOccurence occurence = OptionOccurence::fromString(savedOccurence);
        if (occurence.isNull()) {
            qCCritical(sfdk) << "Error parsing configuration file" << fileName << ":"
                << qPrintable(occurence.errorString());
            return false;
        }
        m_occurences.append(occurence);
    }

    return true;
}

bool ConfigurationScope::save(const QString &fileName) const
{
    QFileInfo info(fileName);
    if (!info.dir().mkpath(".")) {
        qCCritical(sfdk) << "Failed to create configuration directory" << info.absolutePath();
        return false;
    }

    Utils::FileSaver saver(fileName);
    if (saver.hasError()) {
        qCCritical(sfdk) << "Failed to prepare configuration file for writing:"
            << qPrintable(saver.errorString());
        return false;
    }

    QTextStream out(saver.file());
    out << save();

    saver.setResult(&out);

    if (!saver.finalize()) {
        qCCritical(sfdk) << "Failed to write configuration file:"
            << qPrintable(saver.errorString());
        return false;
    }

    return true;
}

QString ConfigurationScope::save() const
{
    QString retv;
    for (const OptionOccurence &occurence : m_occurences) {
        QString continuationPrefix = '\n' + QString(occurence.option()->name.length(), ' ') + '=';
        retv.append(occurence.toString().replace("\n", continuationPrefix));
        retv.append('\n');
    }
    return retv;
}

/*!
 * \class Configuration
 */

Configuration *Configuration::s_instance = nullptr;

Configuration::Configuration()
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_globalScope = std::make_unique<ConfigurationScope>(scopeName(Global));
    m_sessionScope = std::make_unique<ConfigurationScope>(scopeName(Session), m_globalScope.get());
    m_commandScope = std::make_unique<ConfigurationScope>(scopeName(Command), m_sessionScope.get());
}

Configuration::~Configuration()
{
    s_instance = nullptr;
}

bool Configuration::load()
{
    bool ok = true;

    QFileInfo globalFile(globalConfigurationPath());
    if (globalFile.exists())
        ok &= s_instance->m_globalScope->load(globalFile.filePath());

    if (Session::isValid()) {
        cleanUpSessions();
        QFileInfo sessionFile(sessionConfigurationPath());
        if (sessionFile.exists())
            ok &= s_instance->m_sessionScope->load(sessionFile.filePath());
    }

    s_instance->m_loaded = ok;

    return ok;
}

bool Configuration::isLoaded()
{
    return s_instance->m_loaded;
}

bool Configuration::push(Scope scope, const OptionOccurence &occurence)
{
    s_instance->scope(scope)->push(occurence);
    return s_instance->save(scope);
}

bool Configuration::push(Scope scope, const Option *option, const QString &argument)
{
    s_instance->scope(scope)->push(OptionOccurence(option, OptionOccurence::Push, argument));
    return s_instance->save(scope);
}

bool Configuration::pushMask(Scope scope, const Option *option)
{
    s_instance->scope(scope)->push(OptionOccurence(option, OptionOccurence::Mask));
    return s_instance->save(scope);
}

bool Configuration::drop(Scope scope, const Option *option)
{
    s_instance->scope(scope)->drop(option);
    return s_instance->save(scope);
}

QString Configuration::scopeName(Scope scope)
{
    switch (scope) {
    case Global:
        return tr("global scope");
    case Session:
        return tr("session scope");
    case Command:
        return tr("command scope");
    }
    QTC_CHECK(false);
    return {};
}

QList<OptionEffectiveOccurence> Configuration::effectiveState()
{
    return s_instance->m_commandScope->effectiveState();
}

Utils::optional<OptionEffectiveOccurence> Configuration::effectiveState(const Option *option)
{
    for (const OptionEffectiveOccurence &occurence : effectiveState()) {
        if (!occurence.isMasked() && occurence.option() == option)
            return occurence;
    }
    return {};
}

QStringList Configuration::toArguments(const Module *module)
{
    QStringList retv;

    for (const OptionEffectiveOccurence &occurence : effectiveState()) {
        if (!occurence.isMasked() && occurence.option()->module == module) {
            if (occurence.argument().isEmpty()) {
                retv << "--" + occurence.option()->name;
            } else if (occurence.option()->argumentType == Option::MandatoryArgument) {
                retv << "--" + occurence.option()->name << occurence.argument();
            } else {
                retv << "--" + occurence.option()->name + "=" + occurence.argument();
            }
        }
    }

    return retv;
}

QString Configuration::print()
{
    QString retv;
    QTextStream out(&retv);

    QHash<const ConfigurationScope *, QList<OptionEffectiveOccurence>> scopeState;
    for (const OptionEffectiveOccurence &occurence : effectiveState())
        scopeState[occurence.origin()].append(occurence);

    const QList<const ConfigurationScope *> scopes{
        s_instance->m_commandScope.get(),
        s_instance->m_sessionScope.get(),
        s_instance->m_globalScope.get()
    };

    for (const ConfigurationScope *scope : scopes) {
        out << "# ---- " << scope->name() << " ---------" << endl;

        bool clear = true;
        for (const OptionEffectiveOccurence &occurence : scopeState.value(scope)) {
            clear = false;
            if (occurence.isMasked()) {
                out << "# " << tr("masked at %1")
                    .arg(occurence.maskedBy()->name()) << endl;
                out << ";";
            }
            out << occurence.option()->name;
            if (!occurence.argument().isEmpty())
                out << " = " << occurence.argument().replace("\\", "\\\\").replace("\n", "\\n");
            out << endl;
        }
        if (scope == s_instance->m_sessionScope.get() && !Session::isValid()) {
            out << "# " << tr("<not available>") << endl;
        } else if (clear) {
            out << "# " << tr("<clear>") << endl;
        }

        out << endl;
    }

    return retv;
}

QString Configuration::globalConfigurationPath()
{
    // QStandardPaths does not give the same locations
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
            QCoreApplication::organizationName(),
            QCoreApplication::applicationName());
    QFileInfo info(settings.fileName());
    return info.dir().absoluteFilePath(info.completeBaseName() + CONFIGURATION_SUFFIX);
}

QString Configuration::sessionConfigurationPath()
{
    return sessionConfigurationLocation() + QDir::separator() + Session::id() + CONFIGURATION_SUFFIX;
}

void Configuration::cleanUpSessions()
{
    QDirIterator it(sessionConfigurationLocation(), {CONFIGURATION_MASK}, QDir::Files);
    while (it.hasNext()) {
        it.next();

        QDateTime dayAgo = QDateTime::currentDateTime().addDays(-1);
        if (it.fileInfo().lastModified() > dayAgo)
            continue;

        QString sessionId = it.fileInfo().completeBaseName();
        if (Session::isSessionActive(sessionId))
            continue;

        if (!QFile(it.filePath()).remove())
            qCWarning(sfdk) << "Unable to clean up orphan session data:" << it.filePath();
    }
}

QString Configuration::sessionConfigurationLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
        + QDir::separator() + Constants::APP_ID;
}

bool Configuration::save(Scope scope)
{
    switch (scope) {
    case Command:
        return true;

    case Session:
        if (!Session::isValid()) {
            qCWarning(sfdk) << "Session management is not available";
            return false;
        }
        return s_instance->m_sessionScope->save(sessionConfigurationPath());

    case Global:
        return s_instance->m_globalScope->save(globalConfigurationPath());
    }

    QTC_CHECK(false);
    return false;
}

ConfigurationScope *Configuration::scope(Scope scope) const
{
    switch (scope) {
    case Global:
        return m_globalScope.get();
    case Session:
        return m_sessionScope.get();
    case Command:
        return m_commandScope.get();
    }
    QTC_CHECK(false);
    return nullptr;
}
