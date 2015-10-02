/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
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

#ifndef MERADDVMSTARTBUILDSTEPPROJECTLISTENER_H
#define MERADDVMSTARTBUILDSTEPPROJECTLISTENER_H

#include "merprojectlistener.h"

namespace ProjectExplorer {
class BuildConfiguration;
}

namespace Mer {
namespace Internal {

// Rationale: it is not possible to simply override
// QmakeBuildConfigurationFactory::restore() in order to... See differencies in
// implementation of IBuildConfigurationFactory::find() for "create" and
// "restore" case - priority is not considere in latter case.
class MerAddVmStartBuildStepProjectListener : public MerProjectListener
{
    Q_OBJECT
public:
    explicit MerAddVmStartBuildStepProjectListener(QObject *parent = 0);
    ~MerAddVmStartBuildStepProjectListener() override;

protected:
    // From MerProjectListener
    bool handleProject(QmakeProjectManager::QmakeProject *project) override;
    bool forgetProject(ProjectExplorer::Project *project) override;

private:
    static void ensureHasVmStartStep(ProjectExplorer::BuildConfiguration *bc);
};

} // Internal
} // Mer

#endif // MERADDVMSTARTBUILDSTEPPROJECTLISTENER_H
