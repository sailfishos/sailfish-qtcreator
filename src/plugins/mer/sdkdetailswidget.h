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

#ifndef MER_SDKDETAILSWIDGET_H
#define MER_SDKDETAILSWIDGET_H

#include "mersdk.h"

#include <QWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

namespace Ui {
class SdkDetailsWidget;
}

class SdkDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    enum Roles {
        ValidRole = Qt::UserRole + 1,
        InstallRole
    };

    explicit SdkDetailsWidget(QWidget *parent = 0);
    ~SdkDetailsWidget();

    QString searchKeyWordMatchString() const;

    bool isSdkValid() const;

    void setSdk(const MerSdk &sdk, bool validSdk = true);
    MerSdk sdk() const;

    void setValidTargets(const QStringList &targets);
    QStringList validTargets() const;

    QStringList installedTargets() const;

signals:
    void testConnection();
    void updateKits(QStringList);
    void deployPublicKey();
    void createSshKey(QString);

private slots:
    void onSshKeyRadioButton();
    void onSshKeySaveButtonClicked();
    void onSshKeyButtonClicked();
    void onAuthenticationInfoChanged();
    void onAuthenticationInfoUpdated();
    void onCurrentItemChanged(QListWidgetItem *item);
    void onUpdateKitsButtonClicked();
    void onCreateKitButtonClicked();
    void onRemoveKitButtonClicked();
    void onTestConnectionButtonClicked();
    void onAuthorizePublicKeyButtonClicked();

private:
    Ui::SdkDetailsWidget *m_ui;
    MerSdk m_sdk;
    QIcon m_invalidIcon;
    QIcon m_warningIcon;
    bool m_updateConnection;
    bool m_validSdk;
};

} // Internal
} // Mer

#endif // MER_SDKDETAILSWIDGET_H
