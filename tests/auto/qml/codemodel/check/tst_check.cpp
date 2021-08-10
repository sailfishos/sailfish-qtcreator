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

#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/parser/qmljssourcelocation_p.h>
#include <extensionsystem/pluginmanager.h>

#include <QtTest>
#include <algorithm>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;

class tst_Check : public QObject
{
    Q_OBJECT
public:
    tst_Check();

private slots:
    void test();
    void test_data();

    void initTestCase();

private:
    ModelManagerInterface m_modelManager; // we indirectly need an instance of the modelmanager
};

tst_Check::tst_Check()
{
}


#ifdef Q_OS_MAC
#  define SHARE_PATH "/Resources"
#else
#  define SHARE_PATH "/share/qtcreator"
#endif

QString resourcePath()
{
    return QDir::cleanPath(QTCREATORDIR + QLatin1String(SHARE_PATH));
}

void tst_Check::initTestCase()
{
    // the resource path is wrong, have to load things manually
    QFileInfo builtins(resourcePath() + "/qml-type-descriptions/builtins.qmltypes");
    QStringList errors, warnings;
    CppQmlTypesLoader::defaultQtObjects = CppQmlTypesLoader::loadQmlTypes(QFileInfoList() << builtins, &errors, &warnings);

    if (!ModelManagerInterface::instance())
        new ModelManagerInterface;
    if (!ExtensionSystem::PluginManager::instance())
        new ExtensionSystem::PluginManager;
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    QFutureInterface<void> result;
    PathsAndLanguages lPaths;
    QStringList paths(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath));
    for (auto p: paths)
        lPaths.maybeInsert(Utils::FilePath::fromString(p), Dialect::Qml);
    ModelManagerInterface::importScan(result, ModelManagerInterface::workingCopy(), lPaths,
                                      modelManager, false);
    QCoreApplication::processEvents();
    modelManager->test_joinAllThreads();
    QCoreApplication::processEvents();
}

static bool offsetComparator(const Message &lhs, const Message &rhs)
{
    return lhs.location.offset < rhs.location.offset;
}

#define QCOMPARE_NOEXIT(actual, expected) \
    QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)

void tst_Check::test_data()
{
    QTest::addColumn<QString>("path");

    for (QFileInfo it : QDir(TESTSRCDIR).entryInfoList(QStringList("*.qml"), QDir::Files, QDir::Name)) {
        QTest::newRow(it.fileName().toUtf8()) << it.filePath();
    }
}

void tst_Check::test()
{
    QFETCH(QString, path);
    auto mm = ModelManagerInterface::instance();
    Snapshot snapshot =  mm->snapshot();
    Document::MutablePtr doc = Document::create(path, Dialect::Qml);
    QFile file(doc->fileName());
    file.open(QFile::ReadOnly | QFile::Text);
    doc->setSource(QString::fromUtf8(file.readAll()));
    file.close();
    doc->parse();
    snapshot.insert(doc);
    mm->updateDocument(doc);
    QRegularExpression nDiagRe("// *nDiagnosticMessages *= *([0-9]+)");
    auto m = nDiagRe.match(doc->source());
    int nDiagnosticMessages = 0;
    if (m.hasMatch())
        nDiagnosticMessages = m.captured(1).toInt();

    QVERIFY(!doc->source().isEmpty());
    QCOMPARE(doc->diagnosticMessages().size(), nDiagnosticMessages);

    ViewerContext vContext;
    vContext.flags = ViewerContext::Complete;
    ContextPtr context = Link(snapshot, vContext, LibraryInfo())();

    Check checker(doc, context);
    QList<Message> messages = checker();
    std::sort(messages.begin(), messages.end(), &offsetComparator);

    const QRegularExpression messagePattern(" (-?\\d+) (\\d+) (\\d+)\\s*(# false positive|# wrong warning.*)?");

    QList<Message> expectedMessages;
    QHash<int, QString> xfails;
    for (const SourceLocation &comment : doc->engine()->comments()) {
        const QString text = doc->source().mid(comment.begin(), comment.end() - comment.begin());
        const QRegularExpressionMatch match = messagePattern.match(text);
        if (!match.hasMatch())
            continue;
        const int type = match.captured(1).toInt();
        const int columnStart = match.captured(2).toInt();
        const int columnEnd = match.captured(3).toInt() + 1;

        Message message;
        message.location = SourceLocation(
                    comment.offset - comment.startColumn + columnStart,
                    columnEnd - columnStart,
                    comment.startLine,
                    columnStart),
        message.type = static_cast<QmlJS::StaticAnalysis::Type>(type);
        expectedMessages += message;

        if (messagePattern.captureCount() == 4 && !match.captured(4).isEmpty())
            xfails.insert(expectedMessages.size() - 1, match.captured(4));
    }

    for (int i = 0; i < messages.size(); ++i) {
        if (i >= expectedMessages.size())
            break;
        Message actual = messages.at(i);
        Message expected = expectedMessages.at(i);
        bool fail = false;
        fail |= !QCOMPARE_NOEXIT(actual.location.startLine, expected.location.startLine);
        auto xFail = xfails.find(i);
        if (xFail != xfails.end())
            QEXPECT_FAIL(path.toUtf8(), xFail.value().toUtf8(), Continue);
        fail |= !QCOMPARE_NOEXIT((int)actual.type, (int)expected.type);
        if (fail)
            return;
        fail |= !QCOMPARE_NOEXIT(actual.location.startColumn, expected.location.startColumn);
        fail |= !QCOMPARE_NOEXIT(actual.location.offset, expected.location.offset);
        fail |= !QCOMPARE_NOEXIT(actual.location.length, expected.location.length);
        if (fail) {
            qDebug() << "Failed for message on line" << actual.location.startLine << actual.message;
            return;
        }
    }
    if (expectedMessages.size() > messages.size()) {
        for (int i = messages.size(); i < expectedMessages.size(); ++i) {
            Message missingMessage = expectedMessages.at(i);
            qDebug() << "expected message of type" << missingMessage.type << "on line" << missingMessage.location.startLine;
        }
        QVERIFY2(false, "more messages expected");
    }
    if (expectedMessages.size() < messages.size()) {
        for (int i = expectedMessages.size(); i < messages.size(); ++i) {
            Message extraMessage = messages.at(i);
            qDebug() << "unexpected message of type" << extraMessage.type << "on line" << extraMessage.location.startLine
                     << extraMessage.message;
        }
        QFAIL("fewer messages expected");
    }
}

QTEST_MAIN(tst_Check);

#include "tst_check.moc"
