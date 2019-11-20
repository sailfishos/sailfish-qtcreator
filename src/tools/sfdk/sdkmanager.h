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
#include <QProcessEnvironment>
#include <QString>

namespace Mer {
namespace Internal {
    class MerSettings;
}
}

namespace Sfdk {

class BuildEngine;
class Device;
class Emulator;
class Sdk;
class VirtualMachine;

class SdkManager
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::SdkManager)

public:
    SdkManager(bool useSystemSettingsOnly);
    ~SdkManager();

    static bool isValid();

    static QString installationPath();
    static bool startEngine();
    static bool stopEngine();
    static bool isEngineRunning();
    static int runOnEngine(const QString &program, const QStringList &arguments,
            QProcess::InputChannelMode inputChannelMode = QProcess::ManagedInputChannel);

    static void setEnableReversePathMapping(bool enable);

    static Device *configuredDevice(QString *errorMessage);
    static Device *deviceByName(const QString &deviceName, QString *errorMessage);
    static int runOnDevice(const Device &device, const QString &program,
        const QStringList &arguments, QProcess::InputChannelMode inputChannelMode);

    static QList<Emulator *> sortedEmulators();
    static Emulator *emulatorByName(const QString &emulatorName, QString *errorMessage);
    static bool startEmulator(const Emulator &emulator);
    static bool stopEmulator(const Emulator &emulator);
    static bool isEmulatorRunning(const Emulator &emulator);
    static int runOnEmulator(const Emulator &emulator, const QString &program,
            const QStringList &arguments,
            QProcess::InputChannelMode inputChannelMode = QProcess::ManagedInputChannel);

    static bool startReliably(VirtualMachine *virtualMachine);
    static bool stopReliably(VirtualMachine *virtualMachine);
    static bool isRunningReliably(VirtualMachine *virtualMachine);

    static void saveSettings();

    static QString stateUserDefinedMessage() { return tr("user-defined"); }
    static QString stateAutodetectedMessage() { return tr("autodetected"); }

private:
    bool hasEngine() const;
    QString cleanSharedHome() const;
    QString cleanSharedSrc() const;
    bool mapEnginePaths(QString *program, QStringList *arguments, QString *workingDirectory,
            QProcessEnvironment *environment) const;
    QByteArray maybeReverseMapEnginePaths(const QByteArray &commandOutput) const;
    QProcessEnvironment environmentToForwardToEngine() const;

private:
    static SdkManager *s_instance;
    bool m_enableReversePathMapping = true;
    std::unique_ptr<Mer::Internal::MerSettings> m_merSettings;
    std::unique_ptr<Sdk> m_sdk;
    BuildEngine *m_buildEngine = nullptr;
};

} // namespace Sfdk
