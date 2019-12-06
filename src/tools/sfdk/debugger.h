/****************************************************************************
**
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

#include <sfdk/buildengine.h>

#include <QCoreApplication>

namespace Sfdk {

class Device;

class Debugger
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(Sfdk::Debugger)

public:
    Debugger(const Device *device, const BuildTargetData &target);

    int execStart(const QString &executable, const QStringList &arguments,
            const QString &workingDirectory) const;
    int execAttach(const QString &executable, int pid) const;
    int execLoadCore(const QString &executable, const QString &coreFile, bool localCore) const;

    bool isDryRunEnabled() const { return m_dryRunEnabled; }
    void setDryRunEnabled(bool enabled) { m_dryRunEnabled = enabled; }

    QString gdbExecutable() const { return m_gdbExecutable; }
    void setGdbExecutable(const QString &executable) { m_gdbExecutable = executable; }
    QStringList gdbExtraArgs() const { return m_gdbExtraArgs; }
    void setGdbExtraArgs(const QStringList &args) { m_gdbExtraArgs = args; }

    QString gdbServerExecutable() const { return m_gdbServerExecutable; }
    void setGdbServerExecutable(const QString &executable) { m_gdbServerExecutable = executable; }
    QStringList gdbServerExtraArgs() const { return m_gdbServerExtraArgs; }
    void setGdbServerExtraArgs(const QStringList &args) { m_gdbServerExtraArgs = args; }

private:
    int exec(const QString &executable, const QList<QStringList> &gdbLateInit,
            const QString &gdbServerWorkingDirectory) const;
    QStringList findLocalExecutable(const QString &remoteExecutable) const;
    bool isElfExecutable(const QString &fileName) const;

private:
    const Device *const m_device;
    const BuildTargetData m_target;
    bool m_dryRunEnabled = false;
    QString m_gdbExecutable;
    QStringList m_gdbExtraArgs;
    QString m_gdbServerExecutable;
    QStringList m_gdbServerExtraArgs;
};

} // namespace Sfdk
