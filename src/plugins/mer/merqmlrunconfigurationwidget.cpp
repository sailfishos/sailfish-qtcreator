/****************************************************************************
**
** Copyright (C) 2016-2018 Jolla Ltd.
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

#include "merqmlrunconfigurationwidget.h"

#include "merconstants.h"
#include "merqmlrunconfiguration.h"

#include <qmakeprojectmanager/qmakeproject.h>
#include <projectexplorer/target.h>
#include <utils/detailswidget.h>
#include <utils/utilsicons.h>

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Mer {
namespace Internal {

MerQmlRunConfigurationWidget::MerQmlRunConfigurationWidget(
        MerQmlRunConfiguration *runConfiguration, QWidget *parent)
    : QWidget(parent)
    , m_runConfiguration(runConfiguration)
{
    auto topLayout = new QVBoxLayout{this};
    topLayout->setMargin(0);

    /* add disabled-info label */ {
        auto layout = new QHBoxLayout;
        layout->addStretch();
        m_disabledIcon = new QLabel;
        m_disabledIcon->setPixmap(Utils::Icons::WARNING.pixmap());
        layout->addWidget(m_disabledIcon);
        m_disabledReason = new QLabel;
        layout->addWidget(m_disabledReason);
        layout->addStretch();
        topLayout->addLayout(layout);
    }

    m_detailsContainer = new Utils::DetailsWidget{this};
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    /* add details widget */ {
        auto details = new QWidget;
        auto layout = new QFormLayout{details};
        layout->setMargin(0);

        m_command = new QLabel;
        m_command->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
        auto updateCommand = [this]() {
            auto r = m_runConfiguration->runnable();
            m_command->setText(r.executable + QLatin1Char{' '} + r.commandLineArguments);
        };
        updateCommand();
        auto project = qobject_cast<QmakeProject *>(m_runConfiguration->target()->project());
        connect(project, &QmakeProject::proFileUpdated, m_command, updateCommand);
        layout->addRow(tr("Command:"), m_command);

        m_detailsContainer->setWidget(details);
    }

    topLayout->addWidget(m_detailsContainer);

    auto updateDisabledInfo = [this]() {
        bool enabled = m_runConfiguration->isEnabled();
        m_detailsContainer->setEnabled(enabled);
        m_disabledIcon->setVisible(!enabled);
        m_disabledReason->setVisible(!enabled);
        m_disabledReason->setText(m_runConfiguration->disabledReason());
    };
    updateDisabledInfo();
    connect(m_runConfiguration.data(), &RunConfiguration::enabledChanged, this, updateDisabledInfo);
}

} // Internal
} // Mer
