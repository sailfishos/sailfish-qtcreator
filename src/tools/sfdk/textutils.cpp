/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "textutils.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include "sfdkglobal.h"

namespace Sfdk {

namespace {
const int SHIFT_WIDTH = 4;
const int WRAP_MARGIN = 78;
const char PAGER[] = "less";
}

QTextStream &qout()
{
    static QTextStream qout(stdout);
    return qout;
}

QTextStream &qerr()
{
    static QTextStream qerr(stderr);
    return qerr;
}

/*!
 * \class Pager
 */

bool Pager::s_enabled = true;

Pager::Pager()
    : m_stream(stdout)
{
    if (!s_enabled)
        return;

    m_pager.setProgram(PAGER);
    m_pager.setProcessChannelMode(QProcess::ForwardedChannels);
    m_pager.start(); // Using WriteOnly open mode would break it on Windows
    if (m_pager.waitForStarted())
        m_stream.setDevice(&m_pager);
    else
        qCDebug(sfdk) << "Pager not available";
}

Pager::~Pager()
{
    if (m_pager.state() != QProcess::NotRunning) {
        m_pager.closeWriteChannel();
        while (m_pager.state() != QProcess::NotRunning)
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
        if (m_pager.exitStatus() != QProcess::NormalExit
                || m_pager.exitCode() != 0) {
            qCDebug(sfdk) << "Pager exited with error" << m_pager.error()
                << "exitStatus" << m_pager.exitStatus()
                << "exitCode" << m_pager.exitCode();
        }
    }
}

QString indent(int level)
{
    return QString(level * SHIFT_WIDTH, ' ');
}

/*!
 * Indent the \a line according to \a indentLevel and wrap it at the configured wrap margin. Use \a
 * prefix1 at the beginning and \a prefix2 on every continuation line. Write to \a out.
 *
 * Preserves formatting of table-like content.
 */
void wrapLine(QTextStream &out, int indentLevel, const QString &prefix1, const QString &prefix2,
        const QString &line)
{
    QString indent = Sfdk::indent(indentLevel);

    // Do not try to reformat tables
    static QRegularExpression tableRow(R"(^\s*\|.*\|$)");
    if (tableRow.match(line).hasMatch()) {
        out << indent << prefix1 << line << endl;
        return;
    }

    static QRegularExpression spaces("[ \\t]+"); // \s would include non-breakable space
    const QStringList words = line.split(spaces);
    QString retv;
    int currentWidth = 0;
    bool first = true;
    for (const QString &word : words) {
        QString prefix = indent + (first ? prefix1 : prefix2);
        first = false;
        if (currentWidth + word.length() > WRAP_MARGIN) {
            out << endl;
            currentWidth = 0;
        }
        if (currentWidth == 0) {
            out << prefix;
            currentWidth += prefix.length();
        } else {
            out << ' ';
            ++currentWidth;
        }
        out << word;
        currentWidth += word.length();
    }
    out << endl;
}

/*!
 * Indent the \a line according to \a indentLevel and wrap it at the configured wrap margin.  Write
 * to \a out.
 */
void wrapLine(QTextStream &out, int indentLevel, const QString &line)
{
    wrapLine(out, indentLevel, {}, {}, line);
}

/*!
 * Indent all \a lines according to \a indentLevel and wrap them at the configured wrap margin. Use
 * \a prefix1 to prefix the very first line (pad other lines equally) and \a prefix2 for every
 * non-continuation line (pad other lines equally). Lines that start with a space or tab character
 * will be indented by one more level (preserves single level indentation). Write to \a out. Space
 * will be added automatically after every nonempty prefix item. If \a lines is empty, just the
 * prefixes will be printed.
 */
void wrapLines(QTextStream &out, int indentLevel, const QStringList &prefix1,
        const QStringList &prefix2, const QString &lines)
{
    QString prefix1_ = prefix1.join(' ');
    if (!prefix1_.isEmpty())
        prefix1_.append(' ');

    QString prefix2_ = prefix2.join(' ');
    if (!prefix2_.isEmpty())
        prefix2_.append(' ');

    if (lines.isEmpty()) {
        out << indent(indentLevel) << prefix1_ << prefix2.join(' ') << endl;
        return;
    }

    for (const QString &line : lines.split('\n')) {
        // Preserve single(!) level indentation. No one would ever need more than...
        const QString lineIndent = indent(line.startsWith(' ') || line.startsWith('\t') ? 1 : 0);
        wrapLine(out, indentLevel,
                prefix1_ + prefix2_ + lineIndent,
                QString(prefix1_.length() + prefix2_.length() + lineIndent.length(), ' '),
                line.trimmed());
        prefix1_ = QString(prefix1_.length(), ' ');
    }
}

} // namespace Sfdk
