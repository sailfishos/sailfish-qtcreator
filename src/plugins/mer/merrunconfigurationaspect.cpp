/****************************************************************************
**
** Copyright (C) 2016-2019 Jolla Ltd.
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

#include "merrunconfigurationaspect.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QUrl>

#include <coreplugin/icore.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/utilsicons.h>

#include <sfdk/sfdkconstants.h>

#include "merconstants.h"
#include "merdevice.h"
#include "merqmllivebenchmanager.h"
#include "mersettings.h"
#include "ui_merrunconfigurationaspectqmllivedetailswidget.h"

using namespace Core;
using namespace ProjectExplorer;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const char QML_LIVE_ENABLED[] = "MerRunConfiguration.QmlLiveEnabled";
const char QML_LIVE_IPC_PORT_KEY[] = "MerRunConfiguration.QmlLiveIpcPort";
const char QML_LIVE_BENCH_WORKSPACE_KEY[] = "MerRunConfiguration.QmlLiveBenchWorkspace";
const char QML_LIVE_TARGET_WORKSPACE_KEY[] = "MerRunConfiguration.QmlLiveTargetWorkspace";
const char QML_LIVE_OPTIONS_KEY[] = "MerRunConfiguration.QmlLiveOptions";

const MerRunConfigurationAspect::QmlLiveOptions DEFAULT_QML_LIVE_OPTIONS =
    MerRunConfigurationAspect::UpdateOnConnect | MerRunConfigurationAspect::UpdatesAsOverlay;

const char LIST_SEP[] = ":";
const char QMLLIVE_SAILFISH_PRELOAD[] = "/usr/lib/qmllive-sailfish/libsailfishapp-preload.so";

} // namespace anonymous

class MerRunConfigWidget : public QWidget
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
        helpButton->setIcon(helpButton->style()->standardIcon(QStyle::SP_TitleBarContextHelpButton));
        connect(helpButton, &QAbstractButton::clicked, []() {
            QDesktopServices::openUrl(QUrl(QLatin1String(Constants::QML_LIVE_HELP_URL)));
        });
        toolWidget->layout()->addWidget(helpButton);

        qmlLiveWidget->setToolWidget(toolWidget);

        // Init QmlLive details widget

        auto qmlLiveDetailsWidget = new QWidget;
        m_qmlLiveDetailsUi = new Ui::MerRunConfigurationAspectQmlLiveDetailsWidget;
        m_qmlLiveDetailsUi->setupUi(qmlLiveDetailsWidget);
        initQmlLiveDetailsUi();

        // Finish

        qmlLiveWidget->setWidget(qmlLiveDetailsWidget);
        qmlLiveDetailsWidget->setEnabled(qmlLiveWidget->isChecked());
        connect(qmlLiveWidget, &Utils::DetailsWidget::checked,
                qmlLiveDetailsWidget, &QWidget::setEnabled);
        connect(qmlLiveWidget, &Utils::DetailsWidget::checked, [](bool checked) {
            if (checked)
                MerQmlLiveBenchManager::offerToStartBenchIfNotRunning();
        });

        layout()->addWidget(qmlLiveWidget);

        // TODO add 'what are the prerequisites'
    }

    ~MerRunConfigWidget()
    {
        delete m_qmlLiveDetailsUi, m_qmlLiveDetailsUi = 0;
    }

private:
    void initQmlLiveDetailsUi()
    {
        // Init infoWidget

        m_qmlLiveDetailsUi->noControlWorkspaceIconLabel->setPixmap(Utils::Icons::INFO.pixmap());
        m_qmlLiveDetailsUi->noBenchIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());
        m_qmlLiveDetailsUi->configureButton->setText(ICore::msgShowOptionsDialog());
        connect(m_qmlLiveDetailsUi->configureButton, &QAbstractButton::clicked,
                [] { ICore::showOptionsDialog(Constants::MER_GENERAL_OPTIONS_ID); });
        auto updateInfoWidgetVisibility = [this] {
            const bool noBench = !MerSettings::hasValidQmlLiveBenchLocation();
            m_qmlLiveDetailsUi->noBenchIconLabel->setVisible(noBench);
            m_qmlLiveDetailsUi->noBenchLabel->setVisible(noBench);
            const bool noControlWorkspace = !MerSettings::isSyncQmlLiveWorkspaceEnabled();
            m_qmlLiveDetailsUi->noControlWorkspaceIconLabel->setVisible(noControlWorkspace);
            m_qmlLiveDetailsUi->noControlWorkspaceLabel->setVisible(noControlWorkspace);
            m_qmlLiveDetailsUi->infoWidget->setVisible(noBench || noControlWorkspace);
        };
        updateInfoWidgetVisibility();
        connect(MerSettings::instance(), &MerSettings::qmlLiveBenchLocationChanged,
                this, updateInfoWidgetVisibility);
        connect(MerSettings::instance(), &MerSettings::syncQmlLiveWorkspaceEnabledChanged,
                this, updateInfoWidgetVisibility);

        connect(m_qmlLiveDetailsUi->restoreDefaults, &QAbstractButton::clicked,
                m_aspect, &MerRunConfigurationAspect::restoreQmlLiveDefaults);

        // Init port QSpinBox

        auto updatePort = [this] {
            bool useDefault = !m_aspect->qmlLiveIpcPort().isValid();
            m_qmlLiveDetailsUi->port->setValue(useDefault ? 0 : m_aspect->qmlLiveIpcPort().number());
            m_qmlLiveDetailsUi->port->setEnabled(!useDefault);
            m_qmlLiveDetailsUi->useCustomPort->setChecked(!useDefault);
        };
        void (QSpinBox::*QSpinBox_valueChanged)(int) = &QSpinBox::valueChanged;
        connect(m_qmlLiveDetailsUi->port, QSpinBox_valueChanged,
                m_aspect, [this](int value) {
                m_aspect->setQmlLiveIpcPort(value > 0 ? Port(value) : Port());
        });
        connect(m_aspect, &MerRunConfigurationAspect::qmlLiveIpcPortChanged,
                this, updatePort);
        updatePort();
        connect(m_qmlLiveDetailsUi->useCustomPort, &QCheckBox::stateChanged,
                this, [this](int state) {
                m_aspect->setQmlLiveIpcPort(state != Qt::Checked
                        ? Port()
                        : Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT));
        });

        // Init benchWorkspace PathChooser

        m_qmlLiveDetailsUi->benchWorkspace->setExpectedKind(PathChooser::ExistingDirectory);
        m_qmlLiveDetailsUi->benchWorkspace->setBaseDirectory(m_aspect->defaultQmlLiveBenchWorkspace());
        m_qmlLiveDetailsUi->benchWorkspace->setPath(m_aspect->qmlLiveBenchWorkspace());
        auto updateQmlLiveBenchWorkspace = [this] {
            m_aspect->setQmlLiveBenchWorkspace(m_qmlLiveDetailsUi->benchWorkspace->path());
        };
        connect(m_qmlLiveDetailsUi->benchWorkspace, &PathChooser::editingFinished,
                this, updateQmlLiveBenchWorkspace);
        connect(m_qmlLiveDetailsUi->benchWorkspace, &PathChooser::browsingFinished,
                this, updateQmlLiveBenchWorkspace);
        connect(m_aspect, &MerRunConfigurationAspect::qmlLiveBenchWorkspaceChanged,
                m_qmlLiveDetailsUi->benchWorkspace, &PathChooser::setPath);
        m_qmlLiveDetailsUi->benchWorkspace->setEnabled(MerSettings::isSyncQmlLiveWorkspaceEnabled());
        connect(MerSettings::instance(), &MerSettings::syncQmlLiveWorkspaceEnabledChanged,
                m_qmlLiveDetailsUi->benchWorkspace, &PathChooser::setEnabled);

        // Init targetWorkspace QLineEdit

        m_qmlLiveDetailsUi->targetWorkspace->setText(m_aspect->qmlLiveTargetWorkspace());
        connect(m_qmlLiveDetailsUi->targetWorkspace, &QLineEdit::textChanged,
                m_aspect, &MerRunConfigurationAspect::setQmlLiveTargetWorkspace);
        connect(m_aspect, &MerRunConfigurationAspect::qmlLiveTargetWorkspaceChanged,
                m_qmlLiveDetailsUi->targetWorkspace, &QLineEdit::setText);

        // Init options QCheckBoxes

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
        setupQmlLiveOption(m_qmlLiveDetailsUi->allowCreateMissing, MerRunConfigurationAspect::AllowCreateMissing);
        setupQmlLiveOption(m_qmlLiveDetailsUi->loadDummyData, MerRunConfigurationAspect::LoadDummyData);
    }

private:
    MerRunConfigurationAspect *m_aspect;
    Ui::MerRunConfigurationAspectQmlLiveDetailsWidget *m_qmlLiveDetailsUi;
};

MerRunConfigurationAspect::MerRunConfigurationAspect(Target *target)
    : m_target(target)
    , m_qmlLiveEnabled(false)
    , m_qmlLiveBenchWorkspace(defaultQmlLiveBenchWorkspace())
    , m_qmlLiveOptions(DEFAULT_QML_LIVE_OPTIONS)
{
    setId(Constants::MER_RUN_CONFIGURATION_ASPECT);
    setDisplayName(tr("Sailfish OS Application Settings"));
    setConfigWidgetCreator([this] { return new MerRunConfigWidget(this); });
}

QString MerRunConfigurationAspect::defaultQmlLiveBenchWorkspace() const
{
    return m_target->project()->projectDirectory().toString();
}

void MerRunConfigurationAspect::applyTo(ProjectExplorer::Runnable *r) const
{
    if (isQmlLiveEnabled()) {
        r->environment.appendOrSet(QLatin1String("LD_PRELOAD"),
                                   QLatin1String(QMLLIVE_SAILFISH_PRELOAD),
                                   QLatin1String(LIST_SEP));

        Port qmlLiveIpcPort = this->qmlLiveIpcPort();
        if (!qmlLiveIpcPort.isValid()) {
            const IDevice::ConstPtr device = DeviceKitInformation::device(m_target->kit());
            const auto merDevice = device.dynamicCast<const MerDevice>();
            if (merDevice) {
                Utils::PortList ports = merDevice->qmlLivePorts();
                if (ports.hasMore())
                    qmlLiveIpcPort = ports.getNext();
            }
        }

        if (qmlLiveIpcPort.isValid()) {
            r->environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_IPC_PORT"),
                               qmlLiveIpcPort.toString());
        }

        if (!qmlLiveTargetWorkspace().isEmpty()) {
            r->environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_WORKSPACE"),
                               qmlLiveTargetWorkspace());
        }

        if (qmlLiveOptions() & MerRunConfigurationAspect::UpdateOnConnect) {
            r->environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_UPDATE_ON_CONNECT"),
                               QLatin1String("1"));
        }

        if (!(qmlLiveOptions() & MerRunConfigurationAspect::UpdatesAsOverlay)) {
            r->environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_NO_UPDATES_AS_OVERLAY"),
                               QLatin1String("1"));
        }

        if (qmlLiveOptions() & MerRunConfigurationAspect::AllowCreateMissing) {
            r->environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_ALLOW_CREATE_MISSING"),
                               QLatin1String("1"));
        }

        if (qmlLiveOptions() & MerRunConfigurationAspect::LoadDummyData) {
            r->environment.set(QLatin1String("QMLLIVERUNTIME_SAILFISH_LOAD_DUMMY_DATA"),
                               QLatin1String("1"));
        }
    }
}

void MerRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_qmlLiveEnabled = map.value(QLatin1String(QML_LIVE_ENABLED), false).toBool();
    m_qmlLiveIpcPort = Port(map.value(QLatin1String(QML_LIVE_IPC_PORT_KEY), -1).toInt());
    m_qmlLiveBenchWorkspace = map.value(QLatin1String(QML_LIVE_BENCH_WORKSPACE_KEY),
                                        defaultQmlLiveBenchWorkspace()).toString();
    m_qmlLiveTargetWorkspace = map.value(QLatin1String(QML_LIVE_TARGET_WORKSPACE_KEY), QString()).toString();
    m_qmlLiveOptions = static_cast<QmlLiveOption>(map.value(QLatin1String(QML_LIVE_OPTIONS_KEY),
                                                            static_cast<int>(DEFAULT_QML_LIVE_OPTIONS)).toInt());
}

void MerRunConfigurationAspect::toMap(QVariantMap &map) const
{
    map.insert(QLatin1String(QML_LIVE_ENABLED), m_qmlLiveEnabled);
    map.insert(QLatin1String(QML_LIVE_IPC_PORT_KEY), m_qmlLiveIpcPort.isValid() ?  m_qmlLiveIpcPort.number() : -1);
    map.insert(QLatin1String(QML_LIVE_BENCH_WORKSPACE_KEY), m_qmlLiveBenchWorkspace);
    map.insert(QLatin1String(QML_LIVE_TARGET_WORKSPACE_KEY), m_qmlLiveTargetWorkspace);
    map.insert(QLatin1String(QML_LIVE_OPTIONS_KEY), static_cast<int>(m_qmlLiveOptions));
}

void MerRunConfigurationAspect::restoreQmlLiveDefaults()
{
    setQmlLiveIpcPort(Port());
    setQmlLiveBenchWorkspace(defaultQmlLiveBenchWorkspace());
    setQmlLiveTargetWorkspace(QString());
    setQmlLiveOptions(DEFAULT_QML_LIVE_OPTIONS);
}

void MerRunConfigurationAspect::setQmlLiveEnabled(bool qmlLiveEnabled)
{
    if (m_qmlLiveEnabled == qmlLiveEnabled)
        return;

    m_qmlLiveEnabled = qmlLiveEnabled;

    emit qmlLiveEnabledChanged(m_qmlLiveEnabled);
}

void MerRunConfigurationAspect::setQmlLiveIpcPort(Port port)
{
    if (m_qmlLiveIpcPort == port)
        return;

    m_qmlLiveIpcPort = port;

    emit qmlLiveIpcPortChanged();
}

void MerRunConfigurationAspect::setQmlLiveBenchWorkspace(const QString &benchWorkspace)
{
    if (m_qmlLiveBenchWorkspace == benchWorkspace)
        return;

    m_qmlLiveBenchWorkspace = benchWorkspace;

    emit qmlLiveBenchWorkspaceChanged(m_qmlLiveBenchWorkspace);
}

void MerRunConfigurationAspect::setQmlLiveTargetWorkspace(const QString &targetWorkspace)
{
    if (m_qmlLiveTargetWorkspace == targetWorkspace)
        return;

    m_qmlLiveTargetWorkspace = targetWorkspace;

    emit qmlLiveTargetWorkspaceChanged(m_qmlLiveTargetWorkspace);
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
