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

#include "projectexplorer_export.h"

#include "buildinfo.h"
#include "kit.h"
#include "task.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpacerItem;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
} // namespace Utils

namespace ProjectExplorer {
class BuildInfo;

namespace Internal {

class TargetSetupWidget : public QWidget
{
    Q_OBJECT

public:
    TargetSetupWidget(Kit *k, const Utils::FilePath &projectPath);

    Kit *kit() const;
    void clearKit();

    bool isKitSelected() const;
    void setKitSelected(bool b);

    void addBuildInfo(const BuildInfo &info, bool isImport);

    const QList<BuildInfo> selectedBuildInfoList() const;
    void setProjectPath(const Utils::FilePath &projectPath);
    void expandWidget();
    void update(const TasksGenerator &generator);

signals:
    void selectedToggled() const;

private:
    static const QList<BuildInfo> buildInfoList(const Kit *k, const Utils::FilePath &projectPath);

    bool hasSelectedBuildConfigurations() const;

    void toggleEnabled(bool enabled);
    void checkBoxToggled(bool b);
    void pathChanged();
    void targetCheckBoxToggled(bool b);
    void manageKit();

    void reportIssues(int index);
    QPair<Task::TaskType, QString> findIssues(const BuildInfo &info);
    void clear();
    void updateDefaultBuildDirectories();

    Kit *m_kit;
    Utils::FilePath m_projectPath;
    bool m_haveImported = false;
    Utils::DetailsWidget *m_detailsWidget;
    QPushButton *m_manageButton;
    QGridLayout *m_newBuildsLayout;

    struct BuildInfoStore {
        ~BuildInfoStore();
        BuildInfoStore() = default;
        BuildInfoStore(const BuildInfoStore &other) = delete;
        BuildInfoStore(BuildInfoStore &&other);
        BuildInfoStore &operator=(const BuildInfoStore &other) = delete;
        BuildInfoStore &operator=(BuildInfoStore &&other) = delete;

        BuildInfo buildInfo;
        QCheckBox *checkbox = nullptr;
        QLabel *label = nullptr;
        QLabel *issuesLabel = nullptr;
        Utils::PathChooser *pathChooser = nullptr;
        bool isEnabled = false;
        bool hasIssues = false;
        bool customBuildDir = false;
    };
    std::vector<BuildInfoStore> m_infoStore;

    bool m_ignoreChange = false;
    int m_selected = 0; // Number of selected "buildconfigurations"
};

} // namespace Internal
} // namespace ProjectExplorer
