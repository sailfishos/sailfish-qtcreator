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

#include <QByteArray>
#include <QList>
#include <QString>
#include <QJsonValue>
#include <QJsonObject>
#include <QVector>

#include <utils/fileutils.h>

namespace Utils { class ProcessHandle; }

namespace Debugger {
namespace Internal {

class DebuggerResponse;

// Convenience structure to build up backend commands.
class DebuggerCommand
{
public:
    using Callback = std::function<void (const DebuggerResponse &)>;

    DebuggerCommand() = default;
    DebuggerCommand(const QString &f) : function(f) {}
    DebuggerCommand(const QString &f, const QJsonValue &a) : function(f), args(a) {}
    DebuggerCommand(const QString &f, int fl) : function(f), flags(fl) {}
    DebuggerCommand(const QString &f, int fl, const Callback &cb) : function(f), callback(cb), flags(fl) {}
    DebuggerCommand(const QString &f, const Callback &cb) : function(f), callback(cb) {}

    void arg(const char *value);
    void arg(const char *name, bool value);
    void arg(const char *name, int value);
    void arg(const char *name, qlonglong value);
    void arg(const char *name, qulonglong value);
    void arg(const char *name, const QString &value);
    void arg(const char *name, const char *value);
    void arg(const char *name, const QList<int> &list);
    void arg(const char *name, const QStringList &list); // Note: Hex-encodes.
    void arg(const char *name, const QJsonValue &value);
    void arg(const char *name, const Utils::FilePath &filePath);

    QString argsToPython() const;
    QString argsToString() const;

    enum CommandFlag {
        NoFlags = 0,
        // The command needs a stopped inferior.
        NeedsTemporaryStop = 1,
        // No need to wait for the reply before continuing inferior.
        Discardable = 2,
        // Needs a dummy extra command to force GDB output flushing.
        NeedsFlush = 4,
        // The command needs a stopped inferior and will stay stopped afterward.
        NeedsFullStop = 8,
        // Callback expects ResultRunning instead of ResultDone.
        RunRequest = 16,
        // Callback expects ResultExit instead of ResultDone.
        ExitRequest = 32,
        // Auto-set inferior shutdown related states.
        LosesChild = 64,
        // This is a native (non-Python) command that's handled directly by the backend.
        NativeCommand = 256,
        // This is a command that needs to be wrapped into -interpreter-exec console
        ConsoleCommand = 512,
        // This is the UpdateLocals commannd during which we ignore notifications
        InUpdateLocals = 1024,
        // Do not echo to log.
        Silent = 4096
    };

    Q_DECLARE_FLAGS(CommandFlags, CommandFlag)

    QString function;
    QJsonValue args;
    Callback callback;
    uint postTime = 0; // msecsSinceStartOfDay
    int flags = 0;

private:
    void argHelper(const char *name, const QByteArray &value);
};

class DebuggerCommandSequence
{
public:
    DebuggerCommandSequence() = default;
    bool isEmpty() const { return m_commands.isEmpty(); }
    bool wantContinue() const { return m_continue; }
    const QList<DebuggerCommand> &commands() const { return m_commands; }
    void append(const DebuggerCommand &cmd, bool wantContinue) {
        m_commands.append(cmd);
        m_continue = wantContinue;
    }

public:
    QList<DebuggerCommand> m_commands;
    bool m_continue = false;
};

class DebuggerOutputParser
{
public:
    explicit DebuggerOutputParser(const QString &output);

    QChar current() const { return *from; }
    bool isCurrent(QChar c) const { return *from == c; }
    bool isAtEnd() const { return from >= to; }

    void advance() { ++from; }
    void advance(int n) { from += n; }
    QChar lookAhead(int offset) const { return from[offset]; }

    int readInt();
    QChar readChar();
    QString readCString();
    QString readString(const std::function<bool(char)> &isValidChar);

    QString buffer() const { return QString(from, to - from); }
    int remainingChars() const { return int(to - from); }

    void skipCommas();
    void skipSpaces();

private:
    const QChar *from = nullptr;
    const QChar *to = nullptr;
};

class GdbMi
{
public:
    GdbMi() = default;

    QString m_name;
    QString m_data;

    using Children = QVector<GdbMi>;
    enum Type { Invalid, Const, Tuple, List };
    Type m_type = Invalid;

    void addChild(const GdbMi &child) { m_children.push_back(child); }

    Type type() const { return m_type; }
    const QString &name() const { return m_name; }
    bool hasName(const QString &name) const { return m_name == name; }

    bool isValid() const { return m_type != Invalid; }
    bool isList() const { return m_type == List; }

    const QString &data() const { return m_data; }
    Children::const_iterator begin() const { return m_children.begin(); }
    Children::const_iterator end() const { return m_children.end(); }
    int childCount() const { return int(m_children.size()); }

    const GdbMi &childAt(int index) const { return m_children[index]; }
    const GdbMi &operator[](const char *name) const;

    QString toString(bool multiline = false, int indent = 0) const;
    qulonglong toAddress() const;
    Utils::ProcessHandle toProcessHandle() const;
    int toInt() const { return m_data.toInt(); }
    qint64 toLongLong() const { return m_data.toLongLong(); }
    void fromString(const QString &str);
    void fromStringMultiple(const QString &str);

    static QString escapeCString(const QString &ba);
    void parseResultOrValue(DebuggerOutputParser &state);
    void parseValue(DebuggerOutputParser &state);
    void parseTuple(DebuggerOutputParser &state);
    void parseTuple_helper(DebuggerOutputParser &state);
    void parseList(DebuggerOutputParser &state);

private:
    void dumpChildren(QString *str, bool multiline, int indent) const;
    Children m_children;
};

QString fromHex(const QString &str);
QString toHex(const QString &str);


enum ResultClass
{
    // "done" | "running" | "connected" | "error" | "exit"
    ResultUnknown,
    ResultDone,
    ResultRunning,
    ResultConnected,
    ResultError,
    ResultExit
};

class DebuggerResponse
{
public:
    DebuggerResponse() = default;
    QString toString() const;
    static QString stringFromResultClass(ResultClass resultClass);

    int         token = -1;
    ResultClass resultClass = ResultUnknown;
    GdbMi       data;
    QString     logStreamOutput;
    QString     consoleStreamOutput;
};

void extractGdbVersion(const QString &msg,
    int *gdbVersion, int *gdbBuildVersion, bool *isMacGdb, bool *isQnxGdb);


class DebuggerEncoding
{
public:
    enum EncodingType {
        Unencoded,
        HexEncodedLocal8Bit,
        HexEncodedLatin1,
        HexEncodedUtf8,
        HexEncodedUtf16,
        HexEncodedUcs4,
        HexEncodedSignedInteger,
        HexEncodedUnsignedInteger,
        HexEncodedFloat,
        JulianDate,
        MillisecondsSinceMidnight,
        JulianDateAndMillisecondsSinceMidnight,
        IPv6AddressAndHexScopeId,
        DateTimeInternal,
    };

    DebuggerEncoding() = default;
    explicit DebuggerEncoding(const QString &data);
    QString toString() const;

    EncodingType type = Unencoded;
    int size = 0;
    bool quotes = false;
};

// Decode string data as returned by the dumper helpers.
QString decodeData(const QString &baIn, const QString &encoding);


// These enum values correspond to possible value display format requests,
// typically selected by the user using the L&E context menu, under
// "Change Value Display Format". They are passed from the frontend to
// the dumpers.
//
// Keep in sync with dumper.py.
//
// \note Add new enum values only at the end, as the numeric values are
// persisted in user settings.

enum DisplayFormat
{
    AutomaticFormat             =  0, // Based on type for individuals, dumper default for types.
                                      // Could be anything reasonably cheap.
    RawFormat                   =  1, // No formatting at all.

    SimpleFormat                =  2, // Typical simple format (e.g. for QModelIndex row/column)
    EnhancedFormat              =  3, // Enhanced format (e.g. for QModelIndex with resolved display)
    SeparateFormat              =  4, // Display in separate Window

    Latin1StringFormat          =  5,
    SeparateLatin1StringFormat  =  6,
    Utf8StringFormat            =  7,
    SeparateUtf8StringFormat    =  8,
    Local8BitStringFormat       =  9,
    Utf16StringFormat           = 10,
    Ucs4StringFormat            = 11,

    Array10Format               = 12,
    Array100Format              = 13,
    Array1000Format             = 14,
    Array10000Format            = 15,
    ArrayPlotFormat             = 16,

    CompactMapFormat            = 17,
    DirectQListStorageFormat    = 18,
    IndirectQListStorageFormat  = 19,

    BoolTextFormat              = 20, // Bools as "true" or "false" - Frontend internal only.
    BoolIntegerFormat           = 21, // Bools as "0" or "1" - Frontend internal only

    DecimalIntegerFormat        = 22, // Frontend internal only
    HexadecimalIntegerFormat    = 23, // Frontend internal only
    BinaryIntegerFormat         = 24, // Frontend internal only
    OctalIntegerFormat          = 25, // Frontend internal only

    CompactFloatFormat          = 26, // Frontend internal only
    ScientificFloatFormat       = 27  // Frontend internal only
};


// These values are passed from the dumper to the frontend,
// typically as a result of passing a related DisplayFormat value.
// They are never stored in settings.

const char DisplayLatin1String[] = "latin1:separate";
const char DisplayUtf8String[]   = "utf8:separate";
const char DisplayUtf16String[]  = "utf16:separate";
const char DisplayUcs4String[]   = "ucs4:separate";
const char DisplayImageData[]    = "imagedata:separate";
const char DisplayImageFile[]    = "imagefile:separate";
const char DisplayPlotData[]     = "plotdata:separate";
const char DisplayArrayData[]    = "arraydata:separate";

enum LocationType { UnknownLocation, LocationByFile, LocationByAddress };

class ContextData
{
public:
    bool isValid() const { return type != UnknownLocation; }

public:
    LocationType type = UnknownLocation;
    Utils::FilePath fileName;
    int lineNumber = 0;
    quint64 address = 0;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_OPERATORS_FOR_FLAGS(Debugger::Internal::DebuggerCommand::CommandFlags)
