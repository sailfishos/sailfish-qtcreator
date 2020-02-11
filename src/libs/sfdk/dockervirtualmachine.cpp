/****************************************************************************
**
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
const char RUN[] = "run";
const char PRIVILEGED1[] = "--privileged";
const char PRIVILEGED2[] = "--cap-drop=NET_ADMIN";
const char STOP[] = "stop";
const char COMMIT[] = "commit";
const char CONTAINER_LIST[] = "ps";
const char DETACHED[] = "-d";
const char PUBLISH_PORT[] = "-p";
const char VOLUME[] = "-v";
const char CGROUP_MOUNT[] = "/sys/fs/cgroup:/sys/fs/cgroup:ro";
const char TMPFS[] = "--tmpfs";
const char TMPFS1[] = "/run:exec,mode=755";
const char TMPFS2[] = "/run/lock";
const char TMPFS3[] = "/tmp:exec";
const char FILTER_PREFIX[] = "--filter=ancestor=";
const char FORMAT_CONTAINER_LIST[] = "--format={{.Image}} {{.ID}}";
const char COMMAND_INSPECT[] = "inspect";
const char FORMAT_LABEL_LIST[] = "--format={{json .Config.Labels}}";
const char IMAGE[] = "image";
const char LIST[] = "ls";
const char FORMAT_REPOSITORY[] = "--format={{.Repository}}";
const char UNNAMED_IMAGE[] = "<none>";
const char COMMAND_BUILD[] = "build";
const char DOCKERFILE_PREFIX[] = "FROM ";
const char LABEL_PREFIX[] = "--label=";
const char TAG[] = "--tag";
const quint16 GUESTSSHPORT = 22;
const quint16 GUESTWWWPORT = 9292;
} // namespace anonymous

namespace {

QString dockerPath()
{
    static QString path;
    if (!path.isNull())
        return path;

    path = Utils::Environment::systemEnvironment().searchInPath(DOCKER).toString();

    QTC_ASSERT(!path.isEmpty(), return path);
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

private:
    static QString path()
    {
        QString path = dockerPath();

        if (!SdkPrivate::customDockerPath().isEmpty())
            path = SdkPrivate::customDockerPath();

        return path;
    }

    static QProcessEnvironment environment()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

        if (!SdkPrivate::customDockerPath().isEmpty())
            env.insert(SAILFISH_SDK_SYSTEM_DOCKER, dockerPath());

        return env;
    }
};

CommandQueue *commandQueue()
{
    return SdkPrivate::commandQueue();
}

} // namespace anonymous

/*!
 * \class DockerVirtualMachine
 */

DockerVirtualMachine::DockerVirtualMachine(const QString &name, QObject *parent)
    : VirtualMachine(std::make_unique<DockerVirtualMachinePrivate>(this), staticType(), name, parent)
{
    Q_D(DockerVirtualMachine);
    d->setDisplayType(staticDisplayType());
}

DockerVirtualMachine::~DockerVirtualMachine()
{
}

bool DockerVirtualMachine::isAvailable()
{
    return !dockerPath().isEmpty();
}

QString DockerVirtualMachine::staticType()
{
    return QStringLiteral("Docker");
}

QString DockerVirtualMachine::staticDisplayType()
{
    return tr("Docker");
}

void DockerVirtualMachine::fetchRegisteredVirtualMachines(const QObject *context,
        const Functor<const QStringList &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(IMAGE);
    arguments.append(LIST);
    arguments.append(FORMAT_REPOSITORY);

    auto runner = std::make_unique<DockerRunner>(arguments);
    QObject::connect(runner.get(), &DockerRunner::done,
            context, [=, runner = runner.get()](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        QStringList vms = DockerVirtualMachinePrivate::listedImages(QString::fromLocal8Bit(
                runner->process()->readAllStandardOutput()));
        functor(vms, true);
    });
    commandQueue()->enqueue(std::move(runner));
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
    arguments.append(COMMAND_INSPECT);
    arguments.append(q->name());
    arguments.append(FORMAT_LABEL_LIST);

    auto runner = std::make_unique<DockerRunner>(arguments);
    QObject::connect(runner.get(), &DockerRunner::done, context,
            [this, functor, process = runner->process()](bool ok) {
        VirtualMachineInfo info;
        if (ok)
            info = virtualMachineInfoFromOutput(process->readAllStandardOutput().trimmed());

        functor(info, ok);
    });
    commandQueue()->enqueue(std::move(runner));
}

void DockerVirtualMachinePrivate::start(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);
    QTC_ASSERT(cachedInfo().sshPort != 0, return);
    QTC_ASSERT(cachedInfo().wwwPort != 0, return);
    QTC_ASSERT(!cachedInfo().sharedSrc.isEmpty(), return);
    QTC_ASSERT(!cachedInfo().sharedHome.isEmpty(), return);
    QTC_ASSERT(!cachedInfo().sharedTargets.isEmpty(), return);
    QTC_ASSERT(!cachedInfo().sharedConfig.isEmpty(), return);
    QTC_ASSERT(!cachedInfo().sharedSsh.isEmpty(), return);

    QStringList arguments;
    arguments.append(RUN);
    arguments.append(DETACHED);
    arguments.append(PRIVILEGED1);
    arguments.append(PRIVILEGED2);
    arguments.append(VOLUME);
    arguments.append(CGROUP_MOUNT);

    auto addTmpfs = [&arguments](const QString &path) {
        arguments.append(TMPFS);
        arguments.append(path);
    };
    addTmpfs(TMPFS1);
    addTmpfs(TMPFS2);
    addTmpfs(TMPFS3);

    auto forwardPort = [&arguments](quint16 guestPort, quint16 hostPort) {
        arguments.append(PUBLISH_PORT);
        arguments.append(QString::number(hostPort) + ":" + QString::number(guestPort));
    };
    forwardPort(GUESTSSHPORT, cachedInfo().sshPort);
    forwardPort(GUESTWWWPORT, cachedInfo().wwwPort);

    auto sharePath = [&arguments](const QString &guestPath, const QString &hostPath) {
        arguments.append(VOLUME);
        arguments.append(hostPath + ":" + guestPath);
    };
    sharePath(Constants::BUILD_ENGINE_SHARED_HOME_MOUNT_POINT, cachedInfo().sharedHome);
    sharePath(Constants::BUILD_ENGINE_SHARED_SRC_MOUNT_POINT, cachedInfo().sharedSrc);
    sharePath(Constants::BUILD_ENGINE_SHARED_TARGET_MOUNT_POINT, cachedInfo().sharedTargets);
    sharePath(Constants::BUILD_ENGINE_SHARED_CONFIG_MOUNT_POINT, cachedInfo().sharedConfig);
    sharePath(Constants::BUILD_ENGINE_SHARED_SSH_MOUNT_POINT, cachedInfo().sharedSsh);

    arguments.append(q->name());

    auto runner = std::make_unique<DockerRunner>(arguments);
    QObject::connect(runner.get(), &DockerRunner::done, q,
            [this, context, functor, process = runner->process()](bool ok) {
        if (ok)
            containerId = QString::fromLocal8Bit(process->readAllStandardOutput().trimmed());
        if (context)
            functor(ok);
    });
    commandQueue()->enqueue(std::move(runner));
}

void DockerVirtualMachinePrivate::stop(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    const QPointer<const QObject> context_{context};

    auto enqueue = [=](const QStringList &arguments, CommandQueue::BatchId batch) {
        auto runner = std::make_unique<DockerRunner>(arguments);
        QObject::connect(runner.get(), &DockerRunner::failure, Sdk::instance(), [=]() {
            commandQueue()->cancelBatch(batch);
            callIf(context_, functor, false);
        });
        DockerRunner *runner_ = runner.get();
        commandQueue()->enqueue(std::move(runner));
        return runner_;
    };

    const CommandQueue::BatchId batch = commandQueue()->beginBatch();

    QStringList stopArguments;
    stopArguments.append(STOP);
    stopArguments.append(containerId);

    enqueue(stopArguments, batch);

    QStringList commitArguments;
    commitArguments.append(COMMIT);
    commitArguments.append(containerId);
    commitArguments.append(q->name());

    enqueue(commitArguments, batch);

    commandQueue()->enqueueCheckPoint(context, [=]() { functor(true); });
    commandQueue()->endBatch();
}

void DockerVirtualMachinePrivate::probe(const QObject *context,
        const Functor<BasicState, bool> &functor) const
{
    Q_Q(const DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    const QPointer<const QObject> context_{context};

    q->fetchRegisteredVirtualMachines(q, [=](const QStringList &registeredVms, bool ok) {
        if (!ok) {
            if (context_)
                functor({}, false);
            return;
        }

        if (!registeredVms.contains(q->name())) {
            if (context_)
                functor({}, true);
            return;
        }

        QStringList arguments;
        arguments.append(CONTAINER_LIST);
        arguments.append(QString(FILTER_PREFIX) + q->name());
        arguments.append(FORMAT_CONTAINER_LIST);

        auto runner = std::make_unique<DockerRunner>(arguments);
        QObject::connect(runner->process(),
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                q,
                [=, runner = runner.get()](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                if (context_)
                    functor({}, false);
                return;
            }

            VirtualMachinePrivate::BasicState state = VirtualMachinePrivate::Existing;
            const QStringList runningContainers =
                QString::fromLocal8Bit(runner->process()->readAllStandardOutput())
                    .split('\n', QString::SkipEmptyParts);
            for (const QString& runningContainer : runningContainers) {
                QStringList nameAndId = runningContainer.split(' ', QString::SkipEmptyParts);
                QTC_ASSERT(nameAndId.count() == 2 && nameAndId.first() == q->name(), continue);
                state |= VirtualMachinePrivate::Running | VirtualMachinePrivate::Headless;
                if (containerId.isEmpty())
                    containerId = nameAndId.last();
            }
            if (context_)
                functor(state, true);
        });
        commandQueue()->enqueue(std::move(runner));
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

void DockerVirtualMachinePrivate::doSetSharedPath(SharedPath which, const FileName &path,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    buildWithLabel(labelFor(which), path.toString(), context, functor);
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

    buildWithLabel(labelFor(which), QString::number(port), context, functor);
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

QStringList DockerVirtualMachinePrivate::listedImages(const QString &output)
{
    QStringList images;
    const QStringList lines = output.split(QRegularExpression("[\r\n]"), QString::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line != UNNAMED_IMAGE)
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
    parsePort(VirtualMachinePrivate::WwwPort, &info.wwwPort);

    auto parsePath = [=](VirtualMachinePrivate::SharedPath which, QString *path) {
        const QString key = labelFor(which);
        QTC_ASSERT(labels.contains(key), return);
        *path = labels.value(key).toString();
    };

    parsePath(VirtualMachinePrivate::SharedSrc, &info.sharedSrc);
    parsePath(VirtualMachinePrivate::SharedSsh, &info.sharedSsh);
    parsePath(VirtualMachinePrivate::SharedHome, &info.sharedHome);
    parsePath(VirtualMachinePrivate::SharedConfig, &info.sharedConfig);
    parsePath(VirtualMachinePrivate::SharedTargets, &info.sharedTargets);

    return info;
}

void DockerVirtualMachinePrivate::buildWithLabel(const QString& key, const QString& value,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(DockerVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(COMMAND_BUILD);
    arguments.append(LABEL_PREFIX + key + "=" + value);
    arguments.append(TAG);
    arguments.append(q->name());
    arguments.append("-");

    auto runner = std::make_unique<DockerRunner>(arguments);

    QObject::connect(runner->process(), &QProcess::started, q, [q, process = runner->process()]() {
        QString dockerFile(DOCKERFILE_PREFIX + q->name());
        process->write(dockerFile.toUtf8());
        process->closeWriteChannel();
    });

    QObject::connect(runner.get(), &DockerRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

QString DockerVirtualMachinePrivate::labelFor(VirtualMachinePrivate::SharedPath which)
{
    switch (which) {
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
    }
    Q_ASSERT(false);
    return {};
}

QString DockerVirtualMachinePrivate::labelFor(VirtualMachinePrivate::ReservedPort which)
{
    switch (which) {
    case SshPort:
        return {Constants::BUILD_ENGINE_SSH_PORT};
    case WwwPort:
        return {Constants::BUILD_ENGINE_WWW_PORT};
    }
    Q_ASSERT(false);
    return {};
}

} // namespace Sfdk

#include "dockervirtualmachine.moc"
