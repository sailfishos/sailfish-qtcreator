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

#include "googletest.h"

#include <sourcelocations.h>

#include <filepathstoragesources.h>
#include <pchpaths.h>
#include <projectpartartefact.h>
#include <projectpartcontainer.h>
#include <projectpartpch.h>
#include <projectpartstoragestructs.h>
#include <sourceentry.h>
#include <stringcachefwd.h>
#include <symbol.h>
#include <usedmacro.h>

#include <cpptools/usages.h>

#include <utils/optional.h>
#include <utils/smallstring.h>

#include <cstdint>
#include <tuple>
#include <vector>

using ClangBackEnd::FilePathIds;
using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceEntry;
using ClangBackEnd::SourceTimeStamp;
using ClangBackEnd::SourceTimeStamps;
using ClangRefactoring::SourceLocation;
using ClangRefactoring::SourceLocations;
using std::int64_t;
namespace Sources = ClangBackEnd::Sources;
using ClangBackEnd::PrecompiledHeaderTimeStamps;
using ClangBackEnd::UsedMacros;
using ClangBackEnd::Internal::ProjectPartNameId;
using ClangBackEnd::Internal::ProjectPartNameIds;
using ClangRefactoring::Symbol;
using ClangRefactoring::Symbols;

class MockSqliteDatabase;

class MockSqliteReadStatement
{
public:
    MockSqliteReadStatement() = default;
    MockSqliteReadStatement(Utils::SmallStringView sqlStatement, MockSqliteDatabase &)
        : sqlStatement(sqlStatement)
    {}

    MOCK_METHOD4(valuesReturnSourceLocations,
                 SourceLocations(std::size_t, int, int, int));

    MOCK_METHOD4(valuesReturnSourceUsages, CppTools::Usages(std::size_t, int, int, int));

    MOCK_METHOD5(valuesReturnSourceUsages, CppTools::Usages(std::size_t, int, int, int, int));

    MOCK_METHOD1(valuesReturnStdVectorDirectory,
                 std::vector<Sources::Directory>(std::size_t));

    MOCK_METHOD1(valuesReturnStdVectorSource,
                 std::vector<Sources::Source>(std::size_t));

    MOCK_METHOD3(valuesReturnSourceEntries,
                 SourceEntries(std::size_t, int, int));

    MOCK_METHOD2(valuesReturnUsedMacros, UsedMacros(std::size_t, int));

    MOCK_METHOD2(valuesReturnFilePathIds, FilePathIds(std::size_t, int));

    MOCK_METHOD1(valuesReturnProjectPartNameIds, ProjectPartNameIds(std::size_t));

    MOCK_METHOD1(valueReturnInt32, Utils::optional<int>(Utils::SmallStringView));

    MOCK_METHOD2(valueReturnInt32, Utils::optional<int>(int, Utils::SmallStringView));

    MOCK_METHOD1(valueReturnInt32, Utils::optional<int>(int));

    MOCK_METHOD1(valueReturnInt64, Utils::optional<long long>(int));

    MOCK_METHOD1(valueReturnPathString,
                 Utils::optional<Utils::PathString>(int));

    MOCK_METHOD1(valueReturnPathString,
                 Utils::optional<Utils::PathString>(Utils::SmallStringView));

    MOCK_METHOD1(valueReturnFilePath, Utils::optional<ClangBackEnd::FilePath>(int));

    MOCK_METHOD1(valueReturnSmallString,
                 Utils::optional<Utils::SmallString>(int));

    MOCK_METHOD1(valueReturnSourceNameAndDirectoryId,
                 Utils::optional<Sources::SourceNameAndDirectoryId>(int));

    MOCK_METHOD1(valueReturnProjectPartArtefact,
                 Utils::optional<ClangBackEnd::ProjectPartArtefact>(int));

    MOCK_METHOD1(valueReturnProjectPartArtefact,
                 Utils::optional<ClangBackEnd::ProjectPartArtefact>(Utils::SmallStringView));
    MOCK_METHOD1(valuesReturnProjectPartArtefacts, ClangBackEnd::ProjectPartArtefacts(std::size_t));
    MOCK_METHOD1(valueReturnProjectPartContainer,
                 Utils::optional<ClangBackEnd::ProjectPartContainer>(int));
    MOCK_METHOD1(valuesReturnProjectPartContainers, ClangBackEnd::ProjectPartContainers(std::size_t));
    MOCK_METHOD1(valueReturnProjectPartPch, Utils::optional<ClangBackEnd::ProjectPartPch>(int));

    MOCK_METHOD1(valueReturnPchPaths, Utils::optional<ClangBackEnd::PchPaths>(int));

    MOCK_METHOD3(valuesReturnSymbols, Symbols(std::size_t, int, Utils::SmallStringView));

    MOCK_METHOD4(valuesReturnSymbols,
                 Symbols(std::size_t, int, int, Utils::SmallStringView));

    MOCK_METHOD5(valuesReturnSymbols,
                 Symbols(std::size_t, int, int, int, Utils::SmallStringView));

    MOCK_METHOD2(valueReturnSourceLocation,
                 SourceLocation(long long, int));

    MOCK_METHOD1(valueReturnProjectPartId,
                 Utils::optional<ClangBackEnd::ProjectPartId>(Utils::SmallStringView));

    MOCK_METHOD1(valuesReturnSourceTimeStamps, SourceTimeStamps(std::size_t));
    MOCK_METHOD2(valuesReturnSourceTimeStamps, SourceTimeStamps(std::size_t, int sourcePathId));

    MOCK_METHOD1(valuesReturnPrecompiledHeaderTimeStamps,
                 PrecompiledHeaderTimeStamps(int projectPartId));

    template<typename ResultType, int ResultTypeCount = 1, typename... QueryType>
    std::vector<ResultType> values(std::size_t reserveSize, const QueryType &... queryValues);

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryType>
    std::vector<ResultType> values(std::size_t reserveSize);

    template <typename ResultType,
              int ResultTypeCount = 1,
              template <typename...> class QueryContainerType,
              typename QueryElementType>
    std::vector<ResultType> values(std::size_t reserveSize,
                                   const QueryContainerType<QueryElementType> &queryValues);

    template <typename ResultType,
              int ResultTypeCount = 1,
              typename... QueryTypes>
    Utils::optional<ResultType> value(const QueryTypes&... queryValues);

public:
    Utils::SmallString sqlStatement;
};

template <>
SourceLocations
MockSqliteReadStatement::values<SourceLocation, 3>(
        std::size_t reserveSize,
        const int &sourceId,
        const int &line,
        const int &column);

template <>
CppTools::Usages
MockSqliteReadStatement::values<CppTools::Usage, 3>(
        std::size_t reserveSize,
        const int &sourceId,
        const int &line,
        const int &column);

template<>
CppTools::Usages MockSqliteReadStatement::values<CppTools::Usage, 3>(std::size_t reserveSize,
                                                                     const int &sourceId,
                                                                     const int &line,
                                                                     const int &column,
                                                                     const int &locationKind);

template <>
Symbols
MockSqliteReadStatement::values<Symbol, 3>(
        std::size_t reserveSize,
        const int&,
        const Utils::SmallStringView&);

template <>
Symbols
MockSqliteReadStatement::values<Symbol, 3>(
        std::size_t reserveSize,
        const int&,
        const int&,
        const Utils::SmallStringView&);

template <>
Symbols
MockSqliteReadStatement::values<Symbol, 3>(
        std::size_t reserveSize,
        const int&,
        const int&,
        const int&,
        const Utils::SmallStringView&);

template <>
UsedMacros
MockSqliteReadStatement::values<ClangBackEnd::UsedMacro, 2>(
        std::size_t reserveSize,
        const int &sourceId);

template<>
FilePathIds MockSqliteReadStatement::values<ClangBackEnd::FilePathId>(std::size_t reserveSize,
                                                                      const int &projectPartId);

template <>
std::vector<Sources::Directory> MockSqliteReadStatement::values<Sources::Directory, 2>(std::size_t reserveSize);

template<>
std::vector<Sources::Source> MockSqliteReadStatement::values<Sources::Source, 3>(std::size_t reserveSize);

template<>
ProjectPartNameIds MockSqliteReadStatement::values<ProjectPartNameId, 2>(std::size_t reserveSize);

template<>
Utils::optional<int> MockSqliteReadStatement::value<int>(const Utils::SmallStringView &);

template <>
Utils::optional<int>
MockSqliteReadStatement::value<int>(const Utils::PathString&);

template<>
Utils::optional<ClangBackEnd::ProjectPartId> MockSqliteReadStatement::value<ClangBackEnd::ProjectPartId>(
    const Utils::SmallStringView &);

template<>
Utils::optional<ClangBackEnd::FilePath> MockSqliteReadStatement::value<ClangBackEnd::FilePath>(
    const int &);

template <>
Utils::optional<int>
MockSqliteReadStatement::value<int>(const int&, const Utils::SmallStringView&);

template<>
Utils::optional<int> MockSqliteReadStatement::value<int>(const int &);

template <>
Utils::optional<long long>
MockSqliteReadStatement::value<long long>(const ClangBackEnd::FilePathId&);

template <>
Utils::optional<Utils::PathString>
MockSqliteReadStatement::value<Utils::PathString>(const int&);

template <>
Utils::optional<Utils::PathString>
MockSqliteReadStatement::value<Utils::PathString>(const Utils::SmallStringView&);

template <>
Utils::optional<ClangBackEnd::ProjectPartArtefact>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartArtefact, 8>(const int&);

template <>
Utils::optional<ClangBackEnd::ProjectPartArtefact>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartArtefact, 8>(const int&);

template<>
Utils::optional<ClangBackEnd::ProjectPartContainer>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartContainer, 8>(const int &);

template<>
Utils::optional<ClangBackEnd::PchPaths> MockSqliteReadStatement::value<ClangBackEnd::PchPaths, 2>(
    const int &);

template<>
ClangBackEnd::ProjectPartContainers MockSqliteReadStatement::values<ClangBackEnd::ProjectPartContainer,
                                                                    8>(std::size_t reserveSize);

template<>
Utils::optional<ClangBackEnd::ProjectPartPch>
MockSqliteReadStatement::value<ClangBackEnd::ProjectPartPch, 3>(const int &);

template <>
Utils::optional<Utils::SmallString>
MockSqliteReadStatement::value<Utils::SmallString>(const int&);

template <>
Utils::optional<SourceLocation>
MockSqliteReadStatement::value<SourceLocation, 3>(const long long &symbolId, const int &locationKind);

template<>
SourceEntries MockSqliteReadStatement::values<SourceEntry, 4>(std::size_t reserveSize,
                                                              const int &,
                                                              const int &);

template<>
SourceTimeStamps MockSqliteReadStatement::values<SourceTimeStamp, 2>(std::size_t reserveSize);

template<>
SourceTimeStamps MockSqliteReadStatement::values<SourceTimeStamp, 2>(std::size_t reserveSize,
                                                                     const int &sourcePathId);

template <>
Utils::optional<Sources::SourceNameAndDirectoryId>
MockSqliteReadStatement::value<Sources::SourceNameAndDirectoryId, 2>(const int&);

template<>
Utils::optional<ClangBackEnd::PrecompiledHeaderTimeStamps>
MockSqliteReadStatement::value<ClangBackEnd::PrecompiledHeaderTimeStamps, 2>(const int &);
