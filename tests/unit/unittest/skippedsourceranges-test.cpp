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

#include "googletest.h"
#include "unittest-utility-functions.h"

#include <cursor.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangstring.h>
#include <clangtranslationunit.h>
#include <skippedsourceranges.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <unsavedfiles.h>

#include <sourcerangecontainer.h>

#include <QVector>

using ClangBackEnd::Cursor;
using ClangBackEnd::Document;
using ClangBackEnd::Documents;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ClangString;
using ClangBackEnd::SourceRange;
using ClangBackEnd::SkippedSourceRanges;

using testing::IsNull;
using testing::NotNull;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;
using testing::Not;
using testing::IsEmpty;
using testing::SizeIs;
using testing::PrintToString;

namespace {

MATCHER_P4(IsSourceLocation, filePath, line, column, offset,
           std::string(negation ? "isn't" : "is")
           + " source location with file path "+ PrintToString(filePath)
           + ", line " + PrintToString(line)
           + ", column " + PrintToString(column)
           + " and offset " + PrintToString(offset)
           )
{
    if (!arg.filePath().endsWith(filePath)
     || arg.line() != line
     || arg.column() != column
     || arg.offset() != offset) {
        return false;
    }

    return true;
}

struct Data {
    Data()
    {
        document.parse();
    }

    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    Utf8String filePath = Utf8StringLiteral(TESTDATA_DIR"/skippedsourceranges.cpp");
    Utf8StringVector compilationArguments{UnitTest::addPlatformArguments(
        {Utf8StringLiteral("-std=c++11"), {}, Utf8StringLiteral("-DBLAH")})};
    Document document{filePath, compilationArguments, {}, documents};
    TranslationUnit translationUnit{filePath,
                                    filePath,
                                    document.translationUnit().cxIndex(),
                                    document.translationUnit().cxTranslationUnit()};
};

class SkippedSourceRanges : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
    const TranslationUnit &translationUnit = d->translationUnit;
    const Utf8String &filePath = d->filePath;
    ::SkippedSourceRanges skippedSourceRanges{d->translationUnit.skippedSourceRanges()};
    ::SkippedSourceRanges otherSkippedSourceRanges{d->translationUnit.skippedSourceRanges()};
};

Data *SkippedSourceRanges::d;

TEST_F(SkippedSourceRanges, MoveConstructor)
{
    const auto other = std::move(skippedSourceRanges);

    ASSERT_TRUE(skippedSourceRanges.isNull());
    ASSERT_FALSE(other.isNull());
}

TEST_F(SkippedSourceRanges, MoveAssignment)
{
    skippedSourceRanges = std::move(otherSkippedSourceRanges);

    ASSERT_TRUE(otherSkippedSourceRanges.isNull());
    ASSERT_FALSE(skippedSourceRanges.isNull());
}

TEST_F(SkippedSourceRanges, RangeWithZero)
{
    auto ranges = skippedSourceRanges.sourceRanges();

    ASSERT_THAT(ranges, SizeIs(2));
}

TEST_F(SkippedSourceRanges, DISABLED_ON_WINDOWS(RangeOne))
{
    auto ranges = skippedSourceRanges.sourceRanges();

    ASSERT_THAT(ranges[0].start(), IsSourceLocation(filePath, 2, 1, 6));
    ASSERT_THAT(ranges[0].end(), IsSourceLocation(filePath, 5, 1, 18));
}

TEST_F(SkippedSourceRanges, DISABLED_ON_WINDOWS(RangeTwo))
{
    auto ranges = skippedSourceRanges.sourceRanges();

    ASSERT_THAT(ranges[1].start(), IsSourceLocation(filePath, 8, 1, 39));
    ASSERT_THAT(ranges[1].end(), IsSourceLocation(filePath, 12, 1, 57));
}

TEST_F(SkippedSourceRanges, RangeContainerSize)
{
    auto ranges = skippedSourceRanges.toSourceRangeContainers();

    ASSERT_THAT(ranges, SizeIs(2));
}

void SkippedSourceRanges::SetUpTestCase()
{
    d = new Data;
}

void SkippedSourceRanges::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

}
