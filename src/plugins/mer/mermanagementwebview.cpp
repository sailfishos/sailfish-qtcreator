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

#include "mermanagementwebview.h"
#include "ui_mermanagementwebview.h"

#include "merconnection.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL_BASE[] = "http://127.0.0.1/";
const char START_VM_URL[] = "about:blank#startVM";

class MerManagementWebViewSdksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    MerManagementWebViewSdksModel(QObject *parent);

    MerSdk *sdkAt(int row) const;
    int activeSdkIndex() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

signals:
    void activeSdkIndexChanged(int index);

private slots:
    void onStartupProjectChanged(Project *project);
    void onActiveTargetChanged(Target *target);
    void onSdksUpdated();
    void beginReset();
    void endReset();

private:
    QList<MerSdk *> m_sdks;
    QTimer *const m_endResetTimer;
    Project *m_startupProject;
    MerSdk *m_activeSdk;
};

MerManagementWebViewSdksModel::MerManagementWebViewSdksModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_endResetTimer(new QTimer(this))
    , m_startupProject(0)
    , m_activeSdk(0)
{
    m_endResetTimer->setSingleShot(true);
    connect(m_endResetTimer, &QTimer::timeout,
            this, &MerManagementWebViewSdksModel::endReset);

    connect(MerSdkManager::instance(), &MerSdkManager::initialized,
            this, &MerManagementWebViewSdksModel::onSdksUpdated);
    connect(MerSdkManager::instance(), &MerSdkManager::sdksUpdated,
            this, &MerManagementWebViewSdksModel::onSdksUpdated);

    connect(SessionManager::instance(),
            &SessionManager::startupProjectChanged,
            this,
            &MerManagementWebViewSdksModel::onStartupProjectChanged);
}

MerSdk *MerManagementWebViewSdksModel::sdkAt(int row) const
{
    // generosity for easier use with respect to the "<no SDK>" row
    if (m_sdks.count() == 0 && row == 0)
        return 0;
    if (row == -1)
        return 0;
    QTC_ASSERT(row >= 0 && row < m_sdks.count(), return 0);

    return static_cast<MerSdk *>(m_sdks.at(row));
}

int MerManagementWebViewSdksModel::activeSdkIndex() const
{
    return m_sdks.indexOf(m_activeSdk);
}

int MerManagementWebViewSdksModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    else if (m_sdks.isEmpty())
        return 1;
    else
        return m_sdks.count();
}

QVariant MerManagementWebViewSdksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (m_sdks.isEmpty()) {
        switch (role) {
        case Qt::DisplayRole:
            return tr("<no SDK>");
        case Qt::ToolTipRole:
            return tr("Configure a Mer SDK in Options");
        default:
            return QVariant();
        }
    } else {
        MerSdk *sdk = m_sdks.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return sdk->virtualMachineName();
        default:
            return QVariant();
        }
    }
}

void MerManagementWebViewSdksModel::onStartupProjectChanged(Project *project)
{
    if (m_startupProject) {
        disconnect(m_startupProject, &Project::activeTargetChanged,
                this, &MerManagementWebViewSdksModel::onActiveTargetChanged);
    }

    m_startupProject = project;

    if (m_startupProject) {
        connect(m_startupProject, &Project::activeTargetChanged,
                this, &MerManagementWebViewSdksModel::onActiveTargetChanged);
        onActiveTargetChanged(m_startupProject->activeTarget());
    } else {
        onActiveTargetChanged(0);
    }
}

void MerManagementWebViewSdksModel::onActiveTargetChanged(Target *target)
{
    int oldActiveSdkIndex = activeSdkIndex();

    if (target && MerSdkManager::isMerKit(target->kit())) {
        m_activeSdk = MerSdkKitInformation::sdk(target->kit());
        QTC_CHECK(m_activeSdk);
    } else {
        m_activeSdk = 0;
    }

    if (activeSdkIndex() != oldActiveSdkIndex) {
        emit activeSdkIndexChanged(activeSdkIndex());
    }
}

void MerManagementWebViewSdksModel::onSdksUpdated()
{
    QList<MerSdk *> oldSdks = m_sdks;

    m_sdks = MerSdkManager::sdks();

    if (m_sdks != oldSdks) {
        beginReset();
    }
}

void MerManagementWebViewSdksModel::beginReset()
{
    if (!m_endResetTimer->isActive()) {
        beginResetModel();
        m_endResetTimer->start();
    }
}

void MerManagementWebViewSdksModel::endReset()
{
    endResetModel();
}

/*
 * \class MerManagementWebView
 */

MerManagementWebView::MerManagementWebView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MerManagementWebView)
    , m_sdksModel(new MerManagementWebViewSdksModel(this))
    , m_loaded(false)
    , m_autoFailReload(false)
{
    ui->setupUi(this);
    ui->sdksComboBox->setModel(m_sdksModel);
    connect(m_sdksModel, &MerManagementWebViewSdksModel::modelReset,
            this, &MerManagementWebView::selectActiveSdkVm);
    connect(m_sdksModel, &MerManagementWebViewSdksModel::activeSdkIndexChanged,
            this, &MerManagementWebView::selectActiveSdkVm);
    selectActiveSdkVm();
    connect(ui->webView->page(), &QWebPage::linkHovered,
            ui->statusBarLabel, &QLabel::setText);
    connect(ui->webView->page(), &QWebPage::loadFinished,
            this, &MerManagementWebView::handleLoadFinished);
}

MerManagementWebView::~MerManagementWebView()
{
    delete ui;
}

void MerManagementWebView::resetWebView()
{
    QUrl url;

    if (m_selectedSdk) {
        disconnect(m_selectedSdk->connection(), &MerConnection::virtualMachineOffChanged,
                ui->webView, &QWebView::reload);
    }

    m_selectedSdk = m_sdksModel->sdkAt(ui->sdksComboBox->currentIndex());

    if (m_selectedSdk) {
        url = QUrl(QLatin1String(CONTROLCENTER_URL_BASE));
        url.setPort(m_selectedSdk->wwwPort());
        connect(m_selectedSdk->connection(), &MerConnection::virtualMachineOffChanged,
                ui->webView, &QWebView::reload);
    } else {
        url = QLatin1String("about:blank");
    }

    ui->webView->setUrl(url);
}

void MerManagementWebView::on_webView_urlChanged(const QUrl &url)
{
    Q_UNUSED(url);
    ui->statusBarLabel->clear();
}

void MerManagementWebView::on_sdksComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    resetWebView();
}

void MerManagementWebView::on_homeButton_clicked()
{
    resetWebView();
}

void MerManagementWebView::selectActiveSdkVm()
{
    ui->sdksComboBox->setCurrentIndex(qMax(0, m_sdksModel->activeSdkIndex()));
}

void MerManagementWebView::handleLoadFinished(bool success)
{
    if (!success) {
        bool autoReloadHint = false;
        QString vmStatus = QLatin1String("<h1>The SDK VM is not ready.</h1>");
        if (m_selectedSdk) { // one cannot be sure here
            QString vmName = m_selectedSdk->virtualMachineName();
            if (m_selectedSdk->connection()->isVirtualMachineOff()) {
                vmStatus = QString::fromLatin1(
                        "<h1>The \"%1\" VM is not running.</h1>"
                        "<p><a href=\"%2\">Start the virtual machine!</a></p>"
                        )
                    .arg(vmName)
                    .arg(QLatin1String(START_VM_URL));
            } else {
                autoReloadHint = true;
                vmStatus = QString::fromLatin1(
                        "<h1>The \"%1\" VM is not ready.</h1>"
                        "<p>The virtual machine is running but not responding.</p>"
                        )
                    .arg(vmName);
            }
        }

        ui->webView->setHtml(
                QString::fromLatin1(
                    "<html>"
                    "<head></head>"
                    "<body style='margin: 0px; text-align: center;'>"
                    "  <div style='background-color: #EBEBEB; border-bottom: 1px solid #737373; "
                    "              padding-top: 1px; padding-left: 6px; padding-right: 6px;'>"
                    "  %1"
                    "  </div>"
                    "  <div style='padding-left: 24px; padding-right: 24px;'>"
                    "    <p>An SDK virtual machine can be controlled with the "
                    "    <img src=\"qrc:/mer/images/sdk-run.png\"/> button &ndash; available on "
                    "    the lower left side <em>when a SailfishOS project is open</em>.</p>"
                    "  </div>"
                    "</body>"
                    "</html>")
                .arg(vmStatus)
                );
        m_loaded = false;
        if (m_autoFailReload && autoReloadHint)
            QTimer::singleShot(5000, this, &MerManagementWebView::reloadPage);
    } else if (ui->webView->url() == QUrl(QLatin1String(START_VM_URL))) {
        if (m_selectedSdk) {
            m_selectedSdk->connection()->connectTo();
        }
        QTimer::singleShot(5000, this, &MerManagementWebView::reloadPage);
    } else if (ui->webView->url().toString() != QLatin1String("about:blank")) {
        m_loaded = true;
    }
}

void MerManagementWebView::reloadPage()
{
    if (ui->webView->url().isEmpty() ||
            ui->webView->url().matches(QUrl(QLatin1String("about:blank")), QUrl::RemoveFragment)) {
        resetWebView();
    }
}

void MerManagementWebView::setAutoFailReload(bool enabled)
{
    m_autoFailReload = enabled;
    if(enabled && !m_loaded)
        reloadPage();
}

} // namespace Internal
} // namespace Mer

#include "mermanagementwebview.moc"
