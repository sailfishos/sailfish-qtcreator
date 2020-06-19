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

#include "collectbuilddependencyaction.h"

#include <filepathcachingfwd.h>
#include <filepathid.h>

#include <clang/Tooling/Tooling.h>

namespace ClangBackEnd {

class CollectBuildDependencyToolAction final : public clang::tooling::FrontendActionFactory
{
public:
    CollectBuildDependencyToolAction(BuildDependency &buildDependency,
                                     const FilePathCachingInterface &filePathCache,
                                     const ClangBackEnd::FilePaths &excludedIncludes)
        : m_buildDependency(buildDependency)
        , m_filePathCache(filePathCache)
        , m_excludedFilePaths(excludedIncludes)
    {}


    bool runInvocation(std::shared_ptr<clang::CompilerInvocation> invocation,
                       clang::FileManager *fileManager,
                       std::shared_ptr<clang::PCHContainerOperations> pchContainerOperations,
                       clang::DiagnosticConsumer *diagnosticConsumer) override
    {
        if (m_excludedIncludeUIDs.empty())
            m_excludedIncludeUIDs = generateExcludedIncludeFileUIDs(*fileManager);

        return clang::tooling::FrontendActionFactory::runInvocation(invocation,
                                                                    fileManager,
                                                                    pchContainerOperations,
                                                                    diagnosticConsumer);
    }

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<CollectBuildDependencyAction>(
                    m_buildDependency,
                    m_filePathCache,
                    m_excludedIncludeUIDs,
                    m_alreadyIncludedFileUIDs);
    }
#else
    clang::FrontendAction *create() override
    {
        return new CollectBuildDependencyAction(m_buildDependency,
                                                m_filePathCache,
                                                m_excludedIncludeUIDs,
                                                m_alreadyIncludedFileUIDs);
    }
#endif

    std::vector<uint> generateExcludedIncludeFileUIDs(clang::FileManager &fileManager) const
    {
        std::vector<uint> fileUIDs;
        fileUIDs.reserve(m_excludedFilePaths.size());

        for (const FilePath &filePath : m_excludedFilePaths) {
            NativeFilePath nativeFilePath{filePath};
            const clang::FileEntry *file = fileManager.getFile({nativeFilePath.path().data(),
                                                                nativeFilePath.path().size()},
                                                               true)
#if LLVM_VERSION_MAJOR >= 10
                    .get()
#endif
                    ;

            if (file)
                fileUIDs.push_back(file->getUID());
        }

        std::sort(fileUIDs.begin(), fileUIDs.end());

        return fileUIDs;
    }

private:
    std::vector<uint> m_alreadyIncludedFileUIDs;
    std::vector<uint> m_excludedIncludeUIDs;
    BuildDependency &m_buildDependency;
    const FilePathCachingInterface &m_filePathCache;
    const ClangBackEnd::FilePaths &m_excludedFilePaths;
};

} // namespace ClangBackEnd
