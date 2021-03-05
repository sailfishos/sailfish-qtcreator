/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "ioutputparser.h"

#include <projectexplorer/task.h>

#include <QRegularExpression>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GccParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    GccParser();

    static Utils::Id id();

    static QList<OutputLineParser *> gccParserSuite();

protected:
    void createOrAmendTask(
            Task::TaskType type,
            const QString &description,
            const QString &originalLine,
            bool forceAmend = false,
            const Utils::FilePath &file = {},
            int line = -1,
            const LinkSpecs &linkSpecs = {}
            );
    void flush() override;

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    bool isContinuation(const QString &newLine) const;

    QRegularExpression m_regExp;
    QRegularExpression m_regExpScope;
    QRegularExpression m_regExpIncluded;
    QRegularExpression m_regExpInlined;
    QRegularExpression m_regExpGccNames;
    QRegularExpression m_regExpCc1plus;

    Task m_currentTask;
    LinkSpecs m_linkSpecs;
    int m_lines = 0;
};

} // namespace ProjectExplorer
