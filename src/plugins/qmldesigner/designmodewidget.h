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

#include <coreplugin/minisplitter.h>
#include <utils/faketooltip.h>

#include <designdocument.h>
#include <modelnode.h>

#include <QWidget>
#include <QMainWindow>
#include <QScopedPointer>

#include <advanceddockingsystem/dockmanager.h>

namespace Core {
    class SideBar;
    class SideBarItem;
    class EditorToolBar;
    class OutputPanePlaceHolder;
}

namespace QmlDesigner {

class ItemLibraryWidget;
class CrumbleBar;
class DocumentWarningWidget;
class SwitchSplitTabWidget;

namespace Internal {

class DesignMode;
class DocumentWidget;

class DesignModeWidget : public QMainWindow
{
    Q_OBJECT

public:
    DesignModeWidget();
    ~DesignModeWidget() override;

    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    void initialize();

    void readSettings();
    void saveSettings();

    DesignDocument *currentDesignDocument() const;
    ViewManager &viewManager();

    void setupNavigatorHistory(Core::IEditor *editor);

    void enableWidgets();
    void disableWidgets();

    CrumbleBar* crumbleBar() const;
    void showInternalTextEditor();

    void determineWorkspaceToRestoreAtStartup();

    static QWidget *createProjectExplorerWidget(QWidget *parent);

private: // functions
    enum InitializeStatus { NotInitialized, Initializing, Initialized };

    void toolBarOnGoBackClicked();
    void toolBarOnGoForwardClicked();

    void setup();
    bool isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const;
    QmlDesigner::ModelNode nodeForPosition(int cursorPos) const;
    void addNavigatorHistoryEntry(const Utils::FilePath &fileName);
    QWidget *createCenterWidget();
    QWidget *createCrumbleBarFrame();

    void aboutToShowWorkspaces();

private: // variables
    SwitchSplitTabWidget* m_centralTabWidget = nullptr;

    QPointer<QWidget> m_bottomSideBar;
    Core::EditorToolBar *m_toolBar;
    CrumbleBar *m_crumbleBar;
    bool m_isDisabled = false;
    bool m_showSidebars = true;

    InitializeStatus m_initStatus = NotInitialized;

    QStringList m_navigatorHistory;
    int m_navigatorHistoryCounter = -1;
    bool m_keepNavigatorHistory = false;

    QList<QPointer<QWidget> >m_viewWidgets;

    ADS::DockManager *m_dockManager = nullptr;
    ADS::DockWidget *m_outputPaneDockWidget = nullptr;
};

} // namespace Internal
} // namespace Designer
