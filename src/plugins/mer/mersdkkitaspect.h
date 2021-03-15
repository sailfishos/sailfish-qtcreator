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

#ifndef MERSDKKITASPECT_H
#define MERSDKKITASPECT_H

#include <projectexplorer/kitmanager.h>

QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QPushButton);

namespace Sfdk {
class BuildEngine;
class BuildTargetData;
}

namespace Mer {
namespace Internal {

class MerSdkKitAspect;

class MerSdkKitAspectWidget : public ProjectExplorer::KitAspectWidget
{
    Q_OBJECT
public:
    MerSdkKitAspectWidget(ProjectExplorer::Kit *kit, const MerSdkKitAspect *kitAspect);

    void makeReadOnly() override;
    void refresh() override;

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


class MerSdkKitAspect : public ProjectExplorer::KitAspect
{
    Q_OBJECT
public:
    explicit MerSdkKitAspect();
    ~MerSdkKitAspect() override;

    bool isApplicableToKit(const ProjectExplorer::Kit *kit) const override;
    ProjectExplorer::Tasks validate(const ProjectExplorer::Kit *kit) const override;
    ItemList toUserOutput(const ProjectExplorer::Kit *kit) const override;
    ProjectExplorer::KitAspectWidget *createConfigWidget(ProjectExplorer::Kit *kit) const override;
    void addToEnvironment(const ProjectExplorer::Kit *kit, Utils::Environment &env) const override;

    static Utils::Id id();

    static void setBuildTarget(ProjectExplorer::Kit *kit, const Sfdk::BuildEngine* buildEngine,
            const QString &buildTargetName);
    static Sfdk::BuildEngine *buildEngine(const ProjectExplorer::Kit *kit);
    static Sfdk::BuildTargetData buildTarget(const ProjectExplorer::Kit *kit);
    static QString buildTargetName(const ProjectExplorer::Kit *kit);

    static void notifyAboutUpdate(const Sfdk::BuildEngine *buildEngine);

protected:
    using KitAspect::notifyAboutUpdate;

private:
    void notifyAllUpdated();
    static QVariantMap toMap(const QUrl &buildEngineUri, const QString &buildTargetName);
    static bool fromMap(const QVariantMap &data, QUrl *buildEngineUri, QString *buildTargetName);

private:
    static MerSdkKitAspect *s_instance;
};

}
}

#endif // MERSDKKITASPECT_H
