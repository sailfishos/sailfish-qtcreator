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

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/projectconfigurationaspects.h>

namespace WinRt {
namespace Internal {

class WinRtArgumentsAspect : public ProjectExplorer::ProjectConfigurationAspect
{
    Q_OBJECT

public:
    WinRtArgumentsAspect();
    ~WinRtArgumentsAspect() override;

    void addToLayout(ProjectExplorer::LayoutBuilder &builder) override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void setValue(const QString &value);
    QString value() const;

    void setDefaultValue(const QString &value);
    QString defaultValue() const;

    void restoreDefaultValue();

private:
    Utils::FancyLineEdit *m_lineEdit = nullptr;
    QString m_value;
    QString m_defaultValue;
};

class WinRtPackageDeploymentStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    WinRtPackageDeploymentStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    QString defaultWinDeployQtArguments() const;

    void raiseError(const QString &errorMessage);
    void raiseWarning(const QString &warningMessage);

private:
    bool init() override;
    void doRun() override;
    bool processSucceeded(int exitCode, QProcess::ExitStatus status) override;
    void stdOutput(const QString &line) override;

    bool parseIconsAndExecutableFromManifest(QString manifestFileName, QStringList *items, QString *executable);

    WinRtArgumentsAspect *m_argsAspect = nullptr;
    QString m_targetFilePath;
    QString m_targetDirPath;
    QString m_executablePathInManifest;
    QString m_mappingFileContent;
    QString m_manifestFileName;
    bool m_createMappingFile = false;
};

} // namespace Internal
} // namespace WinRt
