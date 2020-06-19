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

#include <QByteArray>
#include <QVector>
#include <QString>

namespace CPlusPlus {

class Environment;

class CPLUSPLUS_EXPORT Macro
{
    typedef Internal::PPToken PPToken;

public:
    Macro();

    QByteArray name() const
    { return _name; }

    QString nameToQString() const
    { return QString::fromUtf8(_name, _name.size()); }

    void setName(const QByteArray &name)
    { _name = name; }

    const QByteArray definitionText() const
    { return _definitionText; }

    const QVector<PPToken> &definitionTokens() const
    { return _definitionTokens; }

    void setDefinition(const QByteArray &definitionText, const QVector<PPToken> &definitionTokens)
    { _definitionText = definitionText; _definitionTokens = definitionTokens; }

    const QVector<QByteArray> &formals() const
    { return _formals; }

    void addFormal(const QByteArray &formal)
    { _formals.append(formal); }

    QString fileName() const
    { return _fileName; }

    void setFileName(const QString &fileName)
    { _fileName = fileName; }

    unsigned fileRevision() const
    { return _fileRevision; }

    void setFileRevision(unsigned fileRevision)
    { _fileRevision = fileRevision; }

    int line() const
    { return _line; }

    void setLine(int line)
    { _line = line; }

    unsigned bytesOffset() const
    { return _bytesOffset; }

    void setBytesOffset(unsigned bytesOffset)
    { _bytesOffset = bytesOffset; }

    unsigned utf16CharOffset() const
    { return _utf16charsOffset; }

    void setUtf16charOffset(unsigned utf16charOffset)
    { _utf16charsOffset = utf16charOffset; }

    unsigned length() const
    { return _length; }

    void setLength(unsigned length)
    { _length = length; }

    bool isHidden() const
    { return f._hidden; }

    void setHidden(bool isHidden)
    { f._hidden = isHidden; }

    bool isFunctionLike() const
    { return f._functionLike; }

    void setFunctionLike(bool isFunctionLike)
    { f._functionLike = isFunctionLike; }

    bool isVariadic() const
    { return f._variadic; }

    void setVariadic(bool isVariadic)
    { f._variadic = isVariadic; }

    QString toString() const;
    QString toStringWithLineBreaks() const;

private:
    friend class Environment;
    Macro *_next;

    QString decoratedName() const;

    struct Flags
    {
        unsigned _hidden: 1;
        unsigned _functionLike: 1;
        unsigned _variadic: 1;
    };

    QByteArray _name;
    QByteArray _definitionText;
    QVector<PPToken> _definitionTokens;
    QVector<QByteArray> _formals;
    QString _fileName;
    unsigned _hashcode;
    unsigned _fileRevision;
    int _line;
    unsigned _bytesOffset;
    unsigned _utf16charsOffset;
    unsigned _length;

    union
    {
        unsigned _state;
        Flags f;
    };
};

} // namespace CPlusPlus
