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

#include <memory>

#include <QCoreApplication>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QCommandLineParser;
class QCommandLineOption;
QT_END_NAMESPACE

namespace Mer {
    class Connection;
}

namespace QSsh {
    class SshConnectionParameters;
}

namespace Sfdk {

class Domain;
class Module;

class Command
{
public:
    using ConstList = QList<const Command *>;
    using ConstUniqueList = std::vector<std::unique_ptr<const Command>>;

    const Domain *domain() const;

    const Module *module = nullptr;
    QString name;
    QString synopsis;
    QString briefDescription;
    QString description;
};

class Worker
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::Worker)

public:
    using ConstList = QList<const Worker *>;
    using ConstUniqueList = std::vector<std::unique_ptr<const Worker>>;

    enum ExitStatus {
        NoSuchCommand,
        BadUsage,
        CrashExit,
        NormalExit
    };

    virtual ~Worker() = default;

    virtual ExitStatus run(const Command *command, const QStringList &arguments, int *exitCode)
        const = 0;

protected:
    static QString crashExitErrorMessage();
    static bool checkVersion(int version, int minSupported, int maxSupported, QString *errorMessage);
};

class BuiltinWorker : public Worker
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::BuiltinWorker)

public:
    ExitStatus run(const Command *command, const QStringList &arguments, int *exitCode) const
        override;

    static std::unique_ptr<Worker> fromMap(const QVariantMap &data, int version, QString *errorString);

private:
    ExitStatus runConfig(const QStringList &arguments0) const;
    ExitStatus runEngine(const QStringList &arguments, int *exitCode) const;
    ExitStatus runMaintain(const QStringList &arguments, int *exitCode) const;
};

class EngineWorker : public Worker
{
public:
    ExitStatus run(const Command *command, const QStringList &arguments, int *exitCode) const
        override;

    static std::unique_ptr<Worker> fromMap(const QVariantMap &data, int version, QString *errorString);

private:
    QString m_program;
    QStringList m_initialArguments;
    bool m_omitSubcommand = false;
};

} // namespace Sfdk
