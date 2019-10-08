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

#ifndef MERRPMVALIDATIONPARSER_H
#define MERRPMVALIDATIONPARSER_H

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

#include <QRegExp>

namespace Mer {
namespace Internal {

class MerRpmValidationParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit MerRpmValidationParser();

    void stdOutput(const QString &line) override;
    void stdError(const QString &line) override;

protected:
    void newTask(const ProjectExplorer::Task &task);
    void doFlush() override;

    void amendDescription(const QString &desc);

private:
    QRegExp m_headingRexp;
    QRegExp m_errorRexp;
    QRegExp m_warningRexp;
    QRegExp m_infoRexp;
    QString m_lastStdOutputLine;
    QString m_section;
    ProjectExplorer::Task m_currentTask;
};

} // namespace Internal
} // namespace Mer

#endif // MERRPMVALIDATIONPARSER_H
