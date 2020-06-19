/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace BareMetal {
namespace Internal {

// IarParser

class IarParser final : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit IarParser();
    static Core::Id id();

private:
    void newTask(const ProjectExplorer::Task &task);
    void amendDescription();
    void amendFilePath();

    bool parseErrorOrFatalErrorDetailsMessage1(const QString &lne);
    bool parseErrorOrFatalErrorDetailsMessage2(const QString &lne);
    bool parseWarningOrErrorOrFatalErrorDetailsMessage1(const QString &lne);
    bool parseErrorInCommandLineMessage(const QString &lne);
    bool parseErrorMessage1(const QString &lne);

    void stdError(const QString &line) final;
    void stdOutput(const QString &line) final;
    void doFlush() final;

    ProjectExplorer::Task m_lastTask;
    int m_lines = 0;
    bool m_expectSnippet = true;
    bool m_expectFilePath = false;
    bool m_expectDescription = false;
    QStringList m_snippets;
    QStringList m_filePathParts;
    QStringList m_descriptionParts;
};

} // namespace Internal
} // namespace BareMetal
