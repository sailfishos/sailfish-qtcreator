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

#ifndef MERSDKKITINFORMATION_H
#define MERSDKKITINFORMATION_H

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitconfigwidget.h>

QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QPushButton);

namespace Mer {
namespace Internal {

class MerSdk;
class MerSdkKitInformation;

class  MerSdkKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    MerSdkKitInformationWidget(ProjectExplorer::Kit *kit, const MerSdkKitInformation *kitInformation);

    QString displayName() const;
    QString toolTip() const;
    void makeReadOnly();
    void refresh();
    bool visibleInKit();

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;

private slots:
    void handleSdksUpdated();
    void handleManageClicked();
    void handleCurrentIndexChanged();

private:
    QComboBox *m_combo;
    QPushButton *m_manageButton;
};


class MerSdkKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    explicit MerSdkKitInformation();
    Core::Id dataId() const;
    unsigned int priority() const;
    QVariant defaultValue(ProjectExplorer::Kit *kit) const;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *kit) const;
    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *kit) const;
    void addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const;

    static void setSdk(ProjectExplorer::Kit *kit, const MerSdk* sdk);
    static MerSdk* sdk(const ProjectExplorer::Kit *kit);
};

}
}

#endif // MERSDKKITINFORMATION_H
