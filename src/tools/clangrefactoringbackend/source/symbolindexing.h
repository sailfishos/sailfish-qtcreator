/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "symbolindexinginterface.h"

#include "symbolindexer.h"
#include "symbolscollector.h"
#include "processormanager.h"
#include "symbolindexertaskqueue.h"
#include "taskscheduler.h"
#include "symbolstorage.h"

#include <builddependenciesstorage.h>
#include <precompiledheaderstorage.h>
#include <projectpartsstorage.h>

#include <filepathcachingfwd.h>
#include <filesystem.h>
#include <modifiedtimechecker.h>
#include <refactoringdatabaseinitializer.h>

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

#include <QDateTime>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include <thread>

namespace ClangBackEnd {

class SymbolsCollectorManager;
class RefactoringServer;

class SymbolsCollectorManager final : public ClangBackEnd::ProcessorManager<SymbolsCollector>
{
public:
    using Processor = SymbolsCollector;
    SymbolsCollectorManager(const ClangBackEnd::GeneratedFiles &generatedFiles,
                            FilePathCaching &filePathCache)
        : ProcessorManager(generatedFiles)
        , m_filePathCache(filePathCache)
    {}

protected:
    std::unique_ptr<SymbolsCollector> createProcessor() const
    {
        return std::make_unique<SymbolsCollector>(m_filePathCache);
    }

private:
    FilePathCaching &m_filePathCache;
};

class SymbolIndexing final : public SymbolIndexingInterface
{
public:
    using BuildDependenciesStorage = ClangBackEnd::BuildDependenciesStorage<Sqlite::Database>;
    using SymbolStorage = ClangBackEnd::SymbolStorage<Sqlite::Database>;
    SymbolIndexing(Sqlite::Database &database,
                   FilePathCaching &filePathCache,
                   const GeneratedFiles &generatedFiles,
                   ProgressCounter::SetProgressCallback &&setProgressCallback,
                   const Environment &environment)
        : m_filePathCache(filePathCache)
        , m_buildDependencyStorage(database)
        , m_precompiledHeaderStorage(database)
        , m_projectPartsStorage(database)
        , m_symbolStorage(database)
        , m_collectorManger(generatedFiles, filePathCache)
        , m_progressCounter(std::move(setProgressCallback))
        , m_indexer(m_indexerQueue,
                    m_symbolStorage,
                    m_buildDependencyStorage,
                    m_precompiledHeaderStorage,
                    m_sourceWatcher,
                    m_filePathCache,
                    m_fileStatusCache,
                    m_symbolStorage.database,
                    m_projectPartsStorage,
                    m_modifiedTimeChecker,
                    environment)
        , m_indexerQueue(m_indexerScheduler, m_progressCounter, database)
        , m_indexerScheduler(m_collectorManger,
                             m_indexerQueue,
                             m_progressCounter,
                             std::thread::hardware_concurrency(),
                             CallDoInMainThreadAfterFinished::Yes)
    {
    }

    ~SymbolIndexing()
    {
        syncTasks();
    }

    SymbolIndexer &indexer()
    {
        return m_indexer;
    }

    void syncTasks()
    {
        m_indexerScheduler.disable();
        while (!m_indexerScheduler.futures().empty()) {
            m_indexerScheduler.syncTasks();
            m_indexerScheduler.slotUsage();
        }
    }

    void updateProjectParts(ProjectPartContainers &&projectParts) override;

private:
    using SymbolIndexerTaskScheduler = TaskScheduler<SymbolsCollectorManager, SymbolIndexerTask::Callable>;
    FilePathCachingInterface &m_filePathCache;
    BuildDependenciesStorage m_buildDependencyStorage;
    PrecompiledHeaderStorage<Sqlite::Database> m_precompiledHeaderStorage;
    ProjectPartsStorage<Sqlite::Database> m_projectPartsStorage;
    SymbolStorage m_symbolStorage;
    FileSystem m_fileSytem{m_filePathCache};
    ClangPathWatcher<QFileSystemWatcher, QTimer> m_sourceWatcher{m_filePathCache, m_fileSytem};
    FileStatusCache m_fileStatusCache{m_fileSytem};
    SymbolsCollectorManager m_collectorManger;
    ProgressCounter m_progressCounter;
    ModifiedTimeChecker<ClangBackEnd::SourceTimeStamps> m_modifiedTimeChecker{m_fileSytem};
    SymbolIndexer m_indexer;
    SymbolIndexerTaskQueue m_indexerQueue;
    SymbolIndexerTaskScheduler m_indexerScheduler;
};

} // namespace ClangBackEnd
