############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

# Debugger start modes. Keep in sync with DebuggerStartMode in debuggerconstants.h


class DebuggerStartMode():
    (
        NoStartMode,
        StartInternal,
        StartExternal,
        AttachExternal,
        AttachCrashedExternal,
        AttachCore,
        AttachToRemoteServer,
        AttachToRemoteProcess,
        StartRemoteProcess,
    ) = range(0, 9)


# Known special formats. Keep in sync with DisplayFormat in debuggerprotocol.h
class DisplayFormat():
    (
        Automatic,
        Raw,
        Simple,
        Enhanced,
        Separate,
        Latin1String,
        SeparateLatin1String,
        Utf8String,
        SeparateUtf8String,
        Local8BitString,
        Utf16String,
        Ucs4String,
        Array10,
        Array100,
        Array1000,
        Array10000,
        ArrayPlot,
        CompactMap,
        DirectQListStorage,
        IndirectQListStorage,
    ) = range(0, 20)


# Breakpoints. Keep synchronized with BreakpointType in breakpoint.h
class BreakpointType():
    (
        UnknownType,
        BreakpointByFileAndLine,
        BreakpointByFunction,
        BreakpointByAddress,
        BreakpointAtThrow,
        BreakpointAtCatch,
        BreakpointAtMain,
        BreakpointAtFork,
        BreakpointAtExec,
        BreakpointAtSysCall,
        WatchpointAtAddress,
        WatchpointAtExpression,
        BreakpointOnQmlSignalEmit,
        BreakpointAtJavaScriptThrow,
    ) = range(0, 14)


# Internal codes for types keep in sync with cdbextensions pytype.cpp
class TypeCode():
    (
        Typedef,
        Struct,
        Void,
        Integral,
        Float,
        Enum,
        Pointer,
        Array,
        Complex,
        Reference,
        Function,
        MemberPointer,
        FortranString,
        Unresolvable,
        Bitfield,
        RValueReference,
    ) = range(0, 16)


def isIntegralTypeName(name):
    return name in (
        "int",
        "unsigned int",
        "signed int",
        "short",
        "unsigned short",
        "long",
        "unsigned long",
        "long long",
        "unsigned long long",
        "char",
        "signed char",
        "unsigned char",
        "bool",
    )


def isFloatingPointTypeName(name):
    return name in ("float", "double", "long double")
