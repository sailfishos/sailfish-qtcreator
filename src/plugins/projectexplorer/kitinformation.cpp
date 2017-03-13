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

#include "kitinformation.h"

#include "devicesupport/desktopdevice.h"
#include "devicesupport/devicemanager.h"
#include "projectexplorerconstants.h"
#include "kit.h"
#include "kitinformationconfigwidget.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <ssh/sshconnection.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// SysRootKitInformation:
// --------------------------------------------------------------------------

SysRootKitInformation::SysRootKitInformation()
{
    setObjectName(QLatin1String("SysRootInformation"));
    setId(SysRootKitInformation::id());
    setPriority(31000);
}

QVariant SysRootKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k)
    return QString();
}

QList<Task> SysRootKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    const Utils::FileName dir = SysRootKitInformation::sysRoot(k);
    if (dir.isEmpty())
        return result;

    const QFileInfo fi = dir.toFileInfo();

    if (!fi.exists()) {
        result << Task(Task::Error, tr("Sys Root \"%1\" does not exist in the file system.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else if (!fi.isDir()) {
        result << Task(Task::Error, tr("Sys Root \"%1\" is not a directory.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else if (QDir(dir.toString()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        result << Task(Task::Error, tr("Sys Root \"%1\" is empty.").arg(dir.toUserOutput()),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

KitConfigWidget *SysRootKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::SysRootInformationConfigWidget(k, this);
}

KitInformation::ItemList SysRootKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Sys Root"), sysRoot(k).toUserOutput());
}

void SysRootKitInformation::addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const
{
    expander->registerFileVariables("SysRoot", tr("Sys Root"), [this, kit]() -> QString {
        return SysRootKitInformation::sysRoot(kit).toString();
    });
}

Core::Id SysRootKitInformation::id()
{
    return "PE.Profile.SysRoot";
}

bool SysRootKitInformation::hasSysRoot(const Kit *k)
{
    if (k)
        return !k->value(SysRootKitInformation::id()).toString().isEmpty();
    return false;
}

Utils::FileName SysRootKitInformation::sysRoot(const Kit *k)
{
    if (!k)
        return Utils::FileName();
    return Utils::FileName::fromString(k->value(SysRootKitInformation::id()).toString());
}

void SysRootKitInformation::setSysRoot(Kit *k, const Utils::FileName &v)
{
    if (k)
        k->setValue(SysRootKitInformation::id(), v.toString());
}

// --------------------------------------------------------------------------
// ToolChainKitInformation:
// --------------------------------------------------------------------------

ToolChainKitInformation::ToolChainKitInformation()
{
    setObjectName(QLatin1String("ToolChainInformation"));
    setId(ToolChainKitInformation::id());
    setPriority(30000);

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &ToolChainKitInformation::kitsWereLoaded);
}

QVariant ToolChainKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);
    QList<ToolChain *> tcList = ToolChainManager::toolChains();
    if (tcList.isEmpty())
        return QString();

    Abi abi = Abi::hostAbi();

    ToolChain *tc = Utils::findOr(tcList, tcList.first(),
                                  Utils::equal(&ToolChain::targetAbi, abi));

    return tc->id();
}

QList<Task> ToolChainKitInformation::validate(const Kit *k) const
{
    QList<Task> result;

    const ToolChain* toolchain = toolChain(k);
    if (!toolchain) {
        result << Task(Task::Error, ToolChainKitInformation::msgNoToolChainInTarget(),
                       Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else {
        result << toolchain->validateKit(k);
    }
    return result;
}

void ToolChainKitInformation::fix(Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return);
    if (toolChain(k))
        return;

    qWarning("No tool chain set from kit \"%s\".",
             qPrintable(k->displayName()));
    setToolChain(k, 0); // make sure to clear out no longer known tool chains
}

void ToolChainKitInformation::setup(Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return);
    const QByteArray id = k->value(ToolChainKitInformation::id()).toByteArray();
    if (id.isEmpty())
        return;

    ToolChain *tc = ToolChainManager::findToolChain(id);
    if (tc)
        return;

    // ID is not found: Might be an ABI string...
    foreach (ToolChain *current, ToolChainManager::toolChains()) {
        if (current->targetAbi().toString() == QString::fromUtf8(id))
            return setToolChain(k, current);
    }
}

KitConfigWidget *ToolChainKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::ToolChainInformationConfigWidget(k, this);
}

QString ToolChainKitInformation::displayNamePostfix(const Kit *k) const
{
    ToolChain *tc = toolChain(k);
    return tc ? tc->displayName() : QString();
}

KitInformation::ItemList ToolChainKitInformation::toUserOutput(const Kit *k) const
{
    ToolChain *tc = toolChain(k);
    return ItemList() << qMakePair(tr("Compiler"), tc ? tc->displayName() : tr("None"));
}

void ToolChainKitInformation::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    ToolChain *tc = toolChain(k);
    if (tc)
        tc->addToEnvironment(env);
}

void ToolChainKitInformation::addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const
{
    // FIXME: Use better strings
    expander->registerVariable("Compiler:Name", tr("Compiler"),
                               [this, kit]() -> QString {
                                   const ToolChain *tc = toolChain(kit);
                                   return tc ? tc->displayName() : tr("None");
                               });

    expander->registerVariable("Compiler:Executable", tr("Path to the compiler executable"),
                               [this, kit]() -> QString {
                                   const ToolChain *tc = toolChain(kit);
                                   return tc ? tc->compilerCommand().toString() : QString();
                               });
}


IOutputParser *ToolChainKitInformation::createOutputParser(const Kit *k) const
{
    ToolChain *tc = toolChain(k);
    if (tc)
        return tc->outputParser();
    return 0;
}

Core::Id ToolChainKitInformation::id()
{
    return "PE.Profile.ToolChain";
}

ToolChain *ToolChainKitInformation::toolChain(const Kit *k)
{
    QTC_ASSERT(ToolChainManager::isLoaded(), return 0);
    if (!k)
        return 0;
    return ToolChainManager::findToolChain(k->value(ToolChainKitInformation::id()).toByteArray());
}

void ToolChainKitInformation::setToolChain(Kit *k, ToolChain *tc)
{
    k->setValue(ToolChainKitInformation::id(), tc ? QString::fromUtf8(tc->id()) : QString());
}

QString ToolChainKitInformation::msgNoToolChainInTarget()
{
    return tr("No compiler set in kit.");
}

void ToolChainKitInformation::kitsWereLoaded()
{
    foreach (Kit *k, KitManager::kits())
        fix(k);

    connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
            this, &ToolChainKitInformation::toolChainRemoved);
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainUpdated,
            this, &ToolChainKitInformation::toolChainUpdated);
}

void ToolChainKitInformation::toolChainUpdated(ToolChain *tc)
{
    auto matcher = KitMatcher([tc, this](const Kit *k) { return toolChain(k) == tc; });
    foreach (Kit *k, KitManager::matchingKits(matcher))
        notifyAboutUpdate(k);
}

void ToolChainKitInformation::toolChainRemoved(ToolChain *tc)
{
    Q_UNUSED(tc);
    foreach (Kit *k, KitManager::kits())
        fix(k);
}

// --------------------------------------------------------------------------
// DeviceTypeKitInformation:
// --------------------------------------------------------------------------

DeviceTypeKitInformation::DeviceTypeKitInformation()
{
    setObjectName(QLatin1String("DeviceTypeInformation"));
    setId(DeviceTypeKitInformation::id());
    setPriority(33000);
}

QVariant DeviceTypeKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);
    return QByteArray(Constants::DESKTOP_DEVICE_TYPE);
}

QList<Task> DeviceTypeKitInformation::validate(const Kit *k) const
{
    Q_UNUSED(k);
    return QList<Task>();
}

KitConfigWidget *DeviceTypeKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DeviceTypeInformationConfigWidget(k, this);
}

KitInformation::ItemList DeviceTypeKitInformation::toUserOutput(const Kit *k) const
{
    Core::Id type = deviceTypeId(k);
    QString typeDisplayName = tr("Unknown device type");
    if (type.isValid()) {
        IDeviceFactory *factory = ExtensionSystem::PluginManager::getObject<IDeviceFactory>(
            [&type](IDeviceFactory *factory) {
                return factory->availableCreationIds().contains(type);
            });

        if (factory)
            typeDisplayName = factory->displayNameForId(type);
    }
    return ItemList() << qMakePair(tr("Device type"), typeDisplayName);
}

const Core::Id DeviceTypeKitInformation::id()
{
    return "PE.Profile.DeviceType";
}

const Core::Id DeviceTypeKitInformation::deviceTypeId(const Kit *k)
{
    return k ? Core::Id::fromSetting(k->value(DeviceTypeKitInformation::id())) : Core::Id();
}

void DeviceTypeKitInformation::setDeviceTypeId(Kit *k, Core::Id type)
{
    k->setValue(DeviceTypeKitInformation::id(), type.toSetting());
}

KitMatcher DeviceTypeKitInformation::deviceTypeMatcher(Core::Id type)
{
    return KitMatcher(std::function<bool(const Kit *)>([type](const Kit *kit) {
        return type.isValid() && deviceTypeId(kit) == type;
    }));
}

QSet<Core::Id> DeviceTypeKitInformation::supportedPlatforms(const Kit *k) const
{
    return { deviceTypeId(k) };
}

QSet<Core::Id> DeviceTypeKitInformation::availableFeatures(const Kit *k) const
{
    Core::Id id = DeviceTypeKitInformation::deviceTypeId(k);
    if (id.isValid())
        return { Core::Id::fromString(QString::fromLatin1("DeviceType.") + id.toString()) };
    return QSet<Core::Id>();
}

// --------------------------------------------------------------------------
// DeviceKitInformation:
// --------------------------------------------------------------------------

DeviceKitInformation::DeviceKitInformation()
{
    setObjectName(QLatin1String("DeviceInformation"));
    setId(DeviceKitInformation::id());
    setPriority(32000);

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &DeviceKitInformation::kitsWereLoaded);
}

QVariant DeviceKitInformation::defaultValue(const Kit *k) const
{
    Core::Id type = DeviceTypeKitInformation::deviceTypeId(k);
    // Use default device if that is compatible:
    IDevice::ConstPtr dev = DeviceManager::instance()->defaultDevice(type);
    if (dev && dev->isCompatibleWith(k))
        return dev->id().toString();
    // Use any other device that is compatible:
    for (int i = 0; i < DeviceManager::instance()->deviceCount(); ++i) {
        dev = DeviceManager::instance()->deviceAt(i);
        if (dev && dev->isCompatibleWith(k))
            return dev->id().toString();
    }
    // Fail: No device set up.
    return QString();
}

QList<Task> DeviceKitInformation::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    QList<Task> result;
    if (dev.isNull())
        result.append(Task(Task::Warning, tr("No device set."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    else if (!dev->isCompatibleWith(k))
        result.append(Task(Task::Error, tr("Device is incompatible with this kit."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));

    return result;
}

void DeviceKitInformation::fix(Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (!dev.isNull() && !dev->isCompatibleWith(k)) {
        qWarning("Device is no longer compatible with kit \"%s\", removing it.",
                 qPrintable(k->displayName()));
        setDeviceId(k, Core::Id());
    }
}

void DeviceKitInformation::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return);
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    if (!dev.isNull() && dev->isCompatibleWith(k))
        return;

    setDeviceId(k, Core::Id::fromSetting(defaultValue(k)));
}

KitConfigWidget *DeviceKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DeviceInformationConfigWidget(k, this);
}

QString DeviceKitInformation::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = device(k);
    return dev.isNull() ? QString() : dev->displayName();
}

KitInformation::ItemList DeviceKitInformation::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = device(k);
    return ItemList() << qMakePair(tr("Device"), dev.isNull() ? tr("Unconfigured") : dev->displayName());
}

void DeviceKitInformation::addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const
{
    expander->registerVariable("Device:HostAddress", tr("Host address"),
        [this, kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().host : QString();
    });
    expander->registerVariable("Device:SshPort", tr("SSH port"),
        [this, kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? QString::number(device->sshParameters().port) : QString();
    });
    expander->registerVariable("Device:UserName", tr("User name"),
        [this, kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().userName : QString();
    });
    expander->registerVariable("Device:KeyFile", tr("Private key file"),
        [this, kit]() -> QString {
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().privateKeyFile : QString();
    });
}

Core::Id DeviceKitInformation::id()
{
    return "PE.Profile.Device";
}

IDevice::ConstPtr DeviceKitInformation::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::instance()->find(deviceId(k));
}

Core::Id DeviceKitInformation::deviceId(const Kit *k)
{
    return k ? Core::Id::fromSetting(k->value(DeviceKitInformation::id())) : Core::Id();
}

void DeviceKitInformation::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Core::Id());
}

void DeviceKitInformation::setDeviceId(Kit *k, Core::Id id)
{
    k->setValue(DeviceKitInformation::id(), id.toSetting());
}

void DeviceKitInformation::kitsWereLoaded()
{
    foreach (Kit *k, KitManager::kits())
        fix(k);

    DeviceManager *dm = DeviceManager::instance();
    connect(dm, &DeviceManager::deviceListReplaced, this, &DeviceKitInformation::devicesChanged);
    connect(dm, &DeviceManager::deviceAdded, this, &DeviceKitInformation::devicesChanged);
    connect(dm, &DeviceManager::deviceRemoved, this, &DeviceKitInformation::devicesChanged);
    connect(dm, &DeviceManager::deviceUpdated, this, &DeviceKitInformation::deviceUpdated);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &DeviceKitInformation::kitUpdated);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &DeviceKitInformation::kitUpdated);
}

void DeviceKitInformation::deviceUpdated(Core::Id id)
{
    foreach (Kit *k, KitManager::kits()) {
        if (deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void DeviceKitInformation::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void DeviceKitInformation::devicesChanged()
{
    foreach (Kit *k, KitManager::kits())
        setup(k); // Set default device if necessary
}

// --------------------------------------------------------------------------
// EnvironmentKitInformation:
// --------------------------------------------------------------------------

EnvironmentKitInformation::EnvironmentKitInformation()
{
    setObjectName(QLatin1String("EnvironmentKitInformation"));
    setId(EnvironmentKitInformation::id());
    setPriority(29000);
}

QVariant EnvironmentKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k)
    return QStringList();
}

QList<Task> EnvironmentKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    const QVariant variant = k->value(EnvironmentKitInformation::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        result.append(Task(Task::Error, tr("The environment setting value is invalid."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }
    return result;
}

void EnvironmentKitInformation::fix(Kit *k)
{
    const QVariant variant = k->value(EnvironmentKitInformation::id());
    if (!variant.isNull() && !variant.canConvert(QVariant::List)) {
        qWarning("Kit \"%s\" has a wrong environment value set.", qPrintable(k->displayName()));
        setEnvironmentChanges(k, QList<Utils::EnvironmentItem>());
    }
}

void EnvironmentKitInformation::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    const QVariant envValue = k->value(EnvironmentKitInformation::id());
    if (envValue.isValid())
        env.modify(Utils::EnvironmentItem::fromStringList(envValue.toStringList()));
}

KitConfigWidget *EnvironmentKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::KitEnvironmentConfigWidget(k, this);
}

KitInformation::ItemList EnvironmentKitInformation::toUserOutput(const Kit *k) const
{
    ItemList retVal;
    QVariant envValue = k->value(EnvironmentKitInformation::id());
    if (envValue.isValid())
        retVal << qMakePair(QLatin1Literal("Environment"), envValue.toStringList().join(QLatin1Literal("<br>")));

    return retVal;
}

Core::Id EnvironmentKitInformation::id()
{
    return "PE.Profile.Environment";
}

QList<Utils::EnvironmentItem> EnvironmentKitInformation::environmentChanges(const Kit *k)
{
     if (k)
         return Utils::EnvironmentItem::fromStringList(k->value(EnvironmentKitInformation::id()).toStringList());
     return QList<Utils::EnvironmentItem>();
}

void EnvironmentKitInformation::setEnvironmentChanges(Kit *k, const QList<Utils::EnvironmentItem> &changes)
{
    if (k)
        k->setValue(EnvironmentKitInformation::id(), Utils::EnvironmentItem::toStringList(changes));
}

} // namespace ProjectExplorer
