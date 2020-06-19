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

#include "filesystem-utilities.h"
#include "mockfilepathcaching.h"
#include "mockrefactoringclient.h"
#include "mocksymbolindexing.h"
#include "sourcerangecontainer-matcher.h"

#include <clangrefactoringmessages.h>
#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>
#include <refactoringserver.h>
#include <sqlitedatabase.h>

#include <utils/temporarydirectory.h>

#include <QTemporaryFile>

namespace {

using testing::AllOf;
using testing::Contains;
using testing::Field;
using testing::IsEmpty;
using testing::NiceMock;
using testing::Not;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::_;

using ClangBackEnd::FilePath;
using ClangBackEnd::IncludeSearchPaths;
using ClangBackEnd::IncludeSearchPathType;
using ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::RequestSourceRangesForQueryMessage;
using ClangBackEnd::SourceLocationsContainer;
using ClangBackEnd::SourceRangesAndDiagnosticsForQueryMessage;
using ClangBackEnd::SourceRangesContainer;
using ClangBackEnd::SourceRangesForQueryMessage;
using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::FileContainers;
using ClangBackEnd::ProjectPartContainer;
using ClangBackEnd::ProjectPartContainers;

MATCHER_P2(IsSourceLocation, line, column,
           std::string(negation ? "isn't " : "is ")
           + "(" + PrintToString(line)
           + ", " + PrintToString(column)
           + ")"
           )
{
    return arg.line == uint(line)
        && arg.column == uint(column);
}

class RefactoringServer : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ClangBackEnd::FilePathId filePathId(Utils::SmallStringView string)
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView{string});
    }

protected:
    NiceMock<MockRefactoringClient> mockRefactoringClient;
    NiceMock<MockSymbolIndexing> mockSymbolIndexing;
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::GeneratedFiles generatedFiles;
    ClangBackEnd::RefactoringServer refactoringServer{mockSymbolIndexing, filePathCache, generatedFiles};
    Utils::SmallString sourceContent{"void f()\n {}"};
    ClangBackEnd::FilePath filePath{TESTDATA_DIR, "query_simplefunction.cpp"};
    FileContainer source{filePath.clone(),
                         filePathCache.filePathId(filePath),
                         sourceContent.clone(),
                         {"cc"}};
    QTemporaryFile temporaryFile{Utils::TemporaryDirectory::masterDirectoryPath()
                                 + "/clangQuery-XXXXXX.cpp"};
    int processingSlotCount = 2;
};

using RefactoringServerSlowTest = RefactoringServer;
using RefactoringServerVerySlowTest = RefactoringServer;

TEST_F(RefactoringServerSlowTest, RequestSingleSourceRangesForQueryMessage)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source.clone()},
                                               {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Field(&SourceRangesForQueryMessage::sourceRanges,
                          Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))));

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerSlowTest, RequestSingleSourceRangesAndDiagnosticsWithUnsavedContentForQueryMessage)
{
    Utils::SmallString unsavedContent{"void f();"};
    FileContainer source{{TESTDATA_DIR, "query_simplefunction.cpp"},
                         filePathCache.filePathId(FilePath{TESTDATA_DIR, "query_simplefunction.cpp"}),
                         "#include \"query_simplefunction.h\"",
                         {"cc"}};
    FileContainer unsaved{{TESTDATA_DIR, "query_simplefunction.h"},
                          filePathCache.filePathId(FilePath{TESTDATA_DIR, "query_simplefunction.h"}),
                          unsavedContent.clone(),
                          {}};
    RequestSourceRangesForQueryMessage message{"functionDecl()", {source.clone()}, {unsaved.clone()}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Field(&SourceRangesForQueryMessage::sourceRanges,
                          Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                Contains(IsSourceRangeWithText(1, 1, 1, 9, unsavedContent))))));

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerSlowTest, RequestTwoSourceRangesForQueryMessage)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source.clone(), source.clone()},
                                               {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Field(&SourceRangesForQueryMessage::sourceRanges,
                          Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))));
    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Field(&SourceRangesForQueryMessage::sourceRanges,
                          Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                Not(Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent)))))));

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerVerySlowTest, RequestManySourceRangesForQueryMessage)
{
    std::vector<FileContainer> sources;
    std::fill_n(std::back_inserter(sources),
                processingSlotCount + 3,
                source.clone());
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               std::move(sources),
                                               {}};

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Field(&SourceRangesForQueryMessage::sourceRanges,
                          Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent))))));
    EXPECT_CALL(mockRefactoringClient,
                sourceRangesForQueryMessage(
                    Field(&SourceRangesForQueryMessage::sourceRanges,
                          Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                Not(Contains(IsSourceRangeWithText(1, 1, 2, 4, sourceContent)))))))
            .Times(processingSlotCount + 2);

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));
}

TEST_F(RefactoringServer, CancelJobs)
{
    std::vector<FileContainer> sources;
    std::fill_n(std::back_inserter(sources),
                std::thread::hardware_concurrency() + 3,
                source.clone());
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               std::move(sources),
                                               {}};
    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    refactoringServer.cancel();

    ASSERT_TRUE(refactoringServer.isCancelingJobs());
}

TEST_F(RefactoringServer, PollTimerIsActiveAfterStart)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source},
                                               {}};

    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    ASSERT_TRUE(refactoringServer.pollTimerIsActive());
}

TEST_F(RefactoringServer, PollTimerIsNotActiveAfterFinishing)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source},
                                               {}};
    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    refactoringServer.waitThatSourceRangesForQueryMessagesAreFinished();

    ASSERT_FALSE(refactoringServer.pollTimerIsActive());
}

TEST_F(RefactoringServer, PollTimerNotIsActiveAfterCanceling)
{
    RequestSourceRangesForQueryMessage message{"functionDecl()",
                                               {source},
                                               {}};
    refactoringServer.requestSourceRangesForQueryMessage(std::move(message));

    refactoringServer.cancel();

    ASSERT_FALSE(refactoringServer.pollTimerIsActive());
}

TEST_F(RefactoringServerSlowTest, ForValidRequestSourceRangesAndDiagnosticsGetSourceRange)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage message("functionDecl()",
                                                             {FilePath(temporaryFile.fileName()),
                                                              filePathCache.filePathId(FilePath(
                                                                  temporaryFile.fileName())),
                                                              "void f() {}",
                                                              {"cc"}});

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    AllOf(
                        Field(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                              Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                    Contains(IsSourceRangeWithText(1, 1, 1, 12, "void f() {}")))),
                        Field(&SourceRangesAndDiagnosticsForQueryMessage::diagnostics,
                              IsEmpty()))));

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(message));
}

TEST_F(RefactoringServerSlowTest, ForInvalidRequestSourceRangesAndDiagnosticsGetDiagnostics)
{
    RequestSourceRangesAndDiagnosticsForQueryMessage message("func()",
                                                             {FilePath(temporaryFile.fileName()),
                                                              filePathCache.filePathId(FilePath(
                                                                  temporaryFile.fileName())),
                                                              "void f() {}",
                                                              {"cc"}});

    EXPECT_CALL(mockRefactoringClient,
                sourceRangesAndDiagnosticsForQueryMessage(
                    AllOf(
                        Field(&SourceRangesAndDiagnosticsForQueryMessage::sourceRanges,
                              Field(&SourceRangesContainer::sourceRangeWithTextContainers,
                                    IsEmpty())),
                        Field(&SourceRangesAndDiagnosticsForQueryMessage::diagnostics,
                              Not(IsEmpty())))));

    refactoringServer.requestSourceRangesAndDiagnosticsForQueryMessage(std::move(message));
}

TEST_F(RefactoringServer, UpdateGeneratedFilesSetMemberWhichIsUsedForSymbolIndexing)
{
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            filePathCache.filePathId(FilePath{TESTDATA_DIR, "query_simplefunction.h"}),
                            "void f();",
                            {}}};

    refactoringServer.updateGeneratedFiles(Utils::clone(unsaved));

    ASSERT_THAT(generatedFiles.fileContainers(), ElementsAre(unsaved.front()));
}

TEST_F(RefactoringServer, RemoveGeneratedFilesSetMemberWhichIsUsedForSymbolIndexing)
{
    FileContainers unsaved{{{TESTDATA_DIR, "query_simplefunction.h"},
                            filePathCache.filePathId(FilePath{TESTDATA_DIR, "query_simplefunction.h"}),
                            "void f();",
                            {}}};
    refactoringServer.updateGeneratedFiles(Utils::clone(unsaved));

    refactoringServer.removeGeneratedFiles({{{TESTDATA_DIR, "query_simplefunction.h"}}});

    ASSERT_THAT(generatedFiles.fileContainers(), IsEmpty());
}

TEST_F(RefactoringServer, UpdateProjectPartsCalls)
{
    NiceMock<MockFilePathCaching> mockFilePathCaching;
    ClangBackEnd::RefactoringServer refactoringServer{mockSymbolIndexing,
                                                      mockFilePathCaching,
                                                      generatedFiles};
    ProjectPartContainers projectParts{
        {{1,
          {"-I", TESTDATA_DIR},
          {{"DEFINE", "1", 1}},
          IncludeSearchPaths{{"/includes", 1, IncludeSearchPathType::BuiltIn},
                             {"/other/includes", 2, IncludeSearchPathType::System}},
          IncludeSearchPaths{{"/project/includes", 1, IncludeSearchPathType::User},
                             {"/other/project/includes", 2, IncludeSearchPathType::User}},
          {filePathId("header1.h")},
          {filePathId("main.cpp")},
          Utils::Language::C,
          Utils::LanguageVersion::C11,
          Utils::LanguageExtension::All}}};

    EXPECT_CALL(mockFilePathCaching, populateIfEmpty());
    EXPECT_CALL(mockSymbolIndexing, updateProjectParts(projectParts));

    refactoringServer.updateProjectParts({Utils::clone(projectParts), {}});
}

TEST_F(RefactoringServer, SetProgress)
{
    EXPECT_CALL(mockRefactoringClient, progress(AllOf(Field(&ClangBackEnd::ProgressMessage::progress, 20),
                                                      Field(&ClangBackEnd::ProgressMessage::total, 30))));

    refactoringServer.setProgress(20, 30);
}

void RefactoringServer::SetUp()
{
    temporaryFile.open();
    refactoringServer.setClient(&mockRefactoringClient);
}

void RefactoringServer::TearDown()
{
    refactoringServer.setGathererProcessingSlotCount(uint(processingSlotCount));
    refactoringServer.waitThatSourceRangesForQueryMessagesAreFinished();
}

}
