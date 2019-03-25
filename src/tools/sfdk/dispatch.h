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

#pragma once

#include "command.h"
#include "configuration.h"

namespace Sfdk {

class Domain;
class Worker;

class Module
{
public:
    using ConstList = QList<const Module *>;
    using ConstUniqueList = std::vector<std::unique_ptr<const Module>>;

    QString fileName;
    const Domain *domain;
    const Worker *worker;
    QString briefDescription;
    QString description;
};

class Domain
{
public:
    using ConstList = QList<const Domain *>;
    using ConstUniqueList = std::vector<std::unique_ptr<const Domain>>;

    QString briefDescription() const;
    Module::ConstList modules() const;
    Command::ConstList commands() const;
    Option::ConstList options() const;

    QString name;
};

class Dispatcher
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::Dispatcher)

public:
    Dispatcher();
    ~Dispatcher();

    static bool load(const QString &moduleFileName);

    static const Domain::ConstUniqueList &domains();
    static const Domain *domain(const QString &name);

    static const Module::ConstUniqueList &modules();

    static const Worker::ConstUniqueList &workers();

    static const Command::ConstUniqueList &commands();
    static const Command *command(const QString &name);

    static const Option::ConstUniqueList &options();
    static const Option *option(const QString &name);

    template<typename Worker>
    static void registerWorkerType(const QString &name)
    {
        s_instance->registerWorkerType(name, [](const QVariantMap &data, int version, QString *errorString) {
            return Worker::fromMap(data, version, errorString);
        });
    }

    static QVariant value(const QVariantMap &data, const QString &key, QVariant::Type type, const
            QVariant &defaultValue, QString *errorString);
    static bool checkKeys(const QVariantMap &data, const QSet<QString> &validKeys, QString
            *errorString);
    static bool checkItems(const QVariantList &list, QVariant::Type type, QString *errorString);

private:
    using WorkerCreator = std::function<std::unique_ptr<Worker> (const QVariantMap &, int, QString *)>;
    void registerWorkerType(const QString &name, const WorkerCreator &creator);

    static QString localizedString(const QVariant &value);
    const Domain *ensureDomain(const QString &name);
    std::unique_ptr<Module> loadModule(const QVariantMap &data, QString *errorString);
    bool loadOptions(const Module *module, const QVariantList &list, QString *errorString);
    bool loadCommands(const Module *module, const QVariantList &list, QString *errorString);
    const Worker *loadWorker(const QVariantMap &data, QString *errorString);

private:
    static Dispatcher *s_instance;
    Domain::ConstUniqueList m_domains;
    Module::ConstUniqueList m_modules;
    Command::ConstUniqueList m_commands;
    QHash<QString, Command *> m_commandByName;
    Option::ConstUniqueList m_options;
    QHash<QString, Option *> m_optionByName;
    QHash<QString, WorkerCreator> m_workerCreators;
    Worker::ConstUniqueList m_workers;
};

} // namespace Sfdk
