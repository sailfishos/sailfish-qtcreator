
#include "merqmakebuildconfigurationfactory.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "mersdkkitinformation.h"

#include <coreplugin/icore.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>



namespace {

bool isFinished(const QProcess &p) {
    return (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
}

QStringList readFrom(QProcess &p) {
    return QString::fromUtf8(p.readAllStandardOutput()).split("\n\r", QString::SkipEmptyParts);
}

}



using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Mer {
namespace Internal {



MerQmakeBuildConfigurationFactory::MerQmakeBuildConfigurationFactory():
    QmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<MerQmakeBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    setSupportedTargetDeviceTypes({Mer::Constants::MER_DEVICE_TYPE});
    //setBasePriority(1);
}



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



MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target):
    QmakeBuildConfiguration(target)
{
    const auto kit = target->kit();
    const auto sdk = MerSdkKitInformation::sdk(kit);
    const auto project = qobject_cast<QmakeProject *>(target->project());

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
    if (isFinished(m_MerSsh))
        m_BuildEngineVariables = readFrom(m_MerSsh);

    //The rest queries are asynchronous
    connect(&m_MerSsh, SIGNAL(finished(int)), SLOT(onMerSshFinished()));
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



void MerQmakeBuildConfiguration::onMerSshFinished()
{
    if (isFinished(m_MerSsh)) {
        const auto buildEngineVariables = readFrom(m_MerSsh);
        if (buildEngineVariables != m_BuildEngineVariables) {
            m_BuildEngineVariables = buildEngineVariables;
            updateCacheAndEmitEnvironmentChanged();
        }
    }
    m_MerSsh.close();
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
