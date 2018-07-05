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

#include "gtest-creator-printing.h"

#include "gtest-qt-printing.h"

#include <gtest/gtest-printers.h>

#include <sourcelocations.h>

#include <clangcodemodelclientmessages.h>
#include <clangcodemodelservermessages.h>
#include <clangpathwatcher.h>
#include <clangrefactoringmessages.h>
#include <filepath.h>
#include <nativefilepath.h>
#include <precompiledheadersupdatedmessage.h>
#include <sourcelocationentry.h>
#include <sourcelocationscontainer.h>
#include <tokeninfos.h>
#include <filepathview.h>
#include <tooltipinfo.h>

#include <cpptools/usages.h>

#include <projectexplorer/projectmacro.h>

#include <coreplugin/find/searchresultitem.h>

void PrintTo(const Utf8String &text, ::std::ostream *os)
{
    *os << text;
}

namespace Core {
namespace Search {

using testing::PrintToString;

class TextPosition;
class TextRange;

void PrintTo(const TextPosition &position, ::std::ostream *os)
{
    *os << "("
        << position.line << ", "
        << position.column << ", "
        << position.offset << ")";
}

void PrintTo(const TextRange &range, ::std::ostream *os)
{
    *os << "("
        << PrintToString(range.begin) << ", "
        << PrintToString(range.end) << ")";
}

} // namespace Search
} // namespace Core

namespace ProjectExplorer {

static const char *typeToString(const MacroType &type)
{
    switch (type) {
        case MacroType::Invalid: return "MacroType::Invalid";
        case MacroType::Define: return "MacroType::Define";
        case MacroType::Undefine: return  "MacroType::Undefine";
    }

    return "";
}

std::ostream &operator<<(std::ostream &out, const MacroType &type)
{
    out << typeToString(type);

    return out;
}

std::ostream &operator<<(std::ostream &out, const Macro &macro)
{
    out << "("
        << macro.key.data() << ", "
        << macro.value.data() << ", "
        << macro.type << ")";

  return out;
}

} // namespace ProjectExplorer

namespace Utils {
void PrintTo(const Utils::SmallString &text, ::std::ostream *os)
{
    *os << text;
}

void PrintTo(const Utils::PathString &text, ::std::ostream *os)
{
    *os << text;
}

} // namespace Utils

namespace ClangBackEnd {

std::ostream &operator<<(std::ostream &out, const FilePathId &id)
{
    return out << "(" << id.directoryId << ", " << id.fileNameId << ")";
}

std::ostream &operator<<(std::ostream &out, const FilePathView &filePathView)
{
    return out << "(" << filePathView.toStringView() << ", " << filePathView.slashIndex() << ")";
}

std::ostream &operator<<(std::ostream &out, const NativeFilePathView &nativeFilePathView)
{
    return out << "(" << nativeFilePathView.toStringView() << ", " << nativeFilePathView.slashIndex() << ")";
}

std::ostream &operator<<(std::ostream &out, const IdPaths &idPaths)
{
    out << "("
        << idPaths.id << ", "
        << idPaths.filePathIds << ")";

    return out;
}

std::ostream &operator<<(std::ostream &out, const SourceLocationEntry &entry)
{
    out << "("
        << entry.filePathId << ", "
        << entry.line << ", "
        << entry.column << ")";

    return out;
}

std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry)
{
    out << "("
        << entry.id << ", "
        << entry.pathId
        << ")";

    return out;
}

std::ostream &operator<<(std::ostream &os, const SourceLocationsContainer &container)
{
    os << "("
       << container.sourceLocationContainers()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const RegisterProjectPartsForEditorMessage &message)
{
    os << "("
       << message.projectContainers()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const FollowSymbolMessage &message)
{
      os << "("
         << message.fileContainer() << ", "
         << message.ticketNumber() << ", "
         << message.sourceRange() << ", "
         << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const CompleteCodeMessage &message)
{
    os << "("
       << message.filePath() << ", "
       << message.line() << ", "
       << message.column() << ", "
       << message.projectPartId() << ", "
       << message.ticketNumber() << ", "
       << message.funcNameStartLine() << ", "
       << message.funcNameStartColumn()

       << ")";

     return os;
}

std::ostream &operator<<(std::ostream &os, const RegisterTranslationUnitForEditorMessage &message)
{
    os << "RegisterTranslationUnitForEditorMessage("
       << message.fileContainers() << ", "
       << message.currentEditorFilePath() << ", "
       << message.visibleEditorFilePaths() << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const EndMessage &/*message*/)
{
    return os << "()";
}

std::ostream &operator<<(std::ostream &os, const CancelMessage &/*message*/)
{
    return os << "()";
}

std::ostream &operator<<(std::ostream &os, const AliveMessage &/*message*/)
{
    return os << "()";
}

#define RETURN_TEXT_FOR_CASE(enumValue) case CompletionCorrection::enumValue: return #enumValue
static const char *completionCorrectionToText(CompletionCorrection correction)
{
    switch (correction) {
        RETURN_TEXT_FOR_CASE(NoCorrection);
        RETURN_TEXT_FOR_CASE(DotToArrowCorrection);
    }

    return "";
}
#undef RETURN_TEXT_FOR_CASE

std::ostream &operator<<(std::ostream &os, const CodeCompletedMessage &message)
{
    os << "("
       << message.codeCompletions() << ", "
       << completionCorrectionToText(message.neededCorrection()) << ", "
       << message.ticketNumber()

       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const DocumentAnnotationsChangedMessage &message)
{
    os << "DocumentAnnotationsChangedMessage("
       << message.fileContainer()
       << "," << message.diagnostics().size()
       << "," << !message.firstHeaderErrorDiagnostic().text().isEmpty()
       << "," << message.tokenInfos().size()
       << "," << message.skippedPreprocessorRanges().size()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const ReferencesMessage &message)
{
      os << "("
         << message.fileContainer() << ", "
         << message.ticketNumber() << ", "
         << message.isLocalVariable() << ", "
         << message.references() << ", "
         << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const ToolTipMessage &message)
{
      os << "("
         << message.fileContainer() << ", "
         << message.ticketNumber() << ", "
         << message.toolTipInfo() << ", "
         << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const EchoMessage &/*message*/)
{
     return os << "()";
}

std::ostream &operator<<(std::ostream &os, const UnregisterProjectPartsForEditorMessage &message)
{
    os << "("
       << message.projectPartIds()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const UnregisterTranslationUnitsForEditorMessage &message)
{
    os << "("
       << message.fileContainers()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const CodeCompletion &message)
{
    os << "("
       << message.text() << ", "
       << message.priority() << ", "
       << message.completionKind() << ", "
       << message.availability() << ", "
       << message.hasParameters()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const CodeCompletionChunk &chunk)
{
    os << "("
       << chunk.kind() << ", "
       << chunk.text();

    if (chunk.isOptional())
        os << ", optional";

    os << ")";

    return os;
}

static const char *severityToText(DiagnosticSeverity severity)
{
    switch (severity) {
        case DiagnosticSeverity::Ignored: return "Ignored";
        case DiagnosticSeverity::Note: return "Note";
        case DiagnosticSeverity::Warning: return "Warning";
        case DiagnosticSeverity::Error: return "Error";
        case DiagnosticSeverity::Fatal: return "Fatal";
    }

    Q_UNREACHABLE();
}

std::ostream &operator<<(std::ostream &os, const DiagnosticContainer &container)
{
    os << "("
       << severityToText(container.severity()) << ": "
       << container.text() << ", "
       << container.category() << ", "
       << container.enableOption() << ", "
       << container.location() << ", "
       << container.ranges() << ", "
       << container.fixIts() << ", "
       << container.children() << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const DynamicASTMatcherDiagnosticContainer &container)
{
    os << "("
       <<  container.messages() << ", "
        << container.contexts()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const DynamicASTMatcherDiagnosticContextContainer &container)
{
    os << "("
       << container.contextTypeText() << ", "
       << container.sourceRange() << ", "
       << container.arguments()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const DynamicASTMatcherDiagnosticMessageContainer &container)
{
    os << "("
       << container.errorTypeText() << ", "
       << container.sourceRange() << ", "
       << container.arguments()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const FileContainer &container)
{
    os << "("
        << container.filePath() << ", "
        << container.projectPartId() << ", "
        << container.fileArguments() << ", "
        << container.documentRevision() << ", "
        << container.textCodecName();

    if (container.hasUnsavedFileContent())
        os << ", "
           << container.unsavedFileContent();

    os << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const FixItContainer &container)
{
    os << "("
       << container.text() << ", "
       << container.range()
       << ")";

    return os;
}

#define RETURN_TEXT_FOR_CASE(enumValue) case HighlightingType::enumValue: return #enumValue
static const char *highlightingTypeToCStringLiteral(HighlightingType type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(Invalid);
        RETURN_TEXT_FOR_CASE(Comment);
        RETURN_TEXT_FOR_CASE(Keyword);
        RETURN_TEXT_FOR_CASE(StringLiteral);
        RETURN_TEXT_FOR_CASE(NumberLiteral);
        RETURN_TEXT_FOR_CASE(Function);
        RETURN_TEXT_FOR_CASE(VirtualFunction);
        RETURN_TEXT_FOR_CASE(Type);
        RETURN_TEXT_FOR_CASE(LocalVariable);
        RETURN_TEXT_FOR_CASE(GlobalVariable);
        RETURN_TEXT_FOR_CASE(Field);
        RETURN_TEXT_FOR_CASE(Enumeration);
        RETURN_TEXT_FOR_CASE(Operator);
        RETURN_TEXT_FOR_CASE(Preprocessor);
        RETURN_TEXT_FOR_CASE(Label);
        RETURN_TEXT_FOR_CASE(FunctionDefinition);
        RETURN_TEXT_FOR_CASE(OutputArgument);
        RETURN_TEXT_FOR_CASE(PreprocessorDefinition);
        RETURN_TEXT_FOR_CASE(PreprocessorExpansion);
        RETURN_TEXT_FOR_CASE(PrimitiveType);
        RETURN_TEXT_FOR_CASE(Declaration);
    }

    return "";
}
#undef RETURN_TEXT_FOR_CASE

std::ostream &operator<<(std::ostream &os, HighlightingType highlightingType)
{
    return os << highlightingTypeToCStringLiteral(highlightingType);
}

std::ostream &operator<<(std::ostream &os, HighlightingTypes types)
{
    os << "("
       << types.mainHighlightingType;

    if (!types.mixinHighlightingTypes.empty())
       os << ", "<< types.mixinHighlightingTypes;

    os << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const TokenInfoContainer &container)
{
    os << "("
       << container.line() << ", "
       << container.column() << ", "
       << container.length() << ", "
       << container.types() << ", "
       << container.isIdentifier() << ", "
       << container.isIncludeDirectivePath()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &out, const NativeFilePath &filePath)
{
    return out << "(" << filePath.path() << ", " << filePath.slashIndex() << ")";
}

std::ostream &operator<<(std::ostream &out, const PrecompiledHeadersUpdatedMessage &message)
{
    out << "("
        << message.projectPartPchs()
        << ")";

    return out;
}

std::ostream &operator<<(std::ostream &os, const ProjectPartContainer &container)
{
    os << "("
        << container.projectPartId()
        << ","
        << container.arguments()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &out, const ProjectPartPch &projectPartPch)
{
    out << "("
        << projectPartPch.id() << ", "
        << projectPartPch.path() << ")";

    return out;
}

std::ostream &operator<<(std::ostream &os, const RegisterUnsavedFilesForEditorMessage &message)
{
    os << "("
       << message.fileContainers()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const RequestDocumentAnnotationsMessage &message)
{
    os << "("
       << message.fileContainer().filePath() << ","
       << message.fileContainer().projectPartId()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &out, const RemovePchProjectPartsMessage &message)
{
    return out << "(" << message.projectsPartIds() << ")";
}

std::ostream &operator<<(std::ostream &os, const RequestFollowSymbolMessage &message)
{
    os << "("
       << message.fileContainer() << ", "
       << message.ticketNumber() << ", "
       << message.line() << ", "
       << message.column() << ", "
       << ")";

     return os;
}

std::ostream &operator<<(std::ostream &os, const RequestReferencesMessage &message)
{
    os << "("
       << message.fileContainer() << ", "
       << message.ticketNumber() << ", "
       << message.line() << ", "
       << message.column() << ", "
       << message.local() << ", "
       << ")";

     return os;
}

std::ostream &operator<<(std::ostream &out, const RequestToolTipMessage &message)
{
    out << "("
        << message.fileContainer() << ", "
        << message.ticketNumber() << ", "
        << message.line() << ", "
        << message.column() << ", "
        << ")";

     return out;
}

std::ostream &operator<<(std::ostream &os, const ToolTipInfo::QdocCategory category)
{
    return os << qdocCategoryToString(category);
}

std::ostream &operator<<(std::ostream &out, const ToolTipInfo &info)
{
    out << "("
       << info.m_text << ", "
       << info.m_briefComment << ", "
       << info.m_qdocIdCandidates << ", "
       << info.m_qdocMark << ", "
       << info.m_qdocCategory
       << info.m_sizeInBytes << ", "
       << ")";

    return out;
}

std::ostream &operator<<(std::ostream &os, const RequestSourceLocationsForRenamingMessage &message)
{

    os << "("
       << message.filePath() << ", "
       << message.line() << ", "
       << message.column() << ", "
       << message.unsavedContent() << ", "
       << message.commandLine()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const RequestSourceRangesAndDiagnosticsForQueryMessage &message)
{
    os << "("
       << message.query() << ", "
       << message.source()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const RequestSourceRangesForQueryMessage &message)
{
    os << "("
       << message.query()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceLocationContainer &container)
{
    os << "("
       << container.filePath() << ", "
       << container.line() << ", "
       << container.column()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceLocationsForRenamingMessage &message)
{
    os << "("
        << message.symbolName() << ", "
        << message.textDocumentRevision() << ", "
        << message.sourceLocations()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceRangeContainer &container)
{
    os << "("
       << container.start() << ", "
       << container.end()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceRangesAndDiagnosticsForQueryMessage &message)
{
    os << "("
        << message.sourceRanges() << ", "
        << message.diagnostics()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceRangesContainer &container)
{
    os << "("
       << container.sourceRangeWithTextContainers()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceRangesForQueryMessage &message)
{
    os << "("
        << message.sourceRanges()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceRangeWithTextContainer &container)
{

    os << "("
        << container.start() << ", "
        << container.end() << ", "
        << container.text()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const UnregisterUnsavedFilesForEditorMessage &message)
{
    os << "("
        << message.fileContainers()
        << ")";

    return os;
}

std::ostream &operator<<(std::ostream &out, const UpdatePchProjectPartsMessage &message)
{
    return out << "("
               << message.projectsParts()
               << ")";
}

std::ostream &operator<<(std::ostream &os, const UpdateTranslationUnitsForEditorMessage &message)
{
    os << "UpdateTranslationUnitsForEditorMessage("
       << message.fileContainers()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const UpdateVisibleTranslationUnitsMessage &message)
{
    os << "("
       << message.currentEditorFilePath()  << ", "
       << message.visibleEditorFilePaths()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const TokenInfo& tokenInfo)
{
    os << "(type: " << tokenInfo.types() << ", "
       << " line: " << tokenInfo.line() << ", "
       << " column: " << tokenInfo.column() << ", "
       << " length: " << tokenInfo.length()
       << ")";

    return  os;
}

std::ostream &operator<<(std::ostream &out, const TokenInfos &tokenInfos)
{
    out << "[";

    for (const TokenInfo &entry : tokenInfos)
        out << entry;

    out << "]";

    return out;
}

std::ostream &operator<<(std::ostream &out, const FilePath &filePath)
{
    return out << "(" << filePath.path() << ", " << filePath.slashIndex() << ")";
}

void PrintTo(const FilePath &filePath, ::std::ostream *os)
{
    *os << filePath;
}

void PrintTo(const FilePathView &filePathView, ::std::ostream *os)
{
    *os << filePathView;
}

namespace V2 {

std::ostream &operator<<(std::ostream &os, const FileContainer &container)
{
    os << "("
        << container.filePath() << ", "
        << container.commandLineArguments() << ", "
        << container.documentRevision();

    if (container.unsavedFileContent().hasContent())
        os << ", \""
            << container.unsavedFileContent();

    os << "\")";

    return os;
}

std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container)
{
    out << "("
        << container.projectPartId() << ", "
        << container.arguments() << ", "
        << container.headerPaths() << ", "
        << container.sourcePaths()<< ")";

    return out;
}

std::ostream &operator<<(std::ostream &os, const SourceLocationContainer &container)
{
    os << "(("
       << container.filePathId().directoryId << ", " << container.filePathId().fileNameId << "), "
       << container.line() << ", "
       << container.column() << ", "
       << container.offset()
       << ")";

    return os;
}

std::ostream &operator<<(std::ostream &os, const SourceRangeContainer &container)
{
    os << "("
       << container.start() << ", "
       << container.end()
       << ")";

    return os;
}

} // namespace V2

} // namespace ClangBackEnd

namespace ClangRefactoring {
std::ostream &operator<<(std::ostream &out, const SourceLocation &location)
{
    return out << "(" << location.filePathId << ", " << location.line << ", " << location.column << ")";
}
} // namespace ClangBackEnd


namespace CppTools {
class Usage;

std::ostream &operator<<(std::ostream &out, const Usage &usage)
{
    return out << "(" << usage.path << ", " << usage.line << ", " << usage.column <<")";
}
} // namespace CppTools
