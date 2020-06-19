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

#include "refactoringserver.h"

#include "clangquery.h"
#include "symbolindexing.h"

#include <refactoringclientinterface.h>
#include <clangrefactoringmessages.h>

#include <QCoreApplication>

#include <functional>
#include <atomic>

namespace ClangBackEnd {

RefactoringServer::RefactoringServer(SymbolIndexingInterface &symbolIndexing,
                                     FilePathCachingInterface &filePathCache,
                                     GeneratedFiles &generatedFiles)
    : m_symbolIndexing(symbolIndexing),
      m_filePathCache(filePathCache),
      m_generatedFiles(generatedFiles)
{
    m_pollTimer.setInterval(100);

    QObject::connect(&m_pollTimer,
                     &QTimer::timeout,
                     std::bind(&RefactoringServer::pollSourceRangesForQueryMessages, this));
}

void RefactoringServer::end()
{
    QCoreApplication::exit();
}

void RefactoringServer::requestSourceRangesAndDiagnosticsForQueryMessage(
        RequestSourceRangesAndDiagnosticsForQueryMessage &&message)
{
    ClangQuery clangQuery(m_filePathCache, message.takeQuery());

    clangQuery.addFile(std::move(message.source.filePath),
                       std::move(message.source.unsavedFileContent),
                       std::move(message.source.commandLineArguments));

    clangQuery.findLocations();

    client()->sourceRangesAndDiagnosticsForQueryMessage({clangQuery.takeSourceRanges(),
                                                         clangQuery.takeDiagnosticContainers()});
}

void RefactoringServer::requestSourceRangesForQueryMessage(RequestSourceRangesForQueryMessage &&message)
{
    gatherSourceRangesForQueryMessages(message.takeSources(),
                                                     message.takeUnsavedContent(),
                                       message.takeQuery());
}

void RefactoringServer::updateProjectParts(UpdateProjectPartsMessage &&message)
{
    m_filePathCache.populateIfEmpty();
    m_symbolIndexing.updateProjectParts(message.takeProjectsParts());
}

void RefactoringServer::updateGeneratedFiles(UpdateGeneratedFilesMessage &&message)
{
    m_generatedFiles.update(message.takeGeneratedFiles());
}

void RefactoringServer::removeProjectParts(RemoveProjectPartsMessage &&)
{
    // TODO
}

void RefactoringServer::removeGeneratedFiles(RemoveGeneratedFilesMessage &&message)
{
    m_generatedFiles.remove(message.generatedFiles);
}

void RefactoringServer::cancel()
{
    m_gatherer.waitForFinished();
    m_gatherer = ClangQueryGatherer();
    m_pollTimer.stop();
}

bool RefactoringServer::isCancelingJobs() const
{
    return m_gatherer.isFinished();
}

void RefactoringServer::pollSourceRangesForQueryMessages()
{
    for (auto &&message : m_gatherer.finishedMessages())
        client()->sourceRangesForQueryMessage(std::move(message));

    if (!m_gatherer.isFinished())
        m_gatherer.startCreateNextSourceRangesMessages();
    else
        m_pollTimer.stop();
}

void RefactoringServer::waitThatSourceRangesForQueryMessagesAreFinished()
{
    while (!m_gatherer.isFinished()) {
        m_gatherer.waitForFinished();
        pollSourceRangesForQueryMessages();
    }
}

bool RefactoringServer::pollTimerIsActive() const
{
    return m_pollTimer.isActive();
}

void RefactoringServer::setGathererProcessingSlotCount(uint count)
{
    m_gatherer.setProcessingSlotCount(count);
}

void RefactoringServer::setProgress(int progress, int total)
{
    if (client())
        client()->progress({ProgressType::Indexing, progress, total});
}

void RefactoringServer::gatherSourceRangesForQueryMessages(
        std::vector<V2::FileContainer> &&sources,
        std::vector<V2::FileContainer> &&unsaved,
        Utils::SmallString &&query)
{
#ifdef _WIN32
    uint freeProcessors = 1;
#else
    uint freeProcessors = std::thread::hardware_concurrency();
#endif

    m_gatherer = ClangQueryGatherer(&m_filePathCache, std::move(sources), std::move(unsaved), std::move(query));
    m_gatherer.setProcessingSlotCount(freeProcessors);

    m_pollTimer.start();
}

} // namespace ClangBackEnd
