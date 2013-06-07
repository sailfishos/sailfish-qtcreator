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
#include <projectexplorer/abstractprocessstep.h>

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

class MerRsyncDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerRsyncDeployStep(ProjectExplorer::BuildStepList *bsl);
    MerRsyncDeployStep(ProjectExplorer::BuildStepList *bsl, MerRsyncDeployStep *bs);

    bool init();
    bool immutable() const;
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    static const Core::Id stepId();
    static QString displayName();
private:
    friend class MerDeployStepFactory;
};

class MerRpmDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerRpmDeployStep(ProjectExplorer::BuildStepList *bsl);
    MerRpmDeployStep(ProjectExplorer::BuildStepList *bsl, MerRpmDeployStep *bs);

    bool init();
    bool immutable() const;
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    static const Core::Id stepId();
    static QString displayName();
    friend class MerDeployStepFactory;
};

class MerDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MerDeployStepWidget(const QString& displayText, const QString& summaryText, MerProcessStep *step);
    QString displayName() const;
    QString summaryText() const;
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
