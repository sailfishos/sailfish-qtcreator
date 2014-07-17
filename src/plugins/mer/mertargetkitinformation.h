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

#ifndef MERTARGETKITINFORMATION_H
#define MERTARGETKITINFORMATION_H

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitconfigwidget.h>
#include "mertarget.h"

QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QPushButton);

namespace Mer {
namespace Internal {

class MerTargetKitInformation;

class  MerTargetKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    MerTargetKitInformationWidget(ProjectExplorer::Kit *kit,
          const MerTargetKitInformation *kitInformation);

    QString displayName() const;
    QString toolTip() const;
    void makeReadOnly();
    void refresh();
    bool visibleInKit();

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;

private slots:
    void handleManageClicked();
    void handleCurrentIndexChanged();
    void handleSdksUpdated();

private:
    QComboBox *m_combo;
    QPushButton *m_manageButton;
};


class MerTargetKitInformation : public ProjectExplorer::KitInformation
{
public:
    explicit MerTargetKitInformation();
    Core::Id dataId() const;
    unsigned int priority() const;
    QVariant defaultValue(ProjectExplorer::Kit *kit) const;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *kit) const;
    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *kit) const;
    void addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const;

    static void setTargetName(ProjectExplorer::Kit *kit, const QString& targetName);
    static QString targetName(const ProjectExplorer::Kit *kit);

};

}
}

#endif // MERTARGETKITINFORMATION_H
