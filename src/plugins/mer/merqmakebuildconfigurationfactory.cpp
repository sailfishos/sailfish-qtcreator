
#include "merqmakebuildconfigurationfactory.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "mersdkkitinformation.h"

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildinfo.h>
#include <qmakeprojectmanager/qmakeproject.h>



using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Mer {
namespace Internal {



int MerQmakeBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    if (QmakeBuildConfigurationFactory::priority(k, projectPath) >= 0
            && MerSdkManager::isMerKit(k))
        return 1;
    return -1;
}



int MerQmakeBuildConfigurationFactory::priority(const Target *parent) const
{
    const auto k = parent->kit();
    if (k && QmakeBuildConfigurationFactory::priority(parent) >= 0
            && MerSdkManager::isMerKit(k))
        return 1;
    return -1;
}



BuildConfiguration *MerQmakeBuildConfigurationFactory::create(Target *parent,
                                                                  const BuildInfo *info) const
{
    auto qmakeInfo = static_cast<const QmakeBuildInfo *>(info);
    auto bc = new MerQmakeBuildConfiguration(parent);
    configureBuildConfiguration(parent, bc, qmakeInfo);

    return bc;
}



BuildConfiguration *MerQmakeBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    auto *oldbc = static_cast<MerQmakeBuildConfiguration *>(source);
    return new MerQmakeBuildConfiguration(parent, oldbc);
}



BuildConfiguration *MerQmakeBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    auto bc = new MerQmakeBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}



MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target):
    QmakeBuildConfiguration(target)
{
    auto project = qobject_cast<QmakeProject *>(target->project());
    if (project)
        connect(project, SIGNAL(proFileUpdated(QmakeProjectManager::QmakeProFile*, bool, bool)),
                this, SLOT(updateEnvironment(QmakeProjectManager::QmakeProFile*, bool, bool)));
}



MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target, MerQmakeBuildConfiguration *source):
    QmakeBuildConfiguration(target, source)
{
}



MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target, Core::Id id):
    QmakeBuildConfiguration(target, id)
{
}



void MerQmakeBuildConfiguration::updateEnvironment(QmakeProjectManager::QmakeProFile *pro, bool validParse, bool parseInProgress)
{
    if (sender() != pro->project() || !validParse || !parseInProgress)
        return;

    disconnect(pro->project(), SIGNAL(proFileUpdated(QmakeProjectManager::QmakeProFile*, bool, bool)),
               this, SLOT(updateEnvironment(QmakeProjectManager::QmakeProFile*, bool, bool)));

    emitEnvironmentChanged();

    connect(pro->project(), SIGNAL(proFileUpdated(QmakeProjectManager::QmakeProFile*, bool, bool)),
            this, SLOT(updateEnvironment(QmakeProjectManager::QmakeProFile*, bool, bool)));
}



QStringList MerQmakeBuildConfiguration::queryBuildEngineVariables(bool *ok) const
{
    QStringList result;
    if (ok) *ok = false;

    if (!target() || !target()->project() || !target()->kit())
        return result;

    const auto kit = target()->kit();
    const auto sdk = MerSdkKitInformation::sdk(kit);
    const auto qtVersion = QtSupport::QtKitInformation::qtVersion(kit);
    if (!sdk || !qtVersion)
        return result;

    const QString sshPort = QString::number(sdk->sshPort());
    const QString sharedHome = QDir::fromNativeSeparators(sdk->sharedHomePath());
    const QString sharedTarget = QDir::fromNativeSeparators(sdk->sharedTargetsPath());
    const QString sharedSrc = QDir::fromNativeSeparators(sdk->sharedSrcPath());
    const auto item = [](const QString &name, const QString &value)->QString {
        Q_ASSERT(!name.contains(QChar::Space));
        return QString("%1=%2").
                arg(name).
                arg(value.contains(QChar::Space) ?
                        "\"" + value + "\"" :
                        value);
    };

    auto sysenv = QProcess::systemEnvironment();
    sysenv << item(Constants::MER_SSH_USERNAME, sdk->userName()) <<
              item(Constants::MER_SSH_PORT, sshPort) <<
              item(Constants::MER_SSH_PRIVATE_KEY, sdk->privateKeyFile()) <<
              item(Constants::MER_SSH_SHARED_HOME, sharedHome) <<
              item(Constants::MER_SSH_SHARED_TARGET, sharedTarget) <<
              item(Constants::MER_SSH_TARGET_NAME, MerSdkManager::targetNameForKit(kit)) <<
              item(Constants::MER_SSH_PROJECT_PATH, target()->project()->projectDirectory().toString()) <<
              item(Constants::MER_SSH_SDK_TOOLS, qtVersion->qmakeCommand().parentDir().toString());
    if (!sharedSrc.isEmpty())
        sysenv << item(Constants::MER_SSH_SHARED_SRC, sharedSrc);


    const QString mersshPath = Core::ICore::libexecPath() + QLatin1String("/merssh") + QStringLiteral(QTC_HOST_EXE_SUFFIX);

    QProcess p;
    p.setEnvironment(sysenv);
    p.start(QDir::toNativeSeparators(mersshPath) + " qmake -env");
    p.waitForFinished();

    if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
        result = QString::fromUtf8(p.readAllStandardOutput()).split("\n\r", QString::SkipEmptyParts);
        if (ok) *ok = true;
    }

    return result;
}



void MerQmakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{    
    bool ok = false;
    const auto vars = queryBuildEngineVariables(&ok);
    if (!ok)
        return;

    foreach (const QString &var, vars) {
        const auto parts = var.split("=");
        if (parts.size() == 2)
            env.appendOrSet(parts.first(), parts.last());
    }
}



} // namespace Internal
} // namespace Mer
