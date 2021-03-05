/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimbletaskstep.h"

#include "nimconstants.h"
#include "nimblebuildsystem.h"
#include "nimbleproject.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardPaths>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

class NimbleTaskStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(Nim::NimbleTaskStep)

public:
    NimbleTaskStep(BuildStepList *parentList, Id id);

private:
    QWidget *createConfigWidget() final;

    void setTaskName(const QString &name);

    void updateTaskList();
    void selectTask(const QString &name);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void uncheckedAllDifferentFrom(QStandardItem *item);

    bool validate();

    StringAspect *m_taskName = nullptr;
    StringAspect *m_taskArgs = nullptr;

    QStandardItemModel m_tasks;
    bool m_selecting = false;
};

NimbleTaskStep::NimbleTaskStep(BuildStepList *parentList, Id id)
    : AbstractProcessStep(parentList, id)
{
    setDefaultDisplayName(tr(Constants::C_NIMBLETASKSTEP_DISPLAY));
    setDisplayName(tr(Constants::C_NIMBLETASKSTEP_DISPLAY));

    setCommandLineProvider([this] {
        QString args = m_taskName->value() + " " + m_taskArgs->value();
        return CommandLine(Nim::nimblePathFromKit(target()->kit()), args, CommandLine::Raw);
    });

    setWorkingDirectoryProvider([this] { return project()->projectDirectory(); });

    m_taskName = addAspect<StringAspect>();
    m_taskName->setSettingsKey(Constants::C_NIMBLETASKSTEP_TASKNAME);

    m_taskArgs = addAspect<StringAspect>();
    m_taskArgs->setSettingsKey(Constants::C_NIMBLETASKSTEP_TASKARGS);
    m_taskArgs->setDisplayStyle(StringAspect::LineEditDisplay);
    m_taskArgs->setLabelText(tr("Task arguments:"));
}

QWidget *NimbleTaskStep::createConfigWidget()
{
    auto widget = new QWidget;

    auto taskList = new QListView(widget);
    taskList->setFrameShape(QFrame::StyledPanel);
    taskList->setSelectionMode(QAbstractItemView::NoSelection);
    taskList->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskList->setModel(&m_tasks);

    LayoutBuilder builder(widget);
    builder.addRow(m_taskArgs);
    builder.addRow({tr("Tasks:"), taskList});

    auto buildSystem = dynamic_cast<NimbleBuildSystem *>(this->buildSystem());
    QTC_ASSERT(buildSystem, return widget);

    updateTaskList();
    selectTask(m_taskName->value());

    connect(&m_tasks, &QAbstractItemModel::dataChanged, this, &NimbleTaskStep::onDataChanged);

    connect(buildSystem, &NimbleBuildSystem::tasksChanged, this, &NimbleTaskStep::updateTaskList);

    setSummaryUpdater([this] {
        return QString("<b>%1:</b> nimble %2 %3")
                .arg(displayName(), m_taskName->value(), m_taskArgs->value());
    });

    return widget;
}

void NimbleTaskStep::updateTaskList()
{
    auto buildSystem = dynamic_cast<NimbleBuildSystem *>(this->buildSystem());
    QTC_ASSERT(buildSystem, return);
    const std::vector<NimbleTask> &tasks = buildSystem->tasks();

    QSet<QString> newTasks;
    for (const NimbleTask &t : tasks)
        newTasks.insert(t.name);

    QSet<QString> currentTasks;
    for (int i = 0; i < m_tasks.rowCount(); ++i)
        currentTasks.insert(m_tasks.item(i)->text());

    const QSet<QString> added = newTasks - currentTasks;
    const QSet<QString> removed = currentTasks - newTasks;

    for (const QString &name : added) {
        auto item = new QStandardItem();
        item->setText(name);
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        item->setEditable(false);
        item->setSelectable(false);
        m_tasks.appendRow(item);
    }

    for (int i = m_tasks.rowCount() - 1; i >= 0; i--)
        if (removed.contains(m_tasks.item(i)->text()))
            m_tasks.removeRow(i);

    m_tasks.sort(0);
}

void NimbleTaskStep::selectTask(const QString &name)
{
    if (m_selecting)
        return;
    m_selecting = true;

    QList<QStandardItem *> items = m_tasks.findItems(name);
    QStandardItem *item = items.empty() ? nullptr : items.front();
    uncheckedAllDifferentFrom(item);
    if (item)
        item->setCheckState(Qt::Checked);

    setTaskName(name);

    m_selecting = false;
}

void NimbleTaskStep::onDataChanged(const QModelIndex &topLeft,
                                   const QModelIndex &bottomRight,
                                   const QVector<int> &roles)
{
    QTC_ASSERT(topLeft == bottomRight, return );
    if (!roles.contains(Qt::CheckStateRole))
        return;

    auto item = m_tasks.itemFromIndex(topLeft);
    if (!item)
        return;

    if (m_selecting)
        return;
    m_selecting = true;

    if (item->checkState() == Qt::Checked) {
        uncheckedAllDifferentFrom(item);
        setTaskName(item->text());
    } else if (item->checkState() == Qt::Unchecked) {
        setTaskName(QString());
    }

    m_selecting = false;
}

void NimbleTaskStep::uncheckedAllDifferentFrom(QStandardItem *toSkip)
{
    for (int i = 0; i < m_tasks.rowCount(); ++i) {
        auto item = m_tasks.item(i);
        if (!item || item == toSkip)
            continue;
        item->setCheckState(Qt::Unchecked);
    }
}

void NimbleTaskStep::setTaskName(const QString &name)
{
    if (m_taskName->value() == name)
        return;
    m_taskName->setValue(name);
    selectTask(name);
}

bool NimbleTaskStep::validate()
{
    if (m_taskName->value().isEmpty())
        return true;

    auto nimbleBuildSystem = dynamic_cast<NimbleBuildSystem*>(buildSystem());
    QTC_ASSERT(nimbleBuildSystem, return false);

    auto matchName = [this](const NimbleTask &task) { return task.name == m_taskName->value(); };
    if (!Utils::contains(nimbleBuildSystem->tasks(), matchName)) {
        emit addTask(BuildSystemTask(Task::Error, tr("Nimble task %1 not found.")
                                     .arg(m_taskName->value())));
        emitFaultyConfigurationMessage();
        return false;
    }

    return true;
}

// Factory

NimbleTaskStepFactory::NimbleTaskStepFactory()
{
    registerStep<NimbleTaskStep>(Constants::C_NIMBLETASKSTEP_ID);
    setDisplayName(NimbleTaskStep::tr("Nimble Task"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN,
                           ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
    setSupportedConfiguration(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setRepeatable(true);
}

} // Nim
