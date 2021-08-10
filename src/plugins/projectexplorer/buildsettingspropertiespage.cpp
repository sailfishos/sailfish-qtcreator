/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "buildsettingspropertiespage.h"
#include "buildinfo.h"
#include "buildstepspage.h"
#include "target.h"
#include "buildconfiguration.h"
#include "projectconfigurationmodel.h"
#include "session.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <utils/stringutils.h>

#include <QMargins>
#include <QCoreApplication>
#include <QComboBox>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace Utils;

///
// BuildSettingsWidget
///

BuildSettingsWidget::~BuildSettingsWidget()
{
    clearWidgets();
}

BuildSettingsWidget::BuildSettingsWidget(Target *target) :
    m_target(target)
{
    Q_ASSERT(m_target);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    if (!BuildConfigurationFactory::find(m_target)) {
        auto noSettingsLabel = new QLabel(this);
        noSettingsLabel->setText(tr("No build settings available"));
        QFont f = noSettingsLabel->font();
        f.setPointSizeF(f.pointSizeF() * 1.2);
        noSettingsLabel->setFont(f);
        vbox->addWidget(noSettingsLabel);
        return;
    }

    { // Edit Build Configuration row
        auto hbox = new QHBoxLayout();
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->addWidget(new QLabel(tr("Edit build configuration:"), this));
        m_buildConfigurationComboBox = new QComboBox(this);
        m_buildConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_buildConfigurationComboBox->setModel(m_target->buildConfigurationModel());
        hbox->addWidget(m_buildConfigurationComboBox);

        m_addButton = new QPushButton(this);
        m_addButton->setText(tr("Add"));
        m_addButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_addButton);
        m_addButtonMenu = new QMenu(this);
        m_addButton->setMenu(m_addButtonMenu);

        m_removeButton = new QPushButton(this);
        m_removeButton->setText(tr("Remove"));
        m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_removeButton);

        m_renameButton = new QPushButton(this);
        m_renameButton->setText(tr("Rename..."));
        m_renameButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_renameButton);

        m_cloneButton = new QPushButton(this);
        m_cloneButton->setText(tr("Clone..."));
        m_cloneButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_cloneButton);

        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        vbox->addLayout(hbox);
    }

    m_buildConfiguration = m_target->activeBuildConfiguration();
    m_buildConfigurationComboBox->setCurrentIndex(
                m_target->buildConfigurationModel()->indexFor(m_buildConfiguration));

    updateAddButtonMenu();
    updateBuildSettings();

    connect(m_buildConfigurationComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BuildSettingsWidget::currentIndexChanged);

    connect(m_removeButton, &QAbstractButton::clicked,
            this, [this]() { deleteConfiguration(m_buildConfiguration); });

    connect(m_renameButton, &QAbstractButton::clicked,
            this, &BuildSettingsWidget::renameConfiguration);

    connect(m_cloneButton, &QAbstractButton::clicked,
            this, &BuildSettingsWidget::cloneConfiguration);

    connect(m_target, &Target::activeBuildConfigurationChanged,
            this, &BuildSettingsWidget::updateActiveConfiguration);

    connect(m_target, &Target::kitChanged, this, &BuildSettingsWidget::updateAddButtonMenu);
}

void BuildSettingsWidget::addSubWidget(NamedWidget *widget)
{
    widget->setParent(this);
    widget->setContentsMargins(0, 10, 0, 0);

    auto label = new QLabel(this);
    label->setText(widget->displayName());
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(0, 10, 0, 0);

    layout()->addWidget(label);
    layout()->addWidget(widget);

    m_labels.append(label);
    m_subWidgets.append(widget);
}

void BuildSettingsWidget::clearWidgets()
{
    qDeleteAll(m_subWidgets);
    m_subWidgets.clear();
    qDeleteAll(m_labels);
    m_labels.clear();
}

void BuildSettingsWidget::updateAddButtonMenu()
{
    m_addButtonMenu->clear();

    if (m_target) {
        BuildConfigurationFactory *factory = BuildConfigurationFactory::find(m_target);
        if (!factory)
            return;
        for (const BuildInfo &info : factory->allAvailableBuilds(m_target)) {
            QAction *action = m_addButtonMenu->addAction(info.typeName);
            connect(action, &QAction::triggered, this, [this, info] {
                createConfiguration(info);
            });
        }
    }
}

void BuildSettingsWidget::updateBuildSettings()
{
    clearWidgets();

    // update buttons
    QList<BuildConfiguration *> bcs = m_target->buildConfigurations();
    m_removeButton->setEnabled(bcs.size() > 1);
    m_renameButton->setEnabled(!bcs.isEmpty());
    m_cloneButton->setEnabled(!bcs.isEmpty());

    if (m_buildConfiguration)
        m_buildConfiguration->addConfigWidgets([this](NamedWidget *w) { addSubWidget(w); });
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    auto buildConfiguration = qobject_cast<BuildConfiguration *>(
                m_target->buildConfigurationModel()->projectConfigurationAt(index));
    SessionManager::setActiveBuildConfiguration(m_target, buildConfiguration, SetActive::Cascade);
}

void BuildSettingsWidget::updateActiveConfiguration()
{
    if (!m_buildConfiguration || m_buildConfiguration == m_target->activeBuildConfiguration())
        return;

    m_buildConfiguration = m_target->activeBuildConfiguration();

    m_buildConfigurationComboBox->setCurrentIndex(
                m_target->buildConfigurationModel()->indexFor(m_buildConfiguration));

    updateBuildSettings();
}

void BuildSettingsWidget::createConfiguration(const BuildInfo &info_)
{
    BuildInfo info = info_;
    if (info.displayName.isEmpty()) {
        bool ok = false;
        info.displayName = QInputDialog::getText(Core::ICore::dialogParent(),
                                                 tr("New Configuration"),
                                                 tr("New configuration name:"),
                                                 QLineEdit::Normal,
                                                 QString(),
                                                 &ok)
                               .trimmed();
        if (!ok || info.displayName.isEmpty())
            return;
    }

    BuildConfiguration *bc = info.factory->create(m_target, info);
    if (!bc)
        return;

    m_target->addBuildConfiguration(bc);
    SessionManager::setActiveBuildConfiguration(m_target, bc, SetActive::Cascade);
}

QString BuildSettingsWidget::uniqueName(const QString & name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList bcNames;
        for (BuildConfiguration *bc : m_target->buildConfigurations()) {
            if (bc == m_buildConfiguration)
                continue;
            bcNames.append(bc->displayName());
        }
        result = makeUniquelyNumbered(result, bcNames);
    }
    return result;
}

void BuildSettingsWidget::renameConfiguration()
{
    QTC_ASSERT(m_buildConfiguration, return);
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for build configuration <b>%1</b>:").
                                            arg(m_buildConfiguration->displayName()),
                                         QLineEdit::Normal,
                                         m_buildConfiguration->displayName(), &ok);
    if (!ok)
        return;

    name = uniqueName(name);
    if (name.isEmpty())
        return;

    m_buildConfiguration->setDisplayName(name);

}

void BuildSettingsWidget::cloneConfiguration()
{
    QTC_ASSERT(m_buildConfiguration, return);
    BuildConfigurationFactory *factory = BuildConfigurationFactory::find(m_target);
    if (!factory)
        return;

    //: Title of a the cloned BuildConfiguration window, text of the window
    QString name = uniqueName(QInputDialog::getText(this,
                                                    tr("Clone Configuration"),
                                                    tr("New configuration name:"),
                                                    QLineEdit::Normal,
                                                    m_buildConfiguration->displayName()));
    if (name.isEmpty())
        return;

    BuildConfiguration *bc = BuildConfigurationFactory::clone(m_target, m_buildConfiguration);
    if (!bc)
        return;

    bc->setDisplayName(name);
    const std::function<bool(const QString &)> isBuildDirOk = [this](const QString &candidate) {
        const auto fp = FilePath::fromString(candidate);
        if (fp.exists())
            return false;
        return !anyOf(m_target->buildConfigurations(), [&fp](const BuildConfiguration *bc) {
            return bc->buildDirectory() == fp; });
    };
    bc->setBuildDirectory(FilePath::fromString(makeUniquelyNumbered(
                                                   bc->buildDirectory().toString(),
                                                   isBuildDirOk)));
    m_target->addBuildConfiguration(bc);
    SessionManager::setActiveBuildConfiguration(m_target, bc, SetActive::Cascade);
}

void BuildSettingsWidget::deleteConfiguration(BuildConfiguration *deleteConfiguration)
{
    if (!deleteConfiguration ||
        m_target->buildConfigurations().size() <= 1)
        return;

    if (BuildManager::isBuilding(deleteConfiguration)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Remove Build Configuration"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Remove Build Configuration %1?").arg(deleteConfiguration->displayName()));
        box.setText(tr("The build configuration <b>%1</b> is currently being built.").arg(deleteConfiguration->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and remove the Build Configuration anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    } else {
        QMessageBox msgBox(QMessageBox::Question, tr("Remove Build Configuration?"),
                           tr("Do you really want to delete build configuration <b>%1</b>?").arg(deleteConfiguration->displayName()),
                           QMessageBox::Yes|QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;
    }

    m_target->removeBuildConfiguration(deleteConfiguration);
}
