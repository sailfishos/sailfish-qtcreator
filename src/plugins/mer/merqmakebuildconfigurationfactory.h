
#pragma once

#include <qmakeprojectmanager/qmakebuildconfiguration.h>

namespace Mer {
namespace Internal {

class MerQmakeBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{
public:
    explicit MerQmakeBuildConfigurationFactory(QObject *parent = 0)
        : QmakeBuildConfigurationFactory(parent)
    { }

    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    int priority(const ProjectExplorer::Target *parent) const override;

    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
};



class MerQmakeBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
    friend class MerQmakeBuildConfigurationFactory;    
public:
    explicit MerQmakeBuildConfiguration(ProjectExplorer::Target *target);
    MerQmakeBuildConfiguration(ProjectExplorer::Target *target, MerQmakeBuildConfiguration *source);
    MerQmakeBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    void addToEnvironment(Utils::Environment &env) const override;
};

} // namespace Internal
} // namespace Mer
