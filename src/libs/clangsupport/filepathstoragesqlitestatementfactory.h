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

#include <sqlitetransaction.h>
#include <sqlitetable.h>

namespace ClangBackEnd {

template<typename DatabaseType>
class FilePathStorageSqliteStatementFactory
{
public:
    using Database = DatabaseType;
    template<int ResultCount>
    using ReadStatement = typename Database::template ReadStatement<ResultCount>;
    using WriteStatement = typename Database::WriteStatement;

    FilePathStorageSqliteStatementFactory(Database &database)
        : database(database)
    {
    }

public:
    Database &database;
    ReadStatement<1> selectDirectoryIdFromDirectoriesByDirectoryPath{
        "SELECT directoryId FROM directories WHERE directoryPath = ?", database};
    ReadStatement<1> selectDirectoryPathFromDirectoriesByDirectoryId{
        "SELECT directoryPath FROM directories WHERE directoryId = ?", database};
    ReadStatement<2> selectAllDirectories{"SELECT directoryPath, directoryId FROM directories",
                                          database};
    WriteStatement insertIntoDirectories{
      "INSERT INTO directories(directoryPath) VALUES (?)",
        database
    };
    ReadStatement<1> selectSourceIdFromSourcesByDirectoryIdAndSourceName{
        "SELECT sourceId FROM sources WHERE directoryId = ? AND sourceName = ?", database};
    ReadStatement<2> selectSourceNameAndDirectoryIdFromSourcesBySourceId{
        "SELECT sourceName, directoryId FROM sources WHERE sourceId = ?", database};
    ReadStatement<1> selectDirectoryIdFromSourcesBySourceId{
        "SELECT directoryId FROM sources WHERE sourceId = ?", database};
    WriteStatement insertIntoSources{
      "INSERT INTO sources(directoryId, sourceName) VALUES (?,?)",
        database
    };
    ReadStatement<3> selectAllSources{"SELECT sourceName, directoryId, sourceId  FROM sources",
                                      database};
};

} // namespace ClangBackEnd

