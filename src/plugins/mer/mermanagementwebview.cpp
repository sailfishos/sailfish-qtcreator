/****************************************************************************
**
** Copyright (C) 2012-2018 Jolla Ltd.
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

#include "mermanagementwebview.h"
#include "ui_mermanagementwebview.h"

#include "mericons.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL_BASE[] = "http://127.0.0.1/";
const char START_VM_URL[] = "about:blank#startVM";
const int AUTO_RELOAD_TIMEOUT_MS = 2000;

class MerManagementWebViewSdksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    MerManagementWebViewSdksModel(QObject *parent);

    BuildEngine *sdkAt(int row) const;
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
    QList<BuildEngine *> m_sdks;
    QTimer *const m_endResetTimer;
    Project *m_startupProject;
    BuildEngine *m_activeSdk;
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
    connect(Sdk::instance(), &Sdk::buildEngineAdded,
            this, &MerManagementWebViewSdksModel::onSdksUpdated);
    connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
            this, &MerManagementWebViewSdksModel::onSdksUpdated, Qt::QueuedConnection);

    connect(SessionManager::instance(),
            &SessionManager::startupProjectChanged,
            this,
            &MerManagementWebViewSdksModel::onStartupProjectChanged);
}

BuildEngine *MerManagementWebViewSdksModel::sdkAt(int row) const
{
    // generosity for easier use with respect to the "<no build engine>" row
    if (m_sdks.count() == 0 && row == 0)
        return 0;
    if (row == -1)
        return 0;
    QTC_ASSERT(row >= 0 && row < m_sdks.count(), return 0);

    return m_sdks.at(row);
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
            return tr("<no build engine>");
        case Qt::ToolTipRole:
            return tr("Configure a Sailfish OS build engine in Options");
        default:
            return QVariant();
        }
    } else {
        BuildEngine *const sdk = m_sdks.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return sdk->name();
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
        m_activeSdk = MerSdkKitInformation::buildEngine(target->kit());
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
    QList<BuildEngine *> oldSdks = m_sdks;

    m_sdks = Sdk::buildEngines();

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

    // Workaround: the sdk-webapp does not like receiving another request while processing one
    connect(ui->webView, &QWebView::loadStarted,
            this, [this]() { setEnabled(false); });
    connect(ui->webView, &QWebView::loadFinished,
            this, [this]() {
                setEnabled(true);
                // Without this it does not repaint (Linux/X11) and leaves blank area
                auto QWidget_update = static_cast<void (QWidget::*)()>(&QWidget::update);
                QTimer::singleShot(200, ui->webView, QWidget_update);
                QTimer::singleShot(200, this, QWidget_update);
            },
            Qt::QueuedConnection);

    connect(ui->webView, &QWebView::loadProgress,
            this, [this](int prog) { ui->progressBar->setValue(prog < 100 ? prog : 0); });
    connect(ui->webView, &QWebView::loadFinished,
            this, [this]() { ui->progressBar->setValue(0); });
}

MerManagementWebView::~MerManagementWebView()
{
    // Prevent crash on exit
    ui->webView->page()->disconnect(this);

    delete ui;
}

void MerManagementWebView::resetWebView()
{
    QUrl url;

    if (m_selectedSdk) {
        disconnect(m_selectedSdk, &BuildEngine::wwwPortChanged,
                this, &MerManagementWebView::resetWebView);
        disconnect(m_selectedSdk->virtualMachine(), &VirtualMachine::virtualMachineOffChanged,
                this, &MerManagementWebView::resetWebView);
    }

    m_selectedSdk = m_sdksModel->sdkAt(ui->sdksComboBox->currentIndex());

    if (m_selectedSdk) {
        url = QUrl(QLatin1String(CONTROLCENTER_URL_BASE));
        url.setPort(m_selectedSdk->wwwPort());
        connect(m_selectedSdk, &BuildEngine::wwwPortChanged,
                this, &MerManagementWebView::resetWebView);
        connect(m_selectedSdk->virtualMachine(), &VirtualMachine::virtualMachineOffChanged,
                this, &MerManagementWebView::resetWebView);
    } else {
        url = QUrl("about:blank");
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
        QString vmStatus = QLatin1String("<h2>The Build Engine is not Ready</h2>");
        if (m_selectedSdk) { // one cannot be sure here
            if (m_selectedSdk->virtualMachine()->isOff()) {
                vmStatus = QString::fromLatin1(
                        "<h2>The Build Engine is not Running</h2>"
                        "<p><a href=\"%2\">Start the build engine!</a></p>"
                        )
                    .arg(QLatin1String(START_VM_URL));
            } else {
                autoReloadHint = true;
                vmStatus = QString::fromLatin1(
                        "<h2>The Build Engine is not Ready</h2>"
                        "<p>The build engine is running but not responding.</p>"
                        );
            }
        }

        ui->webView->setHtml(
                QString::fromLatin1(
                    "<html>"
                    "<head></head>"
                    "<body style='margin: 0px; text-align: center; "
                    "             font-family: \"%1\", \"%2\", sans-serif;'>"
                    "  <div style='background-color: #EBEBEB; border-bottom: 1px solid #737373; "
                    "              padding-top: 1px; padding-left: 6px; padding-right: 6px;'>"
                    "  %3"
                    "  </div>"
                    "  <div style='padding-left: 24px; padding-right: 24px; font-size: small;'>"
                    "    <p>A Sailfish OS build engine can be controlled with the "
                    "    <img src='qrc%4' style='vertical-align: middle;'/>"
                    "    button &ndash; available on the lower left side <em>when "
                    "    a&nbsp;Sailfish&nbsp;OS project is open</em>.</p>"
                    "  </div>"
                    "</body>"
                    "</html>")
                .arg(qApp->font().family())
                .arg(qApp->font().lastResortFamily())
                .arg(vmStatus)
                .arg(Icons::MER_SDK_RUN.imageFileName())
                );
        m_loaded = false;
        if (m_autoFailReload && autoReloadHint)
            QTimer::singleShot(AUTO_RELOAD_TIMEOUT_MS, this, &MerManagementWebView::reloadPage);
    } else if (ui->webView->url() == QUrl(QLatin1String(START_VM_URL))) {
        if (m_selectedSdk) {
            m_selectedSdk->virtualMachine()->connectTo();
        }
        QTimer::singleShot(AUTO_RELOAD_TIMEOUT_MS, this, &MerManagementWebView::reloadPage);
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
