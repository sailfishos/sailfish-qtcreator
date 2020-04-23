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

#include "buildengine_p.h"

#include "dockervirtualmachine_p.h"
#include "sfdkconstants.h"
#include "sdk_p.h"
#include "targetsxmlreader_p.h"
#include "usersettings_p.h"
#include "virtualmachine_p.h"

#include <ssh/sshconnection.h>
#include <utils/algorithm.h>
#include <utils/filesystemwatcher.h>
#include <utils/persistentsettings.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QHostInfo>
#include <QPointer>
#include <QProcess>
#include <QTimer>

using namespace QSsh;
using namespace Utils;

namespace Sfdk {

namespace {

const char* SIMPLE_WRAPPERS[] = {
    Constants::WRAPPER_CMAKE,
    Constants::WRAPPER_QMAKE,
    Constants::WRAPPER_MAKE,
    Constants::WRAPPER_GCC,
    Constants::WRAPPER_DEPLOY,
    Constants::WRAPPER_RPM,
    Constants::WRAPPER_RPMVALIDATION,
    Constants::WRAPPER_LUPDATE,
};

} // namespace anonymous

/*!
 * \class RpmValidationSuiteData
 */

/*!
 * \class BuildTargetData
 */

bool BuildTargetData::isValid() const
{
    return !name.isEmpty()
        && !sysRoot.isEmpty()
        && !toolsPath.isEmpty()
        && !gdb.isEmpty();
}

Utils::FileName BuildTargetData::toolsPathCommonPrefix()
{
    return SdkPrivate::settingsLocation(SdkPrivate::UserScope)
        .appendPath(Constants::BUILD_TARGET_TOOLS);
}

/*!
 * \class BuildEngine::PrivateConstructorTag
 * \internal
 */

struct BuildEngine::PrivateConstructorTag {};

/*!
 * \class BuildEngine
 */

BuildEngine::BuildEngine(QObject *parent, const PrivateConstructorTag &)
    : QObject(parent)
    , d_ptr(std::make_unique<BuildEnginePrivate>(this))
{
    Q_D(BuildEngine);

    d->creationTime = QDateTime::currentDateTime();
    d->wwwProxyType = Constants::WWW_PROXY_DISABLED;
}

BuildEngine::~BuildEngine() = default;

QUrl BuildEngine::uri() const
{
    return d_func()->virtualMachine->uri();
}

QString BuildEngine::name() const
{
    return d_func()->virtualMachine->name();
}

VirtualMachine *BuildEngine::virtualMachine() const
{
    return d_func()->virtualMachine.get();
}

bool BuildEngine::isAutodetected() const
{
    return d_func()->autodetected;
}

Utils::FileName BuildEngine::sharedInstallPath() const
{
    return d_func()->sharedInstallPath;
}

Utils::FileName BuildEngine::sharedHomePath() const
{
    return d_func()->sharedHomePath;
}

Utils::FileName BuildEngine::sharedTargetsPath() const
{
    return d_func()->sharedTargetsPath;
}

Utils::FileName BuildEngine::sharedConfigPath() const
{
    return d_func()->sharedConfigPath;
}

Utils::FileName BuildEngine::sharedSrcPath() const
{
    return d_func()->sharedSrcPath;
}

Utils::FileName BuildEngine::sharedSshPath() const
{
    return d_func()->sharedSshPath;
}

void BuildEngine::setSharedSrcPath(const FileName &sharedSrcPath, const QObject *context,
        const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};

    VirtualMachinePrivate::get(virtualMachine())->setSharedPath(VirtualMachinePrivate::SharedSrc,
            sharedSrcPath, this, [=](bool ok) {
        if (ok)
            d_func()->setSharedSrcPath(sharedSrcPath);
        if (context_)
            functor(ok);
    });
}

quint16 BuildEngine::sshPort() const
{
    return d_func()->virtualMachine->sshParameters().port();
}

void BuildEngine::setSshPort(quint16 sshPort, const QObject *context, const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};
    VirtualMachinePrivate::get(virtualMachine())->setReservedPortForwarding(
            VirtualMachinePrivate::SshPort, sshPort, this, [=](bool ok) {
        Q_D(BuildEngine);
        if (ok) {
            SshConnectionParameters sshParameters = d->virtualMachine->sshParameters();
            sshParameters.setPort(sshPort);
            d->setSshParameters(sshParameters);
        }
        if (context_)
            functor(ok);
    });
}

quint16 BuildEngine::wwwPort() const
{
    return d_func()->wwwPort;
}

void BuildEngine::setWwwPort(quint16 wwwPort, const QObject *context, const Functor<bool> &functor)
{
    QTC_CHECK(virtualMachine()->isLockedDown());

    const QPointer<const QObject> context_{context};
    VirtualMachinePrivate::get(virtualMachine())->setReservedPortForwarding(
            VirtualMachinePrivate::WwwPort, wwwPort, this, [=](bool ok) {
        if (ok)
            d_func()->setWwwPort(wwwPort);
        if (context_)
            functor(ok);
    });
}

QString BuildEngine::wwwProxyType() const
{
    return d_func()->wwwProxyType;
}

QString BuildEngine::wwwProxyServers() const
{
    return d_func()->wwwProxyServers;
}

QString BuildEngine::wwwProxyExcludes() const
{
    return d_func()->wwwProxyExcludes;
}

void BuildEngine::setWwwProxy(const QString &type, const QString &servers, const QString &excludes)
{
    Q_D(BuildEngine);
    // FIXME Introduce an enum for proxy type
    QTC_ASSERT(!type.isEmpty(), return);

    if (type == d->wwwProxyType
            && servers == d->wwwProxyServers
            && excludes == d->wwwProxyExcludes) {
        return;
    }

    d->wwwProxyType = type;
    d->wwwProxyServers = servers;
    d->wwwProxyExcludes = excludes;

    if (d->virtualMachine->state() == VirtualMachine::Connected)
        d->syncWwwProxy();

    emit wwwProxyChanged(type, servers, excludes);
}

QStringList BuildEngine::buildTargetNames() const
{
    return Utils::transform(d_func()->buildTargetsData, [](const BuildTargetData &data) {
        return data.name;
    });
}

QList<BuildTargetData> BuildEngine::buildTargets() const
{
    return d_func()->buildTargetsData;
}

BuildTargetData BuildEngine::buildTarget(const QString &name) const
{
    return Utils::findOrDefault(d_func()->buildTargetsData,
            Utils::equal(&BuildTargetData::name, name));
}

/*!
 * \class BuildEnginePrivate
 */

BuildEnginePrivate::BuildEnginePrivate(BuildEngine *q)
    : q_ptr(q)
{
}

BuildEnginePrivate::~BuildEnginePrivate() = default;

QVariantMap BuildEnginePrivate::toMap() const
{
    QVariantMap data;

    data.insert(Constants::BUILD_ENGINE_VM_URI, virtualMachine->uri());
    data.insert(Constants::BUILD_ENGINE_CREATION_TIME, creationTime);
    data.insert(Constants::BUILD_ENGINE_AUTODETECTED, autodetected);

    data.insert(Constants::BUILD_ENGINE_SHARED_INSTALL, sharedInstallPath.toString());
    data.insert(Constants::BUILD_ENGINE_SHARED_HOME, sharedHomePath.toString());
    data.insert(Constants::BUILD_ENGINE_SHARED_TARGET, sharedTargetsPath.toString());
    data.insert(Constants::BUILD_ENGINE_SHARED_CONFIG, sharedConfigPath.toString());
    data.insert(Constants::BUILD_ENGINE_SHARED_SRC, sharedSrcPath.toString());
    data.insert(Constants::BUILD_ENGINE_SHARED_SSH, sharedSshPath.toString());

    const SshConnectionParameters sshParameters = virtualMachine->sshParameters();
    data.insert(Constants::BUILD_ENGINE_HOST, sshParameters.host());
    data.insert(Constants::BUILD_ENGINE_USER_NAME, sshParameters.userName());
    data.insert(Constants::BUILD_ENGINE_PRIVATE_KEY_FILE, sshParameters.privateKeyFile);
    data.insert(Constants::BUILD_ENGINE_SSH_PORT, sshParameters.port());
    data.insert(Constants::BUILD_ENGINE_SSH_TIMEOUT, sshParameters.timeout);

    data.insert(Constants::BUILD_ENGINE_WWW_PROXY_TYPE, wwwProxyType);
    data.insert(Constants::BUILD_ENGINE_WWW_PROXY_SERVERS, wwwProxyServers);
    data.insert(Constants::BUILD_ENGINE_WWW_PROXY_EXCLUDES, wwwProxyExcludes);

    data.insert(Constants::BUILD_ENGINE_WWW_PORT, wwwPort);
    data.insert(Constants::BUILD_ENGINE_HEADLESS, virtualMachine->isHeadless());

    int count = 0;
    for (const BuildTargetDump &target : buildTargets) {
        const QString key = QString(Constants::BUILD_ENGINE_TARGET_DATA_KEY_PREFIX)
            + QString::number(count);
        QVariantMap targetData = target.toMap();
        QTC_ASSERT(!targetData.isEmpty(), return {});
        data.insert(key, targetData);
        ++count;
    }
    data.insert(Constants::BUILD_ENGINE_TARGETS_COUNT_KEY, count);

    return data;
}

bool BuildEnginePrivate::fromMap(const QVariantMap &data)
{
    Q_Q(BuildEngine);

    const QUrl vmUri = data.value(Constants::BUILD_ENGINE_VM_URI).toUrl();
    QTC_ASSERT(vmUri.isValid(), return false);
    QTC_ASSERT(!virtualMachine || virtualMachine->uri() == vmUri, return false);

    if (!virtualMachine) {
        if (!initVirtualMachine(vmUri))
            return false;
    }

    creationTime = data.value(Constants::BUILD_ENGINE_CREATION_TIME).toDateTime();
    autodetected = data.value(Constants::BUILD_ENGINE_AUTODETECTED).toBool();

    auto toFileName = [](const QVariant &v) { return FileName::fromString(v.toString()); };
    setSharedInstallPath(toFileName(data.value(Constants::BUILD_ENGINE_SHARED_INSTALL)));
    setSharedHomePath(toFileName(data.value(Constants::BUILD_ENGINE_SHARED_HOME)));
    setSharedTargetsPath(toFileName(data.value(Constants::BUILD_ENGINE_SHARED_TARGET)));
    setSharedConfigPath(toFileName(data.value(Constants::BUILD_ENGINE_SHARED_CONFIG)));
    setSharedSrcPath(toFileName(data.value(Constants::BUILD_ENGINE_SHARED_SRC)));
    setSharedSshPath(toFileName(data.value(Constants::BUILD_ENGINE_SHARED_SSH)));

    SshConnectionParameters sshParameters = virtualMachine->sshParameters();
    sshParameters.setHost(data.value(Constants::BUILD_ENGINE_HOST).toString());
    sshParameters.setUserName(data.value(Constants::BUILD_ENGINE_USER_NAME).toString());
    sshParameters.privateKeyFile = data.value(Constants::BUILD_ENGINE_PRIVATE_KEY_FILE).toString();
    sshParameters.setPort(data.value(Constants::BUILD_ENGINE_SSH_PORT).toUInt());
    sshParameters.timeout = data.value(Constants::BUILD_ENGINE_SSH_TIMEOUT).toInt();
    if (sshParameters.timeout == 0)
        sshParameters.timeout = Constants::BUILD_ENGINE_DEFAULT_SSH_TIMEOUT;
    setSshParameters(sshParameters);

    setWwwPort(data.value(Constants::BUILD_ENGINE_WWW_PORT).toUInt());

    q->setWwwProxy(data.value(Constants::BUILD_ENGINE_WWW_PROXY_TYPE,
                Constants::WWW_PROXY_DISABLED).toString(),
            data.value(Constants::BUILD_ENGINE_WWW_PROXY_SERVERS).toString(),
            data.value(Constants::BUILD_ENGINE_WWW_PROXY_EXCLUDES).toString());

    if (virtualMachine->features() & VirtualMachine::OptionalHeadless) {
        const bool headless = data.value(Constants::BUILD_ENGINE_HEADLESS).toBool();
        virtualMachine->setHeadless(headless);
    }

    const int newCount = data.value(Constants::BUILD_ENGINE_TARGETS_COUNT_KEY).toInt();
    QList<BuildTargetDump> newBuildTargets;
    for (int i = 0; i < newCount; ++i) {
        const QString key = QString(Constants::BUILD_ENGINE_TARGET_DATA_KEY_PREFIX)
            + QString::number(i);
        QTC_ASSERT(data.contains(key), return false);
        const QVariantMap targetData = data.value(key).toMap();
        BuildTargetDump target;
        // FIXME handle error?
        target.fromMap(targetData);
        newBuildTargets.append(target);
    }
    updateBuildTargets(newBuildTargets);

    return true;
}

bool BuildEnginePrivate::initVirtualMachine(const QUrl &vmUri)
{
    Q_Q(BuildEngine);
    Q_ASSERT(!virtualMachine);

    virtualMachine = VirtualMachineFactory::create(vmUri);
    QTC_ASSERT(virtualMachine, return false);

    SshConnectionParameters sshParameters;
    sshParameters.setUserName(Constants::BUILD_ENGINE_DEFAULT_USER_NAME);
    sshParameters.setHost(Constants::BUILD_ENGINE_DEFAULT_HOST);
    sshParameters.timeout = Constants::BUILD_ENGINE_DEFAULT_SSH_TIMEOUT;
    sshParameters.hostKeyCheckingMode = SshHostKeyCheckingNone;
    sshParameters.authenticationType = SshConnectionParameters::AuthenticationTypeSpecificKey;
    virtualMachine->setSshParameters(sshParameters);

    QObject::connect(virtualMachine.get(), &VirtualMachine::stateChanged, q, [=]() {
        if (virtualMachine->state() == VirtualMachine::Connected)
            syncWwwProxy();
    });

    return true;
}

void BuildEnginePrivate::enableUpdates()
{
    Q_Q(BuildEngine);
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);
    QTC_ASSERT(!targetsXmlWatcher, return);

    if (SdkPrivate::enableVmAutoConnectInitially())
        virtualMachine->setAutoConnectEnabled(true);

    updateVmProperties(q, [](bool ok) { Q_UNUSED(ok) });

    targetsXmlWatcher = std::make_unique<FileSystemWatcher>(q);
    targetsXmlWatcher->addFile(targetsXmlFile().toString(), FileSystemWatcher::WatchModifiedDate);
    QObject::connect(targetsXmlWatcher.get(), &FileSystemWatcher::fileChanged,
            [this]() { updateBuildTargets(); });
    updateBuildTargets();
}

void BuildEnginePrivate::updateOnce()
{
    QTC_ASSERT(!SdkPrivate::isVersionedSettingsEnabled(), return);

    if (SdkPrivate::enableVmAutoConnectInitially())
        virtualMachine->setAutoConnectEnabled(true);

    // FIXME Not ideal
    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&BuildEnginePrivate::updateVmProperties), this);
    QTC_CHECK(ok);

    updateBuildTargets();
}

void BuildEnginePrivate::updateVmProperties(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(BuildEngine);

    const QPointer<const QObject> context_{context};

    virtualMachine->refreshConfiguration(q, [=](bool ok) {
        if (!ok) {
            if (context_)
                functor(false);
            return;
        }

        const VirtualMachineInfo info =
            VirtualMachinePrivate::get(virtualMachine.get())->cachedInfo();

        setSharedInstallPath(FileName::fromString(info.sharedInstall));
        setSharedHomePath(FileName::fromString(info.sharedHome));
        setSharedTargetsPath(FileName::fromString(info.sharedTargets));
        // FIXME if sharedConfig changes, at least privateKeyFile path needs to be updated
        setSharedConfigPath(FileName::fromString(info.sharedConfig));
        setSharedSrcPath(FileName::fromString(info.sharedSrc));
        setSharedSshPath(FileName::fromString(info.sharedSsh));

        SshConnectionParameters sshParameters = virtualMachine->sshParameters();
        sshParameters.setPort(info.sshPort);
        setSshParameters(sshParameters);

        setWwwPort(info.wwwPort);

        if (context_)
            functor(true);
    });
}

void BuildEnginePrivate::setSharedInstallPath(const Utils::FileName &sharedInstallPath)
{
    if (this->sharedInstallPath == sharedInstallPath)
        return;
    this->sharedInstallPath = sharedInstallPath;
    emit q_func()->sharedInstallPathChanged(sharedInstallPath);
}

void BuildEnginePrivate::setSharedHomePath(const Utils::FileName &sharedHomePath)
{
    if (this->sharedHomePath == sharedHomePath)
        return;
    this->sharedHomePath = sharedHomePath;
    emit q_func()->sharedHomePathChanged(sharedHomePath);
}

void BuildEnginePrivate::setSharedTargetsPath(const Utils::FileName &sharedTargetsPath)
{
    if (this->sharedTargetsPath == sharedTargetsPath)
        return;
    this->sharedTargetsPath = sharedTargetsPath;
    emit q_func()->sharedTargetsPathChanged(sharedTargetsPath);
}

void BuildEnginePrivate::setSharedConfigPath(const Utils::FileName &sharedConfigPath)
{
    if (this->sharedConfigPath == sharedConfigPath)
        return;
    this->sharedConfigPath = sharedConfigPath;
    emit q_func()->sharedConfigPathChanged(sharedConfigPath);

    SshConnectionParameters sshParameters = virtualMachine->sshParameters();
    // FIXME hardcoded
    sshParameters.privateKeyFile = FileName(sharedConfigPath).appendPath("/ssh/private_keys/engine")
        .appendPath(Constants::BUILD_ENGINE_DEFAULT_USER_NAME).toString();
    setSshParameters(sshParameters);
}

void BuildEnginePrivate::setSharedSrcPath(const Utils::FileName &sharedSrcPath)
{
    if (this->sharedSrcPath == sharedSrcPath)
        return;
    this->sharedSrcPath = sharedSrcPath;
    emit q_func()->sharedSrcPathChanged(sharedSrcPath);
}

void BuildEnginePrivate::setSharedSshPath(const Utils::FileName &sharedSshPath)
{
    if (this->sharedSshPath == sharedSshPath)
        return;
    this->sharedSshPath = sharedSshPath;
    emit q_func()->sharedSshPathChanged(sharedSshPath);
}

void BuildEnginePrivate::setSshParameters(const QSsh::SshConnectionParameters &sshParameters)
{
    Q_Q(BuildEngine);

    const SshConnectionParameters old = virtualMachine->sshParameters();
    virtualMachine->setSshParameters(sshParameters);
    if (sshParameters.port() != old.port())
        emit q->sshPortChanged(sshParameters.port());
}

void BuildEnginePrivate::setWwwPort(quint16 wwwPort)
{
    if (this->wwwPort == wwwPort)
        return;
    this->wwwPort = wwwPort;
    emit q_func()->wwwPortChanged(wwwPort);
}

// FIXME This should be only done when the configuration changes, it should be able to block
// certain operations to ensure the build engine is not utilized before the new configuration
// takes effect, and the result should be checked.
void BuildEnginePrivate::syncWwwProxy()
{
    // FIXME
    if (virtualMachine->type() == DockerVirtualMachine::staticType()) {
        qCDebug(engine) << "Not trying to sync WWW proxy on Docker-based build engine";
        return;
    }

    const SshConnectionParameters sshParameters = virtualMachine->sshParameters();

    QStringList args;
    const QString eq("=");
    args.append(Constants::MER_SSH_HOST + eq + sshParameters.host());
    args.append(Constants::MER_SSH_PORT + eq + QString::number(sshParameters.port()));
    args.append(Constants::MER_SSH_PRIVATE_KEY + eq + sshParameters.privateKeyFile);
    args.append(Constants::MER_SSH_SHARED_HOME + eq + sharedHomePath.toString());
    args.append(Constants::MER_SSH_USERNAME + eq + sshParameters.userName());
    args.append("wwwproxy");
    args.append(wwwProxyType);

    if (!wwwProxyServers.isEmpty()
            && (wwwProxyType == Constants::WWW_PROXY_AUTOMATIC
                || wwwProxyType == Constants::WWW_PROXY_MANUAL)) {
        args.append(wwwProxyServers);
    }

    if (!wwwProxyExcludes.isEmpty()
            && wwwProxyType == Constants::WWW_PROXY_MANUAL) {
        args.append(wwwProxyExcludes);
    }

    const QString wrapperBinaryPath = SdkPrivate::libexecPath().appendPath("merssh")
        .appendString(QTC_HOST_EXE_SUFFIX).toString();

    qCDebug(engine) << "About to sync WWW proxy. merssh arguments:" << args;
    QProcess::startDetached(wrapperBinaryPath, args);
}

void BuildEnginePrivate::updateBuildTargets()
{
    Q_Q(BuildEngine);
    const QString targetsXml = targetsXmlFile().toString();
    qCDebug(engine) << "Updating build targets for" << q->uri().toString() << "from" << targetsXml;
    TargetsXmlReader reader(targetsXml);
    QTC_ASSERT(!reader.hasError(), return);
    QTC_ASSERT(reader.version() > 0, return);
    updateBuildTargets(reader.targets());
}

void BuildEnginePrivate::updateBuildTargets(QList<BuildTargetDump> newTargets)
{
    Q_Q(BuildEngine);

    QList<int> toRemove;

    for (int i = 0; i < buildTargets.count(); ++i) {
        if (!newTargets.removeOne(buildTargets.at(i)))
            toRemove.append(i);
    }

    Utils::reverseForeach(toRemove, [=](int i) {
        qCDebug(engine) << "Removing build target" << buildTargets.at(i).name;
        emit q->aboutToRemoveBuildTarget(i);
        if (!SdkPrivate::useSystemSettingsOnly())
            deinitBuildTargetAt(i);
        buildTargets.removeAt(i);
        buildTargetsData.removeAt(i);
    });

    for (const BuildTargetDump &target : qAsConst(newTargets)) {
        qCDebug(engine) << "Adding build target" << target.name;
        buildTargets.append(target);
        buildTargetsData.append(createTargetData(target));
        if (!SdkPrivate::useSystemSettingsOnly())
            initBuildTargetAt(buildTargets.count() - 1);
        emit q->buildTargetAdded(buildTargets.count() - 1);
    }
}

BuildTargetData BuildEnginePrivate::createTargetData(const BuildTargetDump &targetDump) const
{
    BuildTargetData data;

    data.name = targetDump.name;

    data.sysRoot = sysRootForTarget(data.name);
    data.toolsPath = toolsPathForTarget(data.name);

    if (targetDump.gccDumpMachine.contains(Constants::i486_IDENTIFIER))
        data.gdb = FileName::fromLatin1(Constants::DEFAULT_DEBUGGER_i486_FILENAME);
    else if (targetDump.gccDumpMachine.contains(Constants::ARM_IDENTIFIER))
        data.gdb = FileName::fromLatin1(Constants::DEFAULT_DEBUGGER_ARM_FILENAME);

    data.rpmValidationSuites = rpmValidationSuitesFromString(targetDump.rpmValidationSuites);

    return data;
}

QList<RpmValidationSuiteData> BuildEnginePrivate::rpmValidationSuitesFromString(
        const QString &string)
{
    QList<RpmValidationSuiteData> retv;
    QString string_(string);
    QTextStream in(&string_);
    QString line;
    while (in.readLineInto(&line)) {
        RpmValidationSuiteData data;

        QStringList split = line.split(' ', QString::SkipEmptyParts);
        if (split.count() < 3) {
            qCWarning(engine) << "Error parsing listing of RPM validation suites: The corrupted line is:" << line;
            break;
        }

        data.id = split.takeFirst();
        data.essential = split.takeFirst().toLower() == "essential";
        data.website = split.takeFirst();
        if (data.website == "-")
            data.website.clear();
        data.name = split.join(' ');

        retv.append(data);
    }

    return retv;
}

QString BuildEnginePrivate::rpmValidationSuitesToString(const QList<RpmValidationSuiteData> &suites)
{
    QString retv;
    QTextStream out(&retv);
    for (const auto &suite : suites) {
        out << suite.id << ' '
            << (suite.essential ? "Essential " : "Optional ")
            << (suite.website.isEmpty() ? QString("-") : suite.website) << ' '
            << suite.name
            << endl;
    }
    return retv;
}

void BuildEnginePrivate::initBuildTargetAt(int index) const
{
    QTC_ASSERT(!SdkPrivate::useSystemSettingsOnly(), return);

    const BuildTargetDump *dump = &buildTargets[index];
    const BuildTargetData *data = &buildTargetsData[index];

    const FileName toolsPath = toolsPathForTarget(data->name);

    QDir toolsDir(toolsPath.toString());
    if (!toolsDir.exists()) {
        const bool mkpathOk = toolsDir.mkpath(".");
        QTC_ASSERT(mkpathOk, return);
    }

    QString patchedQmakeQuery = dump->qmakeQuery;
    patchedQmakeQuery.replace(":/",
            sysRootForTarget(dump->name).fileName(-1).prepend(':').append('/'));

    QString patchedGccDumpIncludes = dump->gccDumpIncludes;
    patchedGccDumpIncludes.replace(" /",
            sysRootForTarget(dump->name).fileName(-1).prepend(' ').append('/'));

    auto cacheFile = [=](const QString &baseName) {
        return FileName(toolsPath).appendPath(baseName);
    };

    bool ok = true;

    ok &= createCacheFile(cacheFile(Constants::CMAKE_QUERY_CACHE), dump->cmakeQuery);
    ok &= createCacheFile(cacheFile(Constants::QMAKE_QUERY_CACHE), patchedQmakeQuery);
    ok &= createCacheFile(cacheFile(Constants::GCC_DUMP_MACHINE_CACHE), dump->gccDumpMachine);
    ok &= createCacheFile(cacheFile(Constants::GCC_DUMP_MACROS_CACHE), dump->gccDumpMacros);
    ok &= createCacheFile(cacheFile(Constants::GCC_DUMP_INCLUDES_CACHE), patchedGccDumpIncludes);

    QTC_ASSERT(ok, return);

    for (const char *const wrapperName : SIMPLE_WRAPPERS)
        ok &= createSimpleWrapper(dump->name, toolsPath, QString::fromLatin1(wrapperName));

    ok &= createPkgConfigWrapper(toolsPath, sysRootForTarget(dump->name));

    QTC_CHECK(ok);
}

void BuildEnginePrivate::deinitBuildTargetAt(int index) const
{
    QTC_ASSERT(!SdkPrivate::useSystemSettingsOnly(), return);
    QTC_ASSERT(index < buildTargets.count(), return);
    FileUtils::removeRecursively(toolsPathForTarget(buildTargets.at(index).name));
}

bool BuildEnginePrivate::createCacheFile(const FileName &fileName, const QString &data) const
{
    FileSaver saver(fileName.toString(), QIODevice::WriteOnly);
    saver.write(data.toUtf8());
    if (!data.endsWith('\n'))
        saver.write("\n");
    const bool ok = saver.finalize();
    QTC_ASSERT(ok, qCCritical(engine) << saver.errorString(); return false);
    return true;
}

bool BuildEnginePrivate::createSimpleWrapper(const QString &targetName, const FileName &toolsPath,
        const QString &wrapperName) const
{
    const QString commandName = HostOsInfo::isWindowsHost()
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        ? wrapperName.chopped(4) // remove the ".cmd"
#else
        ? [=] { auto chopped = wrapperName; chopped.chop(4); return chopped; }()
#endif
        : wrapperName;

    const QString wrapperBinaryPath = SdkPrivate::libexecPath()
        .appendPath("merssh").appendString(QTC_HOST_EXE_SUFFIX).toString();

    const QString scriptCopyPath = FileName(toolsPath).appendPath(wrapperName).toString();

    QString scriptContent;
    if (HostOsInfo::isWindowsHost()) {
        scriptContent = QStringLiteral(R"(@echo off
SetLocal EnableDelayedExpansion
set ARGUMENTS=
FOR %%a IN (%*) DO set ARGUMENTS=!ARGUMENTS! ^ '%%a'
set {MER_SSH_TARGET_NAME}={targetName}
set {MER_SSH_SDK_TOOLS}={toolsPath}
SetLocal DisableDelayedExpansion
"{wrapperBinaryPath}" {commandName} %ARGUMENTS%
)");
    } else {
        scriptContent = QStringLiteral(R"(#!/bin/sh
ARGUMENTS=""
for ARGUMENT in "$@"; do
    ARGUMENTS="${ARGUMENTS} '${ARGUMENT}'"
done
export {MER_SSH_TARGET_NAME}="{targetName}"
export {MER_SSH_SDK_TOOLS}="{toolsPath}"
exec "{wrapperBinaryPath}" {commandName} ${ARGUMENTS}
)");
    }

    scriptContent
        .replace("{MER_SSH_TARGET_NAME}", Constants::MER_SSH_TARGET_NAME)
        .replace("{MER_SSH_SDK_TOOLS}", Constants::MER_SSH_SDK_TOOLS)
        .replace("{targetName}", targetName)
        .replace("{toolsPath}", QDir::toNativeSeparators(toolsPath.toString()))
        .replace("{wrapperBinaryPath}", QDir::toNativeSeparators(wrapperBinaryPath))
        .replace("{commandName}", commandName);

    bool ok;

    FileSaver saver(scriptCopyPath, QIODevice::WriteOnly);
    saver.write(scriptContent.toUtf8());
    ok = saver.finalize();
    QTC_ASSERT(ok, qCCritical(engine) << saver.errorString(); return false);

    QFileInfo info(scriptCopyPath);
    ok = QFile::setPermissions(scriptCopyPath,
            info.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup);
    QTC_ASSERT(ok, return false);

    return ok;
}

bool BuildEnginePrivate::createPkgConfigWrapper(const FileName &toolsPath, const FileName &sysRoot)
    const
{
    auto nativeSysRooted = [=](const QString &path) {
        return QDir::toNativeSeparators(FileName(sysRoot).appendPath(path).toString());
    };

    const QStringList libDirs = {"/usr/lib/pkgconfig",  "/usr/share/pkgconfig"};
    const QString libDir = Utils::transform(libDirs, nativeSysRooted)
        .join(QDir::listSeparator());

    const QString fileName =
        FileName(toolsPath).appendPath(Constants::WRAPPER_PKG_CONFIG).toString();

    QString scriptContent;
    if (HostOsInfo::isWindowsHost()) {
        const QString real = QDir::toNativeSeparators(SdkPrivate::libexecPath()
                    .appendPath("pkg-config.exe").toString());
        scriptContent = QStringLiteral(R"(@echo off
set PKG_CONFIG_DIR=
set PKG_CONFIG_LIBDIR={libDir}
REM NB, with pkg-config 0.26-1 on Windows it does not work with PKG_CONFIG_SYSROOT_DIR set
set PKG_CONFIG_SYSROOT_DIR=
{real} %*
)");
        scriptContent.replace("{real}", real);
        scriptContent.replace("{libDir}", libDir);
    } else {
        scriptContent = QStringLiteral(R"(#!/bin/sh
export PKG_CONFIG_DIR=
export PKG_CONFIG_LIBDIR="{libDir}"
export PKG_CONFIG_SYSROOT_DIR="{sysRoot}"
# It's useless to say anything here, qmake discards stderr
real=$(which -a pkg-config |sed -n 2p)
exec ${real?} "$@"
)");
        scriptContent.replace("{libDir}", libDir);
        scriptContent.replace("{sysRoot}", sysRoot.toString());
    }

    bool ok;

    FileSaver saver(fileName, QIODevice::WriteOnly);
    saver.write(scriptContent.toUtf8());
    ok = saver.finalize();
    QTC_ASSERT(ok, qCCritical(engine) << saver.errorString(); return false);

    QFileInfo info(fileName);
    ok = QFile::setPermissions(fileName,
            info.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup);
    QTC_ASSERT(ok, return false);

    return true;
}

Utils::FileName BuildEnginePrivate::targetsXmlFile() const
{
    // FIXME
    return FileName(sharedTargetsPath).appendPath("targets.xml");
}

Utils::FileName BuildEnginePrivate::sysRootForTarget(const QString &targetName) const
{
    // FIXME inside MerTarget::finalizeKitCreation FileName::fromUserInput was used in this context
    return FileName(sharedTargetsPath).appendPath(targetName);
}

Utils::FileName BuildEnginePrivate::toolsPathForTarget(const QString &targetName) const
{
    return BuildTargetData::toolsPathCommonPrefix().appendPath(virtualMachine->name())
        .appendPath(targetName);
}

/*!
 * \class BuildTargetDump
 */

bool BuildTargetDump::operator==(const BuildTargetDump &other) const
{
    return name == other.name
        && gccDumpMachine == other.gccDumpMachine
        && gccDumpMacros == other.gccDumpMacros
        && gccDumpIncludes == other.gccDumpIncludes
        && cmakeQuery == other.cmakeQuery
        && qmakeQuery == other.qmakeQuery
        && rpmValidationSuites == other.rpmValidationSuites;
}

QVariantMap BuildTargetDump::toMap() const
{
    QVariantMap data;
    data.insert(Constants::BUILD_TARGET_NAME, name);
    data.insert(Constants::BUILD_TARGET_GCC_DUMP_MACHINE, gccDumpMachine);
    data.insert(Constants::BUILD_TARGET_GCC_DUMP_MACROS, gccDumpMacros);
    data.insert(Constants::BUILD_TARGET_GCC_DUMP_INCLUDES, gccDumpIncludes);
    data.insert(Constants::BUILD_TARGET_CMAKE_QUERY, cmakeQuery);
    data.insert(Constants::BUILD_TARGET_QMAKE_QUERY, qmakeQuery);
    data.insert(Constants::BUILD_TARGET_RPM_VALIDATION_SUITES, rpmValidationSuites);
    return data;
}

void BuildTargetDump::fromMap(const QVariantMap &data)
{
    name = data.value(Constants::BUILD_TARGET_NAME).toString();
    gccDumpMachine = data.value(Constants::BUILD_TARGET_GCC_DUMP_MACHINE).toString();
    gccDumpMacros = data.value(Constants::BUILD_TARGET_GCC_DUMP_MACROS).toString();
    gccDumpIncludes = data.value(Constants::BUILD_TARGET_GCC_DUMP_INCLUDES).toString();
    cmakeQuery = data.value(Constants::BUILD_TARGET_CMAKE_QUERY).toString();
    qmakeQuery = data.value(Constants::BUILD_TARGET_QMAKE_QUERY).toString();
    rpmValidationSuites = data.value(Constants::BUILD_TARGET_RPM_VALIDATION_SUITES).toString();
}

/*!
 * \class BuildEngineManager
 */

BuildEngineManager *BuildEngineManager::s_instance = nullptr;

BuildEngineManager::BuildEngineManager(QObject *parent)
    : QObject(parent)
    , m_userSettings(std::make_unique<UserSettings>(
                Constants::BUILD_ENGINES_SETTINGS_FILE_NAME,
                Constants::BUILD_ENGINES_SETTINGS_DOC_TYPE, this))
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    QHostInfo::lookupHost(QHostInfo::localHostName(), this,
            &BuildEngineManager::completeHostNameLookup);

    if (!SdkPrivate::useSystemSettingsOnly()) {
        // FIXME ugly
        const optional<QVariantMap> userData = m_userSettings->load();
        if (userData)
            fromMap(userData.value());
    }

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        connect(SdkPrivate::instance(), &SdkPrivate::enableUpdatesRequested,
                this, &BuildEngineManager::enableUpdates);
    } else {
        connect(SdkPrivate::instance(), &SdkPrivate::updateOnceRequested,
                this, &BuildEngineManager::updateOnce);
    }

    connect(SdkPrivate::instance(), &SdkPrivate::saveSettingsRequested,
            this, &BuildEngineManager::saveSettings);
}

BuildEngineManager::~BuildEngineManager()
{
    s_instance = nullptr;
}

BuildEngineManager *BuildEngineManager::instance()
{
    return s_instance;
}

QString BuildEngineManager::installDir()
{
    // Not initialized initially. See Sdk for comments.
    QTC_CHECK(!s_instance->m_installDir.isEmpty());
    return s_instance->m_installDir;
}

QString BuildEngineManager::defaultBuildHostName()
{
    if (!s_instance->m_defaultBuildHostName.isNull())
        return s_instance->m_defaultBuildHostName;

    // A non-blocking lookup was initiated during initialization, but it takes
    // longer than anticipated - let's be patient.

    QTime time;
    time.start();

    // TODO singletons vs. const correctness
    s_instance->completeHostNameLookup(QHostInfo::fromName(QHostInfo::localHostName()));

    qCDebug(lib) << "Local host info lookup blocked for" << time.elapsed() << "ms";

    return s_instance->m_defaultBuildHostName;
}

QString BuildEngineManager::effectiveBuildHostName()
{
    return s_instance->m_customBuildHostName.isNull()
        ? defaultBuildHostName()
        : s_instance->m_customBuildHostName;
}

QString BuildEngineManager::customBuildHostName()
{
    return s_instance->m_customBuildHostName;
}

void BuildEngineManager::setCustomBuildHostName(const QString &hostName)
{
    if (s_instance->m_customBuildHostName == hostName)
        return;

    QTC_CHECK(!hostName.isEmpty() || hostName.isNull());
    s_instance->m_customBuildHostName = hostName;
    emit s_instance->customBuildHostNameChanged(s_instance->m_customBuildHostName);
}

QList<BuildEngine *> BuildEngineManager::buildEngines()
{
    return Utils::toRawPointer<QList>(s_instance->m_buildEngines);
}

BuildEngine *BuildEngineManager::buildEngine(const QUrl &uri)
{
    return Utils::findOrDefault(s_instance->m_buildEngines, Utils::equal(&BuildEngine::uri, uri));
}

void BuildEngineManager::createBuildEngine(const QUrl &virtualMachineUri, const QObject *context,
        const Functor<std::unique_ptr<BuildEngine> &&> &functor)
{
    // Needs to be captured by multiple lambdas and their copies
    auto engine = std::make_shared<std::unique_ptr<BuildEngine>>(
            std::make_unique<BuildEngine>(nullptr, BuildEngine::PrivateConstructorTag{}));

    auto engine_d = BuildEnginePrivate::get(engine->get());
    if (!engine_d->initVirtualMachine(virtualMachineUri)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0) // just guessing, not sure when it was fixed
        QTimer::singleShot(0, context, std::bind(functor, nullptr));
#else
        QTimer::singleShot(0, const_cast<QObject *>(context), std::bind(functor, nullptr));
#endif
        return;
    }
    engine_d->updateVmProperties(context, [=](bool ok) {
        QTC_CHECK(ok);
        if (!ok) {
            functor({});
            return;
        }

        functor(std::move(*engine));
    });
}

int BuildEngineManager::addBuildEngine(std::unique_ptr<BuildEngine> &&buildEngine)
{
    QTC_ASSERT(buildEngine, return -1);

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        QTC_ASSERT(SdkPrivate::isUpdatesEnabled(), return -1);
        BuildEnginePrivate::get(buildEngine.get())->enableUpdates();
    } else {
        BuildEnginePrivate::get(buildEngine.get())->updateOnce();
    }

    s_instance->m_buildEngines.emplace_back(std::move(buildEngine));
    const int index = s_instance->m_buildEngines.size() - 1;
    emit s_instance->buildEngineAdded(index);
    return index;
}

void BuildEngineManager::removeBuildEngine(const QUrl &uri)
{
    int index = Utils::indexOf(s_instance->m_buildEngines, Utils::equal(&BuildEngine::uri, uri));
    QTC_ASSERT(index >= 0, return);

    emit s_instance->aboutToRemoveBuildEngine(index);
    s_instance->m_buildEngines.erase(s_instance->m_buildEngines.cbegin() + index);
}

QVariantMap BuildEngineManager::toMap() const
{
    QVariantMap data;
    data.insert(Constants::BUILD_ENGINES_VERSION_KEY, 1);
    data.insert(Constants::BUILD_ENGINES_INSTALL_DIR_KEY, m_installDir);
    data.insert(Constants::BUILD_ENGINES_CUSTOM_BUILD_HOST_NAME_KEY, m_customBuildHostName);

    int count = 0;
    for (const auto &engine : m_buildEngines) {
        const QVariantMap engineData = BuildEnginePrivate::get(engine.get())->toMap();
        QTC_ASSERT(!engineData.isEmpty(), return {});
        data.insert(QString::fromLatin1(Constants::BUILD_ENGINES_DATA_KEY_PREFIX)
                + QString::number(count),
                engineData);
        ++count;
    }
    data.insert(Constants::BUILD_ENGINES_COUNT_KEY, count);

    return data;
}

void BuildEngineManager::fromMap(const QVariantMap &data, bool fromSystemSettings)
{
    const int version = data.value(Constants::BUILD_ENGINES_VERSION_KEY).toInt();
    QTC_ASSERT(version == 1, return);

    m_installDir = data.value(Constants::BUILD_ENGINES_INSTALL_DIR_KEY).toString();
    QTC_ASSERT(!m_installDir.isEmpty(), return);

    if (!fromSystemSettings || m_customBuildHostName.isNull()) {
        setCustomBuildHostName(data.value(Constants::BUILD_ENGINES_CUSTOM_BUILD_HOST_NAME_KEY)
                .toString());
    }

    const int newCount = data.value(Constants::BUILD_ENGINES_COUNT_KEY, 0).toInt();
    QMap<QUrl, QVariantMap> newEnginesData;
    for (int i = 0; i < newCount; ++i) {
        const QString key = QString::fromLatin1(Constants::BUILD_ENGINES_DATA_KEY_PREFIX)
            + QString::number(i);
        QTC_ASSERT(data.contains(key), return);

        const QVariantMap engineData = data.value(key).toMap();
        const QUrl vmUri = engineData.value(Constants::BUILD_ENGINE_VM_URI).toUrl();
        QTC_ASSERT(!vmUri.isEmpty(), return);

        newEnginesData.insert(vmUri, engineData);
    }

    QMap<QUrl, BuildEngine *> existingBuildEngines;
    for (auto it = m_buildEngines.cbegin(); it != m_buildEngines.cend(); ) {
        const bool autodetected = it->get()->isAutodetected();
        const QUrl vmUri = it->get()->virtualMachine()->uri();
        const QDateTime creationTime = BuildEnginePrivate::get(it->get())->creationTime_();
        QTC_CHECK(creationTime.isValid());
        const bool inNewData = newEnginesData.contains(vmUri)
            && newEnginesData.value(vmUri)
                .value(Constants::BUILD_ENGINE_CREATION_TIME).toDateTime() == creationTime;
        if (!inNewData && (!fromSystemSettings || autodetected)) {
            qCDebug(engine) << "Dropping build engine" << vmUri.toString();
            emit aboutToRemoveBuildEngine(it - m_buildEngines.cbegin());
            it = m_buildEngines.erase(it);
        } else if (autodetected && fromSystemSettings) {
            qCDebug(engine) << "Preserving user configuration of build engine" << vmUri.toString();
            QTC_CHECK(inNewData);
            newEnginesData.remove(vmUri);
            ++it;
        } else {
            existingBuildEngines.insert(it->get()->virtualMachine()->uri(), it->get());
            ++it;
        }
    }

    // Update existing/add new engines
    for (const QUrl &vmUri : newEnginesData.keys()) {
        const QVariantMap engineData = newEnginesData.value(vmUri);
        BuildEngine *engine = existingBuildEngines.value(vmUri);
        std::unique_ptr<BuildEngine> newEngine;
        if (!engine) {
            qCDebug(Log::engine) << "Adding build engine" << vmUri.toString();
            newEngine = std::make_unique<BuildEngine>(this, BuildEngine::PrivateConstructorTag{});
            engine = newEngine.get();
        } else {
            qCDebug(Log::engine) << "Updating build engine" << vmUri.toString();
        }

        QTC_ASSERT(!fromSystemSettings || newEngine || engine->isAutodetected(), return);
        const bool ok = BuildEnginePrivate::get(engine)->fromMap(engineData);
        QTC_ASSERT(ok, return);

        if (newEngine) {
            m_buildEngines.emplace_back(std::move(newEngine));
            emit buildEngineAdded(m_buildEngines.size() - 1);
        }
    }
}

void BuildEngineManager::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);

    qCDebug(engine) << "Enabling updates";

    connect(m_userSettings.get(), &UserSettings::updated,
            this, [=](const QVariantMap &data) { fromMap(data); });
    m_userSettings->enableUpdates();

    checkSystemSettings();

    for (const std::unique_ptr<BuildEngine> &engine : m_buildEngines)
        BuildEnginePrivate::get(engine.get())->enableUpdates();
}

void BuildEngineManager::updateOnce()
{
    QTC_ASSERT(!SdkPrivate::isVersionedSettingsEnabled(), return);

    checkSystemSettings();

    for (const std::unique_ptr<BuildEngine> &engine : m_buildEngines)
        BuildEnginePrivate::get(engine.get())->updateOnce();
}

void BuildEngineManager::checkSystemSettings()
{
    qCDebug(engine) << "Checking system-wide configuration file"
        << systemSettingsFile();

    PersistentSettingsReader systemReader;
    if (!systemReader.load(systemSettingsFile())) {
        qCCritical(engine) << "Failed to load system-wide build engine configuration";
        return;
    }

    const QVariantMap systemData = systemReader.restoreValues();

    const bool fromSystemSettings = true;
    fromMap(systemData, fromSystemSettings);
}

void BuildEngineManager::saveSettings(QStringList *errorStrings) const
{
    QString errorString;
    const bool ok = m_userSettings->save(toMap(), &errorString);
    if (!ok)
        errorStrings->append(errorString);
}

void BuildEngineManager::completeHostNameLookup(const QHostInfo &info)
{
    // Sometimes hostname is available while FQDN is not and defaults to the
    // less usefull localhost[.localdomain]
    QStringList candidates{info.hostName(), QHostInfo::localHostName()};
    candidates.removeAll(QString());
    candidates.removeAll(QStringLiteral("localhost"));
    candidates.removeAll(QStringLiteral("localhost.localdomain"));
    candidates.append(QStringLiteral("localhost.localdomain"));
    m_defaultBuildHostName = candidates.first();
    Q_ASSERT(!m_defaultBuildHostName.isEmpty());
}

FileName BuildEngineManager::systemSettingsFile()
{
    return SdkPrivate::settingsFile(SdkPrivate::SystemScope,
            Constants::BUILD_ENGINES_SETTINGS_FILE_NAME);
}

} // namespace Sfdk
