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
#include <QTimer>

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL[] = "http://127.0.0.1:8080/";

MerManagementWebView::MerManagementWebView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MerManagementWebView)
    , m_loaded(false)
    , m_autoFailReload(false)
{
    ui->setupUi(this);
    connect(ui->webView->page(), SIGNAL(linkHovered(QString,QString,QString)), ui->statusBarLabel, SLOT(setText(QString)));
    connect(ui->webView->page(), SIGNAL(loadFinished(bool)), SLOT(handleLoadFinished(bool)));
    on_homeButton_clicked();

    ui->webView->setStyleSheet(QLatin1String("background-color: #3f768b;"));
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
                                           "<body style='background-color: #3f768b;'>"
                                           "<div style='text-align: center; vertical-align: middle;'>"
                                           "<h1>The SDK VM is not responding.</h1>"
                                           "<p><h1>Create a new SailfishOS project (or open an existing one) and press the <em>Start SDK</em> button on the lower left.</h1></p>"
                                           "</div>"
                                           "</body>"
                                           "</html>")
                             );
        m_loaded = false;
        if (m_autoFailReload)
            QTimer::singleShot(5000,this,SLOT(reloadPage()));
    } else if (ui->webView->url().toString() != QLatin1String("about:blank")) {
        m_loaded = true;
    }
}

void MerManagementWebView::reloadPage()
{
    if (ui->webView->url().isEmpty() || ui->webView->url().toString() == QLatin1String("about:blank"))
        setUrl(QUrl(QLatin1String(CONTROLCENTER_URL)));
}

void MerManagementWebView::setAutoFailReload(bool enabled)
{
    m_autoFailReload = enabled;
    if(enabled && !m_loaded)
        reloadPage();
}

} // namespace Internal
} // namespace Mer
