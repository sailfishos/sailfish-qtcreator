/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "merconstants.h"
#include "merdevicefactory.h"
#include "meroptionspage.h"
#include "mersettings.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <ssh/sshconnection.h>

#include <QComboBox>
#include <QDir>
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
    if (engine)
        return engine->name();
    return QString();
}

QList<Task> MerSdkKitInformation::validate(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        const QString name = kit->value(MerSdkKitInformation::id()).toString();
        if (!Sdk::buildEngine(name)) {
            const QString message = QCoreApplication::translate("MerSdk",
                    "No valid Sailfish OS build engine \"%1\" found").arg(name);
            return QList<Task>() << Task(Task::Error, message, FileName(), -1,
                                         Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }
    return QList<Task>();
}

BuildEngine* MerSdkKitInformation::buildEngine(const Kit *kit)
{
    if (!kit)
        return 0;
    return Sdk::buildEngine(kit->value(MerSdkKitInformation::id()).toString());
}

KitInformation::ItemList MerSdkKitInformation::toUserOutput(const Kit *kit) const
{
    if (DeviceTypeKitInformation::deviceTypeId(kit) == Constants::MER_DEVICE_TYPE) {
        QString name;
        BuildEngine *const engine = MerSdkKitInformation::buildEngine(kit);
        if (engine)
            name = engine->name();
        return KitInformation::ItemList()
                << qMakePair(tr("Sailfish OS build engine"), name);
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

void MerSdkKitInformation::setBuildEngine(Kit *kit, const BuildEngine *buildEngine)
{
    if(kit->value(MerSdkKitInformation::id()) != buildEngine->name())
        kit->setValue(MerSdkKitInformation::id(), buildEngine->name());
}

void MerSdkKitInformation::addToEnvironment(const Kit *kit, Environment &env) const
{
    const BuildEngine *engine = MerSdkKitInformation::buildEngine(kit);
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

////////////////////////////////////////////////////////////////////////////////////////////

MerSdkKitInformationWidget::MerSdkKitInformationWidget(Kit *kit,
        const MerSdkKitInformation *kitInformation)
    : KitConfigWidget(kit, kitInformation),
      m_combo(new QComboBox),
      m_manageButton(new QPushButton(tr("Manage...")))
{
    if (!visibleInKit())
        return;

    handleSdksUpdated();
    connect(m_combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MerSdkKitInformationWidget::handleCurrentIndexChanged);
    connect(Sdk::instance(), &Sdk::buildEngineAdded,
            this, &MerSdkKitInformationWidget::handleSdksUpdated);
    connect(Sdk::instance(), &Sdk::aboutToRemoveBuildEngine,
            this, &MerSdkKitInformationWidget::handleSdksUpdated, Qt::QueuedConnection);
    connect(m_manageButton, &QPushButton::clicked,
            this, &MerSdkKitInformationWidget::handleManageClicked);
}

QString MerSdkKitInformationWidget::displayName() const
{
    return tr("Sailfish OS build engine:");
}

QString MerSdkKitInformationWidget::toolTip() const
{
    return tr("Name of the virtual machine used as a Sailfish OS build engine.");
}

void MerSdkKitInformationWidget::makeReadOnly()
{
    m_combo->setEnabled(false);
}

void MerSdkKitInformationWidget::refresh()
{
    const BuildEngine* engine = MerSdkKitInformation::buildEngine(m_kit);
    int i;
    if (engine) {
        for (i = m_combo->count() - 1; i >= 0; --i) {
            if (engine->name() == m_combo->itemText(i))
                break;
        }
    } else {
        i = 0;
    }
    m_combo->setCurrentIndex(i);
}

bool MerSdkKitInformationWidget::visibleInKit()
{
    return DeviceTypeKitInformation::deviceTypeId(m_kit) == Constants::MER_DEVICE_TYPE;
}

QWidget *MerSdkKitInformationWidget::mainWidget() const
{
    return m_combo;
}

QWidget *MerSdkKitInformationWidget::buttonWidget() const
{
    return m_manageButton;
}

void MerSdkKitInformationWidget::handleSdksUpdated()
{
    const QList<BuildEngine *> engines = Sdk::buildEngines();
    m_combo->blockSignals(true);
    m_combo->clear();
    if (engines.isEmpty()) {
        m_combo->addItem(tr("None"));
    } else {
        for (BuildEngine *const engine : engines)
            m_combo->addItem(engine->name());
    }
    m_combo->blockSignals(false);
    refresh();
}

void MerSdkKitInformationWidget::handleManageClicked()
{
    MerOptionsPage *page = PluginManager::getObject<MerOptionsPage>();
    if (page)
        page->setSdk(m_combo->currentText());
    ICore::showOptionsDialog(Constants::MER_OPTIONS_ID);
}

void MerSdkKitInformationWidget::handleCurrentIndexChanged()
{
    BuildEngine *const engine = Sdk::buildEngine(m_combo->currentText());
    if (engine)
        MerSdkKitInformation::setBuildEngine(m_kit, engine);
}

}
}
