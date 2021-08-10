/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "itemdatacache.h"
#include "testsettings.h"

namespace ProjectExplorer { class Project; }

namespace Autotest {

class ITestFramework;
class ITestTool;

namespace Internal {

class TestProjectSettings : public QObject
{
    Q_OBJECT
public:
    explicit TestProjectSettings(ProjectExplorer::Project *project);
    ~TestProjectSettings();

    void setUseGlobalSettings(bool useGlobal);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setRunAfterBuild(RunAfterBuildMode mode) {m_runAfterBuild = mode; }
    RunAfterBuildMode runAfterBuild() const { return m_runAfterBuild; }
    void setActiveFrameworks(const QHash<ITestFramework *, bool> &enabledFrameworks)
    { m_activeTestFrameworks = enabledFrameworks; }
    QHash<ITestFramework *, bool> activeFrameworks() const { return m_activeTestFrameworks; }
    void activateFramework(const Utils::Id &id, bool activate);
    void setActiveTestTools(const QHash<ITestTool *, bool> &enabledTestTools)
    { m_activeTestTools = enabledTestTools; }
    QHash<ITestTool *, bool> activeTestTools() const { return m_activeTestTools; }
    void activateTestTool(const Utils::Id &id, bool activate);
    Internal::ItemDataCache<Qt::CheckState> *checkStateCache() { return &m_checkStateCache; }
private:
    void load();
    void save();

    ProjectExplorer::Project *m_project;
    bool m_useGlobalSettings = true;
    RunAfterBuildMode m_runAfterBuild = RunAfterBuildMode::None;
    QHash<ITestFramework *, bool> m_activeTestFrameworks;
    QHash<ITestTool *, bool> m_activeTestTools;
    Internal::ItemDataCache<Qt::CheckState> m_checkStateCache;
};

} // namespace Internal
} // namespace Autotest
