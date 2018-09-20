
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



namespace {

QStringList readFromProcess(QProcess &p) {
    return (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) ?
                QString::fromUtf8(p.readAllStandardOutput()).split("\n\r", QString::SkipEmptyParts) :
                QStringList();
}

}



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
    init();
}



MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target, MerQmakeBuildConfiguration *source):
    QmakeBuildConfiguration(target, source)
{
    init();
}



MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target, Core::Id id):
    QmakeBuildConfiguration(target, id)
{
    init();
}



void MerQmakeBuildConfiguration::init()
{
    const auto kit = target()->kit();
    const auto sdk = MerSdkKitInformation::sdk(kit);
    const auto project = qobject_cast<QmakeProject *>(target()->project());

    Q_ASSERT(kit);
    Q_ASSERT(sdk);
    Q_ASSERT(project);


    const QString mersshPath = Core::ICore::libexecPath() + QLatin1String("/merssh") + QStringLiteral(QTC_HOST_EXE_SUFFIX);

    m_MerSsh.setProgram(QDir::toNativeSeparators(mersshPath));
    m_MerSsh.setArguments({"qmake", "-env"});

    //For the first time query build engine vars synchronously
    m_MerSsh.setEnvironment(merSshEnvironment());
    m_MerSsh.start();
    m_MerSsh.waitForFinished();
    m_BuildEngineVariables = readFromProcess(m_MerSsh);

    //The rest queries are asynchronous
    connect(&m_MerSsh, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            const auto buildEngineVariables = readFromProcess(m_MerSsh);
            if (buildEngineVariables != m_BuildEngineVariables) {
                m_BuildEngineVariables = buildEngineVariables;
                emitEnvironmentChanged();
            }
        }
        m_MerSsh.close();
    });

    connect(&m_MerSsh, &QProcess::errorOccurred, [this]{
        m_MerSsh.close();
    });

    connect(project, &QmakeProject::proFileUpdated,
            [this, project](QmakeProjectManager::QmakeProFile *pro, bool validParse, bool parseInProgress){
        if (project == pro->project() && validParse == true && parseInProgress == true)
            queryBuildEngineVariables();
    });
}



QStringList MerQmakeBuildConfiguration::merSshEnvironment() const
{
    if (!target() || !target()->project() || !target()->kit())
        return QStringList();

    const auto kit = target()->kit();
    const auto sdk = MerSdkKitInformation::sdk(kit);
    const auto qtVersion = QtSupport::QtKitInformation::qtVersion(kit);
    if (!sdk || !qtVersion)
        return QStringList();

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

    return sysenv;
}



void MerQmakeBuildConfiguration::queryBuildEngineVariables()
{
    if (m_MerSsh.state() == QProcess::NotRunning) {
        m_MerSsh.setEnvironment(merSshEnvironment());
        m_MerSsh.start();
    }
}



void MerQmakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{    
    foreach (const QString &var, m_BuildEngineVariables) {
        const auto parts = var.split("=");
        if (parts.size() == 2)
            env.appendOrSet(parts.first(), parts.last());
    }
}



} // namespace Internal
} // namespace Mer
