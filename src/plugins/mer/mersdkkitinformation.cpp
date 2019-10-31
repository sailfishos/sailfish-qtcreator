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

#include "mersdkkitinformation.h"

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

MerSdkKitInformation *MerSdkKitInformation::s_instance = nullptr;

MerSdkKitInformation::MerSdkKitInformation()
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    setId(MerSdkKitInformation::id());
    setPriority(24);

    connect(MerSettings::instance(), &MerSettings::environmentFilterChanged,
            this, &MerSdkKitInformation::notifyAllUpdated);
}

MerSdkKitInformation::~MerSdkKitInformation()
{
    s_instance = nullptr;
}

QVariant MerSdkKitInformation::defaultValue(const Kit *kit) const
{
    BuildEngine *const engine = MerSdkKitInformation::buildEngine(kit);
    if (!engine || engine->buildTargets().isEmpty())
        return {};
    return toMap(engine->uri(), engine->buildTargets().first().name);
}

QList<Task> MerSdkKitInformation::validate(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        auto error = [](const QString &message) {
            return Task(Task::Error, message, FileName(), -1,
                    Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        };

        QVariantMap data = kit->value(MerSdkKitInformation::id()).toMap();
        QUrl buildEngineUri;
        QString buildTargetName;
        if (!fromMap(data, &buildEngineUri, &buildTargetName)) {
            return {error(QCoreApplication::translate("MerSdk",
                    "No Sailfish SDK information in kit \"%1\"")
                    .arg(kit->displayName()))};
        }
        BuildEngine *const engine = Sdk::buildEngine(buildEngineUri);
        if (!engine) {
            return {error(QCoreApplication::translate("MerSdk",
                    "Unknown Sailfish OS build engine referred by kit \"%1\"")
                    .arg(kit->displayName()))};
        }
        if (!engine->buildTarget(buildTargetName).isValid()) {
            return {error(QCoreApplication::translate("MerSdk",
                    "Unknown Sailfish OS build target referred by kit \"%1\"")
                    .arg(kit->displayName()))};
        }
    }
    return {};
}

BuildEngine* MerSdkKitInformation::buildEngine(const Kit *kit)
{
    if (!kit)
        return nullptr;
    QVariantMap data = kit->value(MerSdkKitInformation::id()).toMap();
    QUrl buildEngineUri;
    QString buildTargetName;
    if (!fromMap(data, &buildEngineUri, &buildTargetName))
        return nullptr;
    return Sdk::buildEngine(buildEngineUri);
}

Sfdk::BuildTargetData MerSdkKitInformation::buildTarget(const Kit *kit)
{
    if (!kit)
        return {};
    QVariantMap data = kit->value(MerSdkKitInformation::id()).toMap();
    QUrl buildEngineUri;
    QString buildTargetName;
    if (!fromMap(data, &buildEngineUri, &buildTargetName))
        return {};
    BuildEngine *const buildEngine = Sdk::buildEngine(buildEngineUri);
    if (!buildEngine)
        return {};
    return buildEngine->buildTarget(buildTargetName);
}

QString MerSdkKitInformation::buildTargetName(const Kit *kit)
{
    if (!kit)
        return {};
    QVariantMap data = kit->value(MerSdkKitInformation::id()).toMap();
    QUrl buildEngineUri;
    QString buildTargetName;
    if (!fromMap(data, &buildEngineUri, &buildTargetName))
        return {};
    return buildTargetName;
}

KitInformation::ItemList MerSdkKitInformation::toUserOutput(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        QString buildEngineName;
        BuildEngine *const engine = MerSdkKitInformation::buildEngine(kit);
        if (engine)
            buildEngineName = engine->name();

        const QString buildTargetName = MerSdkKitInformation::buildTargetName(kit);

        return KitInformation::ItemList()
                << qMakePair(tr("Sailfish OS build engine"), buildEngineName)
                << qMakePair(tr("Sailfish OS build target"), buildTargetName);
    }
    return KitInformation::ItemList();
}

KitConfigWidget *MerSdkKitInformation::createConfigWidget(Kit *kit) const
{
    return new MerSdkKitInformationWidget(kit, this);
}

Core::Id MerSdkKitInformation::id()
{
    return "Mer.Sdk.Kit.Information";
}

void MerSdkKitInformation::setBuildTarget(Kit *kit, const BuildEngine *buildEngine,
        const QString &buildTargetName)
{
    QTC_ASSERT(buildEngine, return);
    QTC_ASSERT(buildEngine->buildTargetNames().contains(buildTargetName), return);

    QVariantMap data = toMap(buildEngine->uri(), buildTargetName);
    if (kit->value(MerSdkKitInformation::id()) != data)
        kit->setValue(MerSdkKitInformation::id(), data);
}

void MerSdkKitInformation::addToEnvironment(const Kit *kit, Environment &env) const
{
    const BuildEngine *engine = MerSdkKitInformation::buildEngine(kit);
    const QString targetName = MerSdkKitInformation::buildTargetName(kit);
    if (engine) {
        const QString sshPort = QString::number(engine->sshPort());
        const QString sharedHome = QDir::fromNativeSeparators(engine->sharedHomePath().toString());
        const QString sharedTarget = QDir::fromNativeSeparators(engine->sharedTargetsPath().toString());
        const QString sharedSrc = QDir::fromNativeSeparators(engine->sharedSrcPath().toString());

        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_USERNAME),
                        QLatin1String(Sfdk::Constants::BUILD_ENGINE_DEFAULT_USER_NAME));
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_PORT), sshPort);
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_PRIVATE_KEY),
                engine->virtualMachine()->sshParameters().privateKeyFile);
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_HOME), sharedHome);
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_TARGET), sharedTarget);
        if (!sharedSrc.isEmpty())
            env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC), sharedSrc);
        if (!MerSettings::isEnvironmentFilterFromEnvironment() &&
                !MerSettings::environmentFilter().isEmpty()) {
            env.appendOrSet(QLatin1String(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER),
                    MerSettings::environmentFilter());
        }
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_TARGET_NAME), targetName);
    }
}

void MerSdkKitInformation::notifyAboutUpdate(const Sfdk::BuildEngine *buildEngine)
{
    auto related = [=](const Kit *k) {
        return MerSdkKitInformation::buildEngine(k) == buildEngine;
    };
    for (Kit *k : KitManager::kits(related))
        s_instance->notifyAboutUpdate(k);
}

void MerSdkKitInformation::notifyAllUpdated()
{
    for (Kit *k : KitManager::kits([](const Kit *k) { return k->hasValue(MerSdkKitInformation::id()); }))
        notifyAboutUpdate(k);
}

QVariantMap MerSdkKitInformation::toMap(const QUrl &buildEngineUri, const QString &buildTargetName)
{
    QVariantMap data;
    data.insert(Constants::BUILD_ENGINE_URI, buildEngineUri);
    data.insert(Constants::BUILD_TARGET_NAME, buildTargetName);
    return data;
}

bool MerSdkKitInformation::fromMap(const QVariantMap &data, QUrl *buildEngineUri,
        QString *buildTargetName)
{
    Q_ASSERT(buildEngineUri);
    Q_ASSERT(buildTargetName);
    *buildEngineUri = data.value(Constants::BUILD_ENGINE_URI).toUrl();
    *buildTargetName = data.value(Constants::BUILD_TARGET_NAME).toString();
    return buildEngineUri->isValid() && !buildTargetName->isEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////////

MerSdkKitInformationWidget::MerSdkKitInformationWidget(Kit *kit,
        const MerSdkKitInformation *kitInformation)
    : KitConfigWidget(kit, kitInformation)
{
    if (!visibleInKit())
        return;

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

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());
    m_manageButton->setContentsMargins(0, 0, 0, 0);

    connect(m_buildEngineComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerSdkKitInformationWidget::handleCurrentEngineIndexChanged);
    connect(m_buildTargetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerSdkKitInformationWidget::handleCurrentTargetIndexChanged);
    connect(Sdk::instance(), &Sdk::buildEngineAdded,
            this, &MerSdkKitInformationWidget::handleSdksUpdated);
    connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
            this, &MerSdkKitInformationWidget::handleSdksUpdated, Qt::QueuedConnection);
    connect(m_manageButton, &QPushButton::clicked,
            this, &MerSdkKitInformationWidget::handleManageClicked);
    handleSdksUpdated();
}

QString MerSdkKitInformationWidget::displayName() const
{
    return tr("Sailfish SDK");
}

QString MerSdkKitInformationWidget::toolTip() const
{
    return tr("Sailfish OS build engine and build target.");
}

void MerSdkKitInformationWidget::makeReadOnly()
{
    m_buildEngineComboBox->setEnabled(false);
    m_buildTargetComboBox->setEnabled(false);
}

void MerSdkKitInformationWidget::refresh()
{
    const BuildEngine* engine = MerSdkKitInformation::buildEngine(m_kit);
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

bool MerSdkKitInformationWidget::visibleInKit()
{
    return DeviceTypeKitInformation::deviceTypeId(m_kit) == Constants::MER_DEVICE_TYPE;
}

QWidget *MerSdkKitInformationWidget::mainWidget() const
{
    return m_mainWidget;
}

QWidget *MerSdkKitInformationWidget::buttonWidget() const
{
    return m_manageButton;
}

void MerSdkKitInformationWidget::handleSdksUpdated()
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
    m_buildEngineComboBox->blockSignals(false);
    refresh();
}

void MerSdkKitInformationWidget::handleManageClicked()
{
    MerPlugin::buildEngineOptionsPage()->setBuildEngine(
            m_buildEngineComboBox->currentData().toUrl());
    ICore::showOptionsDialog(Constants::MER_BUILD_ENGINE_OPTIONS_ID);
}

void MerSdkKitInformationWidget::handleCurrentEngineIndexChanged()
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
    m_buildTargetComboBox->blockSignals(false);

    const QString targetName = MerSdkKitInformation::buildTargetName(m_kit);
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

void MerSdkKitInformationWidget::handleCurrentTargetIndexChanged()
{
    BuildEngine *const engine = Sdk::buildEngine(m_buildEngineComboBox->currentData().toUrl());
    const QString targetName = m_buildTargetComboBox->currentText();
    if (engine)
        MerSdkKitInformation::setBuildTarget(m_kit, engine, targetName);
}

}
}
