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

#ifndef MERMANAGEMENTWEBVIEW_H
#define MERMANAGEMENTWEBVIEW_H

#include <QUrl>
#include <QWidget>

namespace Mer {
namespace Internal {

namespace Ui {
class MerManagementWebView;
}

class MerManagementWebView : public QWidget
{
    Q_OBJECT
    
public:
    explicit MerManagementWebView(QWidget *parent = 0);
    ~MerManagementWebView();

public slots:
    void setUrl(const QUrl &url);
    void setAutoFailReload(bool enabled);

private slots:
    void on_webView_urlChanged(const QUrl &url);
    void on_urlLineEdit_returnPressed();
    void on_homeButton_clicked();
    void handleLoadFinished(bool success);
    void reloadPage();

private:
    Ui::MerManagementWebView *ui;
    bool m_loaded;
    bool m_autoFailReload;
};

} // namespace Internal
} // namespace Mer

#endif // MERMANAGEMENTWEBVIEW_H
