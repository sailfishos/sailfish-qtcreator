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

#include "locator.h"

#include "directoryfilter.h"
#include "executefilter.h"
#include "externaltoolsfilter.h"
#include "filesystemfilter.h"
#include "javascriptfilter.h"
#include "locatorconstants.h"
#include "locatorfiltersfilter.h"
#include "locatormanager.h"
#include "locatorsettingspage.h"
#include "locatorwidget.h"
#include "opendocumentsfilter.h"
#include "spotlightlocatorfilter.h"
#include "urllocatorfilter.h"

#include <coreplugin/coreplugin.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/statusbarmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/editormanager_p.h>
#include <coreplugin/menubarfilter.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/mapreduce.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QSettings>

using namespace Utils;

namespace Core {
namespace Internal {

static Locator *m_instance = nullptr;

const char kDirectoryFilterPrefix[] = "directory";
const char kUrlFilterPrefix[] = "url";

class LocatorData
{
public:
    LocatorData();

    LocatorManager m_locatorManager;
    LocatorSettingsPage m_locatorSettingsPage;

    JavaScriptFilter m_javaScriptFilter;
    OpenDocumentsFilter m_openDocumentsFilter;
    FileSystemFilter m_fileSystemFilter;
    ExecuteFilter m_executeFilter;
    ExternalToolsFilter m_externalToolsFilter;
    LocatorFiltersFilter m_locatorsFiltersFilter;
    MenuBarFilter m_menubarFilter;
    UrlLocatorFilter m_urlFilter{UrlLocatorFilter::tr("Web Search"), "RemoteHelpFilter"};
    UrlLocatorFilter m_bugFilter{UrlLocatorFilter::tr("Qt Project Bugs"), "QtProjectBugs"};
    SpotlightLocatorFilter m_spotlightLocatorFilter;
};

LocatorData::LocatorData()
{
    m_urlFilter.setDefaultShortcutString("r");
    m_urlFilter.addDefaultUrl("https://www.bing.com/search?q=%1");
    m_urlFilter.addDefaultUrl("https://www.google.com/search?q=%1");
    m_urlFilter.addDefaultUrl("https://search.yahoo.com/search?p=%1");
    m_urlFilter.addDefaultUrl("https://stackoverflow.com/search?q=%1");
    m_urlFilter.addDefaultUrl(
        "http://en.cppreference.com/mwiki/index.php?title=Special%3ASearch&search=%1");
    m_urlFilter.addDefaultUrl("https://en.wikipedia.org/w/index.php?search=%1");

    m_bugFilter.setDefaultShortcutString("bug");
    m_bugFilter.addDefaultUrl("https://bugreports.qt.io/secure/QuickSearch.jspa?searchString=%1");
}

Locator::Locator()
{
    m_instance = this;
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() { refresh(filters()); });
}

Locator::~Locator()
{
    delete m_locatorData;
    qDeleteAll(m_customFilters);
}

Locator *Locator::instance()
{
    return m_instance;
}

void Locator::initialize()
{
    m_locatorData = new LocatorData;

    QAction *action = new QAction(Utils::Icons::ZOOM.icon(), tr("Locate..."), this);
    Command *cmd = ActionManager::registerAction(action, Constants::LOCATE);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+K")));
    connect(action, &QAction::triggered, this, [] {
        LocatorManager::show(QString());
    });

    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    mtools->addAction(cmd);

    auto locatorWidget = LocatorManager::createLocatorInputWidget(ICore::mainWindow());
    locatorWidget->setObjectName("LocatorInput"); // used for UI introduction
    StatusBarManager::addStatusBarWidget(locatorWidget, StatusBarManager::First,
                                         Context("LocatorWidget"));
    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &Locator::saveSettings);
}

void Locator::extensionsInitialized()
{
    m_filters = ILocatorFilter::allLocatorFilters();
    Utils::sort(m_filters, [](const ILocatorFilter *first, const ILocatorFilter *second) -> bool {
        if (first->priority() != second->priority())
            return first->priority() < second->priority();
        return first->id().alphabeticallyBefore(second->id());
    });
    setFilters(m_filters);

    Command *openCommand = ActionManager::command(Constants::OPEN);
    Command *locateCommand = ActionManager::command(Constants::LOCATE);
    connect(openCommand, &Command::keySequenceChanged,
            this, &Locator::updateEditorManagerPlaceholderText);
    connect(locateCommand, &Command::keySequenceChanged,
            this, &Locator::updateEditorManagerPlaceholderText);
    updateEditorManagerPlaceholderText();
}

bool Locator::delayedInitialize()
{
    loadSettings();
    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag Locator::aboutToShutdown(
    const std::function<void()> &emitAsynchronousShutdownFinished)
{
    m_shuttingDown = true;
    m_refreshTimer.stop();
    if (m_refreshTask.isRunning()) {
        m_refreshTask.cancel();
        m_refreshTask.waitForFinished();
    }
    return LocatorWidget::aboutToShutdown(emitAsynchronousShutdownFinished);
}

void Locator::loadSettings()
{
    SettingsDatabase *settings = ICore::settingsDatabase();
    // check if we have to read old settings
    // TOOD remove a few versions after 4.15
    const QString settingsGroup = settings->contains("Locator") ? QString("Locator")
                                                                : QString("QuickOpen");
    settings->beginGroup(settingsGroup);
    m_refreshTimer.setInterval(settings->value("RefreshInterval", 60).toInt() * 60000);

    for (ILocatorFilter *filter : qAsConst(m_filters)) {
        if (settings->contains(filter->id().toString())) {
            const QByteArray state = settings->value(filter->id().toString()).toByteArray();
            if (!state.isEmpty())
                filter->restoreState(state);
        }
    }
    settings->beginGroup("CustomFilters");
    QList<ILocatorFilter *> customFilters;
    const QStringList keys = settings->childKeys();
    int count = 0;
    const Id directoryBaseId(Constants::CUSTOM_DIRECTORY_FILTER_BASEID);
    const Id urlBaseId(Constants::CUSTOM_URL_FILTER_BASEID);
    for (const QString &key : keys) {
        ++count;
        ILocatorFilter *filter;
        if (key.startsWith(kDirectoryFilterPrefix)) {
            filter = new DirectoryFilter(directoryBaseId.withSuffix(count));
        } else {
            auto urlFilter = new UrlLocatorFilter(urlBaseId.withSuffix(count));
            urlFilter->setIsCustomFilter(true);
            filter = urlFilter;
        }
        filter->restoreState(settings->value(key).toByteArray());
        customFilters.append(filter);
    }
    setCustomFilters(customFilters);
    settings->endGroup();
    settings->endGroup();

    if (m_refreshTimer.interval() > 0)
        m_refreshTimer.start();
    m_settingsInitialized = true;
    setFilters(m_filters + customFilters);
}

void Locator::updateFilterActions()
{
    QMap<Id, QAction *> actionCopy = m_filterActionMap;
    m_filterActionMap.clear();
    // register new actions, update existent
    for (ILocatorFilter *filter : qAsConst(m_filters)) {
        if (filter->shortcutString().isEmpty() || filter->isHidden())
            continue;
        Id filterId = filter->id();
        Id actionId = filter->actionId();
        QAction *action = nullptr;
        if (!actionCopy.contains(filterId)) {
            // register new action
            action = new QAction(filter->displayName(), this);
            Command *cmd = ActionManager::registerAction(action, actionId);
            cmd->setAttribute(Command::CA_UpdateText);
            connect(action, &QAction::triggered, this, [filter] {
                LocatorManager::showFilter(filter);
            });
        } else {
            action = actionCopy.take(filterId);
            action->setText(filter->displayName());
        }
        m_filterActionMap.insert(filterId, action);
    }

    // unregister actions that are deleted now
    const auto end = actionCopy.end();
    for (auto it = actionCopy.begin(); it != end; ++it) {
        ActionManager::unregisterAction(it.value(), it.key().withPrefix("Locator."));
        delete it.value();
    }
}

void Locator::updateEditorManagerPlaceholderText()
{
    Command *openCommand = ActionManager::command(Constants::OPEN);
    Command *locateCommand = ActionManager::command(Constants::LOCATE);
    const QString placeholderText = tr("<html><body style=\"color:#909090; font-size:14px\">"
          "<div align='center'>"
          "<div style=\"font-size:20px\">Open a document</div>"
          "<table><tr><td>"
          "<hr/>"
          "<div style=\"margin-top: 5px\">&bull; File > Open File or Project (%1)</div>"
          "<div style=\"margin-top: 5px\">&bull; File > Recent Files</div>"
          "<div style=\"margin-top: 5px\">&bull; Tools > Locate (%2) and</div>"
          "<div style=\"margin-left: 1em\">- type to open file from any open project</div>"
          "%4"
          "%5"
          "<div style=\"margin-left: 1em\">- type <code>%3&lt;space&gt;&lt;filename&gt;</code> to open file from file system</div>"
          "<div style=\"margin-left: 1em\">- select one of the other filters for jumping to a location</div>"
          "<div style=\"margin-top: 5px\">&bull; Drag and drop files here</div>"
          "</td></tr></table>"
          "</div>"
          "</body></html>")
         .arg(openCommand->keySequence().toString(QKeySequence::NativeText))
         .arg(locateCommand->keySequence().toString(QKeySequence::NativeText))
         .arg(m_locatorData->m_fileSystemFilter.shortcutString());

    QString classes;
    // not nice, but anyhow
    ILocatorFilter *classesFilter = Utils::findOrDefault(m_filters,
                                                         Utils::equal(&ILocatorFilter::id,
                                                                      Id("Classes")));
    if (classesFilter)
        classes = tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a class definition</div>").arg(classesFilter->shortcutString());

    QString methods;
    // not nice, but anyhow
    ILocatorFilter *methodsFilter = Utils::findOrDefault(m_filters, Utils::equal(&ILocatorFilter::id,
                                                                                 Id("Methods")));
    if (methodsFilter)
        methods = tr("<div style=\"margin-left: 1em\">- type <code>%1&lt;space&gt;&lt;pattern&gt;</code>"
                     " to jump to a function definition</div>").arg(methodsFilter->shortcutString());

    EditorManagerPrivate::setPlaceholderText(placeholderText.arg(classes, methods));
}

void Locator::saveSettings() const
{
    if (!m_settingsInitialized)
        return;

    SettingsDatabase *s = ICore::settingsDatabase();
    s->beginTransaction();
    s->beginGroup("Locator");
    s->remove(QString());
    s->setValue("RefreshInterval", refreshInterval());
    for (ILocatorFilter *filter : m_filters) {
        if (!m_customFilters.contains(filter)) {
            const QByteArray state = filter->saveState();
            s->setValueWithDefault(filter->id().toString(), state);
        }
    }
    s->beginGroup("CustomFilters");
    int i = 0;
    for (ILocatorFilter *filter : m_customFilters) {
        const char *prefix = filter->id().name().startsWith(
                                 Constants::CUSTOM_DIRECTORY_FILTER_BASEID)
                                 ? kDirectoryFilterPrefix
                                 : kUrlFilterPrefix;
        const QByteArray state = filter->saveState();
        s->setValueWithDefault(prefix + QString::number(i), state);
        ++i;
    }
    s->endGroup();
    s->endGroup();
    s->endTransaction();
}

/*!
    Return all filters, including the ones created by the user.
*/
QList<ILocatorFilter *> Locator::filters()
{
    return m_instance->m_filters;
}

/*!
    This returns a subset of all the filters, that contains only the filters that
    have been created by the user at some point (maybe in a previous session).
 */
QList<ILocatorFilter *> Locator::customFilters()
{
    return m_customFilters;
}

void Locator::setFilters(QList<ILocatorFilter *> f)
{
    m_filters = f;
    updateFilterActions();
    updateEditorManagerPlaceholderText(); // possibly some shortcut changed
    emit filtersChanged();
}

void Locator::setCustomFilters(QList<ILocatorFilter *> filters)
{
    m_customFilters = filters;
}

int Locator::refreshInterval() const
{
    return m_refreshTimer.interval() / 60000;
}

void Locator::setRefreshInterval(int interval)
{
    if (interval < 1) {
        m_refreshTimer.stop();
        m_refreshTimer.setInterval(0);
        return;
    }
    m_refreshTimer.setInterval(interval * 60000);
    m_refreshTimer.start();
}

void Locator::refresh(QList<ILocatorFilter *> filters)
{
    if (m_shuttingDown)
        return;

    if (m_refreshTask.isRunning()) {
        m_refreshTask.cancel();
        m_refreshTask.waitForFinished();
        // this is not ideal because some of the previous filters might have finished, but we
        // currently cannot find out which part of a map-reduce has finished
        filters = Utils::filteredUnique(m_refreshingFilters + filters);
    }
    m_refreshingFilters = filters;
    m_refreshTask = Utils::map(filters, &ILocatorFilter::refresh, Utils::MapReduceOption::Unordered);
    ProgressManager::addTask(m_refreshTask, tr("Updating Locator Caches"), Constants::TASK_INDEX);
    Utils::onFinished(m_refreshTask, this, [this](const QFuture<void> &future) {
        if (!future.isCanceled()) {
            saveSettings();
            m_refreshingFilters.clear();
            m_refreshTask = QFuture<void>();
        }
    });
}

} // namespace Internal
} // namespace Core
