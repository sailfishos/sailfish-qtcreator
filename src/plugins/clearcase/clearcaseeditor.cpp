/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "clearcaseeditor.h"
#include "annotationhighlighter.h"
#include "clearcaseconstants.h"
#include "clearcaseplugin.h"

#include <utils/qtcassert.h>
#include <vcsbase/diffandloghighlighter.h>

#include <QDir>
#include <QFileInfo>
#include <QTextBlock>
#include <QTextCursor>

using namespace ClearCase;
using namespace ClearCase::Internal;

ClearCaseEditorWidget::ClearCaseEditorWidget() :
    m_versionNumberPattern(QLatin1String("[\\\\/]main[\\\\/][^ \t\n\"]*"))
{
    QTC_ASSERT(m_versionNumberPattern.isValid(), return);
    // Diff formats:
    // "+++ D:\depot\...\mainwindow.cpp@@\main\3" (versioned)
    // "+++ D:\depot\...\mainwindow.cpp[TAB]Sun May 01 14:22:37 2011" (local)
    setDiffFilePattern("^[-+]{3} ([^\\t]+?)(?:@@|\\t)");
    setLogEntryPattern("version \"([^\"]+)\"");
    setAnnotateRevisionTextFormat(tr("Annotate version \"%1\""));
    setAnnotationEntryPattern("([^|]*)\\|[^\\n]*\\n");
    setAnnotationSeparatorPattern("\\n-{30}");
}

QString ClearCaseEditorWidget::changeUnderCursor(const QTextCursor &c) const
{
    QTextCursor cursor = c;
    // Any number is regarded as change number.
    cursor.select(QTextCursor::BlockUnderCursor);
    if (!cursor.hasSelection())
        return QString();
    QString change = cursor.selectedText();
    // Annotation output has number, log output has revision numbers
    // as r1, r2...
    const QRegularExpressionMatch match = m_versionNumberPattern.match(change);
    if (match.hasMatch())
        return match.captured();
    return QString();
}

VcsBase::BaseAnnotationHighlighter *ClearCaseEditorWidget::createAnnotationHighlighter(const QSet<QString> &changes) const
{
    return new ClearCaseAnnotationHighlighter(changes);
}
