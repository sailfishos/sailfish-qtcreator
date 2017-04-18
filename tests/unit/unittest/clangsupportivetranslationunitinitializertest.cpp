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

#include "dummyclangipcclient.h"
#include "processevents-utilities.h"

#include <clangbackend_global.h>
#include <clangdocuments.h>
#include <clangexceptions.h>
#include <clangsupportivetranslationunitinitializer.cpp>
#include <clangtranslationunit.h>
#include <clangtranslationunits.h>
#include <projects.h>
#include <utf8string.h>

#include <clang-c/Index.h>

#include <memory>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using namespace ClangBackEnd;

using testing::Eq;

namespace {

class Data {
public:
    Data()
    {
        projects.createOrUpdate({ProjectPartContainer(projectPartId)});

        const QVector<FileContainer> fileContainer{FileContainer(filePath, projectPartId)};
        document = documents.create(fileContainer).front();
        documents.setVisibleInEditors({filePath});
        documents.setUsedByCurrentEditor(filePath);

        const auto isDocumentClosed = [this](const Utf8String &filePath,
                                             const Utf8String &projectPartId) {
            return !documents.hasDocument(filePath, projectPartId);
        };
        const auto jobRequestCreator = [this](const Document &document,
                                              JobRequest::Type type,
                                              PreferredTranslationUnit preferredTranslationUnit) {
            return createJobRequest(document, type, preferredTranslationUnit);
        };
        initializer.reset(new ClangBackEnd::SupportiveTranslationUnitInitializer{document, jobs});
        initializer->setIsDocumentClosedChecker(isDocumentClosed);
        initializer->setJobRequestCreator(jobRequestCreator);
    }

    JobRequest createJobRequest(const Document &document,
                                JobRequest::Type type,
                                PreferredTranslationUnit preferredTranslationUnit) const
    {
        JobRequest jobRequest;
        jobRequest.type = type;
        jobRequest.requirements = JobRequest::requirementsForType(type);
        jobRequest.filePath = document.filePath();
        jobRequest.projectPartId = document.projectPartId();
        jobRequest.unsavedFilesChangeTimePoint = unsavedFiles.lastChangeTimePoint();
        jobRequest.documentRevision = document.documentRevision();
        jobRequest.preferredTranslationUnit = preferredTranslationUnit;
        const ProjectPart &projectPart = projects.project(document.projectPartId());
        jobRequest.projectChangeTimePoint = projectPart.lastChangeTimePoint();

        return jobRequest;
    }

public:
    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp")};
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};

    ProjectParts projects;
    UnsavedFiles unsavedFiles;
    Documents documents{projects, unsavedFiles};
    Document document;
    DummyIpcClient dummyClientInterface;

    Jobs jobs{documents, unsavedFiles, projects, dummyClientInterface};

    std::unique_ptr<ClangBackEnd::SupportiveTranslationUnitInitializer> initializer;
};

class SupportiveTranslationUnitInitializer : public ::testing::Test
{
protected:
    void parse();
    Jobs::RunningJob createRunningJob(JobRequest::Type type) const;

    void assertNoJobIsRunningAndEmptyQueue();
    void assertSingleJobRunningAndEmptyQueue();

    bool waitUntilJobChainFinished(int timeOutInMs = 10000) const;

protected:
    Data d;

    Utf8String &filePath = d.filePath;
    Utf8String &projectPartId = d.projectPartId;

    ProjectParts projects = d.projects;
    Document &document = d.document;
    Documents &documents = d.documents;
    Jobs &jobs = d.jobs;
    ClangBackEnd::SupportiveTranslationUnitInitializer &initializer = *d.initializer;
};

TEST_F(SupportiveTranslationUnitInitializer, HasInitiallyNotInitializedState)
{
    ASSERT_THAT(initializer.state(), Eq(ClangBackEnd::SupportiveTranslationUnitInitializer::State::NotInitialized));
}

TEST_F(SupportiveTranslationUnitInitializer, StartInitializingAbortsIfDocumentIsClosed)
{
    documents.remove({FileContainer(filePath, projectPartId)});

    initializer.startInitializing();

    assertNoJobIsRunningAndEmptyQueue();
    ASSERT_THAT(initializer.state(), Eq(ClangBackEnd::SupportiveTranslationUnitInitializer::State::Aborted));
}

TEST_F(SupportiveTranslationUnitInitializer, StartInitializingAddsTranslationUnit)
{
    initializer.startInitializing();

    ASSERT_THAT(document.translationUnits().size(), Eq(2));
    ASSERT_FALSE(document.translationUnits().areAllTranslationUnitsParsed());
}

TEST_F(SupportiveTranslationUnitInitializer, StartInitializingStartsJob)
{
    initializer.startInitializing();

    assertSingleJobRunningAndEmptyQueue();
    const Jobs::RunningJob runningJob = jobs.runningJobs().first();
    ASSERT_THAT(runningJob.jobRequest.type, JobRequest::Type::ParseSupportiveTranslationUnit);
}

TEST_F(SupportiveTranslationUnitInitializer, CheckIfParseJobFinishedAbortsIfDocumentIsClosed)
{
    documents.remove({FileContainer(filePath, projectPartId)});
    initializer.setState(ClangBackEnd::SupportiveTranslationUnitInitializer::State::WaitingForParseJob);
    const Jobs::RunningJob runningJob = createRunningJob(JobRequest::Type::ParseSupportiveTranslationUnit);

    initializer.checkIfParseJobFinished(runningJob);

    assertNoJobIsRunningAndEmptyQueue();
    ASSERT_THAT(initializer.state(), Eq(ClangBackEnd::SupportiveTranslationUnitInitializer::State::Aborted));
}

TEST_F(SupportiveTranslationUnitInitializer, CheckIfParseJobFinishedStartsJob)
{
    parse();
    initializer.setState(ClangBackEnd::SupportiveTranslationUnitInitializer::State::WaitingForParseJob);
    Jobs::RunningJob runningJob = createRunningJob(JobRequest::Type::ParseSupportiveTranslationUnit);

    initializer.checkIfParseJobFinished(runningJob);
    jobs.process();

    assertSingleJobRunningAndEmptyQueue();
    runningJob = jobs.runningJobs().first();
    ASSERT_THAT(runningJob.jobRequest.type, JobRequest::Type::ReparseSupportiveTranslationUnit);
}

TEST_F(SupportiveTranslationUnitInitializer, CheckIfReparseJobFinishedAbortsIfDocumentIsClosed)
{
    documents.remove({FileContainer(filePath, projectPartId)});
    initializer.setState(ClangBackEnd::SupportiveTranslationUnitInitializer::State::WaitingForReparseJob);
    const Jobs::RunningJob runningJob = createRunningJob(JobRequest::Type::ReparseSupportiveTranslationUnit);

    initializer.checkIfReparseJobFinished(runningJob);

    assertNoJobIsRunningAndEmptyQueue();
    ASSERT_THAT(initializer.state(), Eq(ClangBackEnd::SupportiveTranslationUnitInitializer::State::Aborted));
}

TEST_F(SupportiveTranslationUnitInitializer, CheckIfReparseJobFinishedStartsJob)
{
    parse();
    initializer.setState(ClangBackEnd::SupportiveTranslationUnitInitializer::State::WaitingForReparseJob);
    Jobs::RunningJob runningJob = createRunningJob(JobRequest::Type::ReparseSupportiveTranslationUnit);

    initializer.checkIfReparseJobFinished(runningJob);
    jobs.process();

    assertNoJobIsRunningAndEmptyQueue();
    ASSERT_THAT(initializer.state(), Eq(ClangBackEnd::SupportiveTranslationUnitInitializer::State::Initialized));
}

TEST_F(SupportiveTranslationUnitInitializer, FullRun)
{
    parse();
    initializer.startInitializing();

    waitUntilJobChainFinished();
    ASSERT_THAT(initializer.state(), Eq(ClangBackEnd::SupportiveTranslationUnitInitializer::State::Initialized));
}

void SupportiveTranslationUnitInitializer::parse()
{
    projects.createOrUpdate({ProjectPartContainer{projectPartId, Utf8StringVector()}});
    document.parse();
}

Jobs::RunningJob SupportiveTranslationUnitInitializer::createRunningJob(JobRequest::Type type) const
{
    const JobRequest jobRequest = d.createJobRequest(document,
                                                     type,
                                                     PreferredTranslationUnit::LastUninitialized);
    return Jobs::RunningJob{jobRequest, Utf8String(), QFuture<void>()};
}

void SupportiveTranslationUnitInitializer::assertNoJobIsRunningAndEmptyQueue()
{
    ASSERT_TRUE(jobs.runningJobs().isEmpty());
    ASSERT_TRUE(jobs.queue().isEmpty());
}

void SupportiveTranslationUnitInitializer::assertSingleJobRunningAndEmptyQueue()
{
    ASSERT_THAT(jobs.runningJobs().size(), Eq(1));
    ASSERT_TRUE(jobs.queue().isEmpty());
}

bool SupportiveTranslationUnitInitializer::waitUntilJobChainFinished(int timeOutInMs) const
{
    const auto noJobsRunningAnymore = [this]() {
        return jobs.runningJobs().isEmpty() && jobs.queue().isEmpty();
    };

    return ProcessEventUtilities::processEventsUntilTrue(noJobsRunningAnymore, timeOutInMs);
}

} // anonymous namespace
