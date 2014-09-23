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
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"

#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <QTimer>

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL_BASE[] = "http://127.0.0.1/";

class MerManagementWebViewSdksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    MerManagementWebViewSdksModel(QObject *parent);

    MerSdk *sdkAt(int row) const;
    int activeSdkIndex() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;

signals:
    void activeSdkIndexChanged(int index);

private slots:
    void onStartupProjectChanged(ProjectExplorer::Project *project);
    void onActiveTargetChanged(ProjectExplorer::Target *target);
    void onSdksUpdated();
    void beginReset();
    void endReset();

private:
    QList<MerSdk *> m_sdks;
    QTimer *const m_endResetTimer;
    ProjectExplorer::Project *m_startupProject;
    MerSdk *m_activeSdk;
};

MerManagementWebViewSdksModel::MerManagementWebViewSdksModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_endResetTimer(new QTimer(this))
    , m_startupProject(0)
    , m_activeSdk(0)
{
    m_endResetTimer->setSingleShot(true);
    connect(m_endResetTimer, SIGNAL(timeout()), this, SLOT(endReset()));

    connect(MerSdkManager::instance(), SIGNAL(initialized()), this, SLOT(onSdksUpdated()));
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), this, SLOT(onSdksUpdated()));

    connect(ProjectExplorer::SessionManager::instance(),
            SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this,
            SLOT(onStartupProjectChanged(ProjectExplorer::Project*)));
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

void MerManagementWebViewSdksModel::onStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (m_startupProject) {
        disconnect(m_startupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                this, SLOT(onActiveTargetChanged(ProjectExplorer::Target*)));
    }

    m_startupProject = project;

    if (m_startupProject) {
        connect(m_startupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                this, SLOT(onActiveTargetChanged(ProjectExplorer::Target*)));
        onActiveTargetChanged(m_startupProject->activeTarget());
    } else {
        onActiveTargetChanged(0);
    }
}

void MerManagementWebViewSdksModel::onActiveTargetChanged(ProjectExplorer::Target *target)
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

    m_sdks = MerSdkManager::instance()->sdks();

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
    connect(m_sdksModel, SIGNAL(modelReset()), this, SLOT(selectActiveSdkVm()));
    connect(m_sdksModel, SIGNAL(activeSdkIndexChanged(int)), this, SLOT(selectActiveSdkVm()));
    selectActiveSdkVm();
    connect(ui->webView->page(), SIGNAL(linkHovered(QString,QString,QString)), ui->statusBarLabel, SLOT(setText(QString)));
    connect(ui->webView->page(), SIGNAL(loadFinished(bool)), SLOT(handleLoadFinished(bool)));
}

MerManagementWebView::~MerManagementWebView()
{
    delete ui;
}

void MerManagementWebView::resetWebView()
{
    QUrl url;

    if (m_selectedSdk) {
        disconnect(m_selectedSdk->connection(), SIGNAL(virtualMachineOffChanged(bool)),
                ui->webView, SLOT(reload()));
    }

    m_selectedSdk = m_sdksModel->sdkAt(ui->sdksComboBox->currentIndex());

    if (m_selectedSdk) {
        url = QUrl(QLatin1String(CONTROLCENTER_URL_BASE));
        url.setPort(m_selectedSdk->wwwPort());
        connect(m_selectedSdk->connection(), SIGNAL(virtualMachineOffChanged(bool)),
                ui->webView, SLOT(reload()));
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
        // one cannot be sure here
        QString vmName = m_selectedSdk ? m_selectedSdk->virtualMachineName() : QLatin1String("SDK");

        ui->webView->setHtml(
                             QString::fromLatin1(
                                           "<html>"
                                           "<head></head>"
                                           "<body>"
                                           "<div style='text-align: center; vertical-align: middle;'>"
                                           "<h1>The \"%1\" VM is not responding.</h1>"
                                           "<p><h1>Create a new SailfishOS project (or open an existing one) and press the <em>Start SDK</em> button on the lower left.</h1></p>"
                                           "</div>"
                                           "</body>"
                                           "</html>")
                             .arg(vmName)
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
        resetWebView();
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
