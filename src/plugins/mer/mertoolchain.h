/****************************************************************************
**
** Copyright (C) 2012-2016,2019 Jolla Ltd.
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

#ifndef MERTOOLCHAIN_H
#define MERTOOLCHAIN_H

#include "merconstants.h"

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/headerpath.h>

namespace Mer {
namespace Internal {

class MerToolChain : public ProjectExplorer::GccToolChain
{
public:
    MerToolChain(Utils::Id typeId = Constants::MER_TOOLCHAIN_ID);

    void setBuildEngineUri(const QUrl &uri);
    QUrl buildEngineUri() const;
    void setBuildTargetName(const QString &name);
    QString buildTargetName() const;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;

    QList<ProjectExplorer::OutputTaskParser *> createOutputParsers() const override;
    QStringList suggestedMkspecList() const override;
    ProjectExplorer::Tasks validateKit(const ProjectExplorer::Kit *kit) const override;

    bool isJobCountSupported() const override { return false; }

    void addToEnvironment(Utils::Environment &env) const override;
private:
    QUrl m_buildEngineUri;
    QString m_buildTargetName;
    mutable ProjectExplorer::HeaderPaths m_headerPathsOnHost;
};

class MerToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    MerToolChainFactory();

    QList<ProjectExplorer::ToolChain *> autoDetect(const QList<ProjectExplorer::ToolChain *> &alreadyKnown) override;
};

} // Internal
} // Mer

#endif // MERTOOLCHAIN_H
