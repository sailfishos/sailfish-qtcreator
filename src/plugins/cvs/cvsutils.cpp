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

#include "cvsutils.h"

#include <utils/stringutils.h>

#include <QRegularExpression>
#include <QStringList>

namespace Cvs {
namespace Internal {

CvsRevision::CvsRevision(const QString &rev) : revision(rev)
{ }

CvsLogEntry::CvsLogEntry(const QString &f) : file(f)
{ }

/* Parse:
\code
RCS file: /repo/foo.h
Working file: foo.h
head: 1.2
...
----------------------------
revision 1.2
date: 2009-07-14 13:30:25 +0200;  author: <author>;  state: dead;  lines: +0 -0;  commitid: <id>;
<message>
----------------------------
revision 1.1
...
=============================================================================
\endcode */

QList<CvsLogEntry> parseLogEntries(const QString &o,
                                   const QString &directory,
                                   const QString &filterCommitId)
{
    enum ParseState { FileState, RevisionState, StatusLineState };

    QList<CvsLogEntry> rc;
    const QStringList lines = o.split('\n', Qt::SkipEmptyParts);
    ParseState state = FileState;

    const QString workingFilePrefix = QLatin1String("Working file: ");
    const QRegularExpression statusPattern(QLatin1String("^date: ([\\d\\-]+) .*commitid: ([^;]+);$"));
    const QRegularExpression revisionPattern(QLatin1String("^revision ([\\d\\.]+)$"));
    const QChar slash = QLatin1Char('/');
    Q_ASSERT(statusPattern.isValid() && revisionPattern.isValid());
    const QString fileSeparator = QLatin1String("=============================================================================");

    // Parse using a state enumeration and regular expressions as not to fall for weird
    // commit messages in state 'RevisionState'
    QRegularExpressionMatch match;
    for (const QString &line : lines) {
        switch (state) {
            case FileState:
            if (line.startsWith(workingFilePrefix)) {
                QString file = directory;
                if (!file.isEmpty())
                    file += slash;
                file += line.mid(workingFilePrefix.size()).trimmed();
                rc.push_back(CvsLogEntry(file));
                state = RevisionState;
            }
            break;
        case RevisionState:
            match = revisionPattern.match(line);
            if (match.hasMatch()) {
                rc.back().revisions.push_back(CvsRevision(match.captured(1)));
                state = StatusLineState;
            } else {
                if (line == fileSeparator)
                    state = FileState;
            }
            break;
        case StatusLineState:
            match = statusPattern.match(line);
            if (match.hasMatch()) {
                const QString commitId = match.captured(2);
                if (filterCommitId.isEmpty() || filterCommitId == commitId) {
                    rc.back().revisions.back().date = match.captured(1);
                    rc.back().revisions.back().commitId = commitId;
                } else {
                    rc.back().revisions.pop_back();
                }
                state = RevisionState;
            }
        }
    }
    // Purge out files with no matching commits
    if (!filterCommitId.isEmpty()) {
        for (QList<CvsLogEntry>::iterator it = rc.begin(); it != rc.end(); ) {
            if (it->revisions.empty())
                it = rc.erase(it);
            else
                ++it;
        }
    }
    return rc;
}

QString fixDiffOutput(QString d)
{
    if (d.isEmpty())
        return d;
    // Kill all lines starting with '?'
    const QChar questionMark = QLatin1Char('?');
    const QChar newLine = QLatin1Char('\n');
    for (int pos = 0; pos < d.size(); ) {
        const int endOfLinePos = d.indexOf(newLine, pos);
        if (endOfLinePos == -1)
            break;
        const int nextLinePos = endOfLinePos + 1;
        if (d.at(pos) == questionMark)
            d.remove(pos, nextLinePos - pos);
        else
            pos = nextLinePos;
    }
    return d;
}

// Parse "cvs status" output for added/modified/deleted files
// "File: <foo> Status: Up-to-date"
// "File:  <foo> Status: Locally Modified"
// "File: no file <foo> Status: Locally Removed"
// "File: hup Status: Locally Added"
// Not handled for commit purposes: "Needs Patch/Needs Merge"
// In between, we might encounter "cvs status: Examining subdir"...
// As we run the status command from the repository directory,
// we need to add the full path, again.
// stdout/stderr need to be merged to catch directories.

// Parse out status keywords, return state enum or -1
static int stateFromKeyword(const QString &s)
{
    if (s == QLatin1String("Up-to-date"))
        return -1;
    if (s == QLatin1String("Locally Modified"))
        return CvsSubmitEditor::LocallyModified;
    if (s == QLatin1String("Locally Added"))
        return CvsSubmitEditor::LocallyAdded;
    if (s == QLatin1String("Locally Removed"))
        return CvsSubmitEditor::LocallyRemoved;
    return -1;
}

StateList parseStatusOutput(const QString &directory, const QString &output)
{
    StateList changeSet;
    const QString fileKeyword = QLatin1String("File: ");
    const QString statusKeyword = QLatin1String("Status: ");
    const QString noFileKeyword = QLatin1String("no file ");
    const QString directoryKeyword = QLatin1String("cvs status: Examining ");
    const QString dotDir = QString(QLatin1Char('.'));
    const QChar slash = QLatin1Char('/');

    const QStringList list = output.split('\n', Qt::SkipEmptyParts);

    QString path = directory;
    if (!path.isEmpty())
        path += slash;
    foreach (const QString &l, list) {
        // Status line containing file
        if (l.startsWith(fileKeyword)) {
            // Parse state
            const int statusPos = l.indexOf(statusKeyword);
            if (statusPos == -1)
                continue;
            const int state = stateFromKeyword(l.mid(statusPos + statusKeyword.size()).trimmed());
            if (state == -1)
                continue;
            // Concatenate file name, Correct "no file <foo>"
            QString fileName = l.mid(fileKeyword.size(), statusPos - fileKeyword.size()).trimmed();
            if (state == CvsSubmitEditor::LocallyRemoved && fileName.startsWith(noFileKeyword))
                fileName.remove(0, noFileKeyword.size());
            changeSet.push_back(CvsSubmitEditor::StateFilePair(static_cast<CvsSubmitEditor::State>(state), path + fileName));
            continue;
        }
        // Examining a new subdirectory
        if (l.startsWith(directoryKeyword)) {
            path = directory;
            if (!path.isEmpty())
                path += slash;
            const QString newSubDir = l.mid(directoryKeyword.size()).trimmed();
            if (newSubDir != dotDir) { // Skip Examining '.'
                path += newSubDir;
                path += slash;
            }
            continue;
        }
    }
    return changeSet;
}

// Decrement version number "1.2" -> "1.1"
QString previousRevision(const QString &rev)
{
    const int dotPos = rev.lastIndexOf(QLatin1Char('.'));
    if (dotPos == -1)
        return rev;
    const int minor = rev.mid(dotPos + 1).toInt();
    return rev.left(dotPos + 1) + QString::number(minor - 1);
}

// Is "[1.2...].1"?
bool isFirstRevision(const QString &r)
{
    return r.endsWith(QLatin1String(".1"));
}

} // namespace Internal
} // namespace Cvs
