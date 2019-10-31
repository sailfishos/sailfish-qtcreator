/****************************************************************************
**
** Copyright (C) 2012-2015,2017 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "mersshparser.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

void MerSshParser::stdError(const QString &line)
{
    QString lne(line.trimmed());
    if (lne.startsWith(QLatin1String("Project ERROR:"))) {
        const QString description = lne.mid(15);
        emit addTask(Task(Task::Error,
                          description,
                          FileName() /* filename */,
                          -1 /* linenumber */,
                          Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return;
    }
    IOutputParser::stdError(line);
}

} // Internal
} // Mer

// Unit tests:

#ifdef WITH_TESTS
#   include "merplugin.h"

#   include <projectexplorer/outputparser_test.h>

#   include <QTest>

using namespace ProjectExplorer;
using namespace Mer::Internal;

void MerPlugin::testMerSshOutputParsers_data()
{
    const Core::Id categoryBuildSystem = Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM);
    QTest::addColumn<QString>("input");
    QTest::addColumn<OutputParserTester::Channel>("inputChannel");
    QTest::addColumn<QString>("childStdOutLines");
    QTest::addColumn<QString>("childStdErrLines");
    QTest::addColumn<QList<Task> >("tasks");
    QTest::addColumn<QString>("outputLines");


    QTest::newRow("pass-through stdout")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDOUT
            << QString::fromLatin1("Sometext\n") << QString()
            << QList<Task>()
            << QString();
    QTest::newRow("pass-through stderr")
            << QString::fromLatin1("Sometext") << OutputParserTester::STDERR
            << QString() << QString::fromLatin1("Sometext\n")
            << QList<Task>()
            << QString();

    QTest::newRow("merssh error")
            << QString::fromLatin1("Project ERROR: Sailfish OS build engine is not running.")
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        QLatin1String("Sailfish OS build engine is not running."),
                        FileName(), -1,
                        categoryBuildSystem))
            << QString();
}

void MerPlugin::testMerSshOutputParsers()
{
    OutputParserTester testbench;
    testbench.appendOutputParser(new MerSshParser);
    QFETCH(QString, input);
    QFETCH(OutputParserTester::Channel, inputChannel);
    QFETCH(QList<Task>, tasks);
    QFETCH(QString, childStdOutLines);
    QFETCH(QString, childStdErrLines);
    QFETCH(QString, outputLines);

    testbench.testParsing(input, inputChannel,
                          tasks, childStdOutLines, childStdErrLines,
                          outputLines);
}
#endif
