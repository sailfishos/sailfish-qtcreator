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

/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "PPToken.h"

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <QList>
#include <QByteArray>
#include <QString>

namespace CPlusPlus {

class Macro;

class CPLUSPLUS_EXPORT Environment
{
public:
    typedef Macro **iterator;

public:
    Environment();
    ~Environment();

    unsigned macroCount() const;
    Macro *macroAt(unsigned index) const;

    Macro *bind(const Macro &macro);
    Macro *remove(const ByteArrayRef &name);
    Macro *resolve(const ByteArrayRef &name) const;

    iterator firstMacro() const;
    iterator lastMacro() const;

    void reset();
    void addMacros(const QList<Macro> &macros);

    static bool isBuiltinMacro(const ByteArrayRef &name);
    void dump() const;

private:
    void rehash();

public:
    QString currentFile;
    QByteArray currentFileUtf8;
    int currentLine;
    bool hideNext;

private:
    Macro **_macros;
    int _allocated_macros;
    int _macro_count;
    Macro **_hash;
    int _hash_count;
};

} // namespace CPlusPlus
