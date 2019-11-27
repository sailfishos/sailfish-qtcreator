/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
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

#ifndef MEROPTIONSWIDGET_H
#define MEROPTIONSWIDGET_H

#include <QMap>
#include <QTimer>
#include <QUrl>
#include <QWidget>

#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Sfdk {
class BuildEngine;
}

namespace Mer {
namespace Internal {

namespace Ui {
class MerBuildEngineOptionsWidget;
}

// FIXME update to match MerEmulatorOptionsWidget and MerEmulatorDetailsWidget more
class MerBuildEngineOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MerBuildEngineOptionsWidget(QWidget *parent = 0);
    ~MerBuildEngineOptionsWidget() override;

    QString searchKeyWordMatchString() const;
    void setBuildEngine(const QUrl &uri);
    void store();

signals:
    void updateSearchKeys();

private:
    bool lockDownConnectionsOrCancelChangesThatNeedIt(QList<Sfdk::BuildEngine *> *lockedDownBuildEngines);

private slots:
    // Ui
    void onBuildEngineAdded(int index);
    void onAboutToRemoveBuildEngine(int index);
    void onBuildEngineChanged(int index);
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onTestConnectionButtonClicked();
    void onStartVirtualMachineButtonClicked();
    void onStopVirtualMachineButtonClicked();
    //void onLaunchSDKControlCenterClicked();
    void onGenerateSshKey(const QString &path);
    void onAuthorizeSshKey(const QString &file);
    void onSshKeyChanged(const QString &file);
    void onSshTimeoutChanged(int timeout);
    void onSshPortChanged(quint16 port);
    void onHeadlessCheckBoxToggled(bool checked);
    void onSrcFolderApplyButtonClicked(const QString &path);
    void onWwwPortChanged(quint16 port);
    void onWwwProxyChanged(const QString &type, const QString &servers, const QString &excludes);
    void onMemorySizeMbChanged(int sizeMb);
    void onCpuCountChanged(int count);
    void onStorageSizeMbChnaged(int sizeMb);
    void onVmOffChanged(bool vmOff);
    void update();

private:
    Ui::MerBuildEngineOptionsWidget *m_ui;
    QUrl m_virtualMachine;
    QMetaObject::Connection m_vmOffConnection;
    QString m_status;
    QMap<QUrl, Sfdk::BuildEngine *> m_buildEngines;
    std::vector<std::unique_ptr<Sfdk::BuildEngine>> m_newBuildEngines;
    QMap<Sfdk::BuildEngine *, QString> m_sshPrivKeys;
    QMap<Sfdk::BuildEngine *, int> m_sshTimeout;
    QMap<Sfdk::BuildEngine *, quint16> m_sshPort;
    QMap<Sfdk::BuildEngine *, bool> m_headless;
    QMap<Sfdk::BuildEngine *, quint16> m_wwwPort;
    QMap<Sfdk::BuildEngine *, QString> m_wwwProxy;
    QMap<Sfdk::BuildEngine *, QString> m_wwwProxyServers;
    QMap<Sfdk::BuildEngine *, QString> m_wwwProxyExcludes;
    QMap<Sfdk::BuildEngine *, int> m_storageSizeMb;
    QMap<Sfdk::BuildEngine *, int> m_memorySizeMb;
    QMap<Sfdk::BuildEngine *, int> m_cpuCount;
};

} // Internal
} // Mer

#endif // MEROPTIONSWIDGET_H
