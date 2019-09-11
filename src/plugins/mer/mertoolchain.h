/****************************************************************************
**
** Copyright (C) 2012-2016,2019 Jolla Ltd.
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
    MerToolChain(Detection autodetect, Core::Id typeId = Constants::MER_TOOLCHAIN_ID);

    void setBuildEngineName(const QString &name);
    QString buildEngineName() const;
    void setTargetName(const QString &name);
    QString targetName() const;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    QString makeCommand(const Utils::Environment &environment) const override;

    ToolChain *clone() const override;
    ProjectExplorer::IOutputParser *outputParser() const override;
    QList<Utils::FileName> suggestedMkspecList() const override;
    QList<ProjectExplorer::Task> validateKit(const ProjectExplorer::Kit *kit) const override;

    bool isJobCountSupported() const override { return false; }

    void addToEnvironment(Utils::Environment &env) const override;
private:
    QString m_buildEngineName;
    QString m_targetName;
    mutable ProjectExplorer::HeaderPaths m_headerPathsOnHost;
};

} // Internal
} // Mer

#endif // MERTOOLCHAIN_H
