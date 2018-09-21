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

#include <projectexplorer/runnables.h>

#include <QCheckBox>
#include <QHash>
#include <QLabel>
#include <QPair>
#include <QStringList>
#include <QWidget>

namespace qbs { class InstallOptions; }

namespace ProjectExplorer { class BuildStepList; }

namespace QbsProjectManager {
namespace Internal {

class QbsInstallStep;

class QbsRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

    // to change the display name and arguments and set the userenvironmentchanges
    friend class QbsRunConfigurationWidget;
    friend class ProjectExplorer::IRunConfigurationFactory;

public:
    explicit QbsRunConfiguration(ProjectExplorer::Target *target);

    QWidget *createConfigurationWidget() override;

    ProjectExplorer::Runnable runnable() const override;

    QString executable() const;
    Utils::OutputFormatter *createOutputFormatter() const override;

    void addToBaseEnvironment(Utils::Environment &env) const;

    QString buildSystemTarget() const final;
    QString uniqueProductName() const;
    bool isConsoleApplication() const;
    bool usingLibraryPaths() const { return m_usingLibraryPaths; }
    void setUsingLibraryPaths(bool useLibPaths);

signals:
    void targetInformationChanged();
    void usingDyldImageSuffixChanged(bool);

private:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &map) final;
    QString extraId() const final;

    void installStepChanged();
    void installStepToBeRemoved(int pos);
    QString baseWorkingDirectory() const;
    QString defaultDisplayName();

    void updateTarget();

    using EnvCache = QHash<QPair<QStringList, bool>, Utils::Environment>;
    mutable EnvCache m_envCache;

    QbsInstallStep *m_currentInstallStep = nullptr; // We do not take ownership!
    ProjectExplorer::BuildStepList *m_currentBuildStepList = nullptr; // We do not take ownership!
    QString m_uniqueProductName;
    QString m_productDisplayName;
    bool m_usingLibraryPaths = true;
};

class QbsRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    QbsRunConfigurationWidget(QbsRunConfiguration *rc);

private:
    void runConfigurationEnabledChange();
    void targetInformationHasChanged();
    void setExecutableLineText(const QString &text = QString());

    QbsRunConfiguration *m_rc;
    QLabel *m_executableLineLabel;
    QCheckBox *m_usingLibPathsCheckBox;
    bool m_ignoreChange = false;
    bool m_isShown = false;
};

class QbsRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit QbsRunConfigurationFactory(QObject *parent = 0);

    bool canCreateHelper(ProjectExplorer::Target *parent, const QString &suffix) const override;

    QList<ProjectExplorer::BuildTargetInfo>
        availableBuildTargets(ProjectExplorer::Target *parent, CreationMode mode) const override;
};

} // namespace Internal
} // namespace QbsProjectManager
