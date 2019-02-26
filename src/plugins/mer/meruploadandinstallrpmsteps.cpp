/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "meruploadandinstallrpmsteps.h"

#include "merdeploysteps.h"
#include "merrpminstaller.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/abstractuploadandinstallpackageservice.h>
#include <remotelinux/remotelinuxdeployconfiguration.h>
#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
class MerUploadAndInstallPackageService : public AbstractUploadAndInstallPackageService
{
public:
    explicit MerUploadAndInstallPackageService(AbstractRemoteLinuxDeployStep *step)
        : AbstractUploadAndInstallPackageService(step),
          m_installer(new MerRpmInstaller(this))
    {
    }

    AbstractRemoteLinuxPackageInstaller *packageInstaller() const override { return m_installer; }

private:
    QString uploadDir() const
    {
        const QString uname = deviceConfiguration()->sshParameters().userName();
        return uname == QLatin1String("root")
            ? QString::fromLatin1("/root") : QLatin1String("/home/") + uname;
    }


private:
    MerRpmInstaller * const m_installer;
};
} // anonymous namespace


MerUploadAndInstallRpmStep::MerUploadAndInstallRpmStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    m_packagingStep = 0;
    m_deployService = new MerUploadAndInstallPackageService(this);
}

AbstractRemoteLinuxDeployService *MerUploadAndInstallRpmStep::deployService() const
{
    return m_deployService;
}

bool MerUploadAndInstallRpmStep::initInternal(QString *error)
{
    //m_packagingStep = deployConfiguration()->earlierBuildStep<MerRpmPackagingStep>(this);
    m_packagingStep = deployConfiguration()->earlierBuildStep<MerMb2RpmBuildStep>(this);

    if (!m_packagingStep) {
        if (error)
            *error = tr("No previous \"%1\" step found.").arg(MerMb2RpmBuildStep::displayName());
        return false;
    }

    return deployService()->isDeploymentPossible(error);
}

Core::Id MerUploadAndInstallRpmStep::stepId()
{
    return Core::Id("Mer.MerUploadAndInstallLocalRpmStep");
}

QString MerUploadAndInstallRpmStep::displayName()
{
    return tr("Deploy Local RPM package via SFTP upload");
}

void  MerUploadAndInstallRpmStep::run(QFutureInterface<bool> &fi)
{
    const QString packageFile = m_packagingStep->packagesFilePath().first();
    if (!packageFile.endsWith(QLatin1String(".rpm"))){
        const QString message((tr("No package to deploy found in %1")).arg(packageFile));
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit addTask(Task(Task::Error, message, FileName(), -1,
                          Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        reportRunResult(fi, false);
        return;
    }

    m_deployService->setPackageFilePath(packageFile);
    AbstractRemoteLinuxDeployStep::run(fi);
}

} // namespace Internal
} // namespace Mer

#include "moc_meruploadandinstallrpmsteps.cpp"
