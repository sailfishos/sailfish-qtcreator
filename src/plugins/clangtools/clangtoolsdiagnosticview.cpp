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

#include "clangtoolsdiagnosticview.h"

#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/manhattanstyle.h>

#include <debugger/analyzer/diagnosticlocation.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QHeaderView>
#include <QPainter>
#include <QSet>
#include <QUuid>

#include <set>

using namespace CppTools;
using namespace Debugger;

namespace ClangTools {
namespace Internal {

static QString getBaseStyleName()
{
    QStyle *style = QApplication::style();
    if (auto proxyStyle = qobject_cast<QProxyStyle *>(style))
        style = proxyStyle->baseStyle();
    return style->objectName();
}

// Paints the check box indicator disabled if requested by client.
class DiagnosticViewStyle : public ManhattanStyle
{
public:
    DiagnosticViewStyle(const QString &baseStyleName = getBaseStyleName())
        : ManhattanStyle(baseStyleName)
    {}

    void setPaintCheckBoxDisabled(bool paintDisabledCheckbox)
    {
        m_paintCheckBoxDisabled = paintDisabledCheckbox;
    }

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const final
    {
        if (element == QStyle::PE_IndicatorCheckBox && m_paintCheckBoxDisabled) {
            if (const QStyleOptionButton *o = qstyleoption_cast<const QStyleOptionButton *>(
                    option)) {
                QStyleOptionButton myOption = *o;
                myOption.palette.setCurrentColorGroup(QPalette::Disabled);
                ManhattanStyle::drawPrimitive(element, &myOption, painter, widget);
                return;
            }
        }
        ManhattanStyle::drawPrimitive(element, option, painter, widget);
    }

private:
    bool m_paintCheckBoxDisabled = false;
};

// A delegate that allows to paint a disabled check box (indicator).
// This is useful if the rest of the item should not be disabled.
class DiagnosticViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    DiagnosticViewDelegate(DiagnosticViewStyle *style, QObject *parent)
        : QStyledItemDelegate(parent)
        ,  m_style(style)
    {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const final
    {
        const bool paintDisabled = !index.data(ClangToolsDiagnosticModel::CheckBoxEnabledRole)
                                        .toBool();
        if (paintDisabled)
            m_style->setPaintCheckBoxDisabled(true);
        QStyledItemDelegate::paint(painter, option, index);
        if (paintDisabled)
            m_style->setPaintCheckBoxDisabled(false);
    }

private:
    DiagnosticViewStyle *m_style = nullptr;
};

DiagnosticView::DiagnosticView(QWidget *parent)
    : Debugger::DetailedErrorView(parent)
    , m_style(new DiagnosticViewStyle)
    , m_delegate(new DiagnosticViewDelegate(m_style, this))
{
    header()->hide();
    setSelectionMode(ExtendedSelection);

    const QIcon filterIcon = Utils::Icon({{":/utils/images/filtericon.png",
                                           Utils::Theme::PanelTextColorMid}},
                                         Utils::Icon::Tint).icon();

    m_showFilter = new QAction(tr("Filter..."), this);
    m_showFilter->setIcon(filterIcon);
    connect(m_showFilter, &QAction::triggered,
            this, &DiagnosticView::showFilter);
    m_clearFilter = new QAction(tr("Clear Filter"), this);
    m_clearFilter->setIcon(filterIcon);
    connect(m_clearFilter, &QAction::triggered,
            this, &DiagnosticView::clearFilter);
    m_filterForCurrentKind = new QAction(tr("Filter for This Diagnostic Kind"), this);
    m_filterForCurrentKind->setIcon(filterIcon);
    connect(m_filterForCurrentKind, &QAction::triggered,
            this, &DiagnosticView::filterForCurrentKind);
    m_filterOutCurrentKind = new QAction(tr("Filter out This Diagnostic Kind"), this);
    m_filterOutCurrentKind->setIcon(filterIcon);
    connect(m_filterOutCurrentKind, &QAction::triggered,
            this, &DiagnosticView::filterOutCurrentKind);

    m_separator = new QAction(this);
    m_separator->setSeparator(true);

    m_separator2 = new QAction(this);
    m_separator2->setSeparator(true);

    m_help = new QAction(tr("Web Page"), this);
    m_help->setIcon(Utils::Icons::ONLINE.icon());
    connect(m_help, &QAction::triggered,
            this, &DiagnosticView::showHelp);

    m_suppressAction = new QAction(this);
    connect(m_suppressAction, &QAction::triggered,
            this, &DiagnosticView::suppressCurrentDiagnostic);

    m_disableChecksAction = new QAction(this);
    connect(m_disableChecksAction, &QAction::triggered,
            this, &DiagnosticView::disableCheckForCurrentDiagnostic);

    installEventFilter(this);

    setStyle(m_style);
    setItemDelegate(m_delegate);
}

void DiagnosticView::scheduleAllFixits(bool schedule)
{
    const auto proxyModel = static_cast<QSortFilterProxyModel *>(model());
    for (int i = 0, count = proxyModel->rowCount(); i < count; ++i) {
        const QModelIndex filePathItemIndex = proxyModel->index(i, 0);
        for (int j = 0, count = proxyModel->rowCount(filePathItemIndex); j < count; ++j) {
            const QModelIndex proxyIndex = proxyModel->index(j, 0, filePathItemIndex);
            const QModelIndex diagnosticItemIndex = proxyModel->mapToSource(proxyIndex);
            auto item = static_cast<DiagnosticItem *>(diagnosticItemIndex.internalPointer());
            item->setData(DiagnosticView::DiagnosticColumn,
                          schedule ? Qt::Checked : Qt::Unchecked,
                          Qt::CheckStateRole);
        }
    }
}

DiagnosticView::~DiagnosticView()
{
    delete m_style;
}

void DiagnosticView::suppressCurrentDiagnostic()
{
    const QModelIndexList indexes = selectionModel()->selectedRows();

    // If the original project was closed, we work directly on the filter model, otherwise
    // we go via the project settings.
    const auto filterModel = static_cast<DiagnosticFilterModel *>(model());
    ProjectExplorer::Project * const project = filterModel->project();

    SuppressedDiagnosticsList diags;
    for (const QModelIndex &index : indexes) {
        const Diagnostic diag = model()->data(index, ClangToolsDiagnosticModel::DiagnosticRole)
                .value<Diagnostic>();
        if (!diag.isValid())
            continue;
        if (!project) {
            diags << diag;
            continue;
        }
        Utils::FilePath filePath = Utils::FilePath::fromString(diag.location.filePath);
        const Utils::FilePath relativeFilePath
                = filePath.relativeChildPath(project->projectDirectory());
        if (!relativeFilePath.isEmpty())
            filePath = relativeFilePath;
        const SuppressedDiagnostic supDiag(filePath, diag.description,
                                           diag.explainingSteps.count());
        diags << SuppressedDiagnostic(filePath, diag.description, diag.explainingSteps.count());
    }

    if (project)
        ClangToolsProjectSettings::getSettings(project)->addSuppressedDiagnostics(diags);
    else
        filterModel->addSuppressedDiagnostics(diags);
}

void DiagnosticView::disableCheckForCurrentDiagnostic()
{
    ClangToolsSettings * const settings = ClangToolsSettings::instance();
    ClangDiagnosticConfigs configs = settings->diagnosticConfigs();
    Utils::Id activeConfigId = settings->runSettings().diagnosticConfigId();
    ClangToolsProjectSettings::ClangToolsProjectSettingsPtr projectSettings;
    if (ProjectExplorer::Project * const project
            = static_cast<DiagnosticFilterModel *>(model())->project()) {
        projectSettings = ClangToolsProjectSettings::getSettings(project);
        if (!projectSettings->useGlobalSettings())
            activeConfigId = projectSettings->runSettings().diagnosticConfigId();
    }
    ClangDiagnosticConfig config = Utils::findOrDefault(configs,
        [activeConfigId](const ClangDiagnosticConfig &c) { return c.id() == activeConfigId; });
    const bool defaultWasActive = !config.id().isValid();
    if (defaultWasActive) {
        QTC_ASSERT(configs.isEmpty(), return);
        config = builtinConfig();
        config.setIsReadOnly(false);
        config.setId(Utils::Id::fromString(QUuid::createUuid().toString()));
        config.setDisplayName(tr("Custom Configuration"));
        configs << config;
        RunSettings runSettings = settings->runSettings();
        runSettings.setDiagnosticConfigId(config.id());
        settings->setRunSettings(runSettings);
        if (projectSettings && !projectSettings->useGlobalSettings()) {
            runSettings = projectSettings->runSettings();
            runSettings.setDiagnosticConfigId(config.id());
            projectSettings->setRunSettings(runSettings);
        }
    }

    std::set<QString> handledNames;
    const QModelIndexList indexes = selectionModel()->selectedRows();
    for (const QModelIndex &index : indexes) {
        const Diagnostic diag = model()->data(index, ClangToolsDiagnosticModel::DiagnosticRole)
                .value<Diagnostic>();
        if (!diag.isValid())
            continue;
        if (!handledNames.insert(diag.name).second)
            continue;

        if (diag.name.startsWith("clazy-")) {
            if (config.clazyMode() == ClangDiagnosticConfig::ClazyMode::UseDefaultChecks) {
                config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
                const ClazyStandaloneInfo clazyInfo(clazyStandaloneExecutable());
                config.setClazyChecks(clazyInfo.defaultChecks.join(','));
            }
            config.setClazyChecks(removeClazyCheck(config.clazyChecks(), diag.name));
        } else if (config.clangTidyMode() != ClangDiagnosticConfig::TidyMode::UseConfigFile) {
            if (config.clangTidyMode() == ClangDiagnosticConfig::TidyMode::UseDefaultChecks) {
                config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
                const ClangTidyInfo tidyInfo(clangTidyExecutable());
                config.setClangTidyChecks(tidyInfo.defaultChecks.join(','));
            }
            config.setClangTidyChecks(removeClangTidyCheck(config.clangTidyChecks(), diag.name));
        }
    }

    if (!defaultWasActive) {
        for (ClangDiagnosticConfig &c : configs) {
            if (c.id() == config.id()) {
                c = config;
                break;
            }
        }
    }
    settings->setDiagnosticConfigs(configs);
    settings->writeSettings();
}

void DiagnosticView::goNext()
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    selectIndex(getIndex(currentIndex, Next));
    openEditorForCurrentIndex();
}

void DiagnosticView::goBack()
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    selectIndex(getIndex(currentIndex, Previous));
    openEditorForCurrentIndex();
}

QModelIndex DiagnosticView::getIndex(const QModelIndex &index, Direction direction) const
{
    const QModelIndex parentIndex = index.parent();
    QModelIndex followingTopIndex = index;

    if (parentIndex.isValid()) {
        // Use direct sibling for level 2 and 3 items
        const QModelIndex followingIndex = index.sibling(index.row() + direction, 0);
        if (followingIndex.isValid())
            return followingIndex;

        // First/Last level 3 item? Continue on level 2 item.
        if (parentIndex.parent().isValid())
            return direction == Previous ? parentIndex : getIndex(parentIndex, direction);

        followingTopIndex = getTopLevelIndex(parentIndex, direction);
    }

    // Skip top level items without children
    while (!model()->hasChildren(followingTopIndex))
        followingTopIndex = getTopLevelIndex(followingTopIndex, direction);

    // Select first/last level 2 item
    const int row = direction == Next ? 0 : model()->rowCount(followingTopIndex) - 1;
    return model()->index(row, 0, followingTopIndex);
}

QModelIndex DiagnosticView::getTopLevelIndex(const QModelIndex &index, Direction direction) const
{
    QModelIndex following = index.sibling(index.row() + direction, 0);
    if (following.isValid())
        return following;
    const int row = direction == Next ? 0 : model()->rowCount() - 1;
    return model()->index(row, 0);
}

bool DiagnosticView::disableChecksEnabled() const
{
    const QList<QModelIndex> indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return false;
    if (!Utils::anyOf(indexes, &QModelIndex::isValid))
        return false;

    ClangToolsSettings * const settings = ClangToolsSettings::instance();
    const ClangDiagnosticConfigs configs = settings->diagnosticConfigs();
    Utils::Id activeConfigId = settings->runSettings().diagnosticConfigId();
    if (ProjectExplorer::Project * const project
            = static_cast<DiagnosticFilterModel *>(model())->project()) {
        const auto projectSettings = ClangToolsProjectSettings::getSettings(project);
        if (!projectSettings->useGlobalSettings())
            activeConfigId = projectSettings->runSettings().diagnosticConfigId();
    }
    const ClangDiagnosticConfig activeConfig = Utils::findOrDefault(configs,
        [activeConfigId](const ClangDiagnosticConfig &c) { return c.id() == activeConfigId; });

    // If the user has not created any custom configuration yet, then we'll do that for
    // them as an act of kindness. But if custom configurations exist and the default
    // (read-only) one is active, then we don't offer the action, as it's not clear what
    // exactly we should do there.
    if (configs.isEmpty())
        return true;
    if (!activeConfig.id().isValid())
        return false;

    // If all selected diagnostics come from clang-tidy and the active config is controlled
    // by a .clang-tidy file, then we do not offer the action.
    if (activeConfig.clangTidyMode() != ClangDiagnosticConfig::TidyMode::UseConfigFile)
        return true;
    return Utils::anyOf(indexes, [this](const QModelIndex &index) {
        return model()->data(index).toString().startsWith("clazy-");
    });
}

QList<QAction *> DiagnosticView::customActions() const
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();

    const bool hasMultiSelection = selectionModel()->selectedIndexes().size() > 1;
    const bool isDiagnosticItem = currentIndex.parent().isValid();
    const QString docUrl
        = model()->data(currentIndex, ClangToolsDiagnosticModel::DocumentationUrlRole).toString();
    m_help->setEnabled(isDiagnosticItem && !docUrl.isEmpty() && !hasMultiSelection);
    m_filterForCurrentKind->setEnabled(isDiagnosticItem && !hasMultiSelection);
    m_filterOutCurrentKind->setEnabled(isDiagnosticItem && !hasMultiSelection);
    m_suppressAction->setEnabled(isDiagnosticItem || hasMultiSelection);
    m_suppressAction->setText(hasMultiSelection ? tr("Suppress Selected Diagnostics")
                                                : tr("Suppress This Diagnostic"));
    m_disableChecksAction->setEnabled(disableChecksEnabled());
    m_disableChecksAction->setText(hasMultiSelection ? tr("Disable These Checks")
                                                     : tr("Disable This Check"));

    return {
        m_help,
        m_separator,
        m_showFilter,
        m_clearFilter,
        m_filterForCurrentKind,
        m_filterOutCurrentKind,
        m_separator2,
        m_suppressAction,
        m_disableChecksAction
    };
}

bool DiagnosticView::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyRelease: {
        const int key = static_cast<QKeyEvent *>(event)->key();
        switch (key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            openEditorForCurrentIndex();
        }
        return true;
    }
    default:
        return QAbstractItemView::eventFilter(watched, event);
    }
}

void DiagnosticView::mouseDoubleClickEvent(QMouseEvent *event)
{
    openEditorForCurrentIndex();
    Debugger::DetailedErrorView::mouseDoubleClickEvent(event);
}

void DiagnosticView::openEditorForCurrentIndex()
{
    const QVariant v = model()->data(currentIndex(), Debugger::DetailedErrorView::LocationRole);
    const auto loc = v.value<Debugger::DiagnosticLocation>();
    if (loc.isValid())
        Core::EditorManager::openEditorAt(loc.filePath, loc.line, loc.column - 1);
}

} // namespace Internal
} // namespace ClangTools

#include "clangtoolsdiagnosticview.moc"
