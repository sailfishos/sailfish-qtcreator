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

#include "mermanagementwebview.h"
#include "ui_mermanagementwebview.h"
#include "mersdkmanager.h"

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL[] = "http://127.0.0.1:8080/";

MerManagementWebView::MerManagementWebView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MerManagementWebView)
{
    ui->setupUi(this);
    connect(ui->webView->page(), SIGNAL(linkHovered(QString,QString,QString)), ui->statusBarLabel, SLOT(setText(QString)));
    connect(ui->webView->page(), SIGNAL(loadFinished(bool)), SLOT(handleLoadFinished(bool)));
    on_homeButton_clicked();
    connect(MerSdkManager::instance(), SIGNAL(sdkRunningChanged()),
            SLOT(reloadPage()));
}

MerManagementWebView::~MerManagementWebView()
{
    delete ui;
}

void MerManagementWebView::setUrl(const QUrl &url)
{
    ui->webView->setUrl(url);
}

void MerManagementWebView::on_webView_urlChanged(const QUrl &url)
{
    ui->urlLineEdit->setText(url.toString());
    ui->statusBarLabel->clear();
}

void MerManagementWebView::on_urlLineEdit_returnPressed()
{
    setUrl(QUrl::fromUserInput(ui->urlLineEdit->text()));
}

void MerManagementWebView::on_homeButton_clicked()
{
    setUrl(QUrl(QLatin1String(CONTROLCENTER_URL)));
}

void MerManagementWebView::handleLoadFinished(bool success)
{
    if (!success && ui->webView->url().toString().indexOf(QLatin1String(CONTROLCENTER_URL)) != -1) {
        ui->webView->setHtml(
                    QLatin1String(
                        "<html>"
                        "<head></head>"
                        "<body>"
                        "The SDK VM is not responding. Create a new SailfishOS project (or open an existing one) and press the <em>Start SDK</em> button on the lower left."
                        "</body>"
                        "</html>")
                    );
    }
}

void MerManagementWebView::reloadPage()
{
    if (ui->webView->url().isEmpty()
            || ui->webView->url().toString() == QLatin1String("about:blank"))
        setUrl(QUrl(QLatin1String(CONTROLCENTER_URL)));
    ui->webView->reload();
}

} // namespace Internal
} // namespace Mer
