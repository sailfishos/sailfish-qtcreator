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

#include <sfdk/sdk.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/task.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

OutputLineParser::Result MerSshParser::handleLine(const QString &line, OutputFormat type)
{
    if (type == StdOutFormat)
        return Status::NotHandled;

    QString lne(line.trimmed());
    if (lne.startsWith(QLatin1String("Project ERROR:"))) {
        const QString description = lne.mid(15);
        scheduleTask(Task(Task::Error,
                          description,
                          FilePath(),
                          -1 /* linenumber */,
                          Utils::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)),
                1);
        return Status::Done;
    }

    return Status::NotHandled;
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
using namespace Sfdk;

void MerPlugin::testMerSshOutputParsers_data()
{
    const Utils::Id categoryBuildSystem = Utils::Id(Constants::TASK_CATEGORY_BUILDSYSTEM);
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
            << tr("Project ERROR: %1 build engine is not running.").arg(Sdk::osVariant())
            << OutputParserTester::STDERR
            << QString() << QString()
            << (QList<Task>()
                << Task(Task::Error,
                        tr("%1 build engine is not running.").arg(Sdk::osVariant()),
                        FilePath(), -1,
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
