/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

namespace Ui {
class MerOptionsWidget;
}

class MerOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MerOptionsWidget(QWidget *parent = 0);
    ~MerOptionsWidget() override;

    QString searchKeyWordMatchString() const;
    void setSdk(const QString &vmName);
    void store();

signals:
    void updateSearchKeys();

private:
    bool lockDownConnectionsOrCancelChangesThatNeedIt(QList<MerSdk *> *lockedDownSdks);

private slots:
    // Ui
    void onSdksUpdated();
    void onSdkChanged(const QString &text);
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
    void onVmOffChanged(bool vmOff);
    void update();

private:
    Ui::MerOptionsWidget *m_ui;
    QString m_virtualMachine;
    QMetaObject::Connection m_vmOffConnection;
    QString m_status;
    QMap<QString, MerSdk*> m_sdks;
    QMap<MerSdk*, QString> m_sshPrivKeys;
    QMap<MerSdk*, int> m_sshTimeout;
    QMap<MerSdk*, quint16> m_sshPort;
    QMap<MerSdk*, bool> m_headless;
    QMap<MerSdk*, quint16> m_wwwPort;
    QMap<MerSdk*, QString> m_wwwProxy;
    QMap<MerSdk*, QString> m_wwwProxyServers;
    QMap<MerSdk*, QString> m_wwwProxyExcludes;
};

} // Internal
} // Mer

#endif // MEROPTIONSWIDGET_H
