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

#pragma once

#include "testresult.h"

#include <coreplugin/ioutputpane.h>

#include <utils/ansiescapecodehandler.h>
#include <utils/itemviews.h>

QT_BEGIN_NAMESPACE
class QAction;
class QFrame;
class QKeyEvent;
class QLabel;
class QModelIndex;
class QMenu;
class QPlainTextEdit;
class QStackedWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Core {
class IContext;
}

namespace Autotest {

enum class OutputChannel;
class TestResult;

namespace Internal {

class TestResultModel;
class TestResultFilterModel;
class TestEditorMark;

class ResultsTreeView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit ResultsTreeView(QWidget *parent = nullptr);

signals:
    void copyShortcutTriggered();

protected:
    void keyPressEvent(QKeyEvent *event) final;
};

class TestResultsPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    ~TestResultsPane() override;
    static TestResultsPane *instance();

    // IOutputPane interface
    QWidget *outputWidget(QWidget *parent) override;
    QList<QWidget *> toolBarWidgets() const override;
    QString displayName() const override;
    int priorityInStatusBar() const override;
    void clearContents() override;
    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;
    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

    void addTestResult(const TestResultPtr &result);
    void addOutputLine(const QByteArray &outputLine, OutputChannel channel);
    void showTestResult(const QModelIndex &index);
private:
    explicit TestResultsPane(QObject *parent = nullptr);

    void onItemActivated(const QModelIndex &index);
    void onRunAllTriggered();
    void onRunSelectedTriggered();
    void checkAllFilter(bool checked);
    void filterMenuTriggered(QAction *action);

    void initializeFilterMenu();
    void updateSummaryLabel();
    void createToolButtons();
    void onTestRunStarted();
    void onTestRunFinished();
    void onScrollBarRangeChanged(int, int max);
    void onCustomContextMenuRequested(const QPoint &pos);
    const TestResult *getTestResult(const QModelIndex &idx);
    void onCopyItemTriggered(const TestResult *result);
    void onCopyWholeTriggered();
    void onSaveWholeTriggered();
    void onRunThisTestTriggered(TestRunMode runMode, const TestResult *result);
    void toggleOutputStyle();
    QString getWholeOutput(const QModelIndex &parent = QModelIndex());

    void createMarks(const QModelIndex &parent = QModelIndex());
    void clearMarks();

    QStackedWidget *m_outputWidget;
    QFrame *m_summaryWidget;
    QLabel *m_summaryLabel;
    ResultsTreeView *m_treeView;
    TestResultModel *m_model;
    TestResultFilterModel *m_filterModel;
    Core::IContext *m_context;
    QToolButton *m_expandCollapse;
    QToolButton *m_runAll;
    QToolButton *m_runSelected;
    QToolButton *m_runFailed;
    QToolButton *m_runFile;
    QToolButton *m_stopTestRun;
    QToolButton *m_filterButton;
    QToolButton *m_outputToggleButton;
    QPlainTextEdit *m_textOutput;
    QMenu *m_filterMenu;
    bool m_autoScroll = false;
    bool m_atEnd = false;
    bool m_testRunning = false;
    QVector<TestEditorMark *> m_marks;
    QTextCharFormat m_defaultFormat;
    Utils::AnsiEscapeCodeHandler m_stdErrHandler;
    Utils::AnsiEscapeCodeHandler m_stdOutHandler;
};

} // namespace Internal
} // namespace Autotest
