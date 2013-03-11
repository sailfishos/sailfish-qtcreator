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

#ifndef MEROPTIONSWIDGET_H
#define MEROPTIONSWIDGET_H

#include "mersdk.h"

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

namespace Ui {
class MerOptionsWidget;
}

class SdkDetailsWidget;
class SdkToolChainUtils;
class SdkTargetUtils;
class MerOptionsWidget : public QWidget
{
    Q_OBJECT
    enum Roles {
        SdkRole = Qt::UserRole + 1
    };

    enum Command {
        None,
        UpdateTargets,
        InstallTargets
    };

    enum State {
        Idle,
        SaveCurrentData,
        CreateNewScripts,
        UpdateToolChains,
        UpdateQtVersions,
        UpdateKits,
        RemoveInvalidScripts,
        Done
    };

public:
    explicit MerOptionsWidget(QWidget *parent = 0);
    ~MerOptionsWidget();

    QString searchKeyWordMatchString() const;
    void saveModelData();

signals:
    void updateSearchKeys();

private slots:
    void testConnection(bool updateTargets = true);

    // Ui
    void showProgress(const QString &message);
    void onSdkChanged(int currentIndex);
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onUpdateKitsButtonClicked(const QStringList &targets);
    void onTestConnectionButtonClicked();
    void onDeployPublicKeyButtonClicked();
    void onStartVirtualMachineButtonClicked();
    void onLaunchSDKControlCenterClicked();

    // command
    void onInstalledTargets(const QString &sdkName, const QStringList &targets);
    void onConnectionChanged(const QString &sdkName, bool success);
    void onConnectionError(const QString &sdkName, const QString &error);

    // timer
    void changeState();

    // others
    void onCreateSshKey(const QString &path);

    void initModel();

private:

    void setShowDetailWidgets(bool show);
    void fetchVirtualMachineInfo();
    void updateTargets();
    void installTargets();
    void saveCurrentData();

private:
    Ui::MerOptionsWidget *m_ui;
    SdkDetailsWidget *m_detailsWidget;
    QStandardItemModel *m_model;
    SdkToolChainUtils *m_toolChainUtils;
    SdkTargetUtils *m_targetUtils;
    int m_currentSdkIndex;
    Command m_currentCommand;
    QTimer m_timer;
    State m_state;
    MerSdk m_tempSdk;
    QStringList m_targetsToInstall;
};

} // Internal
} // Mer

#endif // MEROPTIONSWIDGET_H
