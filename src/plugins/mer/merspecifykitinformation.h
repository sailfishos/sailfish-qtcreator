/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#ifndef MERSPECIFYKITINFORMATIONWIDGET_H
#define MERSPECIFYKITINFORMATIONWIDGET_H

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitconfigwidget.h>

namespace Utils { class PathChooser; }

namespace Mer {
namespace Internal {

class MerSdk;

class MerSpecifyKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    MerSpecifyKitInformationWidget(ProjectExplorer::Kit *kit);

    QString displayName() const;
    void refresh();
    void makeReadOnly();
    bool visibleInKit();
    QWidget *buttonWidget() const;
    QWidget *mainWidget() const;
    QString toolTip() const;

private slots:
    void pathWasChanged();

private:
    Utils::PathChooser *m_chooser;
    bool m_ignoreChange;
};

class MerSpecifyKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    MerSpecifyKitInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(ProjectExplorer::Kit *k) const;

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const;

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const;

    ItemList toUserOutput(ProjectExplorer::Kit *k) const;

    static Utils::FileName specifyPath(const ProjectExplorer::Kit *k);
    static void setSpecifyPath(ProjectExplorer::Kit *k, const Utils::FileName &v);
};

} // namespace Internal
} // namespace Mer

#endif // MERSPECIFYKITINFORMATIONWIDGET_H
