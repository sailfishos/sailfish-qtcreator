/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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
    MerToolChain(bool autodetect, const QString &id
                 = QLatin1String(Constants::MER_TOOLCHAIN_ID));

    void setVirtualMachine(const QString &name);
    QString virtualMachineName() const;
    void setTargetName(const QString &name);
    QString targetName() const;

    QString type() const;
    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    QString makeCommand(const Utils::Environment &environment) const;

    ToolChain *clone() const;
    ProjectExplorer::IOutputParser *outputParser() const;
    QList<Utils::FileName> suggestedMkspecList() const;
    QList<ProjectExplorer::Task> validateKit(const ProjectExplorer::Kit *kit) const;

    QList<ProjectExplorer::HeaderPath> systemHeaderPaths(const QStringList &cxxflags,
                                                         const Utils::FileName &sysRoot) const;
    void addToEnvironment(Utils::Environment &env) const;
private:
    QString m_vmName;
    QString m_targetName;
    mutable QList<ProjectExplorer::HeaderPath> m_headerPathsOnHost;
};

} // Internal
} // Mer

#endif // MERTOOLCHAIN_H
