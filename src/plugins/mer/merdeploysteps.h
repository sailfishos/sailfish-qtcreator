/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MERDEPLOYSTEPS_H
#define MERDEPLOYSTEPS_H

#include "ui_merdeploystep.h"
#include "merabstractvmstartstep.h"

#include <projectexplorer/abstractprocessstep.h>
#include <ssh/sshconnection.h>

namespace Mer {
namespace Internal {

class MerProcessStep: public ProjectExplorer::AbstractProcessStep
{
public:
    explicit MerProcessStep(ProjectExplorer::BuildStepList *bsl,const Core::Id id);
    MerProcessStep(ProjectExplorer::BuildStepList *bsl, MerProcessStep *bs);
    bool init();
    QString arguments() const;
    void setArguments(const QString &arguments);

private:
    QString m_arguments;
};

class MerEmulatorStartStep : public MerAbstractVmStartStep
{
    Q_OBJECT
public:
    explicit MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl);
    MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl, MerEmulatorStartStep *bs);
    bool init();
    static const Core::Id stepId();
    static QString displayName();
    friend class MerDeployStepFactory;
};

class MerMb2RsyncDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerMb2RsyncDeployStep(ProjectExplorer::BuildStepList *bsl);
    MerMb2RsyncDeployStep(ProjectExplorer::BuildStepList *bsl, MerMb2RsyncDeployStep *bs);
    bool init();
    bool immutable() const;
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    static const Core::Id stepId();
    static QString displayName();
private:
    friend class MerDeployStepFactory;
};

class MerLocalRsyncDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerLocalRsyncDeployStep(ProjectExplorer::BuildStepList *bsl);
    MerLocalRsyncDeployStep(ProjectExplorer::BuildStepList *bsl, MerLocalRsyncDeployStep *bs);
    bool init();
    bool immutable() const;
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    static const Core::Id stepId();
    static QString displayName();
    friend class MerDeployStepFactory;
};

class MerMb2RpmDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerMb2RpmDeployStep(ProjectExplorer::BuildStepList *bsl);
    MerMb2RpmDeployStep(ProjectExplorer::BuildStepList *bsl, MerMb2RpmDeployStep *bs);
    bool init();
    bool immutable() const;
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    static const Core::Id stepId();
    static QString displayName();
    friend class MerDeployStepFactory;
};

//TODO: HACK
class MerMb2RpmBuildStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerMb2RpmBuildStep(ProjectExplorer::BuildStepList *bsl);
    MerMb2RpmBuildStep(ProjectExplorer::BuildStepList *bsl, MerMb2RpmBuildStep *bs);
    bool init();
    bool immutable() const;
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    static const Core::Id stepId();
    static QString displayName();
    friend class MerDeployStepFactory;
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void stdOutput(const QString &line);
    QStringList packagesFilePath() const;
private:
    QString m_sharedHome;
    QString m_sharedSrc;
    QStringList m_packages;
};

class RpmInfo: public QObject
{
    Q_OBJECT
public:
    RpmInfo(const QStringList &list);
public slots:
    void info();
private:
    QStringList m_list;
};

class MerDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MerDeployStepWidget(MerProcessStep *step);
    QString displayName() const;
    QString summaryText() const;
    QString commnadText() const;
    void setCommandText(const QString& commandText);
    void setDisplayName(const QString& summaryText);
    void setSummaryText(const QString& displayText);
private slots:
    void commandArgumentsLineEditTextEdited();
private:
    MerProcessStep *m_step;
    Ui::MerDeployStepWidget m_ui;
    QString m_displayText;
    QString m_summaryText;
};

} // namespace Internal
} // namespace Mer

#endif // MERDEPLOYSTEPS_H
