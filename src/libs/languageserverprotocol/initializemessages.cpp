/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "initializemessages.h"

#include <QCoreApplication>

namespace LanguageServerProtocol {

constexpr const char InitializeRequest::methodName[];
constexpr const char InitializeNotification::methodName[];
constexpr Trace::Values s_trace = Trace::off;

Trace Trace::fromString(const QString &val)
{
    if (val == "messages")
        return messages;
    if (val == "verbose")
        return verbose;
    return off;
}

#define RETURN_CASE(name) case Trace::name: return QString(#name);
QString Trace::toString() const
{
    switch (m_value) {
    RETURN_CASE(off);
    RETURN_CASE(messages);
    RETURN_CASE(verbose);
    }
    return QString("off");
}
#undef RETURN_CASE

TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemCapbilities::CompletionItemCapbilities()
{
}

Utils::optional<QList<MarkupKind>>
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemCapbilities::
documentationFormat() const
{
    Utils::optional<QList<int>> array = optionalArray<int>(documentationFormatKey);
    if (!array)
        return Utils::nullopt;
    return Utils::make_optional(Utils::transform(array.value(), [] (int value) {
        return static_cast<MarkupKind>(value);
    }));
}

void
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemCapbilities::
setDocumentationFormat(const QList<MarkupKind> &documentationFormat)
{
    insert(documentationFormatKey, enumArrayToJsonArray<MarkupKind>(documentationFormat));
}

TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities::CompletionItemKindCapabilities()
{
    setValueSet({CompletionItemKind::Text, CompletionItemKind::Method, CompletionItemKind::Function,
                 CompletionItemKind::Constructor, CompletionItemKind::Field, CompletionItemKind::Variable,
                 CompletionItemKind::Class, CompletionItemKind::Interface, CompletionItemKind::Module,
                 CompletionItemKind::Property, CompletionItemKind::Unit, CompletionItemKind::Value,
                 CompletionItemKind::Enum, CompletionItemKind::Keyword, CompletionItemKind::Snippet,
                 CompletionItemKind::Color, CompletionItemKind::File, CompletionItemKind::Reference,
                 CompletionItemKind::Folder, CompletionItemKind::EnumMember, CompletionItemKind::Constant,
                 CompletionItemKind::Struct, CompletionItemKind::Event, CompletionItemKind::Operator,
                 CompletionItemKind::TypeParameter});
}

Utils::optional<QList<CompletionItemKind::Kind>>
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities::
valueSet() const
{
    Utils::optional<QList<int>> array = optionalArray<int>(valueSetKey);
    if (!array)
        return Utils::nullopt;
    return Utils::make_optional(Utils::transform(array.value(), [] (int value) {
        return static_cast<CompletionItemKind::Kind>(value);
    }));
}

void
TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities::
setValueSet(const QList<CompletionItemKind::Kind> &valueSet)
{
    insert(valueSetKey, enumArrayToJsonArray<CompletionItemKind::Kind>(valueSet));
}

Utils::optional<QList<MarkupKind> > TextDocumentClientCapabilities::HoverCapabilities::contentFormat() const
{
    Utils::optional<QList<int>> array = optionalArray<int>(contentFormatKey);
    if (!array)
        return Utils::nullopt;
    return Utils::make_optional(Utils::transform(array.value(), [] (int value) {
        return static_cast<MarkupKind>(value);
    }));
}

void TextDocumentClientCapabilities::HoverCapabilities::setContentFormat(const QList<MarkupKind> &contentFormat)
{
    insert(contentFormatKey, enumArrayToJsonArray<MarkupKind>(contentFormat));
}

Utils::optional<QList<MarkupKind>>
TextDocumentClientCapabilities::SignatureHelpCapabilities::SignatureInformationCapabilities::
documentationFormat() const
{
    Utils::optional<QList<int>> array = optionalArray<int>(documentationFormatKey);
    if (!array)
        return Utils::nullopt;
    return Utils::make_optional(Utils::transform(array.value(), [] (int value) {
        return static_cast<MarkupKind>(value);
    }));
}

void
TextDocumentClientCapabilities::SignatureHelpCapabilities::SignatureInformationCapabilities::
setDocumentationFormat(const QList<MarkupKind> &documentationFormat)
{
    insert(documentationFormatKey, enumArrayToJsonArray<MarkupKind>(documentationFormat));
}

InitializeParams::InitializeParams()
{
    setProcessId(int(QCoreApplication::applicationPid()));
    setRootUri(LanguageClientValue<DocumentUri>());
    setCapabilities(ClientCapabilities());
    setTrace(s_trace);
}

Utils::optional<Trace> InitializeParams::trace() const
{
    const QJsonValue &traceValue = value(traceKey);
    if (traceValue.isUndefined())
        return Utils::nullopt;
    return Utils::make_optional(Trace(traceValue.toString()));
}

bool InitializeParams::isValid(QStringList *error) const
{
    return check<int, std::nullptr_t>(error, processIdKey)
            && checkOptional<QString, std::nullptr_t>(error, rootPathKey)
            && checkOptional<QString, std::nullptr_t>(error, rootUriKey)
            && check<ClientCapabilities>(error, capabilitiesKey)
            && checkOptional<int>(error, traceKey)
            && (checkOptional<std::nullptr_t>(error, workSpaceFoldersKey)
                || checkOptionalArray<WorkSpaceFolder>(error, workSpaceFoldersKey));
}

InitializeRequest::InitializeRequest(const InitializeParams &params)
    : Request(methodName, params)
{ }

InitializeNotification::InitializeNotification()
    : Notification(methodName)
{ }

} // namespace LanguageServerProtocol
