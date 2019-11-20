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

#include <memory>

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>

#include <utils/optional.h>

namespace Sfdk {

class ConfigurationScope;
class Domain;
class Module;

class Option
{
public:
    using ConstUniqueList = std::vector<std::unique_ptr<const Option>>;
    using ConstList = QList<const Option *>;

    enum ArgumentType {
        NoArgument,
        OptionalArgument,
        MandatoryArgument
    };

    const Domain *domain() const;

    const Module *module;
    QString name;
    QString alias;
    QString description;
    ArgumentType argumentType = NoArgument;
    QString argumentDescription;
};

class OptionOccurence
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::OptionOccurence)

public:
    enum Type {
        Push,
        Mask,
    };

    OptionOccurence() = default;
    OptionOccurence(const Option *option, Type type, const QString &argument = QString())
        : m_option(option)
        , m_type(type)
        , m_argument(argument)
    {
    }

    bool isNull() const { return m_option == nullptr; }
    const Option *option() const { Q_ASSERT(!isNull()); return m_option; }
    Type type() const { Q_ASSERT(!isNull()); return m_type; }
    QString argument() const { Q_ASSERT(!isNull()); return m_argument; }
    QString errorString() const { return m_errorString; }

    QString toString() const;
    static OptionOccurence fromString(const QString &string);

private:
    const Option *m_option = nullptr;
    Type m_type;
    QString m_argument;
    QString m_errorString;
};

class OptionEffectiveOccurence
{
public:
    explicit OptionEffectiveOccurence(const Option *option, const QString &argument,
            const ConfigurationScope *origin)
        : m_option(option)
        , m_origin(origin)
        , m_argument(argument)
    {
    }

    const Option *option() const { return m_option; }
    const ConfigurationScope *origin() const { return m_origin; }
    QString argument() const { return m_argument; }
    void setArgument(const QString &argument) { m_argument = argument; }
    void extendArgument(const QString &extension) { m_argument += extension; }
    bool isMasked() const { return m_maskedBy != nullptr; }
    const ConfigurationScope *maskedBy() const { return m_maskedBy; }
    void setMaskedBy(const ConfigurationScope *configurationScope) { m_maskedBy = configurationScope; }

private:
    const Option *m_option = nullptr;
    const ConfigurationScope *m_origin = nullptr;
    const ConfigurationScope *m_maskedBy = nullptr;
    QString m_argument;
};

class Configuration
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::Configuration)

public:
    enum Scope {
        Global,
        Session,
        Command
    };

    Configuration();
    ~Configuration();

    static bool load();
    static bool isLoaded();

    static bool push(Scope scope, const OptionOccurence &occurence);
    static bool push(Scope scope, const Option *option, const QString &argument = QString());
    static bool pushMask(Scope scope, const Option *option);
    static bool drop(Scope scope, const Option *option);

    static QString scopeName(Scope scope);

    static QList<OptionEffectiveOccurence> effectiveState();
    static Utils::optional<OptionEffectiveOccurence> effectiveState(const Option *option);
    static QStringList toArguments(const Module *module);
    static QString print();

private:
    static QString globalConfigurationPath();
    static QString sessionConfigurationPath();
    static void cleanUpSessions();
    static QString sessionConfigurationLocation();
    bool save(Scope scope);
    ConfigurationScope *scope(Scope scope) const;

private:
    static Configuration *s_instance;
    bool m_loaded = false;
    std::unique_ptr<ConfigurationScope> m_globalScope;
    std::unique_ptr<ConfigurationScope> m_sessionScope;
    std::unique_ptr<ConfigurationScope> m_commandScope;
};

} // namespace Sfdk
