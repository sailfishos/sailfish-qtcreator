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
# include <sys/ioctl.h>
# include <termios.h>
# include <unistd.h>
#endif

using namespace Utils;

namespace Sfdk {

namespace {
const int SCROLL_BOX_HEIGHT = 10;

const int SHIFT_WIDTH = 4;
const int MAN_LEFT_MARGIN = 8;
const int WRAP_MARGIN = 78 - MAN_LEFT_MARGIN;
const QString PAGER = "less";
const QStringList PAGER_ARGUMENTS = {"--ignore-case"};

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
            && (_isatty(_fileno(stdin)) || is_cygpty(_fileno(stdin)))
            && (_isatty(_fileno(stdout)) || is_cygpty(_fileno(stdout)))
            && (_isatty(_fileno(stderr)) || is_cygpty(_fileno(stderr)));
#else
        return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
#endif
    }();

    qCDebug(sfdk) << "Connected to terminal:" << isConnectedToTerminal;

    return isConnectedToTerminal;
}

// QLocale::uiLanguages() does not provide the desired output
QStringList uiLanguages()
{
    const QString name = QLocale::system().name();

    QStringList languages;
    languages.append(name);
    const int pos = name.indexOf('_');
    if (pos != -1)
        languages.append(name.left(pos));

    return languages;
}

/*!
 * \class TerminalInfo
 */

class TerminalInfo
{
    Q_GADGET

public:
    enum Feature {
        NotCompatible = 0,
        CanNarrowScrollRegion = 1<<1,

        // Unix only
        CanIoctlTIOCGWINSZ = 1<<10,

        // Windows only
        CanGetConsoleScreenBufferInfo = 1<<20,
    };
    Q_DECLARE_FLAGS(Features, Feature)
    Q_FLAG(Features)

    static Features features();

    static bool canNarrowScrollRegion()
    { return features() & CanNarrowScrollRegion; }
    static bool canIoctlTIOCGWINSZ()
    { return features() & CanIoctlTIOCGWINSZ; }
    static bool canGetConsoleScreenBufferInfo()
    { return features() & CanGetConsoleScreenBufferInfo; }
};

TerminalInfo::Features TerminalInfo::features()
{
    static Features features = []() -> Features {
        if (!qEnvironmentVariableIsEmpty(Constants::TERMINAL_COMPATIBLE_HINT_ENV_VAR)) {
            Features features(
                qEnvironmentVariableIntValue(Constants::TERMINAL_COMPATIBLE_HINT_ENV_VAR));
            qCDebug(sfdk) << "Forcing TerminalInfo::features:" << features;
            return features;
        }

        Features features;

#if defined(Q_OS_WIN)
        // Under Mintty it is not possible to use GetConsoleScreenBufferInfo and
        // it is not really possible to use ANSI codes to query cursor position
        // and window size there (see AnsiIOWindows::initInput). Therefore
        // disabled.
        //if (isConnectedToTerminal()) {
        //    features |= CanNarrowScrollRegion;
        //}

        const QString windowsTerminalSession = qEnvironmentVariable("WT_SESSION");
        if (!windowsTerminalSession.isEmpty()) {
            features |= CanNarrowScrollRegion;
            features |= CanGetConsoleScreenBufferInfo;
        }
#else
        features |= CanIoctlTIOCGWINSZ;

        const QString term = qEnvironmentVariable("TERM");
        if (term == "vt100"
            || term.startsWith("linux")
            || term == "mintty"
            || term == "rxvt"
            || term.startsWith("screen")
            || term.startsWith("tmux")
            || term.startsWith("xterm")) {
            features |= CanNarrowScrollRegion;
        }
#endif

        qCDebug(sfdk) << "TerminalInfo::features:" << features;

        return features;
    }();

    return features;
}

/*!
 * \class AnsiIO
 */

class AnsiIO
{
public:
    virtual ~AnsiIO() {}
    virtual QTextStream &out() = 0;
    virtual bool getChar(char *c) = 0;
};

/*!
 * \class AnsiIOUnix
 */

class AnsiIOUnix : public AnsiIO
{
public:
    ~AnsiIOUnix() override
    {
        if (m_restoreInput)
            m_restoreInput();
    }

    QTextStream &out() override { return qout(); }

    bool getChar(char *c) override
    {
        if (!m_in) {
            if (!initInput()) {
                static bool once = true;
                if (once) {
                    qCDebug(sfdk) << "Failed to set terminal to non-canonical mode";
                    once = false;
                }
                return false;
            }
        }

        return m_in->getChar(c);
    }

private:
    bool initInput()
    {
        QTC_ASSERT(!m_restoreInput, return true);

#if defined(Q_OS_UNIX)
        struct termios originalAttributes;
        if (tcgetattr(STDIN_FILENO, &originalAttributes) != 0)
            return false;
        struct termios newAttributes = originalAttributes;
        newAttributes.c_lflag &= ~(ICANON | ECHO);
        newAttributes.c_cc[VMIN] = 1;
        newAttributes.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &newAttributes) != 0)
            return false;

        m_restoreInput = [=]() {
            tcsetattr(STDIN_FILENO, TCSANOW, &originalAttributes);
        };
#endif

        m_in = std::make_unique<QFile>();
        if (!m_in->open(stdin, QIODevice::ReadOnly)) {
            m_in.reset();
            return false;
        }

        return true;
    }

private:
    std::unique_ptr<QFile> m_in;
    std::function<void()> m_restoreInput;
};

/*!
 * \class AnsiIOWindows
 */

class AnsiIOWindows : public AnsiIO
{
public:
    AnsiIOWindows()
        : m_outFile(binaryOut(stdout))
        , m_outStream(m_outFile.get())
    {
        if (!initOutput()) {
            static bool once = true;
            if (once) {
                qCDebug(sfdk) << "Failed to enable VT processing";
                once = false;
            }
        }
    }

    ~AnsiIOWindows() override
    {
        if (m_restoreInput)
            m_restoreInput();
    }

public:
    QTextStream &out() override { return m_outStream; }

    bool getChar(char *c) override
    {
        if (!m_getChar) {
            if (!initInput()) {
                static bool once = true;
                if (once) {
                    qCDebug(sfdk) << "Failed to set input console mode";
                    once = false;
                }

                return false;
            }
        }

        return m_getChar(c);
    }

private:
    // The effect of this, though not explicitly reverted by this class, is not
    // necessarily persistent. A child process may restore/override terminal
    // settings at will. Hence the repeated invocation in constructor.
    bool initOutput()
    {
#if !defined(Q_OS_WIN)
        QTC_ASSERT(false, return false);
#else
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
            return false;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode))
            return false;

        dwMode |= ENABLE_PROCESSED_OUTPUT;
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(hOut, dwMode))
            return false;

        return true;
#endif
    }

    bool initInput()
    {
#if !defined(Q_OS_WIN)
        QTC_ASSERT(false, return false);
#else
        // This is rather a dead code left for reference.
        //
        // 1) Under Windows Terminal it is not needed as the native calls like
        // GetConsoleScreenBufferInfo can be used there.
        //
        // 2) Under MSYS/Mintty it is unusable as it communicates with native
        // Windows (MSVC) binaries via Windows Anonymous Pipes to emulate a UNIX
        // pseudo-terminal and compared to the output path, which works
        // reasonably well, there does not seem to be a way to persuade it to
        // pass the terminal reply through to be succesfully read here.
        // The only working way I found utilized some ntdll black magic combined
        // with `stty` invocation in a subprocess to switch the terminal to raw
        // mode. Something to be included only if really needed.

        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn == INVALID_HANDLE_VALUE)
            return false;

        DWORD oldMode;
        if (!GetConsoleMode(hIn, &oldMode))
            return false;

        // Disable line input and echo so we can grab raw data instantly
        DWORD rawMode = oldMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        if (!SetConsoleMode(hIn, rawMode))
            return false;

        m_getChar = [=](char *c) {
            DWORD read;
            return ReadFile(hIn, c, 1, &read, NULL) && read > 0;
        };

        m_restoreInput = [=]() {
            if (!SetConsoleMode(hIn, oldMode)) {
                static bool once = true;
                if (once) {
                    qCDebug(sfdk) << "Failed to restore input console mode.";
                    once = false;
                }
            }
        };

        return true;
#endif
    }

private:
    std::unique_ptr<QFile> m_outFile;
    QTextStream m_outStream;
    std::function<bool(char *)> m_getChar;
    std::function<void()> m_restoreInput;
};

/*
 * \class AnsiControl
 */

class AnsiControl
{
public:
    AnsiControl()
    {
        if (HostOsInfo::isWindowsHost())
            m_io = std::make_unique<AnsiIOWindows>();
        else
            m_io = std::make_unique<AnsiIOUnix>();

    }

    void flush()
    {
        m_io->out().flush();
    }

    void setScrollRegion(int topLine, int bottomLine)
    {
        m_io->out() << "\x1b[" << topLine << ';' << bottomLine << 'r';
    }

    void unsetScrollRegion()
    {
        m_io->out() << "\x1b[r";
    }

    void saveCursorPosition()
    {
        m_io->out() << "\x1b[s";
    }

    void restoreCursorPosition()
    {
        m_io->out() << "\x1b[u";
    }

    void moveCursor(int row, int column = -1)
    {
        m_io->out() << "\x1b[" << row;
        if (column > 0)
            m_io->out() << ';' << column;
        m_io->out() << 'H';
    }

    void askCursorPosition()
    {
        m_io->out() << "\x1b[6n";
    }

    bool readCursorPosition(int *row, int *column)
    {
        QByteArray buf;
        char c;
        do {
            if (!m_io->getChar(&c))
                return false;
            buf.append(c);
        } while (c != 'R');

        buf.append('\0');

        return sscanf(buf.constData(), "\x1b[%d;%dR", row, column) == 2;
    }

    void askTextAreaSize()
    {
        m_io->out() << "\x1b[18t";
    }

    bool readTextAreaSize(int *rows, int *columns)
    {
        QByteArray buf;
        char c;
        do {
            if (!m_io->getChar(&c))
                return false;
            buf.append(c);
        } while (c != 't');

        buf.append('\0');

        return sscanf(buf.constData(), "\x1b[%d;%dt", rows, columns) == 2;
    }

private:
    std::unique_ptr<AnsiIO> m_io;
};

/*!
 * \class TerminalManager
 */

TerminalManager *TerminalManager::s_instance = nullptr;

TerminalManager::TerminalManager()
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

TerminalManager::~TerminalManager()
{
    s_instance = nullptr;
}

void TerminalManager::narrowScrollRegionRequest()
{
    if (!Pager::isEnabled() || !TerminalInfo::canNarrowScrollRegion())
        return;

    if (++s_instance->m_narrowScrollRegionReqCount == 1)
        s_instance->setScrollRegion();
}

void TerminalManager::narrowScrollRegionRelease()
{
    if (!Pager::isEnabled() || !TerminalInfo::canNarrowScrollRegion())
        return;

    QTC_CHECK(s_instance->m_narrowScrollRegionReqCount > 0);
    if (--s_instance->m_narrowScrollRegionReqCount == 0)
        s_instance->resetScrollRegion();
}

void TerminalManager::maybeSuspendScrollRegion(bool suspend)
{
    if (!Pager::isEnabled() || !TerminalInfo::canNarrowScrollRegion())
        return;
    if (s_instance->m_narrowScrollRegionReqCount <= 0)
        return;

    if (suspend)
        s_instance->resetScrollRegion();
    else
        s_instance->setScrollRegion();
}

void TerminalManager::handleResize()
{
    if (!Pager::isEnabled() || !TerminalInfo::canNarrowScrollRegion())
        return;

    if (s_instance->m_narrowScrollRegionReqCount > 0)
        s_instance->setScrollRegion(true /* update */);
}

void TerminalManager::setScrollRegion(bool update)
{
    AnsiControl ansi;

    // Reserve some vertical space to not overwrite the previous content
    if (!update)
        qout() << QString(SCROLL_BOX_HEIGHT, '\n') << flush;

    const int termLines = getLinesCount(ansi).value_or(-1);
    if (termLines < 0)
        return;

    // Should it fail, resort to stick to the bottom of the window.
    const int currentRow = getCurrentRow(ansi).value_or(termLines);

    // Ideally this should start at 0 and grow up to SCROLL_BOX_HEIGHT as more
    // content is written to the output, but that seems rather difficult to do,
    // as single logical line of text may not fit in single visual line.
    const int currentRegionRow = SCROLL_BOX_HEIGHT;

    const int topLine = currentRow - currentRegionRow;
    const int bottomLine = topLine + SCROLL_BOX_HEIGHT;

    ansi.setScrollRegion(topLine, bottomLine);

    if (!update)
        ansi.moveCursor(topLine);
    else
        ansi.moveCursor(currentRow);

    ansi.flush();
}

void TerminalManager::resetScrollRegion()
{
    AnsiControl ansi;

    ansi.saveCursorPosition();
    ansi.unsetScrollRegion();
    ansi.restoreCursorPosition();
    ansi.flush();

    // Print newline to clear the shell prompt
    qout() << endl;
}

// Doing this with ANSI codes would be unnecessarily complicated on Windows (not
// possible to simply read the response with standard library IO as on Linux)
// and also less reliable on Linux during window resizing.
std::optional<int> TerminalManager::getLinesCount(AnsiControl &ansi)
{
#if defined(Q_OS_WIN)
    if (TerminalInfo::canGetConsoleScreenBufferInfo()) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
            static bool once = true;
            if (once) {
                qCWarning(sfdk) << "Failed to determine terminal window size";
                once = false;
            }
            return {};
        }

        return csbi.dwSize.Y;
    }
#else
    if (TerminalInfo::canIoctlTIOCGWINSZ()) {
        struct winsize w;

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != 0) {
            static bool once = true;
            if (once) {
                qCWarning(sfdk) << "Failed to determine terminal window size";
                once = false;
            }
            return {};
        }

        return w.ws_row;
    }
#endif

    ansi.askTextAreaSize();
    ansi.flush();

    int rows = 0;
    int columns = 0;
    bool ok = ansi.readTextAreaSize(&rows, &columns);

    if (!ok) {
        static bool once = true;
        if (once) {
            qCDebug(sfdk) << "Failed to determine lines count";
            once = false;
        }
        return {};
    }

    return rows;
}

std::optional<int> TerminalManager::getCurrentRow(AnsiControl &ansi)
{
#if defined(Q_OS_WIN)
    if (TerminalInfo::canGetConsoleScreenBufferInfo()) {
        Q_UNUSED(ansi);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
            static bool once = true;
            if (once) {
                qCWarning(sfdk) << "Failed to determine cursor position";
                once = false;
            }
            return {};
        }

        // Add one to align with 1-based indexing with ANSI
        return csbi.dwCursorPosition.Y + 1;
    }
#endif

    ansi.askCursorPosition();
    ansi.flush();

    int row = 0;
    int column = 0;
    bool ok = ansi.readCursorPosition(&row, &column);

    if (!ok) {
        static bool once = true;
        if (once) {
            qCDebug(sfdk) << "Failed to determine cursor position";
            once = false;
        }
        return {};
    }

    return row;
}

/*!
 * \class Pager
 */

bool Pager::s_enabled = isConnectedToTerminal();

Pager::Pager()
    : m_stream(stdout)
{
    if (!s_enabled)
        return;

    m_pager.setProgram(PAGER);
    m_pager.setArguments(PAGER_ARGUMENTS);
    m_pager.setProcessChannelMode(QProcess::ForwardedChannels);
    m_pager.start(); // Using WriteOnly open mode would break it on Windows
    if (m_pager.waitForStarted())
        m_stream.setDevice(&m_pager.asQProcess());
    else
        qCDebug(sfdk) << "Pager not available";
}

Pager::~Pager()
{
    if (m_pager.state() != QProcess::NotRunning) {
        m_pager.closeWriteChannel();
        m_pager.waitForFinishedNoBlock(-1);
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
    bool literal = false;
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
                literal = verse = false;
            }
        } else if (line == "+") {
            expectOpenBlock = true;
        } else if (line.contains(attributesRe, &match)) {
            literal = match.captured(1) == "literal";
            verse = match.captured(1) == "verse";
            if (!literal && !verse)
                qCWarning(sfdk) << "Unrecognized AsciiDoc attribute" << match.captured(1);
            expectOpenBlock = true;
        } else {
            if (line.endsWith("::"))
                line.chop(2);

            if (line.contains(boldLeadingWordsRe, &match))
                line = match.capturedTexts().mid(1).join(QString());

            expectOpenBlock = false;

            if (literal || verse || line.startsWith(' ') || line.startsWith('\n')) {
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

QString indentLines(int level, const QString &lines)
{
    return indent(level) + lines.split('\n').join('\n' + indent(level));
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

bool endsWithAnsiEscapeCode(const QByteArray &data, int length)
{
    int i = length < 0
        ? data.length()
        : qMin(data.length(), length);

    if (--i < 0)
        return false;
    if (data.at(i) != 'm' && data.at(i) != 'K')
        return false;

    for (;;) {
        if (--i < 0)
            return false;
        if (data.at(i) >= '0' && data.at(i) <= '9')
            continue;
        if (data.at(i) == ';')
            continue;
        break;
    }

    if (data.at(i) != '[')
        return false;

    if (--i < 0)
        return false;
    if (data.at(i) != '\x1b')
        return false;

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

#include "textutils.moc"
