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

#include "testframeworkmanager.h"
#include "autotestconstants.h"
#include "autotestplugin.h"
#include "iframeworksettings.h"
#include "itestparser.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QSettings>

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.frameworkmanager", QtWarningMsg)

using namespace Core;

namespace Autotest {

static TestFrameworkManager *s_instance = nullptr;

TestFrameworkManager::TestFrameworkManager()
{
    m_testTreeModel = TestTreeModel::instance();
    m_testRunner = Internal::TestRunner::instance();
    s_instance = this;
}

TestFrameworkManager *TestFrameworkManager::instance()
{
    if (!s_instance)
        return new TestFrameworkManager;
    return s_instance;
}

TestFrameworkManager::~TestFrameworkManager()
{
    delete m_testRunner;
    delete m_testTreeModel;
    qDeleteAll(m_frameworkSettingsPages);
    m_frameworkSettingsPages.clear();
    for (ITestFramework *framework : m_registeredFrameworks.values())
        delete framework;
}

bool TestFrameworkManager::registerTestFramework(ITestFramework *framework)
{
    QTC_ASSERT(framework, return false);
    Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix(framework->name());
    QTC_ASSERT(!m_registeredFrameworks.contains(id), delete framework; return false);
    // TODO check for unique priority before registering
    qCDebug(LOG) << "Registering" << id;
    m_registeredFrameworks.insert(id, framework);

    if (framework->hasFrameworkSettings()) {
        QSharedPointer<IFrameworkSettings> frameworkSettings(framework->createFrameworkSettings());
        m_frameworkSettings.insert(id, frameworkSettings);
        if (auto page = framework->createSettingsPage(frameworkSettings))
            m_frameworkSettingsPages.append(page);
    }
    return true;
}

void TestFrameworkManager::activateFrameworksFromSettings(QSharedPointer<Internal::TestSettings> settings)
{
    FrameworkIterator it = m_registeredFrameworks.begin();
    FrameworkIterator end = m_registeredFrameworks.end();
    for ( ; it != end; ++it) {
        it.value()->setActive(settings->frameworks.value(it.key(), false));
        it.value()->setGrouping(settings->frameworksGrouping.value(it.key(), false));
    }
}

QString TestFrameworkManager::frameworkNameForId(const Id &id) const
{
    ITestFramework *framework = m_registeredFrameworks.value(id, nullptr);
    return framework ? QString::fromLatin1(framework->name()) : QString();
}

QList<Id> TestFrameworkManager::registeredFrameworkIds() const
{
    return m_registeredFrameworks.keys();
}

QList<Id> TestFrameworkManager::sortedRegisteredFrameworkIds() const
{
    QList<Id> registered = m_registeredFrameworks.keys();
    Utils::sort(registered, [this] (const Id &lhs, const Id &rhs) {
        return m_registeredFrameworks[lhs]->priority() < m_registeredFrameworks[rhs]->priority();
    });
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registered;
    return registered;
}

QList<Id> TestFrameworkManager::activeFrameworkIds() const
{
    QList<Id> active;
    FrameworkIterator it = m_registeredFrameworks.begin();
    FrameworkIterator end = m_registeredFrameworks.end();
    for ( ; it != end; ++it) {
        if (it.value()->active())
            active.append(it.key());
    }
    return active;
}

QList<Id> TestFrameworkManager::sortedActiveFrameworkIds() const
{
    QList<Id> active = activeFrameworkIds();
    Utils::sort(active, [this] (const Id &lhs, const Id &rhs) {
        return m_registeredFrameworks[lhs]->priority() < m_registeredFrameworks[rhs]->priority();
    });
    qCDebug(LOG) << "Active frameworks sorted by priority" << active;
    return active;
}

TestTreeItem *TestFrameworkManager::rootNodeForTestFramework(const Id &frameworkId) const
{
    ITestFramework *framework = m_registeredFrameworks.value(frameworkId, nullptr);
    return framework ? framework->rootNode() : nullptr;
}

ITestParser *TestFrameworkManager::testParserForTestFramework(const Id &frameworkId) const
{
    ITestFramework *framework = m_registeredFrameworks.value(frameworkId, nullptr);
    if (!framework)
        return nullptr;
    ITestParser *testParser = framework->testParser();
    qCDebug(LOG) << "Setting" << frameworkId << "as Id for test parser";
    testParser->setId(frameworkId);
    return testParser;
}

QSharedPointer<IFrameworkSettings> TestFrameworkManager::settingsForTestFramework(
            const Id &frameworkId) const
{
    return m_frameworkSettings.contains(frameworkId) ? m_frameworkSettings.value(frameworkId)
                                                     : QSharedPointer<IFrameworkSettings>();
}

void TestFrameworkManager::synchronizeSettings(QSettings *s)
{
    Internal::AutotestPlugin::settings()->fromSettings(s);
    for (const Id &id : m_frameworkSettings.keys()) {
        QSharedPointer<IFrameworkSettings> fSettings = settingsForTestFramework(id);
        if (!fSettings.isNull())
            fSettings->fromSettings(s);
    }
}

bool TestFrameworkManager::isActive(const Id &frameworkId) const
{
    ITestFramework *framework = m_registeredFrameworks.value(frameworkId);
    return framework ? framework->active() : false;
}

bool TestFrameworkManager::groupingEnabled(const Id &frameworkId) const
{
    ITestFramework *framework = m_registeredFrameworks.value(frameworkId);
    return framework ? framework->grouping() : false;
}

void TestFrameworkManager::setGroupingEnabledFor(const Id &frameworkId, bool enabled)
{
    if (ITestFramework *framework = m_registeredFrameworks.value(frameworkId))
        framework->setGrouping(enabled);
}

QString TestFrameworkManager::groupingToolTip(const Id &frameworkId) const
{
    if (ITestFramework *framework = m_registeredFrameworks.value(frameworkId))
        return framework->groupingToolTip();
    return QString();
}

bool TestFrameworkManager::hasActiveFrameworks() const
{
    for (ITestFramework *framework : m_registeredFrameworks.values()) {
        if (framework->active())
            return true;
    }
    return false;
}

unsigned TestFrameworkManager::priority(const Id &frameworkId) const
{
    if (ITestFramework *framework = m_registeredFrameworks.value(frameworkId))
        return framework->priority();
    return unsigned(-1);
}

Id ITestFramework::settingsId() const
{
    return Core::Id(Constants::SETTINGSPAGE_PREFIX)
            .withSuffix(QString("%1.%2").arg(priority()).arg(QLatin1String(name())));
}

} // namespace Autotest
