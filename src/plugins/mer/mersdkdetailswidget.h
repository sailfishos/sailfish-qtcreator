/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
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

#ifndef MERSDKDETAILSWIDGET_H
#define MERSDKDETAILSWIDGET_H

#include "mersdk.h"

#include <QIcon>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

namespace Ui {
class MerSdkDetailsWidget;
}

class MerSdkDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    enum Roles {
        ValidRole = Qt::UserRole + 1,
        InstallRole
    };

    explicit MerSdkDetailsWidget(QWidget *parent = 0);
    ~MerSdkDetailsWidget() override;

    QString searchKeyWordMatchString() const;
    void setSdk(const MerSdk *sdk);
    void setStatus(const QString &status);
    void setVmOffStatus(bool vmOff);
    void setPrivateKeyFile(const QString &path);
    void setTestButtonEnabled(bool enabled);
    void setSshTimeout(int timeout);
    void setSshPort(quint16 port);
    void setHeadless(bool enabled);
    void setSrcFolderChooserPath(const QString &path);
    void setWwwPort(quint16 port);
    void setWwwProxy(const QString &type, const QString &servers, const QString &excludes);
    void setMemorySizeMb(int sizeMb);
    void setCpuCount(int count);
    void setVdiCapacityMb(int capacityMb);

signals:
    void generateSshKey(const QString &key);
    void sshKeyChanged(const QString &key);
    void authorizeSshKey(const QString &key);
    void testConnectionButtonClicked();
    void sshPortChanged(quint16 port);
    void sshTimeoutChanged(int timeout);
    void headlessCheckBoxToggled(bool checked);
    void srcFolderApplyButtonClicked(const QString &path);
    void wwwPortChanged(quint16 port);
    void wwwProxyChanged(const QString &type, const QString &servers, const QString &excludes);
    void memorySizeMbChanged(int sizeMb);
    void cpuCountChanged(int count);
    void vdiCapacityMbChnaged(int value);

private slots:
    void onAuthorizeSshKeyButtonClicked();
    void onGenerateSshKeyButtonClicked();
    void onPathChooserEditingFinished();
    void onSrcFolderApplyButtonClicked();
    void onWwwProxyDisabledToggled(bool checked);
    void onWwwProxyAutomaticToggled(bool checked);
    void onWwwProxyManualToggled(bool checked);
    void onWwwProxyServersEdited(const QString &servers);
    void onWwwProxyExcludesEdited(const QString &excludes);

private:
    Ui::MerSdkDetailsWidget *m_ui;
    bool m_updateConnection;
    QString m_wwwProxy;
    QString m_wwwProxyServerUrl;
    QString m_wwwProxyServerList;
    QString m_wwwProxyExcludes;
};

} // Internal
} // Mer

#endif // MERSDKDETAILSWIDGET_H
