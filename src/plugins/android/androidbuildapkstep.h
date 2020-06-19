/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "android_global.h"

#include <projectexplorer/abstractprocessstep.h>

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Android {

class ANDROID_EXPORT AndroidBuildApkStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    AndroidBuildApkStep(ProjectExplorer::BuildStepList *bc, Core::Id id);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    // signing
    Utils::FilePath keystorePath();
    void setKeystorePath(const Utils::FilePath &path);
    void setKeystorePassword(const QString &pwd);
    void setCertificateAlias(const QString &alias);
    void setCertificatePassword(const QString &pwd);

    QAbstractItemModel *keystoreCertificates();
    bool signPackage() const;
    void setSignPackage(bool b);

    bool buildAAB() const;
    void setBuildAAB(bool aab);

    bool openPackageLocation() const;
    void setOpenPackageLocation(bool open);

    bool verboseOutput() const;
    void setVerboseOutput(bool verbose);

    bool useMinistro() const;
    void setUseMinistro(bool b);

    bool addDebugger() const;
    void setAddDebugger(bool debug);

    QString buildTargetSdk() const;
    void setBuildTargetSdk(const QString &sdk);

    QVariant data(Core::Id id) const override;
private:
    Q_INVOKABLE void showInGraphicalShell();

    bool init() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;
    bool verifyKeystorePassword();
    bool verifyCertificatePassword();

    void doRun() override;

    bool m_buildAAB = false;
    bool m_signPackage = false;
    bool m_verbose = false;
    bool m_useMinistro = false;
    bool m_openPackageLocation = false;
    bool m_openPackageLocationForRun = false;
    bool m_addDebugger = true;
    QString m_buildTargetSdk;

    Utils::FilePath m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    QString m_packagePath;

    QString m_command;
    QString m_argumentsPasswordConcealed;
    bool m_skipBuilding = false;
    QString m_inputFile;
};

namespace Internal {

class AndroidBuildApkStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    AndroidBuildApkStepFactory();
};

} // namespace Internal
} // namespace Android
