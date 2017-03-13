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

#include "codecompletionchunk.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

CodeCompletionChunk::CodeCompletionChunk(CodeCompletionChunk::Kind kind,
                                         const Utf8String &text,
                                         bool isOptional)
    : text_(text),
      kind_(kind),
      isOptional_(isOptional)
{
}

CodeCompletionChunk::Kind CodeCompletionChunk::kind() const
{
    return kind_;
}

const Utf8String &CodeCompletionChunk::text() const
{
    return text_;
}

bool CodeCompletionChunk::isOptional() const
{
    return isOptional_;
}

QDataStream &operator<<(QDataStream &out, const CodeCompletionChunk &chunk)
{
    out << static_cast<quint8>(chunk.kind_);
    out << chunk.text_;
    out << chunk.isOptional_;

    return out;
}

QDataStream &operator>>(QDataStream &in, CodeCompletionChunk &chunk)
{
    quint8 kind;

    in >> kind;
    in >> chunk.text_;
    in >> chunk.isOptional_;

    chunk.kind_ = static_cast<CodeCompletionChunk::Kind>(kind);

    return in;
}

bool operator==(const CodeCompletionChunk &first, const CodeCompletionChunk &second)
{
    return first.kind() == second.kind()
            && first.text() == second.text()
            && first.isOptional() == second.isOptional();
}

static const char *completionChunkKindToString(CodeCompletionChunk::Kind kind)
{
    switch (kind) {
       case CodeCompletionChunk::Optional: return "Optional";
       case CodeCompletionChunk::TypedText: return "TypedText";
       case CodeCompletionChunk::Text: return "Text";
       case CodeCompletionChunk::Placeholder: return "Placeholder";
       case CodeCompletionChunk::Informative: return "Informative";
       case CodeCompletionChunk::CurrentParameter: return "CurrentParameter";
       case CodeCompletionChunk::LeftParen: return "LeftParen";
       case CodeCompletionChunk::RightParen: return "RightParen";
       case CodeCompletionChunk::LeftBracket: return "LeftBracket";
       case CodeCompletionChunk::RightBracket: return "RightBracket";
       case CodeCompletionChunk::LeftBrace: return "LeftBrace";
       case CodeCompletionChunk::RightBrace: return "RightBrace";
       case CodeCompletionChunk::LeftAngle: return "LeftAngle";
       case CodeCompletionChunk::RightAngle: return "RightAngle";
       case CodeCompletionChunk::Comma: return "Comma";
       case CodeCompletionChunk::ResultType: return "ResultType";
       case CodeCompletionChunk::Colon: return "Colon";
       case CodeCompletionChunk::SemiColon: return "SemiColon";
       case CodeCompletionChunk::Equal: return "Equal";
       case CodeCompletionChunk::HorizontalSpace: return "HorizontalSpace";
       case CodeCompletionChunk::VerticalSpace: return "VerticalSpace";
       case CodeCompletionChunk::Invalid: return "Invalid";
    }

    return nullptr;
}

QDebug operator<<(QDebug debug, const CodeCompletionChunk &chunk)
{
    debug.nospace() << "CodeCompletionChunk(";
    debug.nospace() << completionChunkKindToString(chunk.kind()) << ", ";
    debug.nospace() << chunk.text();

    if (chunk.isOptional())
        debug.nospace() << ", optional";

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const CodeCompletionChunk &chunk, ::std::ostream* os)
{
    *os << "{";
    *os << completionChunkKindToString(chunk.kind()) << ", ";
    *os << chunk.text().constData();

    if (chunk.isOptional())
        *os << ", optional";

    *os << "}";
}

void PrintTo(const CodeCompletionChunk::Kind &kind, ::std::ostream* os)
{
    *os << completionChunkKindToString(kind);
}

} // namespace ClangBackEnd

