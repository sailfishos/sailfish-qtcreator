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

#include <cursor.h>
#include <clangbackendipc_global.h>
#include <clangstring.h>
#include <projectpart.h>
#include <projects.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <highlightingmark.h>
#include <highlightingmarks.h>
#include <clangtranslationunit.h>
#include <translationunits.h>
#include <unsavedfiles.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::Cursor;
using ClangBackEnd::HighlightingTypes;
using ClangBackEnd::HighlightingMark;
using ClangBackEnd::HighlightingMarks;
using ClangBackEnd::HighlightingType;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceRange;

using testing::PrintToString;
using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;
using testing::Not;
using testing::IsEmpty;
using testing::SizeIs;

namespace {

MATCHER_P4(IsHighlightingMark, line, column, length, type,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(HighlightingMark(line, column, length, type))
           )
{
    const HighlightingMark expected(line, column, length, type);

    return arg == expected;
}

MATCHER_P(HasOnlyType, type,
          std::string(negation ? "isn't " : "is ")
          + PrintToString(type)
          )
{
    return arg.hasOnlyType(type);
}

MATCHER_P2(HasTwoTypes, firstType, secondType,
           std::string(negation ? "isn't " : "is ")
           + PrintToString(firstType)
           + " and "
           + PrintToString(secondType)
           )
{
    return arg.hasMainType(firstType) && arg.hasMixinType(secondType);
}

struct Data {
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/highlightingmarks.cpp"),
                ProjectPart(Utf8StringLiteral("projectPartId"), {Utf8StringLiteral("-std=c++14")}),
                {},
                translationUnits};
};

class HighlightingMarks : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

    SourceRange sourceRange(uint line, uint columnEnd) const;

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
};

TEST_F(HighlightingMarks, CreateNullInformations)
{
    ::HighlightingMarks infos;

    ASSERT_TRUE(infos.isNull());
}

TEST_F(HighlightingMarks, NullInformationsAreEmpty)
{
    ::HighlightingMarks infos;

    ASSERT_TRUE(infos.isEmpty());
}

TEST_F(HighlightingMarks, IsNotNull)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);

    const auto infos = translationUnit.highlightingMarksInRange(aRange);

    ASSERT_FALSE(infos.isNull());
}

TEST_F(HighlightingMarks, IteratorBeginEnd)
{
    const auto aRange = translationUnit.sourceRange(3, 1, 5, 1);
    const auto infos = translationUnit.highlightingMarksInRange(aRange);

    const auto endIterator = std::next(infos.begin(), infos.size());

    ASSERT_THAT(infos.end(), endIterator);
}

TEST_F(HighlightingMarks, ForFullTranslationUnitRange)
{
    const auto infos = translationUnit.highlightingMarks();

    ASSERT_THAT(infos, AllOf(Contains(IsHighlightingMark(1u, 1u, 4u, HighlightingType::Keyword)),
                             Contains(IsHighlightingMark(277u, 5u, 15u, HighlightingType::Function))));
}

TEST_F(HighlightingMarks, Size)
{
    const auto range = translationUnit.sourceRange(5, 5, 5, 10);

    const auto infos = translationUnit.highlightingMarksInRange(range);

    ASSERT_THAT(infos.size(), 1);
}

TEST_F(HighlightingMarks, DISABLED_Keyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(5, 12));

    ASSERT_THAT(infos[0], IsHighlightingMark(5u, 5u, 6u, HighlightingType::Keyword));
}

TEST_F(HighlightingMarks, StringLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(1, 29));

    ASSERT_THAT(infos[4], IsHighlightingMark(1u, 24u, 10u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, Utf8StringLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(2, 33));

    ASSERT_THAT(infos[4], IsHighlightingMark(2u, 24u, 12u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, RawStringLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(3, 34));

    ASSERT_THAT(infos[4], IsHighlightingMark(3u, 24u, 13u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, CharacterLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(4, 28));

    ASSERT_THAT(infos[3], IsHighlightingMark(4u, 24u, 3u, HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, IntegerLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(23, 26));

    ASSERT_THAT(infos[3], IsHighlightingMark(23u, 24u, 1u, HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, FloatLiteral)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(24, 29));

    ASSERT_THAT(infos[3], IsHighlightingMark(24u, 24u, 4u, HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, FunctionDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(45, 20));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, MemberFunctionDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(52, 29));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, FunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(55, 32));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, MemberFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(59, 27));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, MemberFunctionReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(104, 35));

    ASSERT_THAT(infos[0], IsHighlightingMark(104u, 9u, 23u, HighlightingType::Function));
}

TEST_F(HighlightingMarks, FunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(64, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TypeConversionFunction)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(68, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(68u, 14u, 3u, HighlightingType::Type));
}

TEST_F(HighlightingMarks, InbuiltTypeConversionFunction)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(69, 20));

    ASSERT_THAT(infos[1], IsHighlightingMark(69u, 14u, 3u, HighlightingType::Keyword));
}

TEST_F(HighlightingMarks, TypeReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(74, 13));

    ASSERT_THAT(infos[0], IsHighlightingMark(74u, 5u, 3u, HighlightingType::Type));
}

TEST_F(HighlightingMarks, LocalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(79, 13));

    ASSERT_THAT(infos[1], IsHighlightingMark(79u, 9u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(81, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(81u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableFunctionArgumentDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(84, 45));

    ASSERT_THAT(infos[5], IsHighlightingMark(84u, 41u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, LocalVariableFunctionArgumentReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(86, 26));

    ASSERT_THAT(infos[0], IsHighlightingMark(86u, 5u, 3u, HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, ClassVariableDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(90, 21));

    ASSERT_THAT(infos[1], IsHighlightingMark(90u, 9u, 11u, HighlightingType::Field));
}

TEST_F(HighlightingMarks, ClassVariableReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(94, 23));

    ASSERT_THAT(infos[0], IsHighlightingMark(94u, 9u, 11u, HighlightingType::Field));
}

TEST_F(HighlightingMarks, StaticMethodDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(110, 25));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, StaticMethodReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(114, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, Enumeration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(118, 17));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, Enumerator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(120, 15));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(HighlightingMarks, EnumerationReferenceDeclarationType)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, EnumerationReferenceDeclarationVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(125, 28));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, EnumerationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, EnumeratorReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(127, 30));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Enumeration));
}

TEST_F(HighlightingMarks, ClassForwardDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(130, 12));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, ConstructorDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(134, 13));

    ASSERT_THAT(infos[0], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, DestructorDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(135, 15));

    ASSERT_THAT(infos[1], HasTwoTypes(HighlightingType::Function, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, ClassForwardDeclarationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(138, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, ClassTypeReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, ConstructorReferenceVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(140, 32));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, UnionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(145, 12));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, UnionDeclarationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, GlobalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(150, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::GlobalVariable));
}

TEST_F(HighlightingMarks, StructDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(50, 11));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NameSpace)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(160, 22));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NameSpaceAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(164, 38));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, UsingStructInNameSpace)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(165, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NameSpaceReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, StructInNameSpaceReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(166, 35));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, VirtualFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(170, 35));

    ASSERT_THAT(infos[2], HasTwoTypes(HighlightingType::VirtualFunction, HighlightingType::Declaration));
}

TEST_F(HighlightingMarks, DISABLED_NonVirtualFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(177, 46));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, DISABLED_NonVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(180, 54));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, VirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(192, 51));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingMarks, FinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(202, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, NonFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(207, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::VirtualFunction));
}

TEST_F(HighlightingMarks, PlusOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(224, 49));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Operator));
}

TEST_F(HighlightingMarks, PlusAssignOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(226, 24));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Operator));
}

TEST_F(HighlightingMarks, Comment)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(229, 14));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Comment));
}

TEST_F(HighlightingMarks, PreprocessingDirective)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Preprocessor));
}

TEST_F(HighlightingMarks, PreprocessorMacroDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(231, 37));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(HighlightingMarks, PreprocessorFunctionMacroDefinition)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(232, 47));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::PreprocessorDefinition));
}

TEST_F(HighlightingMarks, PreprocessorMacroExpansion)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::PreprocessorExpansion));
}

TEST_F(HighlightingMarks, PreprocessorMacroExpansionArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(236, 27));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, PreprocessorInclusionDirective)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(239, 18));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, GotoLabelStatement)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(242, 12));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Label));
}

TEST_F(HighlightingMarks, GotoLabelStatementReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(244, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Label));
}

TEST_F(HighlightingMarks, TemplateReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(254, 25));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TemplateTypeParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateDefaultParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[10], HasOnlyType(HighlightingType::NumberLiteral));
}

TEST_F(HighlightingMarks, TemplateTemplateParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[17], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTemplateParameterDefaultArgument)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(265, 135));

    ASSERT_THAT(infos[19], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateFunctionDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(266, 63));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TemplateTypeParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTypeParameterDeclarationReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(268, 58));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, NonTypeTemplateParameterReferenceReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(269, 71));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, TemplateTemplateParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTemplateContainerParameterReference)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateTemplateParameterReferenceVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(270, 89));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, ClassFinalVirtualFunctionCallPointer)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(212, 61));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, ClassFinalVirtualFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(277, 23));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, HasFunctionArguments)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(286, 29));

    ASSERT_TRUE(infos[1].hasFunctionArguments());
}

TEST_F(HighlightingMarks, PreprocessorInclusionDirectiveWithAngleBrackets )
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(289, 38));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::StringLiteral));
}

TEST_F(HighlightingMarks, ArgumentInMacroExpansionIsKeyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Keyword));
}

TEST_F(HighlightingMarks, DISABLED_FirstArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, DISABLED_SecondArgumentInMacroExpansionIsLocalVariable)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(302, 36));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, DISABLED_SecondArgumentInMacroExpansionIsField)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(310, 40));

    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}


TEST_F(HighlightingMarks, DISABLED_EnumerationType)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(316, 30));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TypeInStaticCast)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, StaticCastIsKeyword)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Keyword));
}

TEST_F(HighlightingMarks, StaticCastPunctationIsInvalid)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(328, 64));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Invalid));
    ASSERT_THAT(infos[5], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, TypeInReinterpretCast)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(329, 69));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, IntegerAliasDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(333, 41));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, IntegerAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(341, 31));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, SecondIntegerAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(342, 43));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, IntegerTypedef)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(343, 35));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, FunctionAlias)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(344, 16));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, FriendTypeDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(350, 28));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, FriendArgumentTypeDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, FriendArgumentDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(351, 65));

    ASSERT_THAT(infos[8], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, FieldInitialization)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(358, 18));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Field));
}

TEST_F(HighlightingMarks, TemplateFunctionCall)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(372, 29));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Function));
}

TEST_F(HighlightingMarks, TemplatedType)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(377, 21));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplatedTypeDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(384, 49));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, NoOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(389, 24));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, ScopeOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(400, 33));

    ASSERT_THAT(infos[1], HasOnlyType(HighlightingType::Invalid));
}

TEST_F(HighlightingMarks, TemplateClassNamespace)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateClass)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateClassParameter)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[4], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TemplateClassDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(413, 78));

    ASSERT_THAT(infos[6], HasOnlyType(HighlightingType::LocalVariable));
}

TEST_F(HighlightingMarks, TypeDefDeclaration)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(418, 36));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, TypeDefDeclarationUsage)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(419, 48));

    ASSERT_THAT(infos[0], HasOnlyType(HighlightingType::Type));
}

TEST_F(HighlightingMarks, DISABLED_EnumerationTypeDef)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(424, 41));

    ASSERT_THAT(infos[3], HasOnlyType(HighlightingType::Type));
}

// QTCREATORBUG-15473
TEST_F(HighlightingMarks, DISABLED_ArgumentToUserDefinedIndexOperator)
{
    const auto infos = translationUnit.highlightingMarksInRange(sourceRange(434, 19));

    ASSERT_THAT(infos[2], HasOnlyType(HighlightingType::LocalVariable));
}

Data *HighlightingMarks::d;

void HighlightingMarks::SetUpTestCase()
{
    d = new Data;
}

void HighlightingMarks::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

ClangBackEnd::SourceRange HighlightingMarks::sourceRange(uint line, uint columnEnd) const
{
    return translationUnit.sourceRange(line, 1, line, columnEnd);
}

}
