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

#include "mockclangcodemodelclient.h"
#include "processevents-utilities.h"

#include <clangcodemodelserver.h>
#include <highlightingmarkcontainer.h>
#include <clangcodemodelclientproxy.h>
#include <clangcodemodelserverproxy.h>
#include <clangtranslationunits.h>
#include <requestdocumentannotations.h>
#include <clangexceptions.h>

#include <cmbcompletecodemessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>

#include <QCoreApplication>
#include <QFile>

using testing::Property;
using testing::Contains;
using testing::Not;
using testing::Eq;
using testing::PrintToString;
using testing::_;

namespace {

using namespace ClangBackEnd;

MATCHER_P5(HasDirtyDocument,
           filePath,
           projectPartId,
           documentRevision,
           isNeedingReparse,
           hasNewDiagnostics,
           std::string(negation ? "isn't" : "is")
           + " document with file path "+ PrintToString(filePath)
           + " and project " + PrintToString(projectPartId)
           + " and document revision " + PrintToString(documentRevision)
           + " and isNeedingReparse = " + PrintToString(isNeedingReparse)
           + " and hasNewDiagnostics = " + PrintToString(hasNewDiagnostics)
           )
{
    auto &&documents = arg.documentsForTestOnly();
    try {
        auto document = documents.document(filePath, projectPartId);

        if (document.documentRevision() == documentRevision) {
            if (document.isNeedingReparse() && !isNeedingReparse) {
                *result_listener << "isNeedingReparse is true";
                return false;
            } else if (!document.isNeedingReparse() && isNeedingReparse) {
                *result_listener << "isNeedingReparse is false";
                return false;
            }

            return true;
        }

        *result_listener << "revision number is " << PrintToString(document.documentRevision());
        return false;

    } catch (...) {
        *result_listener << "has no translation unit";
        return false;
    }
}

class ClangClangCodeModelServer : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    bool waitUntilAllJobsFinished(int timeOutInMs = 10000);

    void registerProjectPart();
    void changeProjectPartArguments();
    void unregisterProject(const Utf8String &projectPartId);

    void registerProjectAndFile(const Utf8String &filePath,
                                int expectedDocumentAnnotationsChangedMessages = 1);
    void registerProjectAndFileAndWaitForFinished(const Utf8String &filePath,
                                                  int expectedDocumentAnnotationsChangedMessages = 1);
    void registerProjectAndFilesAndWaitForFinished(int expectedDocumentAnnotationsChangedMessages = 2);
    void registerFile(const Utf8String &filePath, const Utf8String &projectFilePath);
    void registerFile(const Utf8String &filePath,
                      int expectedDocumentAnnotationsChangedMessages = 1);
    void registerFiles(int expectedDocumentAnnotationsChangedMessages);
    void registerFileWithUnsavedContent(const Utf8String &filePath, const Utf8String &content);

    void updateUnsavedContent(const Utf8String &filePath,
                              const Utf8String &fileContent,
                              quint32 revisionNumber);

    void unregisterFile(const Utf8String &filePath);
    void unregisterFile(const Utf8String &filePath, const Utf8String &projectPartId);

    void removeUnsavedFile(const Utf8String &filePath);

    void updateVisibilty(const Utf8String &currentEditor, const Utf8String &additionalVisibleEditor);

    void requestDocumentAnnotations(const Utf8String &filePath);

    void completeCode(const Utf8String &filePath, uint line = 1, uint column = 1,
                      const Utf8String &projectPartId = Utf8String());
    void completeCodeInFileA();
    void completeCodeInFileB();

    bool isSupportiveTranslationUnitInitialized(const Utf8String &filePath);

    void expectDocumentAnnotationsChanged(int count);
    void expectCompletion(const CodeCompletion &completion);
    void expectCompletionFromFileA();
    void expectCompletionFromFileBEnabledByMacro();
    void expectCompletionFromFileAUnsavedMethodVersion1();
    void expectCompletionFromFileAUnsavedMethodVersion2();
    void expectCompletionWithTicketNumber(quint64 ticketNumber);
    void expectNoCompletionWithUnsavedMethod();
    void expectDocumentAnnotationsChangedForFileBWithSpecificHighlightingMark();

    static const Utf8String unsavedContent(const QString &unsavedFilePath);

protected:
    MockClangCodeModelClient mockClangCodeModelClient;
    ClangBackEnd::ClangCodeModelServer clangServer;
    const ClangBackEnd::Documents &documents = clangServer.documentsForTestOnly();
    const Utf8String projectPartId = Utf8StringLiteral("pathToProjectPart.pro");

    const Utf8String filePathA = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_function.cpp");
    const QString filePathAUnsavedVersion1
        = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved.cpp");
    const QString filePathAUnsavedVersion2
        = QStringLiteral(TESTDATA_DIR) + QStringLiteral("/complete_extractor_function_unsaved_2.cpp");

    const Utf8String filePathB = Utf8StringLiteral(TESTDATA_DIR"/complete_extractor_variable.cpp");

    const Utf8String aFilePath = Utf8StringLiteral("afile.cpp");
    const Utf8String anExistingFilePath
        = Utf8StringLiteral(TESTDATA_DIR"/complete_translationunit_parse_error.cpp");
    const Utf8String aProjectPartId = Utf8StringLiteral("aproject.pro");
};

TEST_F(ClangClangCodeModelServer, GetCodeCompletion)
{
    registerProjectAndFile(filePathA);

    expectCompletionFromFileA();
    completeCodeInFileA();
}

TEST_F(ClangClangCodeModelServer, RequestDocumentAnnotations)
{
    registerProjectAndFileAndWaitForFinished(filePathB);

    expectDocumentAnnotationsChangedForFileBWithSpecificHighlightingMark();
    requestDocumentAnnotations(filePathB);
}

TEST_F(ClangClangCodeModelServer, NoInitialDocumentAnnotationsForClosedDocument)
{
    const int expectedDocumentAnnotationsChangedCount = 0;
    registerProjectAndFile(filePathA, expectedDocumentAnnotationsChangedCount);

    unregisterFile(filePathA);
}

TEST_F(ClangClangCodeModelServer, NoDocumentAnnotationsForClosedDocument)
{
    const int expectedDocumentAnnotationsChangedCount = 1; // Only for registration.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    updateUnsavedContent(filePathA, Utf8String(), 1);

    unregisterFile(filePathA);
}

TEST_F(ClangClangCodeModelServer, NoInitialDocumentAnnotationsForOutdatedDocumentRevision)
{
    const int expectedDocumentAnnotationsChangedCount = 1; // Only for registration.
    registerProjectAndFile(filePathA, expectedDocumentAnnotationsChangedCount);

    updateUnsavedContent(filePathA, Utf8String(), 1);
}

TEST_F(ClangClangCodeModelServer, NoCompletionsForClosedDocument)
{
    const int expectedDocumentAnnotationsChangedCount = 1; // Only for registration.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    completeCodeInFileA();

    unregisterFile(filePathA);
}

TEST_F(ClangClangCodeModelServer, CodeCompletionDependingOnProject)
{
    const int expectedDocumentAnnotationsChangedCount = 2; // For registration and due to project change.
    registerProjectAndFileAndWaitForFinished(filePathB, expectedDocumentAnnotationsChangedCount);

    expectCompletionFromFileBEnabledByMacro();
    changeProjectPartArguments();
    completeCodeInFileB();
}

TEST_F(ClangClangCodeModelServer, GetCodeCompletionForUnsavedFile)
{
    registerProjectPart();
    expectDocumentAnnotationsChanged(1);
    registerFileWithUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion1));
    expectCompletionFromFileAUnsavedMethodVersion1();

    completeCodeInFileA();
}

TEST_F(ClangClangCodeModelServer, GetNoCodeCompletionAfterRemovingUnsavedFile)
{
    const int expectedDocumentAnnotationsChangedCount = 2; // For registration and update/removal.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    removeUnsavedFile(filePathA);

    expectNoCompletionWithUnsavedMethod();
    completeCodeInFileA();
}

TEST_F(ClangClangCodeModelServer, GetNewCodeCompletionAfterUpdatingUnsavedFile)
{
    const int expectedDocumentAnnotationsChangedCount = 2; // For registration and update/removal.
    registerProjectAndFileAndWaitForFinished(filePathA, expectedDocumentAnnotationsChangedCount);
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);

    expectCompletionFromFileAUnsavedMethodVersion2();
    completeCodeInFileA();
}

TEST_F(ClangClangCodeModelServer, TicketNumberIsForwarded)
{
    registerProjectAndFile(filePathA, 1);
    const CompleteCodeMessage message(filePathA, 20, 1, projectPartId);

    expectCompletionWithTicketNumber(message.ticketNumber());
    clangServer.completeCode(message);
}

TEST_F(ClangClangCodeModelServer, TranslationUnitAfterCreationIsNotDirty)
{
    registerProjectAndFile(filePathA, 1);

    ASSERT_THAT(clangServer, HasDirtyDocument(filePathA, projectPartId, 0U, false, false));
}

TEST_F(ClangClangCodeModelServer, SetCurrentAndVisibleEditor)
{
    registerProjectAndFilesAndWaitForFinished();
    auto functionDocument = documents.document(filePathA, projectPartId);
    auto variableDocument = documents.document(filePathB, projectPartId);

    updateVisibilty(filePathB, filePathA);

    ASSERT_TRUE(variableDocument.isUsedByCurrentEditor());
    ASSERT_TRUE(variableDocument.isVisibleInEditor());
    ASSERT_TRUE(functionDocument.isVisibleInEditor());
}

TEST_F(ClangClangCodeModelServer, StartCompletionJobFirstOnEditThatTriggersCompletion)
{
    registerProjectAndFile(filePathA, 2);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    expectCompletionFromFileA();

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);
    completeCodeInFileA();

    const QList<Jobs::RunningJob> jobs = clangServer.runningJobsForTestsOnly();
    ASSERT_THAT(jobs.size(), Eq(1));
    ASSERT_THAT(jobs.first().jobRequest.type, Eq(JobRequest::Type::CompleteCode));
}

TEST_F(ClangClangCodeModelServer, SupportiveTranslationUnitNotInitializedAfterRegister)
{
    registerProjectAndFile(filePathA, 1);

    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_FALSE(isSupportiveTranslationUnitInitialized(filePathA));
}

TEST_F(ClangClangCodeModelServer, SupportiveTranslationUnitIsSetupAfterFirstEdit)
{
    registerProjectAndFile(filePathA, 2);
    ASSERT_TRUE(waitUntilAllJobsFinished());

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);

    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_TRUE(isSupportiveTranslationUnitInitialized(filePathA));
}

TEST_F(ClangClangCodeModelServer, DoNotRunDuplicateJobs)
{
    registerProjectAndFile(filePathA, 3);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 1);
    ASSERT_TRUE(waitUntilAllJobsFinished());
    ASSERT_TRUE(isSupportiveTranslationUnitInitialized(filePathA));
    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), 2);
    QCoreApplication::processEvents(); // adds + runs a job
    updateVisibilty(Utf8String(), Utf8String());

    updateVisibilty(filePathA, filePathA); // triggers adding + runnings job on next processevents()
}

TEST_F(ClangClangCodeModelServer, OpenDocumentAndEdit)
{
    registerProjectAndFile(filePathA, 4);
    ASSERT_TRUE(waitUntilAllJobsFinished());

    for (unsigned revision = 1; revision <= 3; ++revision) {
        updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion2), revision);
        ASSERT_TRUE(waitUntilAllJobsFinished());
    }
}

TEST_F(ClangClangCodeModelServer, IsNotCurrentCurrentAndVisibleEditorAnymore)
{
    registerProjectAndFilesAndWaitForFinished();
    auto functionDocument = documents.document(filePathA, projectPartId);
    auto variableDocument = documents.document(filePathB, projectPartId);
    updateVisibilty(filePathB, filePathA);

    updateVisibilty(filePathB, Utf8String());

    ASSERT_FALSE(functionDocument.isUsedByCurrentEditor());
    ASSERT_FALSE(functionDocument.isVisibleInEditor());
    ASSERT_TRUE(variableDocument.isUsedByCurrentEditor());
    ASSERT_TRUE(variableDocument.isVisibleInEditor());
}

TEST_F(ClangClangCodeModelServer, TranslationUnitAfterUpdateNeedsReparse)
{
    registerProjectAndFileAndWaitForFinished(filePathA, 2);

    updateUnsavedContent(filePathA, unsavedContent(filePathAUnsavedVersion1), 1U);
    ASSERT_THAT(clangServer, HasDirtyDocument(filePathA, projectPartId, 1U, true, true));
}

void ClangClangCodeModelServer::SetUp()
{
    clangServer.setClient(&mockClangCodeModelClient);
    clangServer.setUpdateDocumentAnnotationsTimeOutInMsForTestsOnly(0);
    clangServer.setUpdateVisibleButNotCurrentDocumentsTimeOutInMsForTestsOnly(0);
}

void ClangClangCodeModelServer::TearDown()
{
    ASSERT_TRUE(waitUntilAllJobsFinished());
}

bool ClangClangCodeModelServer::waitUntilAllJobsFinished(int timeOutInMs)
{
    const auto noJobsRunningAnymore = [this]() {
        return clangServer.runningJobsForTestsOnly().isEmpty()
            && clangServer.queueSizeForTestsOnly() == 0
            && !clangServer.isTimerRunningForTestOnly();
    };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

void ClangClangCodeModelServer::registerProjectAndFilesAndWaitForFinished(
        int expectedDocumentAnnotationsChangedMessages)
{
    registerProjectPart();
    registerFiles(expectedDocumentAnnotationsChangedMessages);

    ASSERT_TRUE(waitUntilAllJobsFinished());
}

void ClangClangCodeModelServer::registerFile(const Utf8String &filePath,
                                  int expectedDocumentAnnotationsChangedMessages)
{
    const FileContainer fileContainer(filePath, projectPartId);
    const RegisterTranslationUnitForEditorMessage message({fileContainer}, filePath, {filePath});

    expectDocumentAnnotationsChanged(expectedDocumentAnnotationsChangedMessages);

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::registerFiles(int expectedDocumentAnnotationsChangedMessages)
{
    const FileContainer fileContainerA(filePathA, projectPartId);
    const FileContainer fileContainerB(filePathB, projectPartId);
    const RegisterTranslationUnitForEditorMessage message({fileContainerA,
                                                           fileContainerB},
                                                           filePathA,
                                                           {filePathA, filePathB});

    expectDocumentAnnotationsChanged(expectedDocumentAnnotationsChangedMessages);

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::expectDocumentAnnotationsChanged(int count)
{
    EXPECT_CALL(mockClangCodeModelClient, documentAnnotationsChanged(_)).Times(count);
}

void ClangClangCodeModelServer::registerFile(const Utf8String &filePath, const Utf8String &projectFilePath)
{
    const FileContainer fileContainer(filePath, projectFilePath);
    const RegisterTranslationUnitForEditorMessage message({fileContainer}, filePath, {filePath});

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::registerFileWithUnsavedContent(const Utf8String &filePath,
                                                    const Utf8String &unsavedContent)
{
    const FileContainer fileContainer(filePath, projectPartId, unsavedContent, true);
    const RegisterTranslationUnitForEditorMessage message({fileContainer}, filePath, {filePath});

    clangServer.registerTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::completeCode(const Utf8String &filePath,
                                  uint line,
                                  uint column,
                                  const Utf8String &projectPartId)
{
    Utf8String theProjectPartId = projectPartId;
    if (theProjectPartId.isEmpty())
        theProjectPartId = this->projectPartId;

    const CompleteCodeMessage message(filePath, line, column, theProjectPartId);

    clangServer.completeCode(message);
}

void ClangClangCodeModelServer::completeCodeInFileA()
{
    completeCode(filePathA, 20, 1);
}

void ClangClangCodeModelServer::completeCodeInFileB()
{
    completeCode(filePathB, 35, 1);
}

bool ClangClangCodeModelServer::isSupportiveTranslationUnitInitialized(const Utf8String &filePath)
{
    Document document = clangServer.documentsForTestOnly().document(filePath, projectPartId);
    DocumentProcessor documentProcessor = clangServer.documentProcessors().processor(document);

    return document.translationUnits().size() == 2
        && documentProcessor.hasSupportiveTranslationUnit()
        && documentProcessor.isSupportiveTranslationUnitInitialized();
}

void ClangClangCodeModelServer::expectCompletion(const CodeCompletion &completion)
{
    EXPECT_CALL(mockClangCodeModelClient,
                codeCompleted(Property(&CodeCompletedMessage::codeCompletions,
                                       Contains(completion))))
            .Times(1);
}

void ClangClangCodeModelServer::expectCompletionFromFileBEnabledByMacro()
{
    const CodeCompletion completion(Utf8StringLiteral("ArgumentDefinitionVariable"),
                                    34,
                                    CodeCompletion::VariableCompletionKind);

    expectCompletion(completion);
}

void ClangClangCodeModelServer::expectCompletionFromFileAUnsavedMethodVersion1()
{
    const CodeCompletion completion(Utf8StringLiteral("Method2"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    expectCompletion(completion);
}

void ClangClangCodeModelServer::expectCompletionFromFileAUnsavedMethodVersion2()
{
    const CodeCompletion completion(Utf8StringLiteral("Method3"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    expectCompletion(completion);
}

void ClangClangCodeModelServer::expectCompletionWithTicketNumber(quint64 ticketNumber)
{
    EXPECT_CALL(mockClangCodeModelClient,
                codeCompleted(Property(&CodeCompletedMessage::ticketNumber,
                                       Eq(ticketNumber))))
        .Times(1);
}

void ClangClangCodeModelServer::expectNoCompletionWithUnsavedMethod()
{
    const CodeCompletion completion(Utf8StringLiteral("Method2"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    EXPECT_CALL(mockClangCodeModelClient,
                codeCompleted(Property(&CodeCompletedMessage::codeCompletions,
                                       Not(Contains(completion)))))
            .Times(1);
}

void ClangClangCodeModelServer::expectCompletionFromFileA()
{
    const CodeCompletion completion(Utf8StringLiteral("Function"),
                                    34,
                                    CodeCompletion::FunctionCompletionKind);

    expectCompletion(completion);
}

void ClangClangCodeModelServer::requestDocumentAnnotations(const Utf8String &filePath)
{
    const RequestDocumentAnnotationsMessage message({filePath, projectPartId});

    clangServer.requestDocumentAnnotations(message);
}

void ClangClangCodeModelServer::expectDocumentAnnotationsChangedForFileBWithSpecificHighlightingMark()
{
    HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Function;
    types.mixinHighlightingTypes.push_back(ClangBackEnd::HighlightingType::Declaration);
    const HighlightingMarkContainer highlightingMark(1, 6, 8, types);

    EXPECT_CALL(mockClangCodeModelClient,
                documentAnnotationsChanged(
                    Property(&DocumentAnnotationsChangedMessage::highlightingMarks,
                             Contains(highlightingMark))))
        .Times(1);
}

void ClangClangCodeModelServer::updateUnsavedContent(const Utf8String &filePath,
                                              const Utf8String &fileContent,
                                              quint32 revisionNumber)
{
    const FileContainer fileContainer(filePath, projectPartId, fileContent, true, revisionNumber);
    const UpdateTranslationUnitsForEditorMessage message({fileContainer});

    clangServer.updateTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::removeUnsavedFile(const Utf8String &filePath)
{
    const FileContainer fileContainer(filePath, projectPartId, Utf8StringVector(), 74);
    const UpdateTranslationUnitsForEditorMessage message({fileContainer});

    clangServer.updateTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::unregisterFile(const Utf8String &filePath)
{
    const QVector<FileContainer> fileContainers = {FileContainer(filePath, projectPartId)};
    const UnregisterTranslationUnitsForEditorMessage message(fileContainers);

    clangServer.unregisterTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::unregisterFile(const Utf8String &filePath, const Utf8String &projectPartId)
{
    const QVector<FileContainer> fileContainers = {FileContainer(filePath, projectPartId)};
    const UnregisterTranslationUnitsForEditorMessage message(fileContainers);

    clangServer.unregisterTranslationUnitsForEditor(message);
}

void ClangClangCodeModelServer::unregisterProject(const Utf8String &projectPartId)
{
    const UnregisterProjectPartsForEditorMessage message({projectPartId});

    clangServer.unregisterProjectPartsForEditor(message);
}

void ClangClangCodeModelServer::registerProjectPart()
{
    RegisterProjectPartsForEditorMessage message({ProjectPartContainer(projectPartId)});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangClangCodeModelServer::registerProjectAndFile(const Utf8String &filePath,
                                                       int expectedDocumentAnnotationsChangedMessages)
{
    registerProjectPart();
    registerFile(filePath, expectedDocumentAnnotationsChangedMessages);
}

void ClangClangCodeModelServer::registerProjectAndFileAndWaitForFinished(
        const Utf8String &filePath,
        int expectedDocumentAnnotationsChangedMessages)
{
    registerProjectAndFile(filePath, expectedDocumentAnnotationsChangedMessages);
    ASSERT_TRUE(waitUntilAllJobsFinished());
}

void ClangClangCodeModelServer::changeProjectPartArguments()
{
    const ProjectPartContainer projectPartContainer(projectPartId,
                                                    {Utf8StringLiteral("-DArgumentDefinition")});
    const RegisterProjectPartsForEditorMessage message({projectPartContainer});

    clangServer.registerProjectPartsForEditor(message);
}

void ClangClangCodeModelServer::updateVisibilty(const Utf8String &currentEditor,
                                     const Utf8String &additionalVisibleEditor)
{
    const UpdateVisibleTranslationUnitsMessage message(currentEditor,
                                                       {currentEditor, additionalVisibleEditor});

    clangServer.updateVisibleTranslationUnits(message);
}

const Utf8String ClangClangCodeModelServer::unsavedContent(const QString &unsavedFilePath)
{
    QFile unsavedFileContentFile(unsavedFilePath);
    const bool isOpen = unsavedFileContentFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if (!isOpen)
        ADD_FAILURE() << "File with the unsaved content cannot be opened!";

    return Utf8String::fromByteArray(unsavedFileContentFile.readAll());
}

} // anonymous
