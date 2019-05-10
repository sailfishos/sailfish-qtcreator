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

#include "merrpmpackagingstep.h"

#include "merconstants.h"
#include "merrpmpackagingwidget.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qtsupport/qtkitinformation.h>

#include <QProcess>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QtSupport;
using namespace Utils;

namespace {
const QLatin1String MagicFileName(".qtcreator");
const QLatin1String SpecFileName("qtcreator.spec");
} // namespace


namespace Mer {
namespace Internal {

MerRpmPackagingStep::MerRpmPackagingStep(BuildStepList *bsl) : AbstractPackagingStep(bsl, stepId())
{
    //TODO: write better pattern
    m_regExp = QRegExp(QLatin1String("Wrote: .*/(rpmbuild/.*\\.rpm)"));
    setDefaultDisplayName(displayName());
}

MerRpmPackagingStep::~MerRpmPackagingStep()
{
}

bool MerRpmPackagingStep::init()
{
    if (!AbstractPackagingStep::init())
        return false;

    m_packagingNeeded = isPackagingNeeded();
    if (!isPackagingNeeded())
        return true;

    if (!target()->activeBuildConfiguration()) {
        raiseError(tr("No Qt build configuration"));
        return false;
    }

    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    if (!version) {
        raiseError(tr("Packaging failed: No Qt version."));
        return false;
    }

    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

    QString merTarget =  MerSdkManager::targetNameForKit(target()->kit());

    if (merTarget.isEmpty()) {
        raiseError(tr("Packaging failed: No Sailfish OS build target set."));
        return false;
    }
    m_fileName = project()->document()->filePath().toFileInfo().baseName();
    const QString wrapperScriptsDir = version->qmakeCommand().parentDir().toString();
    m_rpmCommand = wrapperScriptsDir + QLatin1Char('/') + QLatin1String(Constants::MER_WRAPPER_RPMBUILD);
    QString sharedHome =  MerSdkKitInformation::sdk(target()->kit())->sharedHomePath();
    m_mappedDirectory = cachedPackageDirectory().replace(sharedHome, QLatin1String("$HOME"));
    m_rpmArgs.clear();
    m_rpmArgs << QString::fromLatin1("--define \"_topdir %1/rpmbuild\"").arg(m_mappedDirectory);
    m_rpmArgs << QString::fromLatin1("--skip-prep -bb rpmbuild/qtcreator.spec");
    m_environment = bc->environment();

    return true;
}

void MerRpmPackagingStep::  run(QFutureInterface<bool> &fi)
{
    if (!m_packagingNeeded) {
        emit addOutput(tr("Package up to date."), OutputFormat::NormalMessage);
        reportRunResult(fi, true);
        return;
    }

    setPackagingStarted();
    // TODO: Make the build process asynchronous; i.e. no waitFor()-functions etc.
    QProcess * const buildProc = new QProcess;
    connect(buildProc, &QProcess::readyReadStandardOutput,
            this, &MerRpmPackagingStep::handleBuildOutput);
    connect(buildProc, &QProcess::readyReadStandardError,
            this, &MerRpmPackagingStep::handleBuildOutput);

    const bool success = prepareBuildDir() && createSpecFile() && createPackage(buildProc, fi);

    disconnect(buildProc, 0, this, 0);
    buildProc->deleteLater();
    if (success)
        emit addOutput(tr("Package created."), OutputFormat::NormalMessage);
    setPackagingFinished(success);
    reportRunResult(fi, success);
}


BuildStepConfigWidget *MerRpmPackagingStep::createConfigWidget()
{
    return new MerRpmPackagingWidget(this);
}




void MerRpmPackagingStep::handleBuildOutput()
{
    QProcess * const buildProc = qobject_cast<QProcess *>(sender());
    if (!buildProc)
        return;
    QByteArray stdOut = buildProc->readAllStandardOutput();
    stdOut.replace('\0', QByteArray()); // Output contains NULL characters.
    int pos = m_regExp.indexIn(QLatin1String(stdOut));
    if (pos > -1) {
        m_pkgFileName  = m_regExp.cap(1);
    }
    if (!stdOut.isEmpty())
        emit addOutput(QString::fromLocal8Bit(stdOut), OutputFormat::Stdout,
                BuildStep::DontAppendNewline);
    QByteArray errorOut = buildProc->readAllStandardError();
    errorOut.replace('\0', QByteArray());
    if (!errorOut.isEmpty()) {
        emit addOutput(QString::fromLocal8Bit(errorOut), OutputFormat::Stderr,
            BuildStep::DontAppendNewline);
    }
}

bool MerRpmPackagingStep::isPackagingNeeded() const
{
    if (AbstractPackagingStep::isPackagingNeeded())
        return true;
    return false; //isMetaDataNewerThan(QFileInfo(packageFilePath()).lastModified());
}

QString MerRpmPackagingStep::packageFileName() const
{
    return m_pkgFileName;
}

bool MerRpmPackagingStep::prepareBuildDir()
{
    const bool inSourceBuild = QFileInfo(cachedPackageDirectory()) == project()->projectDirectory().toFileInfo();
    const QString rpmDirPath = cachedPackageDirectory() + QLatin1String("/rpmbuild");
    const QString magicFilePath = rpmDirPath + QLatin1Char('/') + MagicFileName;

    if (inSourceBuild && QFileInfo(rpmDirPath).isDir() && !QFileInfo(magicFilePath).exists()) {
        raiseError(tr("Packaging failed: Foreign rpmbuild directory detected. %1"
            "Qt Creator will not overwrite that directory. Please remove it").arg(QDir::toNativeSeparators(rpmDirPath)));
        return false;
    }

    QString error;

    if (!FileUtils::removeRecursively(FileName::fromString(rpmDirPath), &error)) {
        raiseError(tr("Packaging failed: Could not remove directory \"%1\": %2")
            .arg(rpmDirPath, error));
        return false;
    }

    QDir buildDir(cachedPackageDirectory());
    if (!buildDir.mkdir(QLatin1String("rpmbuild"))) {
        raiseError(tr("Could not create rpmbuild directory \"%1\".").arg(rpmDirPath));
        return false;
    }

    QFile magicFile(magicFilePath);
    if (!magicFile.open(QIODevice::WriteOnly)) {
        raiseError(tr("Error: Could not create file \"%1\".")
            .arg(QDir::toNativeSeparators(magicFilePath)));
        return false;
    }

    return true;
}

bool MerRpmPackagingStep::createSpecFile()
{
    emit addOutput(tr("Creating spec file..."), OutputFormat::NormalMessage);
    const QString rpmDirPath = cachedPackageDirectory() + QLatin1String("/rpmbuild");
    const QString specFilePath = rpmDirPath + QLatin1Char('/') + SpecFileName;
    QFile specFile(specFilePath);
    if (!specFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        raiseError(tr("Error: Could not create file \"%1\".").arg(QDir::toNativeSeparators(specFilePath)));
        return false;
    }

    QTextStream out(&specFile);
    out << "%{!?qtc_qmake:%define qtc_qmake %qmake}" << endl;
    out << "%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}" << endl;
    out << "%{!?qtc_make:%define qtc_make make}" << endl;
    out << "%{?qtc_builddir:%define _builddir %qtc_builddir}" << endl;
    out << "# This file is generated by Qtcreator" << endl;
    out << "Name:      "<< m_fileName << endl;
    out << "Summary:    My Sailfish OS Application" << endl;
    out << "Version:    0.1" << endl;
    out << "Release:    1 "<< endl;
    out << "Group:      Qt/Qt" << endl;
    out << "License:    LICENSE" << endl;
    out << "Requires:   sailfishsilica-qt5" << endl;
    out << "BuildRequires:  pkgconfig(Qt5Core)" << endl;
    out << "BuildRequires:  pkgconfig(Qt5Qml)" << endl;
    out << "BuildRequires:  pkgconfig(Qt5Quick)" << endl;
    out << "BuildRequires:  pkgconfig(sailfishapp)" << endl;
    out << "BuildRequires:  desktop-file-utils" << endl;
    out << "%description" << endl;
    out << "Short description of my Sailfish OS Application" << endl;
    out << "%install" << endl;
    out << "rm -rf %{buildroot}" << endl;
    out << "cd " << m_mappedDirectory << ";" << "make install INSTALL_ROOT=%{buildroot}" << endl;
    out << "%files" << endl;
    out << "%defattr(-,nemo,nemo,-)" << endl;
    out << "/usr" << endl;
    specFile.close();
    return true;
}

bool MerRpmPackagingStep::createPackage(QProcess *buildProc,const QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Creating package file..."), OutputFormat::NormalMessage);
    Q_UNUSED(fi);
    buildProc->setEnvironment(m_environment.toStringList());
    buildProc->setWorkingDirectory(cachedPackageDirectory());

    emit addOutput(tr("Package Creation: Running command \"%1 %2\" .")
                   .arg(m_rpmCommand)
                   .arg(m_rpmArgs.join(QLatin1Char(' '))),
                   OutputFormat::NormalMessage);

    buildProc->start(m_rpmCommand, m_rpmArgs);
    if (!buildProc->waitForStarted()) {
        raiseError(tr("Packaging failed: Could not start command \"%1\". Reason: %2")
            .arg(m_rpmCommand, buildProc->errorString()));
        return false;
    }
    buildProc->waitForFinished(-1);
    if (buildProc->error() != QProcess::UnknownError || buildProc->exitCode() != 0) {
        QString mainMessage = tr("Packaging Error: Command \"%1\"  failed.")
            .arg(m_rpmCommand);
        if (buildProc->error() != QProcess::UnknownError)
            mainMessage += tr(" Reason: %1").arg(buildProc->errorString());
        else
            mainMessage += tr("Exit code: %1").arg(buildProc->exitCode());
        raiseError(mainMessage);
        return false;
    }
    return true;

    QFile::remove(cachedPackageFilePath());

    return true;
}

Core::Id MerRpmPackagingStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmPackagingStep");
}

QString MerRpmPackagingStep::displayName()
{
    return tr("Build RPM Package Locally");
}


} // namespace Internal
} // namespace Mer
