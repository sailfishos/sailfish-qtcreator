/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#ifndef MERMANAGEMENTWEBVIEW_H
#define MERMANAGEMENTWEBVIEW_H

#include <QPointer>
#include <QUrl>
#include <QWidget>

namespace Mer {
namespace Internal {

namespace Ui {
class MerManagementWebView;
}

class MerSdk;
class MerManagementWebViewSdksModel;

class MerManagementWebView : public QWidget
{
    Q_OBJECT
    
public:
    explicit MerManagementWebView(QWidget *parent = 0);
    ~MerManagementWebView() override;

public slots:
    void setAutoFailReload(bool enabled);

private:
    void resetWebView();

private slots:
    void on_webView_urlChanged(const QUrl &url);
    void on_sdksComboBox_currentIndexChanged(int index);
    void on_homeButton_clicked();
    void selectActiveSdkVm();
    void handleLoadFinished(bool success);
    void reloadPage();

private:
    Ui::MerManagementWebView *ui;
    MerManagementWebViewSdksModel *m_sdksModel;
    QPointer<MerSdk> m_selectedSdk;
    bool m_loaded;
    bool m_autoFailReload;
};

} // namespace Internal
} // namespace Mer

#endif // MERMANAGEMENTWEBVIEW_H
