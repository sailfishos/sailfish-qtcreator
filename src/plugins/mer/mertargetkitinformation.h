/****************************************************************************
**
** Copyright (C) 2012-2016,2018-2019 Jolla Ltd.
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

#include "mertarget.h"

#include <projectexplorer/kitconfigwidget.h>
#include <projectexplorer/kitinformation.h>

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

    QString displayName() const override;
    QString toolTip() const override;
    void makeReadOnly() override;
    void refresh() override;
    bool visibleInKit() override;

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;

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
    QVariant defaultValue(const ProjectExplorer::Kit *kit) const override;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *kit) const override;
    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const override;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *kit) const override;
    void addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const override;

    static Core::Id id();
    static void setTargetName(ProjectExplorer::Kit *kit, const QString& targetName);
    static QString targetName(const ProjectExplorer::Kit *kit);
    static MerTarget target(const ProjectExplorer::Kit *kit);
};

}
}

#endif // MERTARGETKITINFORMATION_H
