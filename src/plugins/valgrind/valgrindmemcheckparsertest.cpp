/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#include "valgrindmemcheckparsertest.h"

#include "xmlprotocol/frame.h"
#include "xmlprotocol/parser.h"
#include "xmlprotocol/stack.h"
#include "xmlprotocol/suppression.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runnables.h>

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalSpy>

using namespace Valgrind::XmlProtocol;

QT_BEGIN_NAMESPACE
namespace QTest {

template<>
inline bool qCompare(int const &t1, MemcheckErrorKind const &t2,
                     char const *actual, char const *expected, char const *file, int line)
{
    return qCompare(t1, int(t2), actual, expected, file, line);
}

} // namespace QTest
QT_END_NAMESPACE

namespace Valgrind {
namespace Test {

static void dumpFrame(const Frame &f)
{
    qDebug() << f.instructionPointer() << f.directory() << f.fileName() << f.functionName()
             << f.line() << f.object();
}

void dumpError(const Error &e)
{
    qDebug() << e.kind() << e.leakedBlocks() << e.leakedBytes() << e.what() << e.tid() << e.unique();
    qDebug() << "stacks:" << e.stacks().size();
    for (const Stack &s : e.stacks()) {
        qDebug() << s.auxWhat() << s.directory() << s.file() << s.line() << s.helgrindThreadId();
        qDebug() << "frames:";
        for (const Frame &f : s.frames()) {
            dumpFrame(f);
        }
    }
}

static QString fakeValgrindExecutable()
{
    QString valgrindFakePath(VALGRIND_FAKE_PATH);
    if (Utils::HostOsInfo::isWindowsHost()) {
        QFileInfo fi(QString(valgrindFakePath + "/debug"), "valgrind-fake.exe");
        if (fi.exists())
            return fi.canonicalFilePath();
        fi = QFileInfo(QString(valgrindFakePath + "/release"), "valgrind-fake.exe");
        if (fi.exists())
            return fi.canonicalFilePath();
        // Qbs uses the install-root/bin
        fi = QFileInfo(valgrindFakePath, "valgrind-fake.exe");
        if (fi.exists())
            return fi.canonicalFilePath();
        qFatal("Neither debug nor release build valgrind-fake found.");
    }
    return valgrindFakePath + "/valgrind-fake";
}

static QString dataFile(const QLatin1String &file)
{
    return QLatin1String(PARSERTESTS_DATA_DIR) + QLatin1String("/") + file;
}

void ValgrindMemcheckParserTest::initTestCase()
{
    m_server = new QTcpServer(this);
    QVERIFY(m_server->listen());

    m_socket = 0;
    m_process = 0;
}

void ValgrindMemcheckParserTest::initTest(const QLatin1String &testfile, const QStringList &otherArgs)
{
    QVERIFY(!m_server->hasPendingConnections());

    m_process = new QProcess(m_server);
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    const QString fakeValgrind = fakeValgrindExecutable();
    QFileInfo fileInfo(fakeValgrind);
    QVERIFY2(fileInfo.isExecutable(), qPrintable(fakeValgrind));
    QVERIFY2(!fileInfo.isDir(), qPrintable(fakeValgrind));

    m_process->start(
        fakeValgrind,
        QStringList()
            << QString::fromLatin1("--xml-socket=127.0.0.1:%1").arg(m_server->serverPort())
            << QLatin1String("-i")
            << dataFile(testfile)
            << otherArgs
    );

    QVERIFY(m_process->waitForStarted(5000));
    QCOMPARE(m_process->state(), QProcess::Running);
    QVERIFY2(m_process->error() == QProcess::UnknownError, qPrintable(m_process->errorString()));
    QVERIFY(m_server->waitForNewConnection(5000));
    m_socket = m_server->nextPendingConnection();
    QVERIFY(m_socket);
}

void ValgrindMemcheckParserTest::cleanup()
{
    if (m_socket) {
        delete m_socket;
        m_socket = 0;
    }
    if (m_process) {
        delete m_process;
        m_process = 0;
    }
}

void ValgrindMemcheckParserTest::testHelgrindSample1()
{
    QSKIP("testfile does not exist");

    initTest(QLatin1String("helgrind-output-sample1.xml"));

    QList<Error> expectedErrors;
    {
        Error error1;
        error1.setUnique(0x0);
        error1.setTid(1);
        error1.setKind(LockOrder);
        error1.setWhat(QLatin1String("Thread #1: lock order \"0xA39C270 before 0xA3AC010\" violated"));
        error1.setHelgrindThreadId(1);
        Stack stack1;
        Frame frame11;
        frame11.setInstructionPointer(0x4C2B806);
        frame11.setObject(QLatin1String("/usr/lib/valgrind/vgpreload_helgrind-amd64-linux.so"));
        frame11.setFunctionName(QLatin1String("QMutex::lock()"));
        frame11.setDirectory(QLatin1String("/build/buildd/valgrind-3.6.0~svn20100212/helgrind"));
        frame11.setFileName(QLatin1String("hg_intercepts.c"));
        frame11.setLine(1988);
        Frame frame12;
        frame12.setInstructionPointer(0x72E57EE);
        frame12.setObject(QLatin1String("/home/frank/local/qt4-4.6.3-shared-debug/lib/libQtCore.so.4.6.3"));
        frame12.setFunctionName(QLatin1String("QMutexLocker::relock()"));
        frame12.setDirectory(QLatin1String("/home/frank/source/tarballs/qt-4.6.3-build/src/corelib/../../include/QtCore/../../src/corelib/thread"));
        frame12.setFileName(QLatin1String("qmutex.h"));
        frame12.setLine(120);
        stack1.setFrames(QVector<Frame>() << frame11 << frame12);

        Stack stack2;
        stack2.setAuxWhat(QLatin1String("Required order was established by acquisition of lock at 0xA39C270"));
        Frame frame21;
        frame21.setInstructionPointer(0x4C2B806);
        frame21.setObject(QLatin1String("/usr/lib/valgrind/vgpreload_helgrind-amd64-linux.so"));
        frame21.setFunctionName(QLatin1String("QMutex::lock()"));
        frame21.setDirectory(QLatin1String("/build/buildd/valgrind-3.6.0~svn20100212/helgrind"));
        frame21.setFileName(QLatin1String("hg_intercepts.c"));
        frame21.setLine(1989);
        Frame frame22;
        frame22.setInstructionPointer(0x72E57EE);
        frame22.setObject(QLatin1String("/home/frank/local/qt4-4.6.3-shared-debug/lib/libQtCore.so.4.6.3"));
        frame22.setFunctionName(QLatin1String("QMutexLocker::relock()"));
        frame22.setDirectory(QLatin1String("/home/frank/source/tarballs/qt-4.6.3-build/src/corelib/../../include/QtCore/../../src/corelib/thread"));
        frame22.setFileName(QLatin1String("qmutex.h"));
        frame22.setLine(121);
        stack2.setFrames(QVector<Frame>() << frame21 << frame22);

        Stack stack3;
        stack3.setAuxWhat(QLatin1String("followed by a later acquisition of lock at 0xA3AC010"));
        Frame frame31;
        frame31.setInstructionPointer(0x4C2B806);
        frame31.setObject(QLatin1String("/usr/lib/valgrind/vgpreload_helgrind-amd64-linux.so"));
        frame31.setFunctionName(QLatin1String("QMutex::lock()"));
        frame31.setDirectory(QLatin1String("/build/buildd/valgrind-3.6.0~svn20100212/helgrind"));
        frame31.setFileName(QLatin1String("hg_intercepts.c"));
        frame31.setLine(1990);
        Frame frame32;
        frame32.setInstructionPointer(0x72E57EE);
        frame32.setObject(QLatin1String("/home/frank/local/qt4-4.6.3-shared-debug/lib/libQtCore.so.4.6.3"));
        frame32.setFunctionName(QLatin1String("QMutexLocker::relock()"));
        frame32.setDirectory(QLatin1String("/home/frank/source/tarballs/qt-4.6.3-build/src/corelib/../../include/QtCore/../../src/corelib/thread"));
        frame32.setFileName(QLatin1String("qmutex.h"));
        frame32.setLine(122);

        stack3.setFrames(QVector<Frame>() << frame31 << frame32);
        error1.setStacks(QVector<Stack>() << stack1 << stack2 << stack3);
        expectedErrors.append(error1);
    }

    Parser parser;
    Recorder rec(&parser);

    parser.parse(m_socket);

    m_process->waitForFinished();
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
    QCOMPARE(m_process->state(), QProcess::NotRunning);

    QVERIFY2(parser.errorString().isEmpty(), qPrintable(parser.errorString()));
    const QList<Error> actualErrors = rec.errors;

    if (actualErrors.first() != expectedErrors.first()) {
        dumpError(actualErrors.first());
        dumpError(expectedErrors.first());
    }

    QCOMPARE(actualErrors.first(), expectedErrors.first());

    QCOMPARE(actualErrors.size(), 1);

//    QCOMPARE(rec.errorcounts, expectedErrorCounts);
//    QCOMPARE(rec.suppcounts, expectedSuppCounts);
}

void ValgrindMemcheckParserTest::testMemcheckSample1()
{
    initTest(QLatin1String("memcheck-output-sample1.xml"));

    QList<Error> expectedErrors;
    {
        Error error;
        error.setKind(InvalidRead);
        error.setWhat(QLatin1String("Invalid read of size 4"));
        error.setUnique(0x9);
        error.setTid(1);
        Frame f1;
        f1.setInstructionPointer(0x6E47964);
        f1.setObject(QLatin1String("/usr/lib/libQtGui.so.4.7.0"));
        f1.setFunctionName(QLatin1String("QFrame::frameStyle() const"));
        f1.setDirectory(QLatin1String("/build/buildd/qt4-x11-4.7.0/src/gui/widgets"));
        f1.setFileName(QLatin1String("qframe.cpp"));
        f1.setLine(252);
        Frame f2;
        f2.setInstructionPointer(0x118F2AF7);
        f2.setObject(QLatin1String("/usr/lib/kde4/plugins/styles/oxygen.so"));
        Frame f3;
        f3.setInstructionPointer(0x6A81671);
        f3.setObject(QLatin1String("/usr/lib/libQtGui.so.4.7.0"));
        f3.setFunctionName(QLatin1String("QWidget::event(QEvent*)"));
        f3.setDirectory(QLatin1String("/build/buildd/qt4-x11-4.7.0/src/gui/kernel"));
        f3.setFileName(QLatin1String("qwidget.cpp"));
        f3.setLine(8273);
        Frame f4;
        f4.setInstructionPointer(0x6A2B6EB);
        f4.setObject(QLatin1String("/usr/lib/libQtGui.so.4.7.0"));
        f4.setDirectory(QLatin1String("/build/buildd/qt4-x11-4.7.0/src/gui/kernel"));
        f4.setFileName(QLatin1String("qapplication.cpp"));
        f4.setFunctionName(QLatin1String("QApplicationPrivate::notify_helper(QObject*, QEvent*)"));
        f4.setLine(4396);
        Stack s1;
        s1.setAuxWhat(QLatin1String("Address 0x11527cb8 is not stack'd, malloc'd or (recently) free'd"));
        s1.setFrames(QVector<Frame>() << f1 << f2 << f3 << f4);
        error.setStacks( QVector<Stack>() << s1 );

        expectedErrors << error;
    }

    QVector<QPair<qint64,qint64> > expectedErrorCounts;
    expectedErrorCounts.push_back(QPair<qint64,qint64>(9, 2));

    QVector<QPair<QString,qint64> > expectedSuppCounts;
    expectedSuppCounts.push_back(qMakePair(QString::fromLatin1("X on SUSE11 writev uninit padding"), static_cast<qint64>(12)));
    expectedSuppCounts.push_back(qMakePair(QString::fromLatin1("dl-hack3-cond-1"), static_cast<qint64>(2)));
    expectedSuppCounts.push_back(qMakePair(QString::fromLatin1("glibc-2.5.x-on-SUSE-10.2-(PPC)-2a"), static_cast<qint64>(2)));

    Parser parser;
    Recorder rec(&parser);

    parser.parse(m_socket);

    m_process->waitForFinished();
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
    QCOMPARE(m_process->state(), QProcess::NotRunning);

    QVERIFY2(parser.errorString().isEmpty(), qPrintable(parser.errorString()));
    const QList<Error> actualErrors = rec.errors;

    if (actualErrors.first() != expectedErrors.first()) {
        dumpError(actualErrors.first());
        dumpError(expectedErrors.first());
    }

    QCOMPARE(actualErrors.first(), expectedErrors.first());

    QCOMPARE(actualErrors.size(), 3);

    QCOMPARE(rec.errorcounts, expectedErrorCounts);
    QCOMPARE(rec.suppcounts, expectedSuppCounts);
}

void ValgrindMemcheckParserTest::testMemcheckSample2()
{
    QSKIP("testfile does not exist");

    initTest(QLatin1String("memcheck-output-sample2.xml"));

    Parser parser;
    Recorder rec(&parser);

    parser.parse(m_socket);

    m_process->waitForFinished();
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
    QCOMPARE(m_process->state(), QProcess::NotRunning);
    QVERIFY2(parser.errorString().isEmpty(), qPrintable(parser.errorString()));

    //tests: multiple stacks with auxwhat == stack count - 1.
    //the first auxwhat should be assigned to the _second_ stack.
    const QList<Error> errors = rec.errors;
    QCOMPARE(errors.size(), 1);
    const QVector<Stack> stacks = errors.first().stacks();
    QCOMPARE(stacks.size(), 2);
    QCOMPARE(stacks.first().auxWhat(), QString());
    QCOMPARE(stacks.last().auxWhat(), QLatin1String("Address 0x11b66c50 is 0 bytes inside a block of size 16 free'd"));
}

void ValgrindMemcheckParserTest::testMemcheckSample3()
{
    QSKIP("testfile does not exist");

    initTest(QLatin1String("memcheck-output-sample3.xml"));

    Parser parser;
    Recorder rec(&parser);

    parser.parse(m_socket);

    m_process->waitForFinished();
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
    QCOMPARE(m_process->state(), QProcess::NotRunning);
    QVERIFY2(parser.errorString().isEmpty(), qPrintable(parser.errorString()));

    const QList<Error> errors = rec.errors;
    QCOMPARE(errors.size(), 6);

    {
        const Error error = errors.at(0);
        const QVector<Stack> stacks = error.stacks();

        QCOMPARE(error.unique(), 0x1ll);
        QCOMPARE(error.what(), QLatin1String("Conditional jump or move depends on uninitialised value(s)"));
        QCOMPARE(error.kind(), UninitCondition);
        QCOMPARE(stacks.size(), 1);
        QCOMPARE(stacks.first().frames().size(), 12);
        QVERIFY(!error.suppression().isNull());
        QCOMPARE(error.suppression().frames().count(), stacks.first().frames().size());
        QCOMPARE(error.suppression().kind(), QLatin1String("Memcheck:Cond"));
        QVERIFY(!error.suppression().rawText().trimmed().isEmpty());

        // rawtext contains <...> while <name></name> does not
        QCOMPARE(error.suppression().name(), QLatin1String("insert_a_suppression_name_here"));
        Suppression sup = error.suppression();
        sup.setName(QLatin1String("<insert_a_suppression_name_here>"));
        QCOMPARE(sup.toString().trimmed(), sup.rawText().trimmed());

        QCOMPARE(error.suppression().frames().first().object(),
                 QLatin1String("/usr/lib/kde4/plugins/styles/qtcurve.so"));
        QVERIFY(error.suppression().frames().first().function().isEmpty());
        QCOMPARE(error.suppression().frames().last().function(), QLatin1String("main"));
        QVERIFY(error.suppression().frames().last().object().isEmpty());
    }

    QCOMPARE(rec.suppcounts.count(), 3);
    QCOMPARE(rec.suppcounts.at(0).second, qint64(1));
    QCOMPARE(rec.suppcounts.at(1).second, qint64(2));
    QCOMPARE(rec.suppcounts.at(2).second, qint64(3));
}

void ValgrindMemcheckParserTest::testMemcheckCharm()
{
    QSKIP("testfile does not exist");

    // a somewhat larger file, to make sure buffering and partial I/O works ok
    initTest(QLatin1String("memcheck-output-untitled.xml"));

    Parser parser;
    Recorder rec(&parser);

    parser.parse(m_socket);

    m_process->waitForFinished();
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);
    QCOMPARE(m_process->state(), QProcess::NotRunning);

    const QList<Error> errors = rec.errors;
    QCOMPARE(errors.size(), 102);
    QVERIFY2(parser.errorString().isEmpty(), qPrintable(parser.errorString()));
}

void ValgrindMemcheckParserTest::testValgrindCrash()
{
    initTest(QLatin1String("memcheck-output-sample1.xml"), QStringList() << "--crash");

    Parser parser;
    parser.parse(m_socket);
    m_process->waitForFinished();
    QCOMPARE(m_process->state(), QProcess::NotRunning);
    QCOMPARE(m_process->exitStatus(), QProcess::CrashExit);

    QVERIFY(!parser.errorString().isEmpty());
    QCOMPARE(m_socket->error(), QAbstractSocket::RemoteHostClosedError);
    QCOMPARE(parser.errorString(), m_socket->errorString());
}

void ValgrindMemcheckParserTest::testValgrindGarbage()
{
    initTest(QLatin1String("memcheck-output-sample1.xml"), QStringList() << "--garbage");

    Parser parser;
    parser.parse(m_socket);
    m_process->waitForFinished();
    QCOMPARE(m_process->state(), QProcess::NotRunning);
    QCOMPARE(m_process->exitStatus(), QProcess::NormalExit);

    QVERIFY(!parser.errorString().isEmpty());
    qDebug() << parser.errorString();
}

void ValgrindMemcheckParserTest::testParserStop()
{
    ThreadedParser parser;
    Memcheck::MemcheckRunner runner;
    runner.setValgrindExecutable(fakeValgrindExecutable());
    runner.setParser(&parser);
    runner.setValgrindArguments(QStringList() << QLatin1String("-i")
                                      << dataFile(QLatin1String("memcheck-output-sample1.xml"))
                                      << "--wait" << "5");
    runner.setProcessChannelMode(QProcess::ForwardedChannels);

    runner.setDevice(ProjectExplorer::DeviceManager::instance()->defaultDevice(
                         Core::Id(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)));
    runner.start();
    QTest::qWait(500);
    runner.stop();
}


void ValgrindMemcheckParserTest::testRealValgrind()
{
    const Utils::Environment &sysEnv = Utils::Environment::systemEnvironment();
    auto fileName = sysEnv.searchInPath("valgrind");
    if (fileName.isEmpty())
        QSKIP("This test needs valgrind in PATH");
    QString executable = QProcessEnvironment::systemEnvironment().value("VALGRIND_TEST_BIN", fakeValgrindExecutable());
    qDebug() << "running exe:" << executable << " HINT: set VALGRIND_TEST_BIN to change this";
    ThreadedParser parser;

    ProjectExplorer::StandardRunnable debuggee;
    debuggee.executable = executable;
    debuggee.environment = sysEnv;
    Memcheck::MemcheckRunner runner;
    runner.setValgrindExecutable(QLatin1String("valgrind"));
    runner.setDebuggee(debuggee);
    runner.setDevice(ProjectExplorer::DeviceManager::instance()->defaultDevice(
                         Core::Id(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)));
    runner.setParser(&parser);
    RunnerDumper dumper(&runner, &parser);
    runner.start();
    runner.waitForFinished();
}

void ValgrindMemcheckParserTest::testValgrindStartError_data()
{
    QTest::addColumn<QString>("valgrindExe");
    QTest::addColumn<QStringList>("valgrindArgs");
    QTest::addColumn<QString>("debuggee");
    QTest::addColumn<QString>("debuggeeArgs");

    QTest::newRow("invalid_client") << QString::fromLatin1("valgrind") << QStringList()
                                    << QString::fromLatin1("please-dont-let-this-app-exist") << QString();

    QTest::newRow("invalid_valgrind") << QString::fromLatin1("valgrind-that-does-not-exist") << QStringList()
                                    << fakeValgrindExecutable() << QString();

    QTest::newRow("invalid_valgrind_args") << QString::fromLatin1("valgrind")
                                           << (QStringList() << QString::fromLatin1("--foobar-fail"))
                                           << fakeValgrindExecutable() << QString();
}

void ValgrindMemcheckParserTest::testValgrindStartError()
{
    QFETCH(QString, valgrindExe);
    QFETCH(QStringList, valgrindArgs);
    QFETCH(QString, debuggee);
    QFETCH(QString, debuggeeArgs);

    ThreadedParser parser;

    ProjectExplorer::StandardRunnable debuggeeExecutable;
    debuggeeExecutable.executable = debuggee;
    debuggeeExecutable.environment = Utils::Environment::systemEnvironment();
    debuggeeExecutable.commandLineArguments = debuggeeArgs;

    Memcheck::MemcheckRunner runner;
    runner.setParser(&parser);
    runner.setValgrindExecutable(valgrindExe);
    runner.setValgrindArguments(valgrindArgs);
    runner.setDebuggee(debuggeeExecutable);
    runner.setDevice(ProjectExplorer::DeviceManager::instance()->defaultDevice(
                         Core::Id(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)));
    RunnerDumper dumper(&runner, &parser);
    runner.start();
    runner.waitForFinished();
    QVERIFY(dumper.m_errorReceived);
    // just finish without deadlock and we are fine
}

} // namespace Test
} // namespace Valgrind
