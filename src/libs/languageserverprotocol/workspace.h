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

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolderResult
    : public Utils::variant<QList<WorkSpaceFolder>, std::nullptr_t>
{
public:
    using variant::variant;
    using variant::operator=;
    operator const QJsonValue() const;
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkSpaceFolderRequest : public Request<
    WorkSpaceFolderResult, std::nullptr_t, std::nullptr_t>
{
public:
    WorkSpaceFolderRequest();
    using Request::Request;
    constexpr static const char methodName[] = "workspace/workspaceFolders";

    bool parametersAreValid(QString * /*errorMessage*/) const override { return true; }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceFoldersChangeEvent : public JsonObject
{
public:
    using JsonObject::JsonObject;
    WorkspaceFoldersChangeEvent();

    QList<WorkSpaceFolder> added() const { return array<WorkSpaceFolder>(addedKey); }
    void setAdded(const QList<WorkSpaceFolder> &added) { insertArray(addedKey, added); }

    QList<WorkSpaceFolder> removed() const { return array<WorkSpaceFolder>(removedKey); }
    void setRemoved(const QList<WorkSpaceFolder> &removed) { insertArray(removedKey, removed); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWorkspaceFoldersParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    WorkspaceFoldersChangeEvent event() const
    { return typedValue<WorkspaceFoldersChangeEvent>(eventKey); }
    void setEvent(const WorkspaceFoldersChangeEvent &event) { insert(eventKey, event); }

    bool isValid(ErrorHierarchy *error) const override
    { return check<WorkspaceFoldersChangeEvent>(error, eventKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWorkspaceFoldersNotification : public Notification<
        DidChangeWorkspaceFoldersParams>
{
public:
    explicit DidChangeWorkspaceFoldersNotification(const DidChangeWorkspaceFoldersParams &params);
    constexpr static const char methodName[] = "workspace/didChangeWorkspaceFolders";
    using Notification::Notification;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeConfigurationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QJsonValue settings() const { return typedValue<QJsonValue>(settingsKey); }
    void setSettings(QJsonValue settings) { insert(settingsKey, settings); }

    bool isValid(ErrorHierarchy *error) const override;
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeConfigurationNotification : public Notification<
        DidChangeConfigurationParams>
{
public:
    explicit DidChangeConfigurationNotification(const DidChangeConfigurationParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "workspace/didChangeConfiguration";
};

class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;
    class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationItem : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        Utils::optional<QString> scopeUri() const { return optionalValue<QString>(scopeUriKey); }
        void setScopeUri(const QString &scopeUri) { insert(scopeUriKey, scopeUri); }
        void clearScopeUri() { remove(scopeUriKey); }

        Utils::optional<QString> section() const { return optionalValue<QString>(sectionKey); }
        void setSection(const QString &section) { insert(sectionKey, section); }
        void clearSection() { remove(sectionKey); }

        bool isValid(ErrorHierarchy *error) const override;
    };

    QList<ConfigurationItem> items() const { return array<ConfigurationItem>(itemsKey); }
    void setItems(const QList<ConfigurationItem> &items) { insertArray(itemsKey, items); }

    bool isValid(ErrorHierarchy *error) const override
    { return checkArray<ConfigurationItem>(error, itemsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ConfigurationRequest : public Request<
        LanguageClientArray<QJsonValue>, std::nullptr_t, ConfigurationParams>
{
public:
    explicit ConfigurationRequest(const ConfigurationParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/configuration";
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWatchedFilesParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    class FileEvent : public JsonObject
    {
    public:
        using JsonObject::JsonObject;

        DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
        void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

        int type() const { return typedValue<int>(typeKey); }
        void setType(int type) { insert(typeKey, type); }

        enum FileChangeType {
            Created = 1,
            Changed = 2,
            Deleted = 3
        };

        bool isValid(ErrorHierarchy *error) const override
        { return check<QString>(error, uriKey) && check<int>(error, typeKey); }
    };

    QList<FileEvent> changes() const { return array<FileEvent>(changesKey); }
    void setChanges(const QList<FileEvent> &changes) { insertArray(changesKey, changes); }

    bool isValid(ErrorHierarchy *error) const override
    { return checkArray<FileEvent>(error, changesKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT DidChangeWatchedFilesNotification : public Notification<
        DidChangeWatchedFilesParams>
{
public:
    explicit DidChangeWatchedFilesNotification(const DidChangeWatchedFilesParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "workspace/didChangeWatchedFiles";
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceSymbolParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString query() const { return typedValue<QString>(queryKey); }
    void setQuery(const QString &query) { insert(queryKey, query); }

    bool isValid(ErrorHierarchy *error) const override { return check<QString>(error, queryKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT WorkspaceSymbolRequest : public Request<
        LanguageClientArray<SymbolInformation>, std::nullptr_t, WorkspaceSymbolParams>
{
public:
    WorkspaceSymbolRequest(const WorkspaceSymbolParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/symbol";
};

class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandParams : public JsonObject
{
public:
    explicit ExecuteCommandParams(const Command &command);
    explicit ExecuteCommandParams(const QJsonValue &value) : JsonObject(value) {}
    ExecuteCommandParams() : JsonObject() {}

    QString command() const { return typedValue<QString>(commandKey); }
    void setCommand(const QString &command) { insert(commandKey, command); }
    void clearCommand() { remove(commandKey); }

    Utils::optional<QJsonArray> arguments() const { return typedValue<QJsonArray>(argumentsKey); }
    void setArguments(const QJsonArray &arguments) { insert(argumentsKey, arguments); }
    void clearArguments() { remove(argumentsKey); }

    bool isValid(ErrorHierarchy *error) const override
    {
        return check<QString>(error, commandKey)
                && checkOptionalArray<QJsonValue>(error, argumentsKey);
    }
};

class LANGUAGESERVERPROTOCOL_EXPORT ExecuteCommandRequest : public Request<
        QJsonValue, std::nullptr_t, ExecuteCommandParams>
{
public:
    explicit ExecuteCommandRequest(const ExecuteCommandParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/executeCommand";
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    Utils::optional<QString> label() const { return optionalValue<QString>(labelKey); }
    void setLabel(const QString &label) { insert(labelKey, label); }
    void clearLabel() { remove(labelKey); }

    WorkspaceEdit edit() const { return typedValue<WorkspaceEdit>(editKey); }
    void setEdit(const WorkspaceEdit &edit) { insert(editKey, edit); }

    bool isValid(ErrorHierarchy *error) const override
    { return check<WorkspaceEdit>(error, editKey) && checkOptional<QString>(error, labelKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditResponse : public JsonObject
{
public:
    using JsonObject::JsonObject;

    bool applied() const { return typedValue<bool>(appliedKey); }
    void setApplied(bool applied) { insert(appliedKey, applied); }

    bool isValid(ErrorHierarchy *error) const override { return check<bool>(error, appliedKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT ApplyWorkspaceEditRequest : public Request<
        ApplyWorkspaceEditResponse, std::nullptr_t, ApplyWorkspaceEditParams>
{
public:
    explicit ApplyWorkspaceEditRequest(const ApplyWorkspaceEditParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "workspace/applyEdit";
};

} // namespace LanguageClient
