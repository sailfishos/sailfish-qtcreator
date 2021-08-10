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

#include "quicktesttreeitem.h"
#include "quicktestconfiguration.h"
#include "quicktestframework.h"
#include "quicktestparser.h"
#include "../testframeworkmanager.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

TestTreeItem *QuickTestTreeItem::copyWithoutChildren()
{
    QuickTestTreeItem *copied = new QuickTestTreeItem(framework());
    copied->copyBasicDataFrom(this);
    return copied;
}

QVariant QuickTestTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (type() == TestCase && name().isEmpty())
            return QCoreApplication::translate("QuickTestTreeItem", "<unnamed>");
        break;
    case Qt::ToolTipRole:
        if (type() == TestCase && name().isEmpty())
            return QCoreApplication::translate("QuickTestTreeItem",
                                               "<p>Give all test cases a name to ensure correct "
                                               "behavior when running test cases and to be able to "
                                               "select them.</p>");
        break;
    case Qt::CheckStateRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
        case TestDataTag:
            return QVariant();
        case TestCase:
            return name().isEmpty() ? QVariant() : checked();
        case TestFunction:
            return (parentItem() && !parentItem()->name().isEmpty()) ? checked() : QVariant();
        default:
            return checked();
        }

    case ItalicRole:
        switch (type()) {
        case TestDataFunction:
        case TestSpecialFunction:
            return true;
        case TestCase:
            return name().isEmpty();
        case TestFunction:
            return parentItem() ? parentItem()->name().isEmpty() : false;
        default:
            return false;
        }
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

Qt::ItemFlags QuickTestTreeItem::flags(int column) const
{
    // handle unnamed quick tests and their functions
    switch (type()) {
    case TestCase:
        if (name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    case TestFunction:
        if (parentItem()->name().isEmpty())
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    default: {} // avoid warning regarding unhandled enum values
    }
    return TestTreeItem::flags(column);
}

bool QuickTestTreeItem::canProvideTestConfiguration() const
{
    switch (type()) {
    case TestCase:
        return !name().isEmpty();
    case TestFunction:
        return !parentItem()->name().isEmpty();
    default:
        return false;
    }
}

bool QuickTestTreeItem::canProvideDebugConfiguration() const
{
    return canProvideTestConfiguration();
}

ITestConfiguration *QuickTestTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    QuickTestConfiguration *config = nullptr;
    switch (type()) {
    case TestCase: {
        const QString testName = name();
        QStringList testFunctions;
        forFirstLevelChildren([&testFunctions, &testName](ITestTreeItem *child) {
            if (child->type() == TestTreeItem::TestFunction)
                testFunctions << testName + "::" + child->name();
        });
        config = new QuickTestConfiguration(framework());
        config->setTestCases(testFunctions);
        config->setProjectFile(proFile());
        config->setProject(project);
        break;
    }
    case TestFunction: {
        TestTreeItem *parent = parentItem();
        QStringList testFunction(parent->name() + "::" + name());
        config = new QuickTestConfiguration(framework());
        config->setTestCases(testFunction);
        config->setProjectFile(parent->proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    if (config)
        config->setInternalTargets(internalTargets());
    return config;
}

static QList<ITestConfiguration *> testConfigurationsFor(
        const TestTreeItem *rootNode, const std::function<bool(TestTreeItem *)> &predicate)
{
    QTC_ASSERT(rootNode, return {});
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || rootNode->type() != TestTreeItem::Root)
        return {};

    QHash<QString, QuickTestConfiguration *> configurationForProFiles;
    rootNode->forSelectedChildren([&predicate, &configurationForProFiles](Utils::TreeItem *it) {
        auto treeItem = static_cast<TestTreeItem *>(it);
        if (treeItem->type() == TestTreeItem::Root || treeItem->type() == TestTreeItem::GroupNode)
            return true;
        if (treeItem->type() == TestTreeItem::TestCase) {
            const QString name = treeItem->name();

            QStringList functions;
            treeItem->forFirstLevelChildItems([&functions, &name, &predicate](TestTreeItem *child) {
                if (predicate(child))
                    functions << name + "::" + child->name();
            });
            if (functions.isEmpty())
                return false;

            auto it = configurationForProFiles.find(treeItem->proFile());
            if (it == configurationForProFiles.end()) {
                auto tc = new QuickTestConfiguration(treeItem->framework());
                tc->setProjectFile(treeItem->proFile());
                tc->setProject(ProjectExplorer::SessionManager::startupProject());
                tc->setInternalTargets(treeItem->internalTargets());
                it = configurationForProFiles.insert(treeItem->proFile(), tc);
            }
            it.value()->setTestCases(it.value()->testCases() + functions);
        }
        return false;
    });

    return Utils::static_container_cast<ITestConfiguration *>(configurationForProFiles.values());
}

ITestConfiguration *QuickTestTreeItem::debugConfiguration() const
{
    QuickTestConfiguration *config = static_cast<QuickTestConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

struct Tests {
    int testCount = 0;
    QSet<QString> internalTargets;
};

static void addTestsForItem(Tests &tests, const TestTreeItem *item)
{
    tests.testCount += item->childCount();
    tests.internalTargets = item->internalTargets();
}

QList<ITestConfiguration *> QuickTestTreeItem::getAllTestConfigurations() const
{
    QList<ITestConfiguration *> result;

    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, Tests> testsForProfile;
    forFirstLevelChildItems([&testsForProfile](TestTreeItem *child) {
        // unnamed Quick Tests must be handled separately
        if (child->name().isEmpty()) {
            child->forFirstLevelChildItems([&testsForProfile](TestTreeItem *grandChild) {
                const QString &proFile = grandChild->proFile();
                ++(testsForProfile[proFile].testCount);
                testsForProfile[proFile].internalTargets = grandChild->internalTargets();
            });
            return;
        }
        // named Quick Test
        if (child->type() == TestCase) {
            addTestsForItem(testsForProfile[child->proFile()], child);
        } else if (child->type() == GroupNode) {
            child->forFirstLevelChildItems([&testsForProfile](TestTreeItem *grandChild) {
                addTestsForItem(testsForProfile[grandChild->proFile()], grandChild);
            });
        }
    });
    // create TestConfiguration for each project file
    for (auto it = testsForProfile.begin(), end = testsForProfile.end(); it != end; ++it) {
        QuickTestConfiguration *tc = new QuickTestConfiguration(framework());
        tc->setTestCaseCount(it.value().testCount);
        tc->setProjectFile(it.key());
        tc->setProject(project);
        tc->setInternalTargets(it.value().internalTargets);
        result << tc;
    }
    return result;
}

QList<ITestConfiguration *> QuickTestTreeItem::getSelectedTestConfigurations() const
{
    return testConfigurationsFor(this, [](TestTreeItem *it) {
        return it->checked() == Qt::Checked && it->type() == TestTreeItem::TestFunction;
    });
}

QList<ITestConfiguration *> QuickTestTreeItem::getFailedTestConfigurations() const
{
    return testConfigurationsFor(this, [](TestTreeItem *it) {
        return it->data(0, FailedRole).toBool();
    });
}

QList<ITestConfiguration *> QuickTestTreeItem::getTestConfigurationsForFile(
        const Utils::FilePath &fileName) const
{
    const QString &file = fileName.toString();
    return testConfigurationsFor(this, [&file](TestTreeItem *it) {
        return it->filePath() == file;
    });
}

TestTreeItem *QuickTestTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    switch (type()) {
    case Root:
        if (result->name.isEmpty())
            return unnamedQuickTests();
        if (result->framework->grouping()) {
            const QString path = QFileInfo(result->fileName).absolutePath();
            TestTreeItem *group = findFirstLevelChildItem([path](TestTreeItem *group) {
                    return group->filePath() == path;
            });
            return group ? group->findChildByNameAndFile(result->name, result->fileName) : nullptr;
        }
        return findChildByNameAndFile(result->name, result->fileName);
    case GroupNode:
        return findChildByNameAndFile(result->name, result->fileName);
    case TestCase:
        return name().isEmpty() ? findChildByNameFileAndLine(result->name, result->fileName,
                                                             result->line)
                                : findChildByName(result->name);
    default:
        return nullptr;
    }
}

TestTreeItem *QuickTestTreeItem::findChild(const TestTreeItem *other)
{
    QTC_ASSERT(other, return nullptr);
    const Type otherType = other->type();

    switch (type()) {
    case Root:
        if (otherType == TestCase && other->name().isEmpty())
            return unnamedQuickTests();
        return findChildByFileNameAndType(other->filePath(), other->name(), otherType);
    case GroupNode:
        return findChildByFileNameAndType(other->filePath(), other->name(), otherType);
    case TestCase:
        if (otherType != TestFunction && otherType != TestDataFunction && otherType != TestSpecialFunction)
            return nullptr;
        return name().isEmpty() ? findChildByNameFileAndLine(other->name(), other->filePath(),
                                                             other->line())
                                : findChildByName(other->name());
    default:
        return nullptr;
    }
}

bool QuickTestTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestCase:
        return result->name.isEmpty() ? false : modifyTestCaseOrSuiteContent(result);
    case TestFunction:
    case TestDataFunction:
    case TestSpecialFunction:
        return modifyTestFunctionContent(result);
    default:
        return false;
    }
}

bool QuickTestTreeItem::lessThan(const ITestTreeItem *other, TestTreeItem::SortMode mode) const
{
    // handle special item (<unnamed>)
    if (name().isEmpty())
        return false;
    if (other->name().isEmpty())
        return true;
    return ITestTreeItem::lessThan(other, mode);
}

bool QuickTestTreeItem::isGroupNodeFor(const TestTreeItem *other) const
{
    QTC_ASSERT(other, return false);
    if (other->name().isEmpty()) // unnamed quick tests will not get grouped
        return false;
    return TestTreeItem::isGroupNodeFor(other);
}

bool QuickTestTreeItem::removeOnSweepIfEmpty() const
{
    return TestTreeItem::removeOnSweepIfEmpty()
            || (type() == TestCase && name().isEmpty()); // remove pseudo item '<unnamed>'
}

TestTreeItem *QuickTestTreeItem::createParentGroupNode() const
{
    const QFileInfo fileInfo(filePath());
    const QFileInfo base(fileInfo.absolutePath());
    return new QuickTestTreeItem(framework(), base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
}

bool QuickTestTreeItem::isGroupable() const
{
    return type() == TestCase && !name().isEmpty() && !filePath().isEmpty();
}

QSet<QString> QuickTestTreeItem::internalTargets() const
{
    QSet<QString> result;
    const auto cppMM = CppTools::CppModelManager::instance();
    const auto projectInfo = cppMM->projectInfo(ProjectExplorer::SessionManager::startupProject());
    for (const CppTools::ProjectPart::Ptr &projectPart : projectInfo.projectParts()) {
        if (projectPart->buildTargetType != ProjectExplorer::BuildTargetType::Executable)
            continue;
        if (projectPart->projectFile == proFile())
            result.insert(projectPart->buildSystemTarget);
    }
    return result;
}

void QuickTestTreeItem::markForRemovalRecursively(const QString &filePath)
{
    TestTreeItem::markForRemovalRecursively(filePath);
    auto parser = dynamic_cast<QuickTestParser *>(framework()->testParser());
    const QString proFile = parser->projectFileForMainCppFile(filePath);
    if (!proFile.isEmpty()) {
        TestTreeItem *root = framework()->rootNode();
        root->forAllChildItems([proFile](TestTreeItem *it) {
            if (it->proFile() == proFile)
                it->markForRemoval(true);
        });
    }
}

TestTreeItem *QuickTestTreeItem::findChildByFileNameAndType(const QString &filePath,
                                                            const QString &name,
                                                            TestTreeItem::Type tType)

{
    return findFirstLevelChildItem([filePath, name, tType](const TestTreeItem *other) {
        return other->type() == tType && other->name() == name && other->filePath() == filePath;
    });
}

TestTreeItem *QuickTestTreeItem::findChildByNameFileAndLine(const QString &name,
                                                            const QString &filePath, int line)
{
    return findFirstLevelChildItem([name, filePath, line](const TestTreeItem *other) {
        return other->filePath() == filePath && other->line() == line && other->name() == name;
    });
}

TestTreeItem *QuickTestTreeItem::unnamedQuickTests() const
{
    if (type() != Root)
        return nullptr;

    return findFirstLevelChildItem([](TestTreeItem *child) { return child->name().isEmpty(); });
}

} // namespace Internal
} // namespace Autotest
