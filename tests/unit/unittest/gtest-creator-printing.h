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

#include <utils/smallstringio.h>

#include <clangsupport_global.h>

#include <QtGlobal>

#include <iosfwd>

#include <gtest/gtest-printers.h>

class Utf8String;
void PrintTo(const Utf8String &text, ::std::ostream *os);

namespace Core {
namespace Search {

class TextPosition;
class TextRange;

void PrintTo(const TextPosition &position, ::std::ostream *os);
void PrintTo(const TextRange &range, ::std::ostream *os);

} // namespace TextPosition
} // namespace TextPosition

namespace ProjectExplorer {

enum class MacroType;
class Macro;

std::ostream &operator<<(std::ostream &out, const MacroType &type);
std::ostream &operator<<(std::ostream &out, const Macro &macro);

} // namespace ClangRefactoring

namespace Utils {
void PrintTo(const Utils::SmallString &text, ::std::ostream *os);
void PrintTo(const Utils::PathString &text, ::std::ostream *os);
} // namespace ProjectExplorer

namespace ClangBackEnd {
class SourceLocationEntry;
class IdPaths;
class FilePathId;
class FilePath;
class WatcherEntry;
class SourceLocationsContainer;
class RegisterProjectPartsForEditorMessage;
class CancelMessage;
class AliveMessage;
class CodeCompletedMessage;
class EchoMessage;
class DocumentAnnotationsChangedMessage;
class ReferencesMessage;
class ToolTipMessage;
class FollowSymbolMessage;
class CompleteCodeMessage;
class EndMessage;
class RegisterTranslationUnitForEditorMessage;
class UnregisterProjectPartsForEditorMessage;
class UnregisterTranslationUnitsForEditorMessage;
class CodeCompletion;
class CodeCompletionChunk;
class DiagnosticContainer;
class DynamicASTMatcherDiagnosticContainer;
class DynamicASTMatcherDiagnosticContextContainer;
class DynamicASTMatcherDiagnosticMessageContainer;
class FileContainer;
class FixItContainer;
class HighlightingMarkContainer;
class NativeFilePath;
class PrecompiledHeadersUpdatedMessage;
class ProjectPartContainer;
class ProjectPartPch;
class RegisterUnsavedFilesForEditorMessage;
class RemovePchProjectPartsMessage;
class RequestDocumentAnnotationsMessage;
class RequestFollowSymbolMessage;
class RequestReferencesMessage;
class RequestToolTipMessage;
class RequestSourceLocationsForRenamingMessage;
class RequestSourceRangesAndDiagnosticsForQueryMessage;
class RequestSourceRangesForQueryMessage;
class SourceLocationContainer;
class SourceLocationsForRenamingMessage;
class SourceRangeContainer;
class SourceRangesAndDiagnosticsForQueryMessage;
class SourceRangesContainer;
class SourceRangesForQueryMessage;
class SourceRangeWithTextContainer;
class UnregisterUnsavedFilesForEditorMessage;
class UpdatePchProjectPartsMessage;
class UpdateTranslationUnitsForEditorMessage;
class UpdateVisibleTranslationUnitsMessage;
class FilePath;
class TokenInfo;
class TokenInfos;
template <char WindowsSlash>
class AbstractFilePathView;
using FilePathView = AbstractFilePathView<'/'>;
using NativeFilePathView = AbstractFilePathView<'\\'>;
class ToolTipInfo;

std::ostream &operator<<(std::ostream &out, const SourceLocationEntry &entry);
std::ostream &operator<<(std::ostream &out, const IdPaths &idPaths);
std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry);
std::ostream &operator<<(std::ostream &out, const SourceLocationsContainer &container);
std::ostream &operator<<(std::ostream &out, const RegisterProjectPartsForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const CancelMessage &message);
std::ostream &operator<<(std::ostream &out, const AliveMessage &message);
std::ostream &operator<<(std::ostream &out, const CodeCompletedMessage &message);
std::ostream &operator<<(std::ostream &out, const EchoMessage &message);
std::ostream &operator<<(std::ostream &out, const DocumentAnnotationsChangedMessage &message);
std::ostream &operator<<(std::ostream &out, const ReferencesMessage &message);
std::ostream &operator<<(std::ostream &out, const ToolTipMessage &message);
std::ostream &operator<<(std::ostream &out, const FollowSymbolMessage &message);
std::ostream &operator<<(std::ostream &out, const CompleteCodeMessage &message);
std::ostream &operator<<(std::ostream &out, const EndMessage &message);
std::ostream &operator<<(std::ostream &out, const RegisterTranslationUnitForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const UnregisterProjectPartsForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const UnregisterTranslationUnitsForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const CodeCompletion &message);
std::ostream &operator<<(std::ostream &out, const CodeCompletionChunk &chunk);
std::ostream &operator<<(std::ostream &out, const DiagnosticContainer &container);
std::ostream &operator<<(std::ostream &out, const DynamicASTMatcherDiagnosticContainer &container);
std::ostream &operator<<(std::ostream &out, const DynamicASTMatcherDiagnosticContextContainer &container);
std::ostream &operator<<(std::ostream &out, const DynamicASTMatcherDiagnosticMessageContainer &container);
std::ostream &operator<<(std::ostream &out, const FileContainer &container);
std::ostream &operator<<(std::ostream &out, const FixItContainer &container);
std::ostream &operator<<(std::ostream &out, HighlightingType highlightingType);
std::ostream &operator<<(std::ostream &out, HighlightingTypes types);
std::ostream &operator<<(std::ostream &out, const HighlightingMarkContainer &container);
std::ostream &operator<<(std::ostream &out, const NativeFilePath &filePath);
std::ostream &operator<<(std::ostream &out, const PrecompiledHeadersUpdatedMessage &message);
std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container);
std::ostream &operator<<(std::ostream &out, const ProjectPartPch &projectPartPch);
std::ostream &operator<<(std::ostream &out, const RegisterUnsavedFilesForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const RemovePchProjectPartsMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestDocumentAnnotationsMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestFollowSymbolMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestReferencesMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestToolTipMessage &message);
std::ostream &operator<<(std::ostream &out, const ToolTipInfo &info);
std::ostream &operator<<(std::ostream &out, const RequestSourceLocationsForRenamingMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestSourceRangesAndDiagnosticsForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const RequestSourceRangesForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceLocationContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceLocationsForRenamingMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceRangeContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangesAndDiagnosticsForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceRangesContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangesForQueryMessage &message);
std::ostream &operator<<(std::ostream &out, const SourceRangeWithTextContainer &container);
std::ostream &operator<<(std::ostream &out, const UnregisterUnsavedFilesForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const UpdatePchProjectPartsMessage &message);
std::ostream &operator<<(std::ostream &out, const UpdateTranslationUnitsForEditorMessage &message);
std::ostream &operator<<(std::ostream &out, const UpdateVisibleTranslationUnitsMessage &message);
std::ostream &operator<<(std::ostream &out, const FilePath &filePath);
std::ostream &operator<<(std::ostream &out, const FilePathId &filePathId);
std::ostream &operator<<(std::ostream &out, const TokenInfo& tokenInfo);
std::ostream &operator<<(std::ostream &out, const TokenInfos &tokenInfos);
std::ostream &operator<<(std::ostream &out, const FilePathView &filePathView);
std::ostream &operator<<(std::ostream &out, const NativeFilePathView &nativeFilePathView);

void PrintTo(const FilePath &filePath, ::std::ostream *os);
void PrintTo(const FilePathView &filePathView, ::std::ostream *os);

namespace V2 {
class FileContainer;
class ProjectPartContainer;
class SourceRangeContainer;
class SourceLocationContainer;

std::ostream &operator<<(std::ostream &out, const FileContainer &container);
std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceLocationContainer &container);
std::ostream &operator<<(std::ostream &out, const SourceRangeContainer &container);
}  // namespace V2

} // namespace ClangBackEnd

namespace ClangRefactoring {
class SourceLocation;

std::ostream &operator<<(std::ostream &out, const SourceLocation &location);
} // namespace ClangRefactoring


namespace CppTools {
class Usage;

std::ostream &operator<<(std::ostream &out, const Usage &usage);
} // namespace CppTools
