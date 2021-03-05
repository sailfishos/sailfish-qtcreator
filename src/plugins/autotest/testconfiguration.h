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

#include "autotestconstants.h"

#include <projectexplorer/project.h>
#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>

#include <QFutureInterface>
#include <QObject>
#include <QPointer>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {
class TestRunConfiguration;
} // namespace Internal

class ITestFramework;
class TestOutputReader;
class TestResult;
enum class TestRunMode;

using TestResultPtr = QSharedPointer<TestResult>;

class TestConfiguration
{
public:
    explicit TestConfiguration(ITestFramework *framework);
    virtual ~TestConfiguration();

    void completeTestInformation(TestRunMode runMode);
    void completeTestInformation(ProjectExplorer::RunConfiguration *rc, TestRunMode runMode);

    void setTestCases(const QStringList &testCases);
    void setTestCaseCount(int count);
    void setExecutableFile(const QString &executableFile);
    void setProjectFile(const QString &projectFile);
    void setWorkingDirectory(const QString &workingDirectory);
    void setBuildDirectory(const QString &buildDirectory);
    void setDisplayName(const QString &displayName);
    void setEnvironment(const Utils::Environment &env);
    void setProject(ProjectExplorer::Project *project);
    void setInternalTarget(const QString &target);
    void setInternalTargets(const QSet<QString> &targets);
    void setOriginalRunConfiguration(ProjectExplorer::RunConfiguration *runConfig);

    ITestFramework *framework() const;
    QStringList testCases() const { return m_testCases; }
    int testCaseCount() const { return m_testCaseCount; }
    QString executableFilePath() const;
    QString workingDirectory() const;
    QString buildDirectory() const { return m_buildDir; }
    QString projectFile() const { return m_projectFile; }
    QString displayName() const { return m_displayName; }
    Utils::Environment environment() const { return m_runnable.environment; }
    ProjectExplorer::Project *project() const { return m_project.data(); }
    QSet<QString> internalTargets() const { return m_buildTargets; }
    ProjectExplorer::RunConfiguration *originalRunConfiguration() const { return m_origRunConfig; }
    Internal::TestRunConfiguration *runConfiguration() const { return m_runConfig; }
    bool hasExecutable() const;
    bool isDeduced() const { return m_deducedConfiguration; }
    QString runConfigDisplayName() const { return m_deducedConfiguration ? m_deducedFrom
                                                                         : m_displayName; }

    ProjectExplorer::Runnable runnable() const { return m_runnable; }
    virtual TestOutputReader *outputReader(const QFutureInterface<TestResultPtr> &fi,
                                           QProcess *app) const = 0;
    virtual QStringList argumentsForTestRunner(QStringList *omitted = nullptr) const = 0;
    virtual Utils::Environment filteredEnvironment(const Utils::Environment &original) const = 0;

private:
    ITestFramework *m_framework;
    QStringList m_testCases;
    int m_testCaseCount = 0;
    QString m_projectFile;
    QString m_buildDir;
    QString m_displayName;
    QString m_deducedFrom;
    QPointer<ProjectExplorer::Project> m_project;
    bool m_deducedConfiguration = false;
    Internal::TestRunConfiguration *m_runConfig = nullptr;
    QSet<QString> m_buildTargets;
    ProjectExplorer::RunConfiguration *m_origRunConfig = nullptr;
    ProjectExplorer::Runnable m_runnable;
};

class DebuggableTestConfiguration : public TestConfiguration
{
public:
    explicit DebuggableTestConfiguration(ITestFramework *framework, TestRunMode runMode = TestRunMode::Run)
        : TestConfiguration(framework), m_runMode(runMode) {}

    void setRunMode(TestRunMode mode) { m_runMode = mode; }
    TestRunMode runMode() const { return m_runMode; }
    bool isDebugRunMode() const;
    void setMixedDebugging(bool enable) { m_mixedDebugging = enable; }
    bool mixedDebugging() const { return m_mixedDebugging; }
private:
    TestRunMode m_runMode;
    bool m_mixedDebugging = false;
};

} // namespace Autotest
