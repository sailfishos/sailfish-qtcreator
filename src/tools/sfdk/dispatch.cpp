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

#include "dispatch.h"

#include "command.h"
#include "configuration.h"
#include "script.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <sfdk/sdk.h>

#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>

using namespace Sfdk;

namespace {

const char ALIAS_KEY[] = "alias";
const char COMMANDS_KEY[] = "commands";
const char COMMAND_LINE_FILTER[] = "commandLineFilter";
const char CONFIG_OPTIONS_KEY[] = "configOptions";
const char DOMAIN_KEY[] = "domain";
const char DYNAMIC_KEY[] = "dynamic";
const char DYNAMIC_SUBCOMMANDS_KEY[] = "dynamicSubcommands";
const char EXTERN_HOOKS_KEY[] = "externHooks";
const char EXTERN_OPTIONS_KEY[] = "externOptions";
const char HOOKS_KEY[] = "hooks";
const char NAME_KEY[] = "name";
const char OPTIONS_KEY[] = "options";
const char POST_RUN_KEY[] = "postRun";
const char PRE_RUN_KEY[] = "preRun";
const char TR_ARGUMENT_KEY[] = "trArgument";
const char TR_BRIEF_KEY[] = "trBrief";
const char TR_DESCRIPTION_KEY[] = "trDescription";
const char TR_SYNOPSIS_KEY[] = "trSynopsis";
const char TYPE_KEY[] = "type";
const char VALIDATOR_KEY[] = "validator";
const char VERSION_KEY[] = "version";
const char WORKER_KEY[] = "worker";

const char OS_VARIANT_MACRO[] = "%{OsVariant}";
const char OS_VARIANT_NOSPACE_MACRO[] = "%{OsVariant:NoSpace}";
const char OS_VARIANT_UNDERLINE_MACRO[] = "%{OsVariant:Underline}";
const char IDE_VARIANT_MACRO[] = "%{IdeVariant}";
const char SDK_VARIANT_MACRO[] = "%{SdkVariant}";

}

/*!
 * \class Domain
 */

QString Domain::briefDescription() const
{
    const Module *firstModule = Utils::findOrDefault(Dispatcher::modules(),
            Utils::equal(&Module::domain, this));
    QTC_ASSERT(firstModule, return {});
    return firstModule->briefDescription;
}

Module::ConstList Domain::modules() const
{
    return Utils::filtered(Utils::toRawPointer<QList>(Dispatcher::modules()),
            Utils::equal(&Module::domain, this));
}

Command::ConstList Domain::commands() const
{
    return Utils::filtered(Utils::toRawPointer<QList>(Dispatcher::commands()),
            Utils::equal(&Command::domain, this));
}

Option::ConstList Domain::options() const
{
    QSet<const Option *> options;
    for (const Command *command : commands())
        options += command->configOptions.toSet();

    auto isDomainOption = [=](const Option *option) {
        return option->module->domain->name == Constants::GENERAL_DOMAIN_NAME
            || options.contains(option);
    };
    return Utils::filtered(Utils::toRawPointer<QList>(Dispatcher::options()),
            isDomainOption);
}

Hook::ConstList Domain::hooks() const
{
    QSet<const Hook *> hooks;
    for (const Command *command : commands())
        hooks += command->hooks.toSet();

    auto isDomainHook = [=](const Hook *hook) {
        return hooks.contains(hook);
    };
    return Utils::filtered(Utils::toRawPointer<QList>(Dispatcher::hooks()),
            isDomainHook);
}

/*!
 * \class Dispatcher
 */

Dispatcher *Dispatcher::s_instance = nullptr;

Dispatcher::Dispatcher()
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

Dispatcher::~Dispatcher()
{
    s_instance = nullptr;
}

bool Dispatcher::load(const QString &moduleFileName)
{
    QFile moduleFile(moduleFileName);
    if (!moduleFile.open(QIODevice::ReadOnly)) {
        qCCritical(sfdk) << "Failed to open module" << moduleFileName << "for reading";
        return false;
    }

    const QByteArray moduleData = moduleFile.readAll();

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(moduleData, &error);

    if (error.error != QJsonParseError::NoError) {
        int line = 1;
        int column = 1;
        for (int i = 0; i < error.offset; ++i) {
            if (moduleData.at(i) == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        qCCritical(sfdk).noquote() << tr("Error parsing module: %1:%2:%3: %4")
            .arg(moduleFileName).arg(line).arg(column)
            .arg(error.errorString());
        return false;
    }

    if (!json.isObject()) {
        qCCritical(sfdk).noquote() << tr("No root JSON object in module '%1'").arg(moduleFileName);
        return false;
    }

    QVariantMap data = json.object().toVariantMap();

    QString errorString;
    std::unique_ptr<Module> module = s_instance->loadModule(data, &errorString);
    if (!module) {
        qCCritical(sfdk).noquote() << tr("Data error in module file '%1': %2")
            .arg(moduleFileName)
            .arg(errorString);
        return false;
    }

    module->fileName = moduleFileName;

    s_instance->m_modules.emplace_back(std::move(module));

    return true;
}

const Domain::ConstUniqueList &Dispatcher::domains()
{
    return s_instance->m_domains;
}

const Domain *Dispatcher::domain(const QString &name)
{
    return Utils::findOrDefault(s_instance->m_domains, Utils::equal(&Domain::name, name));
}

const Module::ConstUniqueList &Dispatcher::modules()
{
    return s_instance->m_modules;
}

const Worker::ConstUniqueList &Dispatcher::workers()
{
    return s_instance->m_workers;
}

const Command::ConstUniqueList &Dispatcher::commands()
{
    return s_instance->m_commands;
}

const Command *Dispatcher::command(const QString &name)
{
    return s_instance->m_commandByName.value(name);
}

const Hook::ConstUniqueList &Dispatcher::hooks()
{
    return s_instance->m_hooks;
}

const Hook *Dispatcher::hook(const QString &name)
{
    return s_instance->m_hookByName.value(name);
}

const Option::ConstUniqueList &Dispatcher::options()
{
    return s_instance->m_options;
}

const Option *Dispatcher::option(const QString &name)
{
    return s_instance->m_optionByName.value(name);
}

QVariant Dispatcher::value(const QVariantMap &data, const QString &key, QVariant::Type type, const
        QVariant &defaultValue, QString *errorString)
{
    QVariant value = data.value(key);
    if (!value.isValid()) {
        if (defaultValue.isValid())
            return defaultValue;
        *errorString = tr("Required key not found: '%1'").arg(key);
        return {};
    }

    if (value.type() != type) {
        *errorString = tr("Unexpected value type for key '%1'. Expected '%2', got '%3'")
            .arg(key)
            .arg(QLatin1String(QVariant::typeToName(type)))
            .arg(QLatin1String(value.typeName()));
        return {};
    }

    // value.isNull() does not work here
    if (type == QVariant::String && value.toString().isEmpty()) {
        *errorString = tr("Value empty for key '%1'").arg(key);
        return {};
    }

    return value;
}

bool Dispatcher::checkKeys(const QVariantMap &data, const QSet<QString> &validKeys, QString
        *errorString)
{
    for (const QString &key : data.keys()) {
        if (!validKeys.contains(key)) {
            *errorString = tr("Unexpected key '%1'").arg(key);
            return false;
        }
    }
    return true;
}

bool Dispatcher::checkItems(const QVariantList &list, QVariant::Type type, QString *errorString)
{
    for (const QVariant &item : list) {
        if (item.type() != type) {
            *errorString = tr("Unexpected list item type '%1'. Expected '%2'")
                .arg(QLatin1String(QVariant::typeToName(type)))
                .arg(QLatin1String(item.typeName()));
            return false;
        }
    }
    return true;
}

JSEngine *Dispatcher::jsEngine()
{
    if (!s_instance->m_jsEngine)
        s_instance->m_jsEngine = std::make_unique<JSEngine>();

    return s_instance->m_jsEngine.get();
}

void Dispatcher::registerWorkerType(const QString &name, const WorkerCreator &creator)
{
    m_workerCreators.insert(name, creator);
}

QString Dispatcher::brandedString(const QString &value)
{
    if (value.isNull())
        return {};

    QString brandedValue = value;

    brandedValue.replace(OS_VARIANT_MACRO, Sdk::osVariant());
    brandedValue.replace(OS_VARIANT_NOSPACE_MACRO, Sdk::osVariant(TextStyle::CamelCase));
    brandedValue.replace(OS_VARIANT_UNDERLINE_MACRO, Sdk::osVariant(TextStyle::SnakeCase));

    brandedValue.replace(SDK_VARIANT_MACRO, Sdk::sdkVariant());
    brandedValue.replace(IDE_VARIANT_MACRO, Sdk::ideVariant());

    return brandedValue;
}

QString Dispatcher::localizedString(const QVariant &value)
{
    if (value.isNull())
        return {};

    return brandedString(QCoreApplication::translate(Constants::DISPATCH_TR_CONTEXT, value.toByteArray()));
}

const Domain *Dispatcher::ensureDomain(const QString &name)
{
    const Domain *existing = domain(name);
    if (existing != nullptr)
        return existing;

    auto domain = std::make_unique<Domain>();

    domain->name = name;

    m_domains.emplace_back(std::move(domain));
    return m_domains.back().get();
}

std::unique_ptr<Module> Dispatcher::loadModule(const QVariantMap &data, QString *errorString)
{
    auto addContext = [](const QString &errorString, const QString &element) {
        return tr("Under element '%1': %2").arg(element).arg(errorString);
    };

    auto module = std::make_unique<Module>();

    QSet<QString> validKeys{VERSION_KEY, DOMAIN_KEY, TR_BRIEF_KEY, TR_DESCRIPTION_KEY, WORKER_KEY,
        COMMANDS_KEY, EXTERN_HOOKS_KEY, EXTERN_OPTIONS_KEY, OPTIONS_KEY, HOOKS_KEY};
    if (!checkKeys(data, validKeys, errorString))
        return {};

    QVariant version = value(data, VERSION_KEY, QVariant::LongLong, {}, errorString);
    if (!version.isValid())
        return {};
    if (version.toInt() < 3 || version.toInt() > 4) {
        *errorString = tr("Version unsupported: %1").arg(version.toInt());
        return {};
    }

    QVariant domain = value(data, DOMAIN_KEY, QVariant::String, {}, errorString);
    if (!domain.isValid())
        return {};
    module->domain = ensureDomain(domain.toString());

    QVariant briefDescription = value(data, TR_BRIEF_KEY, QVariant::String, {}, errorString);
    if (!briefDescription.isValid())
        return {};
    module->briefDescription = localizedString(briefDescription.toString());

    QVariant description = value(data, TR_DESCRIPTION_KEY, QVariant::String, QString(), errorString);
    if (!description.isValid())
        return {};
    module->description = localizedString(description.toString());

    QVariant workerData = value(data, WORKER_KEY, QVariant::Map, {}, errorString);
    if (!workerData.isValid())
        return {};

    module->worker = loadWorker(workerData.toMap(), errorString);
    if (module->worker == nullptr) {
        *errorString = addContext(*errorString, WORKER_KEY);
        return {};
    }

    Option::ConstList allModuleOptions;

    QVariant externOptionsData = value(data, EXTERN_OPTIONS_KEY, QVariant::List, QVariantList(),
            errorString);
    if (!externOptionsData.isValid())
        return {};

    if (!loadExternOptions(externOptionsData.toList(), &allModuleOptions, errorString)) {
        *errorString = addContext(*errorString, EXTERN_OPTIONS_KEY);
        return {};
    }

    QVariant optionsData = value(data, OPTIONS_KEY, QVariant::List, {}, errorString);
    if (!optionsData.isValid())
        return {};

    if (!loadOptions(module.get(), optionsData.toList(), &allModuleOptions, errorString)) {
        *errorString = addContext(*errorString, OPTIONS_KEY);
        return {};
    }

    Hook::ConstList allModuleHooks;

    QVariant externHooksData = value(data, EXTERN_HOOKS_KEY, QVariant::List, QVariantList(),
            errorString);
    if (!externHooksData.isValid())
        return {};

    if (!loadExternHooks(externHooksData.toList(), &allModuleHooks, errorString)) {
        *errorString = addContext(*errorString, EXTERN_HOOKS_KEY);
        return {};
    }

    QVariant hooksData = value(data, HOOKS_KEY, QVariant::List, QVariantList(), errorString);
    if (!hooksData.isValid())
        return {};

    if (!loadHooks(module.get(), hooksData.toList(), &allModuleHooks, errorString)) {
        *errorString = addContext(*errorString, HOOKS_KEY);
        return {};
    }

    QVariant commandsData = value(data, COMMANDS_KEY, QVariant::List, {}, errorString);
    if (!commandsData.isValid())
        return {};

    if (!loadCommands(module.get(), commandsData.toList(), allModuleOptions, allModuleHooks,
                errorString)) {
        *errorString = addContext(*errorString, COMMANDS_KEY);
        return {};
    }

    return module;
}

bool Dispatcher::loadExternOptions(const QVariantList &list, Option::ConstList *allModuleOptions,
        QString *errorString)
{
    if (!checkItems(list, QVariant::String, errorString))
        return false;

    for (const QString &spec : QVariant(list).toStringList()) {
        QStringList names;
        if (!expandCompacted(spec, &names)) {
            *errorString = tr("Invalid compacted option specification: '%1'").arg(spec);
            return false;
        }
        for (const QString &name : names) {
            const Option *const option = m_optionByName.value(name);
            if (!option) {
                *errorString = tr("Unrecognized external option: '%1'").arg(name);
                return false;
            }

            allModuleOptions->append(option);
        }
    }

    return true;
}

bool Dispatcher::loadOptions(const Module *module, const QVariantList &list,
        Option::ConstList *allModuleOptions, QString *errorString)
{
    if (!checkItems(list, QVariant::Map, errorString))
        return false;

    for (const QVariant &item : list) {
        QVariantMap data = item.toMap();

        auto option = std::make_unique<Option>();
        option->module = module;

        QSet<QString> validKeys{NAME_KEY, ALIAS_KEY, TR_DESCRIPTION_KEY, TR_ARGUMENT_KEY,
            VALIDATOR_KEY};
        if (!checkKeys(data, validKeys, errorString))
            return false;

        QVariant name = value(data, NAME_KEY, QVariant::String, {}, errorString);
        if (!name.isValid())
            return false;
        option->name = name.toString();

        QVariant alias = value(data, ALIAS_KEY, QVariant::String, QString(), errorString);
        if (!alias.isValid())
            return false;
        option->alias = alias.toString();

        QVariant description = value(data, TR_DESCRIPTION_KEY, QVariant::String, {}, errorString);
        if (!description.isValid())
            return false;
        option->description = localizedString(description.toString());

        QVariant argumentDescription = value(data, TR_ARGUMENT_KEY, QVariant::String, QString(),
                errorString);
        if (!argumentDescription.isValid())
            return false;
        option->argumentDescription = localizedString(argumentDescription.toString());

        if (!option->argumentDescription.isNull()) {
            option->argumentType =
                option->argumentDescription.startsWith('[') && option->argumentDescription.endsWith(']')
                ? Option::OptionalArgument
                : Option::MandatoryArgument;
        } else {
            option->argumentType = Option::NoArgument;
        }

        QVariant validator = value(data, VALIDATOR_KEY, QVariant::String, QString(), errorString);
        if (!validator.isValid())
            return false;
        option->validatorJSFunctionName = validator.toString();

        if (!option->validatorJSFunctionName.isEmpty()) {
            const QRegularExpression qualifiedIdentifier("^\\w[\\w.]+$");
            if (!qualifiedIdentifier.match(option->validatorJSFunctionName).hasMatch()) {
                *errorString = tr("Not recognized as a (qualified) identifier: '%1'")
                    .arg(option->validatorJSFunctionName);
                return false;
            }
        }

        allModuleOptions->append(option.get());

        m_optionByName.insert(option->name, option.get());
        m_options.emplace_back(std::move(option));
    }

    return true;
}

bool Dispatcher::loadExternHooks(const QVariantList &list, Hook::ConstList *allModuleHooks,
        QString *errorString)
{
    if (!checkItems(list, QVariant::String, errorString))
        return false;

    for (const QString &spec : QVariant(list).toStringList()) {
        QStringList names;
        if (!expandCompacted(spec, &names)) {
            *errorString = tr("Invalid compacted hook specification: '%1'").arg(spec);
            return false;
        }
        for (const QString &name : names) {
            const Hook *const hook = m_hookByName.value(name);
            if (!hook) {
                *errorString = tr("Unrecognized external hook: '%1'").arg(name);
                return false;
            }

            allModuleHooks->append(hook);
        }
    }

    return true;
}

bool Dispatcher::loadHooks(const Module *module, const QVariantList &list,
        Hook::ConstList *allModuleHooks, QString *errorString)
{
    if (!checkItems(list, QVariant::Map, errorString))
        return false;

    for (const QVariant &item : list) {
        QVariantMap data = item.toMap();

        auto hook = std::make_unique<Hook>();
        hook->module = module;

        QSet<QString> validKeys{NAME_KEY, TR_SYNOPSIS_KEY, TR_DESCRIPTION_KEY};
        if (!checkKeys(data, validKeys, errorString))
            return false;

        QVariant name = value(data, NAME_KEY, QVariant::String, {}, errorString);
        if (!name.isValid())
            return false;
        hook->name = name.toString();

        QVariant synopsis = value(data, TR_SYNOPSIS_KEY, QVariant::String, QString(), errorString);
        if (!synopsis.isValid())
            return false;
        hook->synopsis = synopsis.toString();

        QVariant description = value(data, TR_DESCRIPTION_KEY, QVariant::String, {}, errorString);
        if (!description.isValid())
            return false;
        hook->description = localizedString(description.toString());

        allModuleHooks->append(hook.get());

        m_hookByName.insert(hook->name, hook.get());
        m_hooks.emplace_back(std::move(hook));
    }

    return true;
}

bool Dispatcher::loadCommands(const Module *module, const QVariantList &list,
        const Option::ConstList &allModuleOptions, const Hook::ConstList &allModuleHooks,
        QString *errorString)
{
    if (!checkItems(list, QVariant::Map, errorString))
        return false;

    for (const QVariant &item : list) {
        QVariantMap data = item.toMap();

        auto command = std::make_unique<Command>();
        command->module = module;

        QSet<QString> validKeys{NAME_KEY, TR_SYNOPSIS_KEY, TR_BRIEF_KEY, TR_DESCRIPTION_KEY,
            CONFIG_OPTIONS_KEY, HOOKS_KEY, DYNAMIC_KEY, DYNAMIC_SUBCOMMANDS_KEY,
            COMMAND_LINE_FILTER, PRE_RUN_KEY, POST_RUN_KEY};
        if (!checkKeys(data, validKeys, errorString))
            return false;

        QVariant name = value(data, NAME_KEY, QVariant::String, {}, errorString);
        if (!name.isValid())
            return false;
        command->name = name.toString();

        QVariant synopsis = value(data, TR_SYNOPSIS_KEY, QVariant::String, QString(), errorString);
        if (!synopsis.isValid())
            return false;
        command->synopsis = localizedString(synopsis.toString());

        QVariant briefDescription = value(data, TR_BRIEF_KEY, QVariant::String, {},
                errorString);
        if (!briefDescription.isValid())
            return false;
        command->briefDescription = localizedString(briefDescription.toString());

        QVariant description = value(data, TR_DESCRIPTION_KEY, QVariant::String, {}, errorString);
        if (!description.isValid())
            return false;
        command->description = localizedString(description.toString());

        QVariant configOptionsData = value(data, CONFIG_OPTIONS_KEY, QVariant::List, QVariantList(),
                errorString);
        if (!configOptionsData.isValid())
            return false;

        if (!loadCommandConfigOptions(command.get(), configOptionsData.toList(), allModuleOptions,
                    errorString)) {
            return false;
        }

        QVariant hooksData = value(data, HOOKS_KEY, QVariant::List, QVariantList(), errorString);
        if (!hooksData.isValid())
            return false;

        if (!loadCommandHooks(command.get(), hooksData.toList(), allModuleHooks, errorString)) {
            return false;
        }

        QVariant dynamic = value(data, DYNAMIC_KEY, QVariant::Bool, false, errorString);
        if (!dynamic.isValid())
            return false;
        command->dynamic = dynamic.toBool();

        QVariant dynamicSubcommandsData = value(data, DYNAMIC_SUBCOMMANDS_KEY, QVariant::List,
                QVariantList(), errorString);
        if (!dynamicSubcommandsData.isValid())
            return false;

        if (!loadDynamicSubcommands(command.get(), dynamicSubcommandsData.toList(), errorString))
            return false;

        QVariant commandLineFilter = value(data, COMMAND_LINE_FILTER, QVariant::String, QString(),
                errorString);
        if (!commandLineFilter.isValid())
            return false;
        command->commandLineFilterJSFunctionName = commandLineFilter.toString();

        QVariant preRun = value(data, PRE_RUN_KEY, QVariant::String, QString(), errorString);
        if (!preRun.isValid())
            return false;
        command->preRunJSFunctionName = preRun.toString();

        QVariant postRun = value(data, POST_RUN_KEY, QVariant::String, QString(), errorString);
        if (!postRun.isValid())
            return false;
        command->postRunJSFunctionName = postRun.toString();

        m_commandByName.insert(command->name, command.get());
        m_commands.emplace_back(std::move(command));
    }

    return true;
}

bool Dispatcher::loadCommandConfigOptions(Command *command, const QVariantList &list,
        const Option::ConstList &allModuleOptions, QString *errorString)
{
    if (!checkItems(list, QVariant::String, errorString))
        return false;

    for (QString optionSpec : QVariant(list).toStringList()) {
        if (optionSpec == "*") {
            command->configOptions.append(allModuleOptions);
            continue;
        }

        bool exclude = false;
        bool mandatory = false;

        if (optionSpec.startsWith("-")) {
            exclude = true;
            optionSpec.remove(0, 1);
        }
        if (optionSpec.endsWith("!")) {
            mandatory = true;
            optionSpec.chop(1);
        }

        QStringList names;
        if (!expandCompacted(optionSpec, &names)) {
            *errorString = tr("Invalid compacted option specification: '%1'").arg(optionSpec);
            return false;
        }
        for (const QString &name : names) {
            const Option *const option = m_optionByName.value(name);
            if (!option) {
                *errorString = tr("Unrecognized option: '%1'").arg(name);
                return false;
            }

            if (exclude) {
                command->configOptions.removeOne(option);
            } else {
                if (!command->configOptions.contains(option))
                    command->configOptions.append(option);
                if (mandatory)
                    command->mandatoryConfigOptions.append(option);
            }
        }
    }

    // Help keeping the module definition clean
    QTC_CHECK(command->configOptions.toSet().count() == command->configOptions.count());
    QTC_CHECK(command->mandatoryConfigOptions.toSet().count() == command->mandatoryConfigOptions.count());

    return true;
}

bool Dispatcher::loadCommandHooks(Command *command, const QVariantList &list,
        const Hook::ConstList &allModuleHooks, QString *errorString)
{
    if (!checkItems(list, QVariant::String, errorString))
        return false;

    for (QString hookSpec : QVariant(list).toStringList()) {
        if (hookSpec == "*") {
            command->hooks.append(allModuleHooks);
            continue;
        }

        bool exclude = false;
        if (hookSpec.startsWith("-")) {
            exclude = true;
            hookSpec.remove(0, 1);
        }

        QStringList names;
        if (!expandCompacted(hookSpec, &names)) {
            *errorString = tr("Invalid compacted hook specification: '%1'").arg(hookSpec);
            return false;
        }
        for (const QString &name : names) {
            const Hook *const hook = m_hookByName.value(name);
            if (!hook) {
                *errorString = tr("Unrecognized hook: '%1'").arg(name);
                return false;
            }

            if (exclude)
                command->hooks.removeOne(hook);
            else if (!command->hooks.contains(hook))
                command->hooks.append(hook);
        }
    }

    // Help keeping the module definition clean
    QTC_CHECK(command->hooks.toSet().count() == command->hooks.count());

    return true;
}

bool Dispatcher::loadDynamicSubcommands(Command *command, const QVariantList &list,
        QString *errorString)
{
    if (!checkItems(list, QVariant::String, errorString))
        return false;

    for (const QString &subcommand : QVariant(list).toStringList()) {
        if (subcommand.startsWith("-")) {
            *errorString = tr("Subcommand name may not start with dash: '%1'").arg(subcommand);
            return false;
        }
        command->dynamicSubcommands.append(subcommand);
    }

    // Help keeping the module definition clean
    QTC_CHECK(command->dynamicSubcommands.toSet().count() == command->dynamicSubcommands.count());

    return true;
}

const Worker *Dispatcher::loadWorker(const QVariantMap &data, QString *errorString)
{
    QVariant type = value(data, TYPE_KEY, QVariant::String, {}, errorString);
    if (!type.isValid())
        return nullptr;

    WorkerCreator creator = m_workerCreators.value(type.toString());
    if (creator == nullptr) {
        *errorString = tr("Unrecognized worker type: '%1'").arg(type.toString());
        return nullptr;
    }

    QVariant version = value(data, VERSION_KEY, QVariant::LongLong, {}, errorString);
    if (!version.isValid())
        return nullptr;

    QVariantMap untypedData = data;
    untypedData.remove(TYPE_KEY);
    untypedData.remove(VERSION_KEY);

    auto worker = creator(untypedData, version.toInt(), errorString);
    if (worker == nullptr)
        return nullptr;

    Worker *raw = worker.get();
    m_workers.emplace_back(std::move(worker));
    return raw;
}
