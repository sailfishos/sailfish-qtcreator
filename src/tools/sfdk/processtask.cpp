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

#include "processtask.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>

using namespace Sfdk;

/*!
 * \class ProcessTaskBase
 */

void ProcessTaskBase::init()
{
    connect(&asQProcess(), &QProcess::started,
            this, &ProcessTask::onProcessStarted);
    connect(&asQProcess(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessTask::onProcessFinished);
}

void ProcessTaskBase::beginTerminate()
{
    // Nothing to do here - we only spawn foreground child processes
    const bool ok = true;
    endTerminate(ok);
}

void ProcessTaskBase::beginStop()
{
    // Nothing to do here - we only spawn foreground child processes
    const bool ok = true;
    endStop(ok);
}

void ProcessTaskBase::beginContinue()
{
    // Nothing to do here - we only spawn foreground child processes
    const bool ok = true;
    endContinue(ok);
}

void ProcessTaskBase::onProcessStarted()
{
    Task::started();
}

void ProcessTaskBase::onProcessFinished()
{
    Task::exited();
}

/*!
 * \class ProcessTask
 */

ProcessTask::ProcessTask(QObject *parent)
    : ProcessTaskBase(parent)
    , QProcess(static_cast<Task *>(this))
{
    init();
}
