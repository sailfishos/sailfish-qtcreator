/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#include "merrunconfigurationaspect.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QUrl>

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>

#include "merconstants.h"
#include "mersettings.h"
#include "ui_merrunconfigurationaspectqmllivedetailswidget.h"

using namespace Core;
using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

namespace {
const char QML_LIVE_ENABLED[] = "MerRunConfiguration.QmlLiveEnabled";
const char QML_LIVE_IPC_PORT_KEY[] = "MerRunConfiguration.QmlLiveIpcPort";
const char QML_LIVE_WORKSPACE_KEY[] = "MerRunConfiguration.QmlLiveWorkspace";
const char QML_LIVE_OPTIONS_KEY[] = "MerRunConfiguration.QmlLiveOptions";
const char QML_LIVE_HELP_URL[] =
    "qthelp://org.qt-project.qtcreator/doc/creator-qtquick-qmllive-sailfish.html";

const MerRunConfigurationAspect::QmlLiveOptions DEFAULT_QML_LIVE_OPTIONS =
    MerRunConfigurationAspect::UpdatesAsOverlay;
} // namespace anonymous

class MerRunConfigWidget : public ProjectExplorer::RunConfigWidget
{
    Q_OBJECT

public:
    MerRunConfigWidget(MerRunConfigurationAspect *aspect)
        : m_aspect(aspect)
    {
        setLayout(new QHBoxLayout);
        layout()->setContentsMargins(0, 0, 0, 0);

        auto qmlLiveWidget = new Utils::DetailsWidget;
        qmlLiveWidget->setUseCheckBox(true);
        qmlLiveWidget->setSummaryText(tr("Enable receiving updates with Qt QmlLive"));

        qmlLiveWidget->setChecked(m_aspect->isQmlLiveEnabled());
        connect(qmlLiveWidget, &Utils::DetailsWidget::checked,
                m_aspect, &MerRunConfigurationAspect::setQmlLiveEnabled);

        auto toolWidget = new Utils::FadingWidget;
        toolWidget->setLayout(new QHBoxLayout);
        toolWidget->layout()->setContentsMargins(4, 4, 4, 4);

        auto helpButton = new QToolButton;
        helpButton->setAutoRaise(true);
        helpButton->setToolTip(tr("Help"));
        // Idea from ProjectExplorer::ToolWidget::ToolWidget()
        helpButton->setFixedSize(QSize(20, Utils::HostOsInfo::isMacHost() ? 20 : 26));
        // TODO Use better icon
        helpButton->setIcon(QIcon(QLatin1String(":/core/images/help.png")));
        connect(helpButton, &QAbstractButton::clicked, []() {
            QDesktopServices::openUrl(QUrl(QLatin1String(QML_LIVE_HELP_URL)));
        });
        toolWidget->layout()->addWidget(helpButton);

        qmlLiveWidget->setToolWidget(toolWidget);

        auto qmlLiveDetailsWidget = new QWidget;
        m_qmlLiveDetailsUi = new Ui::MerRunConfigurationAspectQmlLiveDetailsWidget;
        m_qmlLiveDetailsUi->setupUi(qmlLiveDetailsWidget);

        m_qmlLiveDetailsUi->warningIconLabel->setPixmap(Core::Icons::WARNING.pixmap());
        m_qmlLiveDetailsUi->configureButton->setText(ICore::msgShowOptionsDialog());
        connect(m_qmlLiveDetailsUi->configureButton, &QAbstractButton::clicked,
                [] { ICore::showOptionsDialog(Constants::MER_GENERAL_OPTIONS_ID); });
        m_qmlLiveDetailsUi->warningWidget->setVisible(!MerSettings::hasValidQmlLiveBenchLocation());
        connect(MerSettings::instance(), &MerSettings::qmlLiveBenchLocationChanged, this, [this]() {
            m_qmlLiveDetailsUi->warningWidget->setVisible(!MerSettings::hasValidQmlLiveBenchLocation());
        });

        connect(m_qmlLiveDetailsUi->restoreDefaults, &QAbstractButton::clicked,
                m_aspect, &MerRunConfigurationAspect::restoreQmlLiveDefaults);

        m_qmlLiveDetailsUi->port->setValue(m_aspect->qmlLiveIpcPort());
        void (QSpinBox::*QSpinBox_valueChanged)(int) = &QSpinBox::valueChanged;
        connect(m_qmlLiveDetailsUi->port, QSpinBox_valueChanged,
                m_aspect, &MerRunConfigurationAspect::setQmlLiveIpcPort);
        connect(m_aspect, &MerRunConfigurationAspect::qmlLiveIpcPortChanged,
                m_qmlLiveDetailsUi->port, &QSpinBox::setValue);

        m_qmlLiveDetailsUi->workspace->setText(m_aspect->qmlLiveWorkspace());
        connect(m_qmlLiveDetailsUi->workspace, &QLineEdit::textChanged,
                m_aspect, &MerRunConfigurationAspect::setQmlLiveWorkspace);
        connect(m_aspect, &MerRunConfigurationAspect::qmlLiveWorkspaceChanged,
                m_qmlLiveDetailsUi->workspace, &QLineEdit::setText);

        auto setupQmlLiveOption = [this](QCheckBox *checkBox, MerRunConfigurationAspect::QmlLiveOption option) {
            checkBox->setChecked(m_aspect->qmlLiveOptions() & option);
            connect(checkBox, &QCheckBox::stateChanged, [this, option](int state) {
                if (state == Qt::Checked)
                    m_aspect->setQmlLiveOptions(m_aspect->qmlLiveOptions() | option);
                else
                    m_aspect->setQmlLiveOptions(m_aspect->qmlLiveOptions() & ~option);
            });
            connect(m_aspect, &MerRunConfigurationAspect::qmlLiveOptionsChanged, [this, checkBox, option]() {
                checkBox->setChecked(m_aspect->qmlLiveOptions() & option);
            });
        };
        setupQmlLiveOption(m_qmlLiveDetailsUi->updateOnConnect, MerRunConfigurationAspect::UpdateOnConnect);
        setupQmlLiveOption(m_qmlLiveDetailsUi->updatesAsOverlay, MerRunConfigurationAspect::UpdatesAsOverlay);
        setupQmlLiveOption(m_qmlLiveDetailsUi->loadDummyData, MerRunConfigurationAspect::LoadDummyData);

        qmlLiveWidget->setWidget(qmlLiveDetailsWidget);
        qmlLiveDetailsWidget->setEnabled(qmlLiveWidget->isChecked());
        connect(qmlLiveWidget, &Utils::DetailsWidget::checked,
                qmlLiveDetailsWidget, &QWidget::setEnabled);

        layout()->addWidget(qmlLiveWidget);

        // TODO add 'what are the prerequisites'
    }

    ~MerRunConfigWidget()
    {
        delete m_qmlLiveDetailsUi, m_qmlLiveDetailsUi = 0;
    }

    QString displayName() const override { return m_aspect->displayName(); }

private:
    MerRunConfigurationAspect *m_aspect;
    Ui::MerRunConfigurationAspectQmlLiveDetailsWidget *m_qmlLiveDetailsUi;
};

MerRunConfigurationAspect::MerRunConfigurationAspect(ProjectExplorer::RunConfiguration *rc)
    : ProjectExplorer::IRunConfigurationAspect(rc)
    , m_qmlLiveEnabled(false)
    , m_qmlLiveIpcPort(Constants::DEFAULT_QML_LIVE_PORT)
    , m_qmlLiveOptions(DEFAULT_QML_LIVE_OPTIONS)
{
    setId(Constants::MER_RUN_CONFIGURATION_ASPECT);
    setDisplayName(tr("Sailfish OS Application Settings"));
}

ProjectExplorer::IRunConfigurationAspect *MerRunConfigurationAspect::create(
        ProjectExplorer::RunConfiguration *runConfig) const
{
    return new MerRunConfigurationAspect(runConfig);
}

ProjectExplorer::RunConfigWidget *MerRunConfigurationAspect::createConfigurationWidget()
{
    return new MerRunConfigWidget(this);
}

void MerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_qmlLiveEnabled = map.value(QLatin1String(QML_LIVE_ENABLED), false).toBool();
    m_qmlLiveIpcPort = map.value(QLatin1String(QML_LIVE_IPC_PORT_KEY), Constants::DEFAULT_QML_LIVE_PORT).toInt();
    m_qmlLiveWorkspace = map.value(QLatin1String(QML_LIVE_WORKSPACE_KEY), QString()).toString();
    m_qmlLiveOptions = static_cast<QmlLiveOption>(map.value(QLatin1String(QML_LIVE_OPTIONS_KEY),
                                                            static_cast<int>(DEFAULT_QML_LIVE_OPTIONS)).toInt());
}

void MerRunConfigurationAspect::toMap(QVariantMap &map) const
{
    map.insert(QLatin1String(QML_LIVE_ENABLED), m_qmlLiveEnabled);
    map.insert(QLatin1String(QML_LIVE_IPC_PORT_KEY), m_qmlLiveIpcPort);
    map.insert(QLatin1String(QML_LIVE_WORKSPACE_KEY), m_qmlLiveWorkspace);
    map.insert(QLatin1String(QML_LIVE_OPTIONS_KEY), static_cast<int>(m_qmlLiveOptions));
}

void MerRunConfigurationAspect::restoreQmlLiveDefaults()
{
    setQmlLiveIpcPort(Constants::DEFAULT_QML_LIVE_PORT);
    setQmlLiveWorkspace(QString());
    setQmlLiveOptions(DEFAULT_QML_LIVE_OPTIONS);
}

void MerRunConfigurationAspect::setQmlLiveEnabled(bool qmlLiveEnabled)
{
    if (m_qmlLiveEnabled == qmlLiveEnabled)
        return;

    m_qmlLiveEnabled = qmlLiveEnabled;

    emit qmlLiveEnabledChanged(m_qmlLiveEnabled);
}

void MerRunConfigurationAspect::setQmlLiveIpcPort(int port)
{
    if (m_qmlLiveIpcPort == port)
        return;

    m_qmlLiveIpcPort = port;

    emit qmlLiveIpcPortChanged(m_qmlLiveIpcPort);
}

void MerRunConfigurationAspect::setQmlLiveWorkspace(const QString &workspace)
{
    if (m_qmlLiveWorkspace == workspace)
        return;

    m_qmlLiveWorkspace = workspace;

    emit qmlLiveWorkspaceChanged(m_qmlLiveWorkspace);
}

void MerRunConfigurationAspect::setQmlLiveOptions(QmlLiveOptions options)
{
    if (m_qmlLiveOptions == options)
        return;

    m_qmlLiveOptions = options;

    emit qmlLiveOptionsChanged();
}

} // Internal
} // Mer

#include "merrunconfigurationaspect.moc"
