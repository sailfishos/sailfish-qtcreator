/****************************************************************************
**
** Copyright (C) 2014-2015 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "merrpmvalidationparser.h"

#include "merconstants.h"
#include "mersettings.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

MerRpmValidationParser::MerRpmValidationParser()
    : m_headingRexp(QLatin1String("^===+$"))
    , m_errorRexp(QLatin1String("^ERROR\\s+"))
    , m_warningRexp(QLatin1String("^WARNING\\s+"))
    , m_infoRexp(QLatin1String("^INFO\\s+"))
{
    setObjectName(QLatin1String("RpmValidationParser"));
}

OutputLineParser::Result MerRpmValidationParser::handleLine(const QString &line, OutputFormat type)
{
    if (type == StdErrFormat)
        return Status::NotHandled;

    QString trimmed = line.trimmed();

    if (m_headingRexp.indexIn(trimmed) != -1) {
        if (!m_lastStdOutputLine.isEmpty()) {
            m_section = m_lastStdOutputLine;
        }
    } else if (m_errorRexp.indexIn(trimmed) != -1) {
        trimmed.remove(m_errorRexp);
        const QString message(tr("RPM Validation: %1: %2")
                .arg(m_section)
                .arg(trimmed));
        newTask(Task(Task::Error, message, FilePath(), -1,
                     Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return Status::Done;
    } else if (m_warningRexp.indexIn(trimmed) != -1) {
        trimmed.remove(m_warningRexp);
        const QString message(tr("RPM Validation: %1: %2")
                .arg(m_section)
                .arg(trimmed));
        newTask(Task(Task::Warning, message, FilePath(), -1,
                     Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return Status::Done;
    } else if (m_infoRexp.indexIn(trimmed) != -1) {
        trimmed.remove(m_infoRexp);
        amendDescription(trimmed);
        return Status::Done;
    }

    m_lastStdOutputLine = trimmed;

    flush();
    return Status::NotHandled;
}

void MerRpmValidationParser::newTask(const Task &task)
{
    flush();
    m_currentTask = task;
}

void MerRpmValidationParser::flush()
{
    if (m_currentTask.isNull())
        return;
    Task t = m_currentTask;
    m_currentTask.clear();
    scheduleTask(t, 1);
}

void MerRpmValidationParser::amendDescription(const QString &desc)
{
    if (m_currentTask.isNull())
        return;

    m_currentTask.description.append(QLatin1Char('\n'));
    m_currentTask.description.append(desc);
}

} // Internal
} // Mer
