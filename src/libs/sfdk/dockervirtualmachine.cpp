/****************************************************************************
**
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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
** conditions contained in a signed written agreement between you and
** The Qt Company.
**
****************************************************************************/

#include "dockervirtualmachine_p.h"

#include "asynchronous_p.h"
#include "sdk_p.h"
#include "sfdkconstants.h"
#include "usersettings_p.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>

using namespace Utils;

namespace Sfdk {

namespace {
const char DOCKER[] = "docker";
const char SAILFISH_SDK_SYSTEM_DOCKER[] = "SAILFISH_SDK_SYSTEM_DOCKER";
const quint16 GUESTSSHPORT = 22;
const quint16 GUESTDBUSPORT = 777;
const char SAILFISH_SDK_DOCKER_RUN_PRIVILEGED[] = "SAILFISH_SDK_DOCKER_RUN_PRIVILEGED";
const char SAILFISH_SDK_DOCKER_CAP_ADD[] = "SAILFISH_SDK_DOCKER_CAP_ADD";
} // namespace anonymous

namespace {

QString dockerPath()
{
    static QString path;
    if (!path.isNull())
        return path;

    path = Utils::Environment::systemEnvironment().searchInPath(DOCKER).toString();

    return path;
}

class DockerRunner : public ProcessRunner
{
    Q_OBJECT

public:
    explicit DockerRunner(const QStringList &arguments, QObject *parent = 0)
        : ProcessRunner(path(), arguments, parent)
    {
        process()->setProcessEnvironment(environment());
    }

    static QString path()
    {
        static QString path;
        if (!path.isNull())
            return path;

        path = dockerPath();

        if (!SdkPrivate::customDockerPath().isEmpty())
            path = SdkPrivate::customDockerPath();

        if (!path.isEmpty())
            qCDebug(vms) << "Using docker tool at" << path;
        else
            qCDebug(vms) << "docker tool not found";

        return path;
    }

private:
    static QProcessEnvironment environment()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

        if (!SdkPrivate::customDockerPath().isEmpty())
            env.insert(SAILFISH_SDK_SYSTEM_DOCKER, dockerPath());

        return env;
    }
};

} // namespace anonymous

/*!
 * \class DockerVirtualMachine
 */

DockerVirtualMachine::DockerVirtualMachine(const QString &name, VirtualMachine::Features featureMask,
        std::unique_ptr<ConnectionUi> &&connectionUi, QObject *parent)
    : VirtualMachine(std::make_unique<DockerVirtualMachinePrivate>(this), staticType(),
            staticFeatures() & featureMask, name, std::move(connectionUi), parent)
{
    Q_D(DockerVirtualMachine);
    d->setDisplayType(staticDisplayType());
}

DockerVirtualMachine::~DockerVirtualMachine()
{
}

bool DockerVirtualMachine::isAvailable()
{
    return !DockerRunner::path().isEmpty();
}

QString DockerVirtualMachine::staticType()
{
    return QStringLiteral("Docker");
}

QString DockerVirtualMachine::staticDisplayType()
{
    return tr("Docker");
}

VirtualMachine::Features DockerVirtualMachine::staticFeatures()
{
    return VirtualMachine::NoFeatures;
}

void DockerVirtualMachine::fetchRegisteredVirtualMachines(const QObject *context,
        const Functor<const QStringList &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append("image");
    arguments.append("ls");
    arguments.append("--format={{.Repository}}");

    auto *runner = BatchComposer::enqueue<DockerRunner>(arguments);
    connect(runner, &DockerRunner::done, context, [=](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        QStringList vms = DockerVirtualMachinePrivate::listedImages(QString::fromLocal8Bit(
                runner->process()->readAllStandardOutput()));
        functor(vms, true);
    });
}

/*!
 * \class DockerVirtualMachinePrivate
 * \internal
 */

void DockerVirtualMachinePrivate::fetchInfo(VirtualMachineInfo::ExtraInfos extraInfo,
        const QObject *context, const Functor<const VirtualMachineInfo &, bool> &functor) const
{
    Q_Q(const DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    // TODO: fetch snapshot info if requested
    Q_UNUSED(extraInfo)

    QStringList arguments;
    arguments.append("image");
    arguments.append("inspect");
    arguments.append(q->name());
    arguments.append("--format={{json .Config.Labels}}");

    auto *runner = BatchComposer::enqueue<DockerRunner>(arguments);
    connect(runner, &DockerRunner::done, context, [=](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        const VirtualMachineInfo info = virtualMachineInfoFromOutput(
                runner->process()->readAllStandardOutput().trimmed());
        functor(info, true);
    });
}

void DockerVirtualMachinePrivate::start(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    BatchComposer composer = BatchComposer::createBatch("DockerVirtualMachinePrivate::start");
    composer.batch()->setPropagateFailure(true);
    connect(composer.batch(), &CommandRunner::done, context, functor);

    {
        BatchComposer composer = BatchComposer::createBatch("ensureContainerExists");
        composer.batch()->setPropagateFailure(true);

        QStringList lsArguments;
        lsArguments.append("container");
        lsArguments.append("ls");
        lsArguments.append("--all");
        lsArguments.append("--filter=name=" + q->name());
        lsArguments.append("--format={{.Image}}");

        auto *lsRunner = BatchComposer::enqueue<DockerRunner>(lsArguments);
        connect(lsRunner, &DockerRunner::success, context, [=, batch = composer.batch()]() {
            const QString out = QString::fromLocal8Bit(lsRunner->process()->readAllStandardOutput())
                .trimmed();
            bool containerExists = !out.isEmpty();
            const QString imageName = out;

            if (!containerExists) {
                BatchComposer composer = BatchComposer::extendBatch(batch);
                BatchComposer::enqueue<DockerRunner>(makeCreateArguments());
            } else if (imageName != q->name()) {
                BatchComposer composer = BatchComposer::extendBatch(batch);
                // Recreate the container if it does not use the desired (latest) image.
                // This may happen e.g. when stop() fails to 'create' after 'commit'.
                QStringList rmArguments;
                rmArguments.append("container");
                rmArguments.append("rm");
                rmArguments.append(q->name());

                BatchComposer::enqueue<DockerRunner>(rmArguments);
                BatchComposer::enqueue<DockerRunner>(makeCreateArguments());
            }
        });
    }

    QStringList startArguments;
    startArguments.append("container");
    startArguments.append("start");
    startArguments.append(q->name());

    BatchComposer::enqueue<DockerRunner>(startArguments);
}

void DockerVirtualMachinePrivate::stop(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList stopArguments;
    stopArguments.append("stop");
    stopArguments.append(q->name());

    auto *runner = BatchComposer::enqueue<DockerRunner>(stopArguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void DockerVirtualMachinePrivate::commit(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    BatchComposer composer = BatchComposer::createBatch("DockerVirtualMachinePrivate::commit");
    composer.batch()->setPropagateFailure(true);
    connect(composer.batch(), &CommandRunner::done, context, functor);

    QStringList commitArguments;
    commitArguments.append("commit");
    commitArguments.append(q->name());
    commitArguments.append(q->name());

    BatchComposer::enqueue<DockerRunner>(commitArguments);

    QStringList rmArguments;
    rmArguments.append("container");
    rmArguments.append("rm");
    rmArguments.append(q->name());

    BatchComposer::enqueue<DockerRunner>(rmArguments);

    BatchComposer::enqueue<DockerRunner>(makeCreateArguments());
}

void DockerVirtualMachinePrivate::probe(const QObject *context,
        const Functor<BasicState, bool> &functor) const
{
    Q_Q(const DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    const QPointer<const QObject> context_{context};

    BatchComposer composer = BatchComposer::createBatch("DockerVirtualMachinePrivate::probe");
    composer.batch()->setPropagateFailure(true);
    connect(composer.batch(), &CommandRunner::failure,
            context, std::bind(functor, BasicState{}, false));

    q->fetchRegisteredVirtualMachines(context,
            [=, batch = composer.batch()](const QStringList &registeredVms, bool ok) {
        auto state = std::make_shared<VirtualMachinePrivate::BasicState>();

        if (!ok)
            return;

        if (!registeredVms.contains(q->name())) {
            functor(*state, true);
            return;
        }

        *state |= VirtualMachinePrivate::Existing;

        BatchComposer composer = BatchComposer::extendBatch(batch);

        QStringList arguments;
        arguments.append("container");
        arguments.append("ls");
        arguments.append("--filter=name=" + q->name());
        arguments.append("--quiet");

        auto *runner = BatchComposer::enqueue<DockerRunner>(arguments);
        connect(runner, &DockerRunner::success, context, [=]() {
            if (!runner->process()->readAllStandardOutput().isEmpty())
                *state |= VirtualMachinePrivate::Running | VirtualMachinePrivate::Headless;

            functor(*state, true);
        });
    });
}

void DockerVirtualMachinePrivate::setVideoMode(const QSize &size, int depth,
        const QString &deviceModelName, Qt::Orientation orientation, int scaleDownFactor,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(size)
    Q_UNUSED(depth)
    Q_UNUSED(deviceModelName)
    Q_UNUSED(orientation)
    Q_UNUSED(scaleDownFactor)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doSetMemorySizeMb(int memorySizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(memorySizeMb)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doSetSwapSizeMb(int swapSizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(swapSizeMb)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doSetCpuCount(int cpuCount, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(cpuCount)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doSetStorageSizeMb(int storageSizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(storageSizeMb)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doSetSharedPath(SharedPath which, const FilePath &path,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    rebuildWithLabel(labelFor(which), path.toString(), context, functor);
}

void DockerVirtualMachinePrivate::doAddPortForwarding(const QString &ruleName,
        const QString &protocol, quint16 hostPort, quint16 emulatorVmPort,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(ruleName)
    Q_UNUSED(protocol)
    Q_UNUSED(hostPort)
    Q_UNUSED(emulatorVmPort)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doRemovePortForwarding(const QString &ruleName,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(ruleName)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doSetReservedPortForwarding(ReservedPort which, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    rebuildWithLabel(labelFor(which), QString::number(port), context, functor);
}

void DockerVirtualMachinePrivate::doSetReservedPortListForwarding(ReservedPortList which,
        const QList<Utils::Port> &ports, const QObject *context,
        const Functor<const QMap<QString, quint16> &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(which)
    Q_UNUSED(ports)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doTakeSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(snapshotName)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doRestoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(snapshotName)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

void DockerVirtualMachinePrivate::doRemoveSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    Q_UNUSED(snapshotName)
    Q_UNUSED(context)
    Q_UNUSED(functor)
    QTC_CHECK(false);
}

QStringList DockerVirtualMachinePrivate::listedImages(const QString &output)
{
    QStringList images;
    const QStringList lines = output.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line != "<none>")
            images.append(line);
    }
    return images;
}

VirtualMachineInfo DockerVirtualMachinePrivate::virtualMachineInfoFromOutput(const QByteArray &output)
{
    VirtualMachineInfo info;

    QJsonDocument document = QJsonDocument::fromJson(output);
    QTC_ASSERT(!document.isNull(), return info);

    info.memorySizeMb = VirtualMachine::availableMemorySizeMb();
    info.cpuCount = VirtualMachine::availableCpuCount();

    QJsonObject labels = document.object();

    auto parsePort = [=](VirtualMachinePrivate::ReservedPort which, quint16 *port) {
        const QString key = labelFor(which);
        QTC_ASSERT(labels.contains(key), return);
        *port = labels.value(key).toString().toUShort();
    };

    parsePort(VirtualMachinePrivate::SshPort, &info.sshPort);
    parsePort(VirtualMachinePrivate::DBusPort, &info.dBusPort);

    auto parsePath = [=](VirtualMachinePrivate::SharedPath which, QString *path) {
        const QString key = labelFor(which);
        QTC_ASSERT(labels.contains(key), return);
        *path = labels.value(key).toString();
    };

    parsePath(VirtualMachinePrivate::SharedInstall, &info.sharedInstall);
    parsePath(VirtualMachinePrivate::SharedSrc, &info.sharedSrc);
    parsePath(VirtualMachinePrivate::SharedSsh, &info.sharedSsh);
    parsePath(VirtualMachinePrivate::SharedHome, &info.sharedHome);
    parsePath(VirtualMachinePrivate::SharedConfig, &info.sharedConfig);
    parsePath(VirtualMachinePrivate::SharedTargets, &info.sharedTargets);

    return info;
}

QStringList DockerVirtualMachinePrivate::makeCreateArguments() const
{
    Q_Q(const DockerVirtualMachine);
    QTC_ASSERT(cachedInfo().sshPort != 0, return {});
    QTC_ASSERT(cachedInfo().dBusPort != 0, return {});
    QTC_ASSERT(!cachedInfo().sharedInstall.isEmpty(), return {});
    QTC_ASSERT(!cachedInfo().sharedSrc.isEmpty(), return {});
    QTC_ASSERT(!cachedInfo().sharedHome.isEmpty(), return {});
    QTC_ASSERT(!cachedInfo().sharedTargets.isEmpty(), return {});
    QTC_ASSERT(!cachedInfo().sharedConfig.isEmpty(), return {});
    QTC_ASSERT(!cachedInfo().sharedSsh.isEmpty(), return {});

    const FilePath seccompProfile = FilePath::fromString(Sdk::installationPath())
        .stringAppended(Constants::BUILD_ENGINE_HOST_SECCOMP_PATH_POSTFIX);

    QStringList arguments;
    arguments.append("create");
    arguments.append("--env=container=docker");
    arguments.append("--stop-signal=RTMIN+3");
    arguments.append("--cap-add=NET_ADMIN");
    arguments.append("--security-opt=seccomp=" + (seccompProfile.exists() ?
                seccompProfile.toUserOutput()
                : QString("unconfined")));
    arguments.append("--volume");
    arguments.append("/sys/fs/cgroup:/sys/fs/cgroup:ro");
    arguments.append("--network");
    arguments.append("sailfish-sdk");

#ifdef Q_OS_WIN
    arguments.append("--cap-add=SYS_ADMIN");
    arguments.append("--device=/dev/fuse");
#endif

    if (qEnvironmentVariableIntValue(SAILFISH_SDK_DOCKER_RUN_PRIVILEGED))
        arguments.append("--privileged");
    for (const QString &capability : qEnvironmentVariable(SAILFISH_SDK_DOCKER_CAP_ADD)
            .split(',', Qt::SkipEmptyParts)) {
        arguments.append("--cap-add=" + capability);
    }

    auto addTmpfs = [&arguments](const QString &path) {
        arguments.append("--tmpfs");
        arguments.append(path);
    };
    addTmpfs("/run:exec,mode=755");
    addTmpfs("/run/lock");
    addTmpfs("/tmp:exec");

    auto forwardPort = [&arguments](quint16 guestPort, quint16 hostPort) {
        arguments.append("--publish");
        arguments.append(QString::number(hostPort) + ":" + QString::number(guestPort));
    };
    forwardPort(GUESTSSHPORT, cachedInfo().sshPort);
    forwardPort(GUESTDBUSPORT, cachedInfo().dBusPort);

    auto sharePath = [&arguments](const QString &guestPath, const QString &hostPath,
            const QString &alignedMountPointName = {}) {
        arguments.append("--volume");
        arguments.append(hostPath + ":" + guestPath);

        if (!alignedMountPointName.isEmpty()) {
            const QString envName = QString::fromLatin1(Constants::BUILD_ENGINE_ALIGNED_MOUNT_POINT_ENV_TEMPLATE)
                .arg(alignedMountPointName.toUpper());
            arguments.append("--env");
            arguments.append(envName + "=" + guestPath);
        }
    };
    sharePath(Constants::BUILD_ENGINE_SHARED_INSTALL_MOUNT_POINT, cachedInfo().sharedInstall);
    sharePath(Constants::BUILD_ENGINE_SHARED_HOME_MOUNT_POINT, cachedInfo().sharedHome);
    sharePath(alignedMountPointFor(cachedInfo().sharedSrc), cachedInfo().sharedSrc, "src1");
    sharePath(Constants::BUILD_ENGINE_SHARED_TARGET_MOUNT_POINT, cachedInfo().sharedTargets);
    sharePath(Constants::BUILD_ENGINE_SHARED_CONFIG_MOUNT_POINT, cachedInfo().sharedConfig);
    sharePath(Constants::BUILD_ENGINE_SHARED_SSH_MOUNT_POINT, cachedInfo().sharedSsh);

    arguments.append("--name");
    arguments.append(q->name());

    arguments.append(q->name());
    arguments.append({"/usr/bin/setarch", "i386", "/sbin/init"});

    return arguments;
}

void DockerVirtualMachinePrivate::rebuildWithLabel(const QString& key, const QString& value,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append("build");
    arguments.append("--label=" + key + "=" + value);
    arguments.append("--tag");
    arguments.append(q->name());
    arguments.append("-");

    auto *runner = BatchComposer::enqueue<DockerRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);

    connect(runner->process(), &QProcess::started, q, [q, process = runner->process()]() {
        const QString dockerFile("FROM " + q->name());
        process->write(dockerFile.toUtf8());
        process->closeWriteChannel();
    });
}

QString DockerVirtualMachinePrivate::labelFor(VirtualMachinePrivate::SharedPath which)
{
    switch (which) {
    case SharedInstall:
        return {Constants::BUILD_ENGINE_SHARED_INSTALL};
    case SharedSrc:
        return {Constants::BUILD_ENGINE_SHARED_SRC};
    case SharedHome:
        return {Constants::BUILD_ENGINE_SHARED_HOME};
    case SharedConfig:
        return {Constants::BUILD_ENGINE_SHARED_CONFIG};
    case SharedSsh:
        return {Constants::BUILD_ENGINE_SHARED_SSH};
    case SharedTargets:
        return {Constants::BUILD_ENGINE_SHARED_TARGET};
    case SharedMedia:
        return {Constants::EMULATOR_SHARED_MEDIA};
    }
    Q_ASSERT(false);
    return {};
}

QString DockerVirtualMachinePrivate::labelFor(VirtualMachinePrivate::ReservedPort which)
{
    switch (which) {
    case SshPort:
        return {Constants::BUILD_ENGINE_SSH_PORT};
    case DBusPort:
        return {Constants::BUILD_ENGINE_DBUS_PORT};
    }
    Q_ASSERT(false);
    return {};
}

} // namespace Sfdk

#include "dockervirtualmachine.moc"
