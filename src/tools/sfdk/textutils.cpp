/****************************************************************************
**
** Copyright (C) 2019,2021 Jolla Ltd.
** Copyright (C) 2019,2020 Open Mobile Platform LLC.
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

#include "sfdkconstants.h"
#include "sfdkglobal.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>

#if defined(Q_OS_WIN)
extern "C" {
# include "3rdparty/iscygpty.h"
}

# include <windows.h>
# include <fcntl.h>
# include <handleapi.h>
# include <io.h>
# include <stdio.h>
#endif

#if defined(Q_OS_UNIX)
# include <unistd.h>
#endif

using namespace Utils;

namespace Sfdk {

namespace {
const int SHIFT_WIDTH = 4;
const int MAN_LEFT_MARGIN = 8;
const int WRAP_MARGIN = 78 - MAN_LEFT_MARGIN;
const char PAGER[] = "less";

const int TREE_SHIFT_WIDTH = 4;
const QString TREE_LAST_ITEM_PREFIX = "└── ";
const QString TREE_LAST_DESCENDANTS_PREFIX = "    ";
const QString TREE_ITEM_PREFIX = "├── ";
const QString TREE_DESCENDANTS_PREFIX = "│   ";
const QString TREE_COLUMN_SEPARATOR = "  ";
} // namespace anonymous

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

std::unique_ptr<QFile> binaryOut(FILE *out)
{
    auto binaryFile = std::make_unique<QFile>();

#if defined(Q_OS_WIN)
    intptr_t osfhandle = _get_osfhandle(_fileno(out));
    QTC_CHECK(osfhandle >= 0);
    int binaryFd = _open_osfhandle(osfhandle, _O_WRONLY);
    QTC_CHECK(binaryFd >= 0);
    binaryFile->open(binaryFd, QIODevice::WriteOnly);
#else
    binaryFile->open(out, QIODevice::WriteOnly);
#endif

    QTC_CHECK(binaryFile->isOpen());

    return binaryFile;
}

/*
 * The main use case for checking this is deciding whether a remote command
 * should be executed in terminal or not, so let's compare this with SSH
 * behavior.
 *
 * SSH client decides based on stdin only. I believe it does not check stdout
 * simply because it only allocates pseudo-terminal for interactive sessions
 * by default. Interactive sessions are those that invoke the default shell
 * (no command specified). It is not likely someone would redirect or pipe
 * output of an interactive session, so this works well as the default.
 *
 * sfdk cannot always do similar distinction between interactive and
 * non-interactive sessions.  E.g. commands executed by EngineWorker are
 * considered interactive by default, but their (error) output may be piped or
 * redirected too. Hence the need to check all stdin, stdout and stderr here.
 */
bool isConnectedToTerminal()
{
    static const bool isConnectedToTerminal = []() -> bool {
        if (!qEnvironmentVariableIsEmpty(Constants::CONNECTED_TO_TERMINAL_HINT_ENV_VAR))
            return qEnvironmentVariableIntValue(Constants::CONNECTED_TO_TERMINAL_HINT_ENV_VAR);

#if defined(Q_OS_WIN)
        return GetConsoleWindow()
            && is_cygpty(_fileno(stdin)) && is_cygpty(_fileno(stdout)) && is_cygpty(_fileno(stderr));
#else
        return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
#endif
    }();

    qCDebug(sfdk) << "Connected to terminal:" << isConnectedToTerminal;

    return isConnectedToTerminal;
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

/*!
 * \class AsciiMan
 * \brief Minimal rendered for AsciiDoc-like formatted man pages
 *
 * Recognized AsciiDoc constructs:
 *
 * - One line section titles without trailing title delimiter
 *   - "== TITLE", not "== TITLE =="
 * - Indented literal paragraphs
 *   - Any indented text will be passed through as is
 * - Open blocks styled as "verse"
 * - List item continuation and/or nesting realized with open blocks
 *   - Use of open blocks is mandatory here. Any indented text would be treated
 *     as an indented literal paragraph
 * - Bold words at the start of a line
 *   - The enclosing asterisks are dropped
 *
 * Anything else will be passed as is.
 */

AsciiMan::AsciiMan(QTextStream &out)
    : m_in(&m_buf)
    , m_out(out)
{
}

AsciiMan::~AsciiMan()
{
    flush();
}

void AsciiMan::flush()
{
    m_in.seek(0);
    render(m_in, m_out);
    m_buf.clear();
}

void AsciiMan::render(QTextStream &in, QTextStream &out)
{
    bool prevBlank = true;
    bool expectOpenBlock = false;
    bool verse = false;
    int indentLevel = 0;
    QString section;

    const QRegularExpression headingRe("^=+ ");
    const QRegularExpression attributesRe("^\\[(.*)\\]$");
    const QRegularExpression boldLeadingWordsRe(R"(^\*([^\*]+)\*(| .*)$)");

    QString line;
    while (!in.atEnd()) {
        line = in.readLine();

        QRegularExpressionMatch match;
        if (prevBlank && line.contains(headingRe, &match)) {
            indentLevel = qMax(0, match.capturedLength() - 3);
            line.remove(0, match.capturedLength());
            if (indentLevel == 0)
                line = line.toUpper();
            section = line;
            out << indent(indentLevel) << line << endl;
            indentLevel++;
            prevBlank = true;
            continue;
        } else if (line == "--") {
            if (expectOpenBlock) {
                if (!verse)
                    indentLevel++;
            } else {
                if (!verse)
                    indentLevel--;
                verse = false;
            }
        } else if (line == "+") {
            expectOpenBlock = true;
        } else if (line.contains(attributesRe, &match)) {
            verse = match.captured(1) == "verse";
            if (!verse)
                qCWarning(sfdk) << "Unrecognized AsciiDoc attribute" << match.captured(1);
            expectOpenBlock = true;
        } else {
            if (line.endsWith("::"))
                line.chop(2);

            if (line.contains(boldLeadingWordsRe, &match))
                line = match.capturedTexts().mid(1).join(QString());

            expectOpenBlock = false;

            if (verse || line.startsWith(' ') || line.startsWith('\n')) {
                int extraIndent = 0;
                if (section == "SYNOPSIS")
                    extraIndent = 1;
                out << indent(indentLevel + extraIndent) << line << endl;
            } else {
                wrapLine(out, indentLevel, {}, {}, line);
            }
        }

        prevBlank = line.isEmpty();
    }
}

void AsciiMan::header(QTextStream &out, const QString &title)
{
    out << "= " << title << "(1)" << endl;
}

void AsciiMan::section(QTextStream &out, const QString &title)
{
    out << "== " << title << endl;
}

void AsciiMan::subsection(QTextStream &out, const QString &title)
{
    out << "=== " << title << endl;
}

void AsciiMan::nameSection(QTextStream &out)
{
    section(out, "NAME");
}

void AsciiMan::synopsisSection(QTextStream &out)
{
    section(out, "SYNOPSIS");
}

void AsciiMan::verseBegin(QTextStream &out)
{
    out << "[verse]" << endl;
    out << "--" << endl;
}

void AsciiMan::verseEnd(QTextStream &out)
{
    out << "--" << endl;
}

void AsciiMan::labeledListItemBegin(QTextStream &out, const QString &label)
{
    out << label << "::" << endl;
    out << "+" << endl;
    out << "--" << endl;
}

void AsciiMan::labeledListItemEnd(QTextStream &out)
{
    out << "--" << endl;
}

QString AsciiMan::boldLeadingWords(const QString &string)
{
    return '*' + string + '*';
}

/*!
 * \class LineEndPostprocessingMessageHandler
 *
 * Unix-only: Screen-oriented programs turn TTY output postprocessing off.
 * This is also the case when SshRemoteProcessRunner::runInTerminal() runs
 * `ssh -t`. While that is running, we need to do do the LF -> CRLF
 * translation on our side or the possible log output would not be aligned.
 */

int LineEndPostprocessingMessageHandler::counter = 0;
QtMessageHandler LineEndPostprocessingMessageHandler::defaultHandler;

LineEndPostprocessingMessageHandler::LineEndPostprocessingMessageHandler()
{
    if (!HostOsInfo::isAnyUnixHost())
        return;

    if (++counter > 1)
        return;

    defaultHandler = qInstallMessageHandler(handler);
}

LineEndPostprocessingMessageHandler::~LineEndPostprocessingMessageHandler()
{
    if (!HostOsInfo::isAnyUnixHost())
        return;

    if (--counter > 0)
        return;

    qInstallMessageHandler(defaultHandler);
}

void LineEndPostprocessingMessageHandler::handler(QtMsgType type, const QMessageLogContext &context,
        const QString &msg)
{
    defaultHandler(type, context, QString(msg)
            // Replace possible internal line endings
            .replace("\n", "\n\r")
            // At this point the message does not contain the final LF yet, so append a CR
            + '\r');
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
        const QStringList &prefix2, const QString &lines, int indentCorrection)
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

    static const QRegularExpression leadingSpace("^[ \\t]+"); // keep non-breakable space
    static const QRegularExpression trailingSpace("\\s+$");

    for (const QString &line : lines.split('\n')) {
        // Preserve single(!) level indentation. No one would ever need more than...
        const QString lineIndent = indent(line.startsWith(' ') || line.startsWith('\t') ? 1 : 0);
        // But try to not trim non-breakable space following leading space
        const QString trimmed = line.leftRef(10).contains(QChar::Nbsp)
            ? QString(line).remove(leadingSpace).remove(trailingSpace)
            : line.trimmed();
        wrapLine(out, indentLevel,
                prefix1_ + prefix2_ + lineIndent,
                QString(prefix1_.length() + prefix2_.length() + lineIndent.length() + indentCorrection, ' '),
                trimmed);
        prefix1_ = QString(prefix1_.length() + indentCorrection, ' ');
    }
}

/*
 * Simple expansion "[foo-|bar-]baz" -> {"baz", "foo-baz", "bar-baz"}. Supports up to one [] pair
 * at the very start of a string.
 */
bool expandCompacted(const QString &string, QStringList *expanded)
{
    const int leftCount = string.count('[');
    const int rightCount = string.count(']');
    const int left = string.indexOf('[');
    const int right = string.indexOf(']');
    if (leftCount != rightCount || leftCount > 1 || right < left || left > 0)
        return false;

    if (leftCount == 0) {
        *expanded << string;
        return true;
    }

    const QString root = string.mid(right + 1);
    const QString prefixes = string.mid(1, right - 1);

    *expanded << root;
    for (const QString &prefix : prefixes.split('|'))
        *expanded << prefix + root;

    return true;
}

TreePrinter::Tree TreePrinter::build(const QList<QStringList> &table, int idColumn,
        int parentIdColumn)
{
    Tree tree;

    for (const QStringList &row : topoSort(table, idColumn, parentIdColumn)) {
        QTC_ASSERT(row.count() > idColumn, return {});
        QTC_ASSERT(row.count() > parentIdColumn, return {});
        const QString id = row.at(idColumn);
        QTC_ASSERT(!findItem(tree, id, idColumn), return {});
        const QString parentId = row.at(parentIdColumn);

        std::vector<Item> *siblinks;
        if (!parentId.isEmpty()) {
            Item *const parent = findItem(tree, parentId, idColumn);
            QTC_ASSERT(parent, return {});
            siblinks = &parent->children;
        } else {
            siblinks = &tree;
        }

        siblinks->emplace_back(row);
    }

    return tree;
}

void TreePrinter::sort(Tree *tree, int level, int column, bool ascending)
{
    QTC_ASSERT(tree, return);
    QTC_ASSERT(level >= 0, return);
    QTC_ASSERT(column >= 0, return);

    if (level == 0) {
        Utils::sort(*tree, [=](const Item &i1, const Item &i2) -> bool {
            QTC_ASSERT(i1.columns.count() > column, return {});
            QTC_ASSERT(i2.columns.count() > column, return {});
            return ascending
                ? i1.columns.at(column) < i2.columns.at(column)
                : i1.columns.at(column) > i2.columns.at(column);
        });
    } else {
        for (auto it = tree->begin(); it != tree->end(); ++it)
            sort(&it->children, level - 1, column, ascending);
    }
}

void TreePrinter::sort(Tree *tree, int column, bool ascending)
{
    QTC_ASSERT(tree, return);
    QTC_ASSERT(column >= 0, return);

    sort(tree, 0, column, ascending);
    for (auto it = tree->begin(); it != tree->end(); ++it)
        sort(&it->children, column, ascending);
}

void TreePrinter::print(QTextStream &out, const Tree &tree, const QList<int> &columns)
{
    QList<int> widths;
    bool first = true;
    for (int column : columns) {
        widths << walk<int>(tree, 0, [=](int maxWidth, int depth, const Item &item) {
            int myWidth = item.columns.at(column).length();
            if (first)
                myWidth += depth * TREE_SHIFT_WIDTH;
            return qMax(maxWidth, myWidth);
        });
        first = false;
    }

    for (const Item &item : tree)
        print(out, item, {}, columns, widths);
}

void TreePrinter::print(QTextStream &out, const Item &item, const QString &linePrefix,
        const QList<int> &columns, const QList<int> &widths)
{
    for (int i = 0; i < columns.count(); ++i) {
        if (i != 0)
            out << TREE_COLUMN_SEPARATOR;
        const QString content = item.columns.at(columns.at(i));
        out << content;
        int pad = widths.at(i) - content.length();
        if (i == 0)
            pad -= linePrefix.length();
        out << QString(pad, ' ');
    }
    out << endl;

    for (auto it = item.children.cbegin(); it != item.children.cend(); ++it) {
        if (it + 1 == item.children.cend()) {
            out << linePrefix << TREE_LAST_ITEM_PREFIX;
            print(out, *it, linePrefix + TREE_LAST_DESCENDANTS_PREFIX, columns, widths);
        } else {
            out << linePrefix << TREE_ITEM_PREFIX;
            print(out, *it, linePrefix + TREE_DESCENDANTS_PREFIX, columns, widths);
        }
    }
}

TreePrinter::Item *TreePrinter::findItem(Tree &tree, const QString &id,
        int idColumn)
{
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        QTC_ASSERT(it->columns.count() > idColumn, return {});
        if (it->columns.at(idColumn) == id)
            return &*it;
        if (Item *descendant = findItem(it->children, id, idColumn))
            return descendant;
    }
    return nullptr;
}

QList<QStringList> TreePrinter::topoSort(const QList<QStringList> &table, int idColumn, int parentIdColumn)
{
    QSet<QString> seen;
    QList<QStringList> in = table;
    QList<QStringList> out;

    while (!in.isEmpty()) {
        QList<QStringList> now;
        QList<QStringList> later;
        std::tie(now, later) = Utils::partition(in, [&](const QStringList &row) {
            const QString parentId = row.at(parentIdColumn);
            return parentId.isEmpty() || seen.contains(parentId);
        });
        const QStringList nowIds = Utils::transform(now,
                [=](const QStringList &row) { return row.at(idColumn); });
        seen += nowIds.toSet();
        out += now;
        in = later;
    }

    return out;
}

} // namespace Sfdk
