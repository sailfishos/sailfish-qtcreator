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

#include "projectupdater.h"

#include "pchmanagerclient.h"

#include <clangindexingprojectsettings.h>
#include <clangindexingsettingsmanager.h>
#include <filepathid.h>
#include <pchmanagerserverinterface.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/projectpart.h>
#include <cpptools/headerpathfilter.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

#include <utils/algorithm.h>
#include <utils/namevalueitem.h>

#include <QDirIterator>

#include <algorithm>
#include <functional>
#include <iostream>

namespace ClangPchManager {

class HeaderAndSources
{
public:
    void reserve(std::size_t size)
    {
        headers.reserve(size);
        sources.reserve(size);
    }

    ClangBackEnd::FilePathIds headers;
    ClangBackEnd::FilePathIds sources;
};

void ProjectUpdater::updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts,
                                        Utils::SmallStringVector &&toolChainArguments)
{
    addProjectFilesToFilePathCache(projectParts);
    fetchProjectPartIds(projectParts);

    m_server.updateProjectParts(
        ClangBackEnd::UpdateProjectPartsMessage{toProjectPartContainers(projectParts),
                                                std::move(toolChainArguments)});
}

void ProjectUpdater::removeProjectParts(ClangBackEnd::ProjectPartIds projectPartIds)
{
    auto sortedIds{projectPartIds};
    std::sort(sortedIds.begin(), sortedIds.end());

    m_server.removeProjectParts(ClangBackEnd::RemoveProjectPartsMessage{std::move(sortedIds)});
}

void ProjectUpdater::updateGeneratedFiles(ClangBackEnd::V2::FileContainers &&generatedFiles)
{
    std::sort(generatedFiles.begin(), generatedFiles.end());

    m_generatedFiles.update(generatedFiles);

    m_excludedPaths = createExcludedPaths(m_generatedFiles.fileContainers());

    m_server.updateGeneratedFiles(
                ClangBackEnd::UpdateGeneratedFilesMessage{std::move(generatedFiles)});
}

void ProjectUpdater::removeGeneratedFiles(ClangBackEnd::FilePaths &&filePaths)
{
    m_generatedFiles.remove(filePaths);

    m_excludedPaths = createExcludedPaths(m_generatedFiles.fileContainers());

    m_server.removeGeneratedFiles(
                ClangBackEnd::RemoveGeneratedFilesMessage{std::move(filePaths)});
}

void ProjectUpdater::setExcludedPaths(ClangBackEnd::FilePaths &&excludedPaths)
{
    m_excludedPaths = std::move(excludedPaths);
}

const ClangBackEnd::FilePaths &ProjectUpdater::excludedPaths() const
{
    return m_excludedPaths;
}

const ClangBackEnd::GeneratedFiles &ProjectUpdater::generatedFiles() const
{
    return m_generatedFiles;
}

void ProjectUpdater::addToHeaderAndSources(HeaderAndSources &headerAndSources,
                                           const CppTools::ProjectFile &projectFile) const
{
    using ClangBackEnd::FilePathView;

    Utils::PathString path = projectFile.path;
    bool exclude = std::binary_search(m_excludedPaths.begin(), m_excludedPaths.end(), path);

    if (!exclude) {
        ClangBackEnd::FilePathId filePathId = m_filePathCache.filePathId(FilePathView(path));
        if (projectFile.isSource())
            headerAndSources.sources.push_back(filePathId);
        else if (projectFile.isHeader())
            headerAndSources.headers.push_back(filePathId);
    }
}

HeaderAndSources ProjectUpdater::headerAndSourcesFromProjectPart(
        CppTools::ProjectPart *projectPart) const
{
    HeaderAndSources headerAndSources;
    headerAndSources.reserve(std::size_t(projectPart->files.size()) * 3 / 2);

    for (const CppTools::ProjectFile &projectFile : projectPart->files) {
        if (projectFile.active)
            addToHeaderAndSources(headerAndSources, projectFile);
    }

    std::sort(headerAndSources.sources.begin(), headerAndSources.sources.end());
    std::sort(headerAndSources.headers.begin(), headerAndSources.headers.end());

    return headerAndSources;
}

QStringList ProjectUpdater::toolChainArguments(CppTools::ProjectPart *projectPart)
{
    using CppTools::CompilerOptionsBuilder;
    CompilerOptionsBuilder builder(*projectPart, CppTools::UseSystemHeader::Yes);

    builder.addWordWidth();
    // builder.addTargetTriple(); TODO resarch why target triples are different
    builder.addExtraCodeModelFlags();
    builder.undefineClangVersionMacrosForMsvc();

    builder.undefineCppLanguageFeatureMacrosForMsvc2015();
    builder.addProjectConfigFileInclude();
    builder.addMsvcCompatibilityVersion();

    return builder.options();
}

namespace {
void cleanupMacros(ClangBackEnd::CompilerMacros &macros)
{
    auto newEnd = std::partition(macros.begin(),
                                 macros.end(),
                                 [](const ClangBackEnd::CompilerMacro &macro) {
                                     return macro.key != "QT_TESTCASE_BUILDDIR";
                                 });

    macros.erase(newEnd, macros.end());
}

void updateWithSettings(ClangBackEnd::CompilerMacros &macros,
                        Utils::NameValueItems &&settingsItems,
                        int &index)
{
    std::sort(settingsItems.begin(), settingsItems.end(), [](const auto &first, const auto &second) {
        return std::tie(first.operation, first.name, first.value)
               < std::tie(first.operation, second.name, second.value);
    });

    auto point = std::partition_point(settingsItems.begin(), settingsItems.end(), [](const auto &entry) {
        return entry.operation == Utils::NameValueItem::SetEnabled;
    });

    std::transform(
        settingsItems.begin(),
        point,
        std::back_inserter(macros),
        [&](const Utils::NameValueItem &settingsMacro) {
            return ClangBackEnd::CompilerMacro{settingsMacro.name, settingsMacro.value, ++index};
        });

    std::sort(macros.begin(), macros.end(), [](const auto &first, const auto &second) {
        return std::tie(first.key, first.value) < std::tie(second.key, second.value);
    });

    ClangBackEnd::CompilerMacros result;
    result.reserve(macros.size());

    ClangBackEnd::CompilerMacros convertedSettingsMacros;
    convertedSettingsMacros.resize(std::distance(point, settingsItems.end()));
    std::transform(
        point,
        settingsItems.end(),
        std::back_inserter(convertedSettingsMacros),
        [&](const Utils::NameValueItem &settingsMacro) {
            return ClangBackEnd::CompilerMacro{settingsMacro.name, settingsMacro.value, ++index};
        });

    std::set_difference(macros.begin(),
                        macros.end(),
                        convertedSettingsMacros.begin(),
                        convertedSettingsMacros.end(),
                        std::back_inserter(result),
                        [](const ClangBackEnd::CompilerMacro &first,
                           const ClangBackEnd::CompilerMacro &second) {
                            return std::tie(first.key, first.value)
                                   < std::tie(second.key, second.value);
                        });

    macros = std::move(result);
}

} // namespace

ClangBackEnd::CompilerMacros ProjectUpdater::createCompilerMacros(
    const ProjectExplorer::Macros &projectMacros, Utils::NameValueItems &&settingsMacros)
{
    int index = 0;
    auto macros = Utils::transform<ClangBackEnd::CompilerMacros>(
        projectMacros, [&](const ProjectExplorer::Macro &macro) {
            return ClangBackEnd::CompilerMacro{macro.key, macro.value, ++index};
        });

    cleanupMacros(macros);
    updateWithSettings(macros, std::move(settingsMacros), index);

    std::sort(macros.begin(), macros.end());

    return macros;
}

namespace  {
ClangBackEnd::IncludeSearchPathType convertType(ProjectExplorer::HeaderPathType sourceType)
{
    using ProjectExplorer::HeaderPathType;
    using ClangBackEnd::IncludeSearchPathType;

    switch (sourceType) {
    case HeaderPathType::User:
        return IncludeSearchPathType::User;
    case HeaderPathType::System:
        return IncludeSearchPathType::System;
    case HeaderPathType::BuiltIn:
        return IncludeSearchPathType::BuiltIn;
    case HeaderPathType::Framework:
        return IncludeSearchPathType::Framework;
    }

    return IncludeSearchPathType::Invalid;
}

ClangBackEnd::IncludeSearchPaths convertToIncludeSearchPaths(ProjectExplorer::HeaderPaths headerPaths)
{
    ClangBackEnd::IncludeSearchPaths paths;
    paths.reserve(Utils::usize(headerPaths));

    int index = 0;
    for (const ProjectExplorer::HeaderPath &headerPath : headerPaths)
        paths.emplace_back(headerPath.path, ++index, convertType(headerPath.type));

    std::sort(paths.begin(), paths.end());

    return paths;
}

ClangBackEnd::IncludeSearchPaths convertToIncludeSearchPaths(
    ProjectExplorer::HeaderPaths headerPaths, ProjectExplorer::HeaderPaths headerPaths2)
{
    ClangBackEnd::IncludeSearchPaths paths;
    paths.reserve(Utils::usize(headerPaths) + Utils::usize(headerPaths2));

    int index = 0;
    for (const ProjectExplorer::HeaderPath &headerPath : headerPaths)
        paths.emplace_back(headerPath.path, ++index, convertType(headerPath.type));

    for (const ProjectExplorer::HeaderPath &headerPath : headerPaths2)
        paths.emplace_back(headerPath.path, ++index, convertType(headerPath.type));

    std::sort(paths.begin(), paths.end());

    return paths;
}

QString projectDirectory(ProjectExplorer::Project *project)
{
    if (project)
        return project->rootProjectDirectory().toString();

    return {};
}

QString buildDirectory(ProjectExplorer::Project *project)
{
    if (project && project->activeTarget() && project->activeTarget()->activeBuildConfiguration())
        return project->activeTarget()->activeBuildConfiguration()->buildDirectory().toString();

    return {};
}
} // namespace

ProjectUpdater::SystemAndProjectIncludeSearchPaths ProjectUpdater::createIncludeSearchPaths(
    const CppTools::ProjectPart &projectPart)
{
    CppTools::HeaderPathFilter filter(projectPart,
                                      CppTools::UseTweakedHeaderPaths::Yes,
                                      CLANG_VERSION,
                                      CLANG_RESOURCE_DIR,
                                      projectDirectory(projectPart.project),
                                      buildDirectory(projectPart.project));
    filter.process();

    return {convertToIncludeSearchPaths(filter.systemHeaderPaths, filter.builtInHeaderPaths),
            convertToIncludeSearchPaths(filter.userHeaderPaths)};
}

ClangBackEnd::ProjectPartContainer ProjectUpdater::toProjectPartContainer(
        CppTools::ProjectPart *projectPart) const
{

    QStringList arguments = toolChainArguments(projectPart);

    HeaderAndSources headerAndSources = headerAndSourcesFromProjectPart(projectPart);

    auto includeSearchPaths = createIncludeSearchPaths(*projectPart);

    ClangBackEnd::ProjectPartId projectPartId = m_projectPartIdCache.stringId(
        Utils::PathString{projectPart->id()}, [&](Utils::SmallStringView projectPartName) {
            return m_projectPartsStorage.fetchProjectPartId(projectPartName);
        });

    ClangIndexingProjectSettings *settings = m_settingsManager.settings(projectPart->project);

    return ClangBackEnd::ProjectPartContainer(projectPartId,
                                              Utils::SmallStringVector(arguments),
                                              createCompilerMacros(projectPart->projectMacros,
                                                                   settings->readMacros()),
                                              std::move(includeSearchPaths.system),
                                              std::move(includeSearchPaths.project),
                                              std::move(headerAndSources.headers),
                                              std::move(headerAndSources.sources),
                                              projectPart->language,
                                              projectPart->languageVersion,
                                              static_cast<Utils::LanguageExtension>(
                                                  int(projectPart->languageExtensions)));
}

ClangBackEnd::ProjectPartContainers ProjectUpdater::toProjectPartContainers(
        std::vector<CppTools::ProjectPart *> projectParts) const
{
    using namespace std::placeholders;

    projectParts.erase(std::remove_if(projectParts.begin(),
                                      projectParts.end(),
                                      [](const CppTools::ProjectPart *projectPart) {
                                          return !projectPart->selectedForBuilding;
                                      }),
                       projectParts.end());

    std::vector<ClangBackEnd::ProjectPartContainer> projectPartContainers;
    projectPartContainers.reserve(projectParts.size());

    std::transform(projectParts.begin(),
                   projectParts.end(),
                   std::back_inserter(projectPartContainers),
                   std::bind(&ProjectUpdater::toProjectPartContainer, this, _1));

    std::sort(projectPartContainers.begin(), projectPartContainers.end());

    return projectPartContainers;
}

ClangBackEnd::FilePaths ProjectUpdater::createExcludedPaths(
        const ClangBackEnd::V2::FileContainers &generatedFiles)
{
    ClangBackEnd::FilePaths excludedPaths;
    excludedPaths.reserve(generatedFiles.size());

    auto convertToPath = [] (const ClangBackEnd::V2::FileContainer &fileContainer) {
        return fileContainer.filePath;
    };

    std::transform(generatedFiles.begin(),
                   generatedFiles.end(),
                   std::back_inserter(excludedPaths),
                   convertToPath);

    std::sort(excludedPaths.begin(), excludedPaths.end());

    return excludedPaths;
}

QString ProjectUpdater::fetchProjectPartName(ClangBackEnd::ProjectPartId projectPartId) const
{
    return QString(m_projectPartIdCache.string(projectPartId.projectPathId,
                                               [&](ClangBackEnd::ProjectPartId projectPartId) {
                                                   return m_projectPartsStorage.fetchProjectPartName(
                                                       projectPartId);
                                               }));
}

ClangBackEnd::ProjectPartIds ProjectUpdater::toProjectPartIds(
    const QStringList &projectPartNamesAsQString) const
{
    ClangBackEnd::ProjectPartIds projectPartIds;
    projectPartIds.reserve(projectPartIds.size());

    auto projectPartNames = Utils::transform<std::vector<Utils::PathString>>(
        projectPartNamesAsQString, [&](const QString &projectPartName) { return projectPartName; });

    return m_projectPartIdCache.stringIds(projectPartNames, [&](Utils::SmallStringView projectPartName) {
        return m_projectPartsStorage.fetchProjectPartId(projectPartName);
    });
}

void ProjectUpdater::addProjectFilesToFilePathCache(const std::vector<CppTools::ProjectPart *> &projectParts)
{
    ClangBackEnd::FilePaths filePaths;
    filePaths.reserve(10000);

    for (CppTools::ProjectPart *projectPart : projectParts) {
        for (const CppTools::ProjectFile &file : projectPart->files)
            if (file.active)
                filePaths.emplace_back(file.path);
    }

    m_filePathCache.addFilePaths(filePaths);
}

void ProjectUpdater::fetchProjectPartIds(const std::vector<CppTools::ProjectPart *> &projectParts)
{
    std::unique_ptr<Sqlite::DeferredTransaction> transaction;

    auto projectPartNames = Utils::transform<std::vector<Utils::PathString>>(
        projectParts, [](CppTools::ProjectPart *projectPart) { return projectPart->id(); });

    auto projectPartNameViews = Utils::transform<std::vector<Utils::SmallStringView>>(
        projectPartNames,
        [](const Utils::PathString &projectPartName) { return projectPartName.toStringView(); });

    m_projectPartIdCache
        .addStrings(std::move(projectPartNameViews), [&](Utils::SmallStringView projectPartName) {
            if (!transaction)
                transaction = std::make_unique<Sqlite::DeferredTransaction>(
                    m_projectPartsStorage.transactionBackend());
            return m_projectPartsStorage.fetchProjectPartIdUnguarded(projectPartName);
        });

    if (transaction)
        transaction->commit();

} // namespace ClangPchManager

} // namespace ClangPchManager
