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

#include "gtestparser.h"
#include "gtestframework.h"
#include "gtesttreeitem.h"
#include "gtestvisitors.h"
#include "gtest_utils.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpart.h>

namespace Autotest {
namespace Internal {

TestTreeItem *GTestParseResult::createTestTreeItem() const
{
    if (itemType != TestTreeItem::TestSuite && itemType != TestTreeItem::TestCase)
        return nullptr;
    GTestTreeItem *item = new GTestTreeItem(framework, name, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);

    if (parameterized)
        item->setState(GTestTreeItem::Parameterized);
    if (typed)
        item->setState(GTestTreeItem::Typed);
    if (disabled)
        item->setState(GTestTreeItem::Disabled);
    for (const TestParseResult *testSet : children)
        item->appendChild(testSet->createTestTreeItem());
    return item;
}

static bool includesGTest(const CPlusPlus::Document::Ptr &doc,
                          const CPlusPlus::Snapshot &snapshot)
{
    static const QString gtestH("gtest/gtest.h");
    for (const CPlusPlus::Document::Include &inc : doc->resolvedIncludes()) {
        if (inc.resolvedFileName().endsWith(gtestH))
            return true;
    }

    for (const QString &include : snapshot.allIncludesForDocument(doc->fileName())) {
        if (include.endsWith(gtestH))
            return true;
    }

    return false;
}

static bool hasGTestNames(const CPlusPlus::Document::Ptr &document)
{
    for (const CPlusPlus::Document::MacroUse &macro : document->macroUses()) {
        if (!macro.isFunctionLike())
            continue;
        if (GTestUtils::isGTestMacro(QLatin1String(macro.macro().name()))) {
            const QVector<CPlusPlus::Document::Block> args = macro.arguments();
            if (args.size() != 2)
                continue;
            return true;
        }
    }
    return false;
}

bool GTestParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                  const QString &fileName)
{
    CPlusPlus::Document::Ptr doc = document(fileName);
    if (doc.isNull() || !includesGTest(doc, m_cppSnapshot) || !hasGTestNames(doc))
        return false;

    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &filePath = doc->fileName();
    const QByteArray &fileContent = getFileContent(filePath);
    CPlusPlus::Document::Ptr document = m_cppSnapshot.preprocessedDocument(fileContent, filePath);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    GTestVisitor visitor(document);
    visitor.accept(ast);

    const QMap<GTestCaseSpec, GTestCodeLocationList> result = visitor.gtestFunctions();
    QString proFile;
    const QList<CppTools::ProjectPart::Ptr> &ppList = modelManager->projectPart(filePath);
    if (!ppList.isEmpty())
        proFile = ppList.first()->projectFile;
    else
        return false; // happens if shutting down while parsing

    for (auto it = result.cbegin(); it != result.cend(); ++it) {
        const GTestCaseSpec &testSpec = it.key();
        GTestParseResult *parseResult = new GTestParseResult(framework());
        parseResult->itemType = TestTreeItem::TestSuite;
        parseResult->fileName = filePath;
        parseResult->name = testSpec.testCaseName;
        parseResult->parameterized = testSpec.parameterized;
        parseResult->typed = testSpec.typed;
        parseResult->disabled = testSpec.disabled;
        parseResult->proFile = proFile;

        for (const GTestCodeLocationAndType &location : it.value()) {
            GTestParseResult *testSet = new GTestParseResult(framework());
            testSet->name = location.m_name;
            testSet->fileName = filePath;
            testSet->line = location.m_line;
            testSet->column = location.m_column;
            testSet->disabled = location.m_state & GTestTreeItem::Disabled;
            testSet->itemType = location.m_type;
            testSet->proFile = proFile;

            parseResult->children.append(testSet);
        }

        futureInterface.reportResult(TestParseResultPtr(parseResult));
    }
    return !result.isEmpty();
}

} // namespace Internal
} // namespace Autotest
