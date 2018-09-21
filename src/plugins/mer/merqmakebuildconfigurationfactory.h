
#pragma once

#include <QProcess>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>

namespace QmakeProjectManager {
class QmakeProFile;
}

namespace Mer {
namespace Internal {

class MerQmakeBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{
public:
    explicit MerQmakeBuildConfigurationFactory();

    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const Q_DECL_OVERRIDE;
    int priority(const ProjectExplorer::Target *parent) const Q_DECL_OVERRIDE;
};



class MerQmakeBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
    Q_OBJECT
    friend class MerQmakeBuildConfigurationFactory;

public:
    explicit MerQmakeBuildConfiguration(ProjectExplorer::Target *target);    
    void addToEnvironment(Utils::Environment &env) const Q_DECL_OVERRIDE;

private slots:
    void onMerSshFinished();

private:
    QProcess m_MerSsh;
    QStringList m_BuildEngineVariables;

    void queryBuildEngineVariables();    
    QStringList merSshEnvironment() const;
};

} // namespace Internal
} // namespace Mer
