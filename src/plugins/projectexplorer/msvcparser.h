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
#include "task.h"

#include <QRegularExpression>
#include <QString>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT MsvcParser :  public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    MsvcParser();

    static Utils::Id id();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    Result processCompileLine(const QString &line);

    QRegularExpression m_compileRegExp;
    QRegularExpression m_additionalInfoRegExp;

    Task m_lastTask;
    LinkSpecs m_linkSpecs;
    int m_lines = 0;
};

class PROJECTEXPLORER_EXPORT ClangClParser :  public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    ClangClParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    const QRegularExpression m_compileRegExp;
    Task m_lastTask;
    int m_linkedLines = 0;
};

} // namespace ProjectExplorer
