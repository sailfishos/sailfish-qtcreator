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
#include "namedwidget.h"
#include <utils/detailsbutton.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QToolButton;
class QLabel;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer {
namespace Internal {

class ToolWidget : public Utils::FadingPanel
{
    Q_OBJECT
public:
    explicit ToolWidget(QWidget *parent = nullptr);

    void fadeTo(qreal value) override;
    void setOpacity(qreal value) override;

    void setBuildStepEnabled(bool b);
    void setUpEnabled(bool b);
    void setDownEnabled(bool b);
    void setRemoveEnabled(bool b);
    void setUpVisible(bool b);
    void setDownVisible(bool b);

signals:
    void disabledClicked();
    void upClicked();
    void downClicked();
    void removeClicked();

private:
    QToolButton *m_disableButton;
    QToolButton *m_upButton;
    QToolButton *m_downButton;
    QToolButton *m_removeButton;

    bool m_buildStepEnabled = true;
    Utils::FadingWidget *m_firstWidget;
    Utils::FadingWidget *m_secondWidget;
    qreal m_targetOpacity = .999;
};

class BuildStepsWidgetData
{
public:
    BuildStepsWidgetData(BuildStep *s);
    ~BuildStepsWidgetData();

    BuildStep *step;
    QWidget *widget;
    Utils::DetailsWidget *detailsWidget;
    ToolWidget *toolWidget;
};

class BuildStepListWidget : public NamedWidget
{
    Q_OBJECT

public:
    explicit BuildStepListWidget(BuildStepList *bsl);
    ~BuildStepListWidget() override;

private:
    void updateAddBuildStepMenu();
    void addBuildStep(int pos);
    void stepMoved(int from, int to);
    void removeBuildStep(int pos);

    void setupUi();
    void updateBuildStepButtonsState();

    BuildStepList *m_buildStepList = nullptr;

    QList<Internal::BuildStepsWidgetData *> m_buildStepsData;

    QVBoxLayout *m_vbox = nullptr;

    QLabel *m_noStepsLabel = nullptr;
    QPushButton *m_addButton = nullptr;
};

} // Internal
} // ProjectExplorer
