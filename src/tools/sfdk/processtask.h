/****************************************************************************
**
** Copyright (C) 2026 Jolla Mobile Ltd
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

#include "task.h"

#include <QProcess>

namespace Sfdk {

class ProcessTaskBase : public Task
{
    Q_OBJECT

public:
    virtual QProcess &asQProcess() = 0;

protected:
    using Task::Task;
    void init();

    void beginTerminate() override;
    void beginStop() override;
    void beginContinue() override;

protected slots:
    void onProcessStarted();
    void onProcessFinished();
};

class ProcessTask : public ProcessTaskBase, private QProcess
{
public:
    explicit ProcessTask(QObject *parent = 0);

    QProcess &asQProcess() override { return *this; }

    using QProcess::program;
    using QProcess::setProgram;
    using QProcess::arguments;
    using QProcess::setArguments;
    using QProcess::setProcessEnvironment;
    using QProcess::setWorkingDirectory;

    using QProcess::start;
    using QProcess::waitForStarted;
    using QProcess::waitForFinished;

    using QProcess::state;
    using QProcess::error;
    using QProcess::exitCode;
    using QProcess::exitStatus;

    using QProcess::setInputChannelMode;
    using QProcess::setProcessChannelMode;
    using QProcess::closeWriteChannel;
    using QProcess::readAllStandardOutput;
    using QProcess::readAllStandardError;
};

} // namespace Sfdk
