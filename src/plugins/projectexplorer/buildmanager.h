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

#include "buildstep.h"
#include "projectexplorer_export.h"

#include <QObject>
#include <QStringList>

namespace ProjectExplorer {
class RunConfiguration;

namespace Internal { class CompileOutputSettings; }

class Task;
class Project;

enum class BuildForRunConfigStatus { Building, NotBuilding, BuildFailed };
enum class ConfigSelection { All, Active };

class PROJECTEXPLORER_EXPORT BuildManager : public QObject
{
    Q_OBJECT

public:
    explicit BuildManager(QObject *parent, QAction *cancelBuildAction);
    ~BuildManager() override;
    static BuildManager *instance();

    static void extensionsInitialized();

    static void buildProjectWithoutDependencies(Project *project);
    static void cleanProjectWithoutDependencies(Project *project);
    static void rebuildProjectWithoutDependencies(Project *project);
    static void buildProjectWithDependencies(
            Project *project,
            ConfigSelection configSelection = ConfigSelection::Active
            );
    static void cleanProjectWithDependencies(Project *project, ConfigSelection configSelection);
    static void rebuildProjectWithDependencies(Project *project, ConfigSelection configSelection);
    static void buildProjects(const QList<Project *> &projects, ConfigSelection configSelection);
    static void cleanProjects(const QList<Project *> &projects, ConfigSelection configSelection);
    static void rebuildProjects(const QList<Project *> &projects, ConfigSelection configSelection);

    static void deployProjects(const QList<Project *> &projects);

    static BuildForRunConfigStatus potentiallyBuildForRunConfig(RunConfiguration *rc);

    static bool isBuilding();
    static bool isDeploying();
    static bool tasksAvailable();

    static bool buildLists(const QList<BuildStepList *> bsls,
                           const QStringList &preambelMessage = QStringList());
    static bool buildList(BuildStepList *bsl);

    static bool isBuilding(const Project *p);
    static bool isBuilding(const Target *t);
    static bool isBuilding(const ProjectConfiguration *p);
    static bool isBuilding(BuildStep *step);

    // Append any build step to the list of build steps (currently only used to add the QMakeStep)
    static void appendStep(BuildStep *step, const QString &name);

    static int getErrorTaskCount();

    static void setCompileOutputSettings(const Internal::CompileOutputSettings &settings);
    static const Internal::CompileOutputSettings &compileOutputSettings();

    static QString displayNameForStepId(Utils::Id stepId);

public slots:
    static void cancel();
    // Shows without focus
    static void showTaskWindow();
    static void toggleTaskWindow();
    static void toggleOutputWindow();
    static void aboutToRemoveProject(ProjectExplorer::Project *p);

signals:
    void buildStateChanged(ProjectExplorer::Project *pro);
    void buildQueueFinished(bool success);

private:
    static void addToTaskWindow(const ProjectExplorer::Task &task, int linkedOutputLines, int skipLines);
    static void addToOutputWindow(const QString &string, BuildStep::OutputFormat format,
                           BuildStep::OutputNewlineSetting newlineSettings = BuildStep::DoAppendNewline);

    static void nextBuildQueue();
    static void progressChanged(int percent, const QString &text);
    static void emitCancelMessage();
    static void showBuildResults();
    static void updateTaskCount();
    static void finish();

    static void startBuildQueue();
    static void nextStep();
    static void clearBuildQueue();
    static bool buildQueueAppend(const QList<BuildStep *> &steps, QStringList names, const QStringList &preambleMessage = QStringList());
    static void incrementActiveBuildSteps(BuildStep *bs);
    static void decrementActiveBuildSteps(BuildStep *bs);
    static void disconnectOutput(BuildStep *bs);
};

} // namespace ProjectExplorer
