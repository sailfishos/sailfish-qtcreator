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

#include <QApplication>
#include <QTimer>
#include <QWebEngineSettings>

using namespace ProjectExplorer;
using namespace Utils;

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL_BASE[] = "http://127.0.0.1/";
const char START_VM_FRAGMENT[] = "startVM";

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
 * \class MerManagementWebPage
 */

// Workaround: The QWebEngineView::urlChanged signal cannot be used to handle
// command links because with ErrorPageEnabled disabled urlChanged is never
// emitted with the actual failed-to-load URL; "about:blank" is reported
// instead.  (And "about:blank#command" URL cannot be used for command links
// because fragment is always stripped from "about:blank" URL by QtWebEngine.)
class MerManagementWebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    using QWebEnginePage::QWebEnginePage;

signals:
    void navigationRequested(const QUrl &url);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        emit navigationRequested(url);
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }
};

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

    auto page = new MerManagementWebPage(ui->webView);
    ui->webView->setPage(page);

    connect(page, &QWebEnginePage::linkHovered,
            ui->statusBarLabel, &QLabel::setText);
    connect(page, &QWebEnginePage::loadFinished,
            this, &MerManagementWebView::handleLoadFinished);
    connect(page, &MerManagementWebPage::navigationRequested,
            this, &MerManagementWebView::onNavigationRequested);

    // Workaround: the sdk-webapp does not like receiving another request while processing one
    connect(page, &QWebEnginePage::loadStarted,
            this, [this]() { setEnabled(false); });
    connect(page, &QWebEnginePage::loadFinished,
            this, [this]() { setEnabled(true); },
            Qt::QueuedConnection);

    QWebEngineSettings *webSettings = ui->webView->settings();
    webSettings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
    webSettings->setFontFamily(QWebEngineSettings::StandardFont, QStringLiteral("Sans Serif"));
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
                ui->webView, &QWebEngineView::reload);
    }

    m_selectedSdk = m_sdksModel->sdkAt(ui->sdksComboBox->currentIndex());

    if (m_selectedSdk) {
        url = QUrl(QLatin1String(CONTROLCENTER_URL_BASE));
        url.setPort(m_selectedSdk->wwwPort());
        connect(m_selectedSdk->connection(), &MerConnection::virtualMachineOffChanged,
                ui->webView, &QWebEngineView::reload);
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
        QString vmStatus = QLatin1String("<h2>The SDK VM is not ready.</h2>");
        if (m_selectedSdk) { // one cannot be sure here
            QString vmName = m_selectedSdk->virtualMachineName();
            if (m_selectedSdk->connection()->isVirtualMachineOff()) {
                QUrl startVmUrl = QUrl(QLatin1String(CONTROLCENTER_URL_BASE));
                startVmUrl.setPort(m_selectedSdk->wwwPort());
                startVmUrl.setFragment(QLatin1String(START_VM_FRAGMENT));
                vmStatus = QString::fromLatin1(
                        "<h2>The \"%1\" VM is not running.</h2>"
                        "<p><a href=\"%2\">Start the virtual machine!</a></p>"
                        )
                    .arg(vmName)
                    .arg(startVmUrl.toString());
            } else {
                autoReloadHint = true;
                QString detail;
                switch (m_selectedSdk->connection()->state()) {
                case MerConnection::Disconnected:
                case MerConnection::StartingVm:
                case MerConnection::Connecting:
                case MerConnection::Connected:
                    detail = QLatin1String("The virtual machine is starting&hellip;");
                    break;
                case MerConnection::Error:
                    detail = QLatin1String("Error connecting to the virtual machine.");
                    break;
                case MerConnection::Disconnecting:
                case MerConnection::ClosingVm:
                    detail = QLatin1String("The virtual machine is closing&hellip;");
                }
                vmStatus = QString::fromLatin1(
                        "<h2>The \"%1\" VM is not ready.</h2>"
                        "<p>%2</p>"
                        )
                    .arg(vmName)
                    .arg(detail);
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
                    "    <p>An SDK virtual machine can be controlled with the "
                    "    <img src='qrc:/mer/images/sdk-run.png' style='vertical-align: middle;'/>"
                    "    button &ndash; available on the lower left side <em>when "
                    "    a&nbsp;Sailfish&nbsp;OS project is open</em>.</p>"
                    "  </div>"
                    "</body>"
                    "</html>")
                .arg(qApp->font().family())
                .arg(qApp->font().lastResortFamily())
                .arg(vmStatus)
                );
        m_loaded = false;
        if (m_autoFailReload && autoReloadHint)
            QTimer::singleShot(5000, this, &MerManagementWebView::reloadPage);
    } else if (ui->webView->url().toString() != QLatin1String("about:blank")) {
        m_loaded = true;
    }
}

void MerManagementWebView::reloadPage()
{
    if (ui->webView->url().isEmpty() ||
            ui->webView->url().matches(QUrl(QLatin1String("about:blank")), QUrl::None)) {
        resetWebView();
    }
}

void MerManagementWebView::setAutoFailReload(bool enabled)
{
    m_autoFailReload = enabled;
    if(enabled && !m_loaded)
        reloadPage();
}

void MerManagementWebView::onNavigationRequested(const QUrl &url)
{
    if (url.fragment() == QLatin1String(START_VM_FRAGMENT)) {
        if (m_selectedSdk) {
            m_selectedSdk->connection()->connectTo();
        }
        QTimer::singleShot(5000, this, &MerManagementWebView::reloadPage);
    }
}

} // namespace Internal
} // namespace Mer

#include "mermanagementwebview.moc"
