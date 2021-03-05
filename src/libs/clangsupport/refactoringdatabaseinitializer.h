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

#include <createtablesqlstatementbuilder.h>

#include <sqlitetransaction.h>
#include <sqlitetable.h>

namespace ClangBackEnd {

template<typename DatabaseType>
class RefactoringDatabaseInitializer
{
public:
    RefactoringDatabaseInitializer(DatabaseType &database)
        : database(database)
    {
        if (!database.isInitialized()) {
            Sqlite::ExclusiveTransaction transaction{database};

            createSymbolsTable();
            createLocationsTable();
            createSourcesTable();
            createDirectoriesTable();
            createProjectPartsTable();
            createProjectPartsFilesTable();
            createUsedMacrosTable();
            createFileStatusesTable();
            createSourceDependenciesTable();
            createPrecompiledHeadersTable();
            createProjectPartsHeadersTable();
            createProjectPartsSourcesTable();

            transaction.commit();

            database.setIsInitialized(true);
        }
    }

    void createSymbolsTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("symbols");
        table.addColumn("symbolId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        const Sqlite::Column &usrColumn = table.addColumn("usr", Sqlite::ColumnType::Text);
        const Sqlite::Column &symbolNameColumn = table.addColumn("symbolName", Sqlite::ColumnType::Text);
        const Sqlite::Column &symbolKindColumn = table.addColumn("symbolKind", Sqlite::ColumnType::Integer);
        table.addColumn("signature", Sqlite::ColumnType::Text);
        table.addIndex({usrColumn});
        table.addIndex({symbolKindColumn, symbolNameColumn});

        table.initialize(database);
    }

    void createLocationsTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("locations");
        const Sqlite::Column &symbolIdColumn = table.addColumn("symbolId",
                                                               Sqlite::ColumnType::Integer);
        const Sqlite::Column &lineColumn = table.addColumn("line", Sqlite::ColumnType::Integer);
        const Sqlite::Column &columnColumn = table.addColumn("column", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &locationKindColumn = table.addColumn("locationKind", Sqlite::ColumnType::Integer);
        table.addUniqueIndex({sourceIdColumn, lineColumn, columnColumn});
        table.addIndex({sourceIdColumn, locationKindColumn});
        table.addIndex({symbolIdColumn});

        table.initialize(database);
    }

    void createSourcesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("sources");
        table.addColumn("sourceId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        const Sqlite::Column &directoryIdColumn = table.addColumn("directoryId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceNameColumn = table.addColumn("sourceName", Sqlite::ColumnType::Text);
        table.addUniqueIndex({directoryIdColumn, sourceNameColumn});

        table.initialize(database);
    }

    void createDirectoriesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("directories");
        table.addColumn("directoryId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        const Sqlite::Column &directoryPathColumn = table.addColumn("directoryPath", Sqlite::ColumnType::Text);
        table.addUniqueIndex({directoryPathColumn});

        table.initialize(database);
    }

    void createProjectPartsTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("projectParts");
        table.addColumn("projectPartId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        const Sqlite::Column &projectPartNameColumn = table.addColumn("projectPartName", Sqlite::ColumnType::Text);
        table.addColumn("toolChainArguments", Sqlite::ColumnType::Text);
        table.addColumn("compilerMacros", Sqlite::ColumnType::Text);
        table.addColumn("systemIncludeSearchPaths", Sqlite::ColumnType::Text);
        table.addColumn("projectIncludeSearchPaths", Sqlite::ColumnType::Text);
        table.addColumn("language", Sqlite::ColumnType::Integer);
        table.addColumn("languageVersion", Sqlite::ColumnType::Integer);
        table.addColumn("languageExtension", Sqlite::ColumnType::Integer);
        table.addUniqueIndex({projectPartNameColumn});

        table.initialize(database);
    }

    void createProjectPartsFilesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("projectPartsFiles");
        const Sqlite::Column &projectPartIdColumn = table.addColumn("projectPartId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &sourceType = table.addColumn("sourceType", Sqlite::ColumnType::Integer);
        table.addColumn("pchCreationTimeStamp", Sqlite::ColumnType::Integer);
        table.addColumn("hasMissingIncludes", Sqlite::ColumnType::Integer);
        table.addUniqueIndex({sourceIdColumn, projectPartIdColumn});
        table.addIndex({projectPartIdColumn, sourceType});

        table.initialize(database);
    }

    void createUsedMacrosTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("usedMacros");
        table.addColumn("usedMacroId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &macroNameColumn = table.addColumn("macroName", Sqlite::ColumnType::Text);
        table.addIndex({sourceIdColumn, macroNameColumn});
        table.addIndex({macroNameColumn});

        table.initialize(database);
    }

    void createFileStatusesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("fileStatuses");
        table.addColumn("sourceId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        table.addColumn("size", Sqlite::ColumnType::Integer);
        table.addColumn("lastModified", Sqlite::ColumnType::Integer);
        table.addColumn("indexingTimeStamp", Sqlite::ColumnType::Integer);
        table.initialize(database);
    }

    void createSourceDependenciesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("sourceDependencies");
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &dependencySourceIdColumn = table.addColumn("dependencySourceId", Sqlite::ColumnType::Integer);
        table.addIndex({sourceIdColumn, dependencySourceIdColumn});
        table.addIndex({dependencySourceIdColumn, sourceIdColumn});

        table.initialize(database);
    }

    void createPrecompiledHeadersTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("precompiledHeaders");
        table.addColumn("projectPartId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        table.addColumn("projectPchPath", Sqlite::ColumnType::Text);
        table.addColumn("projectPchBuildTime", Sqlite::ColumnType::Integer);
        table.addColumn("systemPchPath", Sqlite::ColumnType::Text);
        table.addColumn("systemPchBuildTime", Sqlite::ColumnType::Integer);
        table.initialize(database);
    }

    void createProjectPartsHeadersTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("projectPartsHeaders");
        const Sqlite::Column &projectPartIdColumn = table.addColumn("projectPartId",
                                                                    Sqlite::ColumnType::Integer);
        table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        table.addIndex({projectPartIdColumn});

        table.initialize(database);
    }

    void createProjectPartsSourcesTable()
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("projectPartsSources");
        const Sqlite::Column &projectPartIdColumn = table.addColumn("projectPartId",
                                                                    Sqlite::ColumnType::Integer);
        table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        table.addIndex({projectPartIdColumn});

        table.initialize(database);
    }

public:
    DatabaseType &database;
};

} // namespace ClangBackEnd
