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

#include "googletest.h"

#include "mocksqlitedatabase.h"
#include "mocksqlitereadstatement.h"

#include <querysqlitestatementfactory.h>
#include <refactoringdatabaseinitializer.h>
#include <symbolquery.h>

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using ClangRefactoring::QuerySqliteStatementFactory;
using Sqlite::Database;

using MockStatementFactory = QuerySqliteStatementFactory<MockSqliteDatabase,
                                                         MockSqliteReadStatement>;
using MockQuery = ClangRefactoring::SymbolQuery<MockStatementFactory>;

using RealStatementFactory = QuerySqliteStatementFactory<Sqlite::Database,
                                                         Sqlite::ReadStatement>;
using RealQuery = ClangRefactoring::SymbolQuery<RealStatementFactory>;

class SymbolQuery : public testing::Test
{
protected:
    void SetUp() override
    {
        database.execute("INSERT INTO sources VALUES (1, 1, \"filename.h\", 1)");
        database.execute("INSERT INTO sources VALUES (2, 1, \"filename.cpp\", 1)");
        database.execute("INSERT INTO directories VALUES (1, \"/path/to\")");
        database.execute("INSERT INTO locations VALUES (1, 2, 3, 1)");
        database.execute("INSERT INTO locations VALUES (1, 4, 6, 2)");
        database.execute("INSERT INTO symbols VALUES (1, \"functionusr\", \"function\")");
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    RealStatementFactory realStatementFactory{database};
    RealQuery realQuery{realStatementFactory};
    NiceMock<MockSqliteDatabase> mockDatabase;
    MockStatementFactory mockStatementFactory{mockDatabase};
    MockSqliteReadStatement &selectLocationsForSymbolLocation = mockStatementFactory.selectLocationsForSymbolLocation;
    MockSqliteReadStatement &selectSourceUsagesForSymbolLocation = mockStatementFactory.selectSourceUsagesForSymbolLocation;
    SourceLocations locations{{{1, 1}, 1, 1},
                              {{1, 1}, 2, 3},
                              {{1, 2}, 1, 1},
                              {{1, 2}, 3, 1},
                              {{1, 4}, 1, 1},
                              {{1, 4}, 1, 3}};
    MockQuery mockQuery{mockStatementFactory};
};

TEST_F(SymbolQuery, LocationsAtCallsValues)
{
    EXPECT_CALL(selectLocationsForSymbolLocation, valuesReturnSourceLocations(_, 42, 14, 7));

    mockQuery.locationsAt({1, 42}, 14, 7);
}

TEST_F(SymbolQuery, LocationsAt)
{
    auto locations = realQuery.locationsAt({1, 2}, 4, 6);

    ASSERT_THAT(locations,
                UnorderedElementsAre(SourceLocation({1, 1}, 2, 3),
                                     SourceLocation({1, 2}, 4, 6)));
}

TEST_F(SymbolQuery, SourceUsagesAtCallsValues)
{
    EXPECT_CALL(selectSourceUsagesForSymbolLocation, valuesReturnSourceUsages(_, 42, 14, 7));

    mockQuery.sourceUsagesAt({1, 42}, 14, 7);
}

TEST_F(SymbolQuery, SourceUsagesAt)
{
    auto usages = realQuery.sourceUsagesAt({1, 2}, 4, 6);

    ASSERT_THAT(usages,
                UnorderedElementsAre(CppTools::Usage("/path/to/filename.h", 2, 3),
                                     CppTools::Usage("/path/to/filename.cpp", 4, 6)));
}

}
