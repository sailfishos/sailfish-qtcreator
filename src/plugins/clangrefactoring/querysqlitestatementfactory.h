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

namespace ClangRefactoring {

template<typename Database>
class QuerySqliteStatementFactory
{
public:
    using DatabaseType = Database;
    template<int ResultCount>
    using ReadStatement = typename Database::template ReadStatement<ResultCount>;

    QuerySqliteStatementFactory(Database &database)
        : database(database)
    {}
    Database &database;
    ReadStatement<3> selectLocationsForSymbolLocation{
        "SELECT sourceId, line, column FROM locations WHERE symbolId = "
        "  (SELECT symbolId FROM locations WHERE sourceId=? AND line=? AND column=?) "
        "ORDER BY sourceId, line, column",
        database};
    ReadStatement<3> selectSourceUsagesForSymbolLocation{
        "SELECT directoryPath || '/' || sourceName, line, column "
        "FROM locations NATURAL JOIN sources NATURAL JOIN directories "
        "WHERE symbolId = (SELECT symbolId FROM locations WHERE sourceId=? AND line=? AND "
        "column=?)",
        database};
    ReadStatement<3> selectSourceUsagesOrderedForSymbolLocation{
        "SELECT directoryPath || '/' || sourceName, line, column "
        "FROM locations NATURAL JOIN sources NATURAL JOIN directories "
        "WHERE symbolId = (SELECT symbolId FROM locations WHERE sourceId=? AND line=? AND "
        "column=?) ORDER BY locationKind LIMIT 2",
        database};
    ReadStatement<3> selectSourceUsagesByLocationKindForSymbolLocation{
        "SELECT directoryPath || '/' || sourceName, line, column "
        "FROM locations NATURAL JOIN sources NATURAL JOIN directories "
        "WHERE symbolId = (SELECT symbolId FROM locations WHERE sourceId=? AND line=? AND "
        "column=?) AND locationKind = ?",
        database};
    ReadStatement<3> selectSymbolsForKindAndStartsWith{
        "SELECT symbolId, symbolName, signature FROM symbols WHERE symbolKind = ? AND symbolName "
        "LIKE ?",
        database};
    ReadStatement<3> selectSymbolsForKindAndStartsWith2{
        "SELECT symbolId, symbolName, signature FROM symbols WHERE symbolKind IN (?,?) AND "
        "symbolName LIKE ?",
        database};
    ReadStatement<3> selectSymbolsForKindAndStartsWith3{
        "SELECT symbolId, symbolName, signature FROM symbols WHERE symbolKind IN (?,?,?) AND "
        "symbolName LIKE ?",
        database};
    ReadStatement<3> selectLocationOfSymbol{
        "SELECT sourceId, line, column FROM locations AS l WHERE symbolId = ? AND locationKind = ?",
        database};
};

} // namespace ClangRefactoring
