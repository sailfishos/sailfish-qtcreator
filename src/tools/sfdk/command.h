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

#include "configuration.h"
#include "sdkmanager.h"

#include <QCoreApplication>
#include <QList>
#include <QString>

#include <memory>

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

class Device;
class Domain;
class Emulator;
class Module;

class Hook
{
public:
    using ConstList = QList<const Hook *>;
    using ConstUniqueList = std::vector<std::unique_ptr<const Hook>>;

    const Module *module = nullptr;
    QString name;
    QString synopsis;
    QString description;
};

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
    Option::ConstList configOptions;
    Option::ConstList mandatoryConfigOptions;
    Hook::ConstList hooks;
    bool dynamic = false;
    QStringList dynamicSubcommands;
    QString commandLineFilterJSFunctionName;
    QString preRunJSFunctionName;
    QString postRunJSFunctionName;
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
        NormalExit
    };

    virtual ~Worker() = default;

    ExitStatus run(const Command *command, const QStringList &arguments, int *exitCode) const;

protected:
    virtual ExitStatus doRun(const Command *command, const QStringList &arguments, int *exitCode)
        const = 0;
    static QString crashExitErrorMessage();
    static bool checkVersion(int version, int minSupported, int maxSupported, QString *errorMessage);
};

class PropertiesAccessor;
class SetPropertiesTask;
class BuiltinWorker : public Worker
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::BuiltinWorker)

public:
    static std::unique_ptr<Worker> fromMap(const QVariantMap &data, int version, QString *errorString);

protected:
    ExitStatus doRun(const Command *command, const QStringList &arguments, int *exitCode) const
        override;

private:
    ExitStatus runConfig(const QStringList &arguments0) const;
    ExitStatus runDebug(const QStringList &arguments0, int *exitCode) const;
    ExitStatus runDevice(const QStringList &arguments, int *exitCode) const;
    ExitStatus runEmulator(const QStringList &arguments, int *exitCode) const;
    ExitStatus runEngine(const QStringList &arguments, int *exitCode) const;
    ExitStatus runMaintain(const QStringList &arguments, int *exitCode) const;
    ExitStatus runMisc(const QStringList &arguments, int *exitCode) const;
    ExitStatus runTools(const QStringList &arguments, int *exitCode) const;

    static void listDevices();
    static Device *deviceForNameOrIndex(const QString &deviceNameOrIndex,
            QString *errorString);
    static bool listEmulators(SdkManager::ListEmulatorsOptions options);
    static Emulator *defaultEmulator(QString *errorString);
    static bool listTools(SdkManager::ListToolsOptions options, bool listToolings,
            bool listTargets);
    static bool stopVirtualMachines();

    static void printProperties(const PropertiesAccessor &accessor);
    static ExitStatus setProperties(SetPropertiesTask *task, const QStringList &assignments,
            int *exitCode);

    static QString runningYesNoMessage(bool running);
    static QString toString(ToolsInfo::Flags flags, bool saySdkProvided);
    static QString toString(EmulatorInfo::Flags flags, bool saySdkProvided, bool indicateDefault);
};

class EngineWorker : public Worker
{
public:
    static std::unique_ptr<Worker> fromMap(const QVariantMap &data, int version, QString *errorString);

protected:
    ExitStatus doRun(const Command *command, const QStringList &arguments, int *exitCode) const
        override;

private:
    bool makeGlobalArguments(const Command *command, QStringList *arguments, QString *errorString)
        const;
    QStringList makeGlobalArguments(const Command *command,
            const OptionEffectiveOccurence &occurence) const;
    void maybeMakeCustomGlobalArguments(const Command *command,
            const OptionEffectiveOccurence &optionOccurence, QStringList *arguments) const;
    void maybeDoQtCreatorDeploymentTxtMapping() const;

private:
    QString m_program;
    QStringList m_initialArguments;
    bool m_omitSubcommand = false;
    QString m_optionFormatterJSFunctionName;
};

} // namespace Sfdk
