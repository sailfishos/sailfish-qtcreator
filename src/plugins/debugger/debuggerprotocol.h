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

#include <functional>

namespace Debugger {
namespace Internal {

class DebuggerResponse;

// Convenience structure to build up backend commands.
class DebuggerCommand
{
public:
    typedef std::function<void(const DebuggerResponse &)> Callback;

    DebuggerCommand() {}
    DebuggerCommand(const QString &f) : function(f), flags(0) {}
    DebuggerCommand(const QString &f, const QJsonValue &a) : function(f), args(a), flags(0) {}
    DebuggerCommand(const QString &f, int fl) : function(f), flags(fl) {}
    DebuggerCommand(const QString &f, int fl, const Callback &cb) : function(f), callback(cb), flags(fl) {}

    void arg(const char *value);
    void arg(const char *name, bool value);
    void arg(const char *name, int value);
    void arg(const char *name, qlonglong value);
    void arg(const char *name, qulonglong value);
    void arg(const char *name, const QString &value);
    void arg(const char *name, const char *value);
    void arg(const char *name, const QList<int> &list);
    void arg(const char *name, const QJsonValue &value);

    QString argsToPython() const;
    QString argsToString() const;

    QString function;
    QJsonValue args;
    Callback callback;
    uint postTime; // msecsSinceStartOfDay
    int flags;

private:
    void argHelper(const char *name, const QByteArray &value);
};

// FIXME: rename into GdbMiValue
class GdbMi
{
public:
    GdbMi() : m_type(Invalid) {}

    QString m_name;
    QString m_data;
    QVector<GdbMi> m_children;

    enum Type { Invalid, Const, Tuple, List };

    Type m_type;

    Type type() const { return m_type; }
    const QString &name() const { return m_name; }
    bool hasName(const QString &name) const { return m_name == name; }

    bool isValid() const { return m_type != Invalid; }
    bool isList() const { return m_type == List; }

    const QString &data() const { return m_data; }
    const QVector<GdbMi> &children() const { return m_children; }
    int childCount() const { return int(m_children.size()); }

    const GdbMi &childAt(int index) const { return m_children[index]; }
    const GdbMi &operator[](const char *name) const;

    QString toString(bool multiline = false, int indent = 0) const;
    qulonglong toAddress() const;
    int toInt() const { return m_data.toInt(); }
    void fromString(const QString &str);
    void fromStringMultiple(const QString &str);

    static QString parseCString(const QChar *&from, const QChar *to);
    static QString escapeCString(const QString &ba);
    void parseResultOrValue(const QChar *&from, const QChar *to);
    void parseValue(const QChar *&from, const QChar *to);
    void parseTuple(const QChar *&from, const QChar *to);
    void parseTuple_helper(const QChar *&from, const QChar *to);
    void parseList(const QChar *&from, const QChar *to);

private:
    void dumpChildren(QString *str, bool multiline, int indent) const;
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
    DebuggerResponse() : token(-1), resultClass(ResultUnknown) {}
    QString toString() const;
    static QString stringFromResultClass(ResultClass resultClass);

    int         token;
    ResultClass resultClass;
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

    DebuggerEncoding() {}
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

} // namespace Internal
} // namespace Debugger
