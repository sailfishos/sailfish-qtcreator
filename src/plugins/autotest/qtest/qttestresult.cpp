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

#include "qttestresult.h"

namespace Autotest {
namespace Internal {

QtTestResult::QtTestResult(const QString &className)
    : TestResult(className)
{
}

const QString QtTestResult::outputString(bool selected) const
{
    const QString &desc = description();
    const QString &className = name();
    QString output;
    switch (result()) {
    case Result::Pass:
    case Result::Fail:
    case Result::ExpectedFail:
    case Result::UnexpectedPass:
    case Result::BlacklistedFail:
    case Result::BlacklistedPass:
        output = className + QLatin1String("::") + m_function;
        if (!m_dataTag.isEmpty())
            output.append(QString::fromLatin1(" (%1)").arg(m_dataTag));
        if (selected && !desc.isEmpty()) {
            output.append(QLatin1Char('\n')).append(desc);
        }
        break;
    case Result::Benchmark:
        output = className + QLatin1String("::") + m_function;
        if (!m_dataTag.isEmpty())
            output.append(QString::fromLatin1(" (%1)").arg(m_dataTag));
        if (!desc.isEmpty()) {
            int breakPos = desc.indexOf(QLatin1Char('('));
            output.append(QLatin1String(": ")).append(desc.left(breakPos));
            if (selected)
                output.append(QLatin1Char('\n')).append(desc.mid(breakPos));
        }
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split(QLatin1Char('\n')).first();
    }
    return output;
}

} // namespace Internal
} // namespace Autotest
