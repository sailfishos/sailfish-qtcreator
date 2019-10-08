/****************************************************************************
**
** Copyright (C) 2012-2015,2019 Jolla Ltd.
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

#ifndef MERRPMPACKAGINGSTEP_H
#define MERRPMPACKAGINGSTEP_H

#include "mersdk.h"

#include <remotelinux/abstractpackagingstep.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QProcess;
QT_END_NAMESPACE

namespace QmakeProjectManager {
    class QmakeBuildConfiguration;
}

namespace RemoteLinux {
    class RemoteLinuxDeployConfiguration;
}

namespace Mer {
namespace Internal {

class MerRpmPackagingStep : public RemoteLinux::AbstractPackagingStep
{
    Q_OBJECT
public:
    MerRpmPackagingStep(ProjectExplorer::BuildStepList *bsl);
    ~MerRpmPackagingStep() override;
    static Core::Id stepId();
    static QString displayName();

protected:
    bool init() override;

private slots:
    void handleBuildOutput();

private:
    void ctor();
    void doRun() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    QString packageFileName() const;
    bool isPackagingNeeded() const;

    bool createPackage(QProcess *buildProc);
    bool prepareBuildDir();
    bool createSpecFile();

    bool m_packagingNeeded;
    Utils::Environment m_environment;
    QString m_rpmCommand;
    QStringList m_rpmArgs;
    QString m_fileName;
    QString m_pkgFileName;
    QString m_mappedDirectory;
    QRegExp m_regExp;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPACKAGECREATIONSTEP_H
