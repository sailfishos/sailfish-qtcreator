/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "mersdkkitaspect.h"

#include "merbuildengineoptionspage.h"
#include "merconstants.h"
#include "merdevicefactory.h"
#include "merplugin.h"
#include "mersettings.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QDir>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerSdkKitAspect *MerSdkKitAspect::s_instance = nullptr;

MerSdkKitAspect::MerSdkKitAspect()
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    setId(MerSdkKitAspect::id());
    setDisplayName(Sdk::sdkVariant());
    setDescription(tr("%1 build engine and build target.").arg(Sdk::osVariant()));
    setPriority(24);

    connect(MerSettings::instance(), &MerSettings::environmentFilterChanged,
            this, &MerSdkKitAspect::notifyAllUpdated);
}

MerSdkKitAspect::~MerSdkKitAspect()
{
    s_instance = nullptr;
}

bool MerSdkKitAspect::isApplicableToKit(const Kit *kit) const
{
    return DeviceTypeKitAspect::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE;
}

Tasks MerSdkKitAspect::validate(const Kit *kit) const
{
    if (DeviceTypeKitAspect::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        auto error = [](const QString &message) {
            return Task(Task::Error, message, FilePath(), -1,
                    Utils::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        };

        QVariantMap data = kit->value(MerSdkKitAspect::id()).toMap();
        QUrl buildEngineUri;
        QString buildTargetName;
        if (!fromMap(data, &buildEngineUri, &buildTargetName)) {
            return {error(QCoreApplication::translate("MerSdk",
                    "No %1 information in kit \"%2\"")
                    .arg(Sdk::sdkVariant())
                    .arg(kit->displayName()))};
        }
        BuildEngine *const engine = Sdk::buildEngine(buildEngineUri);
        if (!engine) {
            return {error(QCoreApplication::translate("MerSdk",
                    "Unknown %1 build engine referred by kit \"%2\"")
                    .arg(Sdk::osVariant())
                    .arg(kit->displayName()))};
        }
        if (!engine->buildTarget(buildTargetName).isValid()) {
            return {error(QCoreApplication::translate("MerSdk",
                    "Unknown %1 build target referred by kit \"%2\"")
                    .arg(Sdk::osVariant())
                    .arg(kit->displayName()))};
        }
    }
    return {};
}

BuildEngine* MerSdkKitAspect::buildEngine(const Kit *kit)
{
    if (!kit)
        return nullptr;
    QVariantMap data = kit->value(MerSdkKitAspect::id()).toMap();
    QUrl buildEngineUri;
    QString buildTargetName;
    if (!fromMap(data, &buildEngineUri, &buildTargetName))
        return nullptr;
    return Sdk::buildEngine(buildEngineUri);
}

Sfdk::BuildTargetData MerSdkKitAspect::buildTarget(const Kit *kit)
{
    if (!kit)
        return {};
    QVariantMap data = kit->value(MerSdkKitAspect::id()).toMap();
    QUrl buildEngineUri;
    QString buildTargetName;
    if (!fromMap(data, &buildEngineUri, &buildTargetName))
        return {};
    BuildEngine *const buildEngine = Sdk::buildEngine(buildEngineUri);
    if (!buildEngine)
        return {};
    return buildEngine->buildTarget(buildTargetName);
}

QString MerSdkKitAspect::buildTargetName(const Kit *kit)
{
    if (!kit)
        return {};
    QVariantMap data = kit->value(MerSdkKitAspect::id()).toMap();
    QUrl buildEngineUri;
    QString buildTargetName;
    if (!fromMap(data, &buildEngineUri, &buildTargetName))
        return {};
    return buildTargetName;
}

KitAspect::ItemList MerSdkKitAspect::toUserOutput(const Kit *kit) const
{
    if (DeviceTypeKitAspect::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        QString buildEngineName;
        BuildEngine *const engine = MerSdkKitAspect::buildEngine(kit);
        if (engine)
            buildEngineName = engine->name();

        const QString buildTargetName = MerSdkKitAspect::buildTargetName(kit);

        return KitAspect::ItemList()
                << qMakePair(tr("%1 build engine").arg(Sdk::osVariant()), buildEngineName)
                << qMakePair(tr("%1 build target").arg(Sdk::osVariant()), buildTargetName);
    }
    return KitAspect::ItemList();
}

KitAspectWidget *MerSdkKitAspect::createConfigWidget(Kit *kit) const
{
    return new MerSdkKitAspectWidget(kit, this);
}

Utils::Id MerSdkKitAspect::id()
{
    return "Mer.Sdk.Kit.Information";
}

void MerSdkKitAspect::setBuildTarget(Kit *kit, const BuildEngine *buildEngine,
        const QString &buildTargetName)
{
    QTC_ASSERT(buildEngine, return);
    QTC_ASSERT(buildEngine->buildTargetNames().contains(buildTargetName), return);

    QVariantMap data = toMap(buildEngine->uri(), buildTargetName);
    if (kit->value(MerSdkKitAspect::id()) != data)
        kit->setValue(MerSdkKitAspect::id(), data);
}

void MerSdkKitAspect::addToEnvironment(const Kit *kit, Environment &env) const
{
    const BuildEngine *engine = MerSdkKitAspect::buildEngine(kit);
    const QString targetName = MerSdkKitAspect::buildTargetName(kit);
    if (engine) {
        const QString sharedTarget = QDir::fromNativeSeparators(engine->sharedTargetsPath().toString());
        const QString sharedSrc = QDir::fromNativeSeparators(engine->sharedSrcPath().toString());

        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_TARGET), sharedTarget);
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC), sharedSrc);
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC_MOUNT_POINT),
                engine->sharedSrcMountPoint());
        if (!MerSettings::isEnvironmentFilterFromEnvironment() &&
                !MerSettings::environmentFilter().isEmpty()) {
            env.appendOrSet(QLatin1String(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER),
                    MerSettings::environmentFilter());
        }
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_TARGET_NAME), targetName);
    }
}

void MerSdkKitAspect::notifyAboutUpdate(const Sfdk::BuildEngine *buildEngine)
{
    for (Kit *const k : KitManager::kits()) {
        if (MerSdkKitAspect::buildEngine(k) == buildEngine)
            s_instance->notifyAboutUpdate(k);
    }
}

void MerSdkKitAspect::notifyAllUpdated()
{
    for (Kit *const k : KitManager::kits()) {
        if (k->hasValue(MerSdkKitAspect::id()))
            notifyAboutUpdate(k);
    }
}

QVariantMap MerSdkKitAspect::toMap(const QUrl &buildEngineUri, const QString &buildTargetName)
{
    QVariantMap data;
    data.insert(Constants::BUILD_ENGINE_URI, buildEngineUri);
    data.insert(Constants::BUILD_TARGET_NAME, buildTargetName);
    return data;
}

bool MerSdkKitAspect::fromMap(const QVariantMap &data, QUrl *buildEngineUri,
        QString *buildTargetName)
{
    Q_ASSERT(buildEngineUri);
    Q_ASSERT(buildTargetName);
    *buildEngineUri = data.value(Constants::BUILD_ENGINE_URI).toUrl();
    *buildTargetName = data.value(Constants::BUILD_TARGET_NAME).toString();
    return buildEngineUri->isValid() && !buildTargetName->isEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////////

MerSdkKitAspectWidget::MerSdkKitAspectWidget(Kit *kit,
        const MerSdkKitAspect *kitAspect)
    : KitAspectWidget(kit, kitAspect)
{
    m_mainWidget = new QWidget;
    m_mainWidget->setContentsMargins(0, 0, 0, 0);

    auto *const layout = new QGridLayout(m_mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setColumnStretch(1, 2);

    layout->addWidget(new QLabel(tr("Build engine:")), 0, 0);
    m_buildEngineComboBox = new QComboBox;
    m_buildEngineComboBox->setSizePolicy(QSizePolicy::Ignored,
            m_buildEngineComboBox->sizePolicy().verticalPolicy());
    layout->addWidget(m_buildEngineComboBox, 0, 1);

    layout->addWidget(new QLabel(tr("Build target:")), 1, 0);
    m_buildTargetComboBox = new QComboBox;
    m_buildTargetComboBox->setSizePolicy(QSizePolicy::Ignored,
            m_buildTargetComboBox->sizePolicy().verticalPolicy());
    layout->addWidget(m_buildTargetComboBox, 1, 1);

    m_manageButton = new QPushButton(KitAspectWidget::msgManage());
    m_manageButton->setContentsMargins(0, 0, 0, 0);

    connect(m_buildEngineComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerSdkKitAspectWidget::handleCurrentEngineIndexChanged);
    connect(m_buildTargetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerSdkKitAspectWidget::handleCurrentTargetIndexChanged);
    connect(Sdk::instance(), &Sdk::buildEngineAdded,
            this, &MerSdkKitAspectWidget::handleSdksUpdated);
    connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
            this, &MerSdkKitAspectWidget::handleSdksUpdated, Qt::QueuedConnection);
    connect(m_manageButton, &QPushButton::clicked,
            this, &MerSdkKitAspectWidget::handleManageClicked);
    handleSdksUpdated();
}

void MerSdkKitAspectWidget::makeReadOnly()
{
    m_buildEngineComboBox->setEnabled(false);
    m_buildTargetComboBox->setEnabled(false);
}

void MerSdkKitAspectWidget::refresh()
{
    const BuildEngine* engine = MerSdkKitAspect::buildEngine(m_kit);
    int i;
    if (engine) {
        for (i = m_buildEngineComboBox->count() - 1; i >= 0; --i) {
            if (engine->uri() == m_buildEngineComboBox->itemData(i))
                break;
        }
    } else {
        i = 0;
    }

    m_buildEngineComboBox->setCurrentIndex(i);
}

QWidget *MerSdkKitAspectWidget::mainWidget() const
{
    return m_mainWidget;
}

QWidget *MerSdkKitAspectWidget::buttonWidget() const
{
    return m_manageButton;
}

void MerSdkKitAspectWidget::handleSdksUpdated()
{
    const QList<BuildEngine *> engines = Sdk::buildEngines();
    m_buildEngineComboBox->blockSignals(true);
    m_buildEngineComboBox->clear();
    if (engines.isEmpty()) {
        m_buildEngineComboBox->addItem(tr("None"));
    } else {
        for (BuildEngine *const engine : engines)
            m_buildEngineComboBox->addItem(engine->name(), engine->uri());
    }
    m_buildEngineComboBox->setCurrentIndex(-1);
    m_buildEngineComboBox->blockSignals(false);
    refresh();
}

void MerSdkKitAspectWidget::handleManageClicked()
{
    MerPlugin::buildEngineOptionsPage()->setBuildEngine(
            m_buildEngineComboBox->currentData().toUrl());
    ICore::showOptionsDialog(Constants::MER_BUILD_ENGINE_OPTIONS_ID);
}

void MerSdkKitAspectWidget::handleCurrentEngineIndexChanged()
{
    BuildEngine *const engine = Sdk::buildEngine(m_buildEngineComboBox->currentData().toUrl());

    m_buildTargetComboBox->blockSignals(true);
    m_buildTargetComboBox->clear();
    if (!engine || engine->buildTargets().isEmpty()) {
        m_buildTargetComboBox->addItem(tr("None"));
    } else {
        for (const QString &targetName : engine->buildTargetNames())
            m_buildTargetComboBox->addItem(targetName);
    }
    m_buildTargetComboBox->setCurrentIndex(-1);
    m_buildTargetComboBox->blockSignals(false);

    const QString targetName = MerSdkKitAspect::buildTargetName(m_kit);
    int i;
    if (!targetName.isEmpty()) {
        for (i = m_buildTargetComboBox->count() - 1; i >= 0; --i) {
            if (targetName == m_buildTargetComboBox->itemText(i))
                break;
        }
    } else {
        i = 0;
    }
    m_buildTargetComboBox->setCurrentIndex(i);
}

void MerSdkKitAspectWidget::handleCurrentTargetIndexChanged()
{
    BuildEngine *const engine = Sdk::buildEngine(m_buildEngineComboBox->currentData().toUrl());
    const QString targetName = m_buildTargetComboBox->currentText();
    if (engine)
        MerSdkKitAspect::setBuildTarget(m_kit, engine, targetName);
}

}
}
