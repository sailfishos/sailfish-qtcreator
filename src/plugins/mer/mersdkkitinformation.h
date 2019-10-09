/****************************************************************************
**
** Copyright (C) 2012-2016,2018 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include <projectexplorer/kitconfigwidget.h>
#include <projectexplorer/kitinformation.h>

QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QPushButton);

namespace Sfdk {
class BuildEngine;
class BuildTargetData;
}

namespace Mer {
namespace Internal {

class MerSdkKitInformation;

class MerSdkKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    MerSdkKitInformationWidget(ProjectExplorer::Kit *kit, const MerSdkKitInformation *kitInformation);

    QString displayName() const override;
    QString toolTip() const override;
    void makeReadOnly() override;
    void refresh() override;
    bool visibleInKit() override;

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;

private slots:
    void handleSdksUpdated();
    void handleManageClicked();
    void handleCurrentEngineIndexChanged();
    void handleCurrentTargetIndexChanged();

private:
    QWidget *m_mainWidget = nullptr;
    QComboBox *m_buildEngineComboBox = nullptr;
    QComboBox *m_buildTargetComboBox = nullptr;
    QPushButton *m_manageButton = nullptr;
};


class MerSdkKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    explicit MerSdkKitInformation();
    ~MerSdkKitInformation() override;

    QVariant defaultValue(const ProjectExplorer::Kit *kit) const override;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *kit) const override;
    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const override;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *kit) const override;
    void addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const override;

    static Core::Id id();

    static void setBuildTarget(ProjectExplorer::Kit *kit, const Sfdk::BuildEngine* buildEngine,
            const QString &buildTargetName);
    static Sfdk::BuildEngine *buildEngine(const ProjectExplorer::Kit *kit);
    static Sfdk::BuildTargetData buildTarget(const ProjectExplorer::Kit *kit);
    static QString buildTargetName(const ProjectExplorer::Kit *kit);

    static void notifyAboutUpdate(const Sfdk::BuildEngine *buildEngine);

protected:
    using KitInformation::notifyAboutUpdate;

private:
    void notifyAllUpdated();
    static QVariantMap toMap(const QUrl &buildEngineUri, const QString &buildTargetName);
    static bool fromMap(const QVariantMap &data, QUrl *buildEngineUri, QString *buildTargetName);

private:
    static MerSdkKitInformation *s_instance;
};

}
}

#endif // MERSDKKITINFORMATION_H
