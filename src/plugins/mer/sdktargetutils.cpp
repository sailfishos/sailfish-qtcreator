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

#include "sdktargetutils.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "merqtversion.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionfactory.h>

#include <QFileInfo>
#include <QDir>

namespace Mer {
namespace Internal {

SdkTargetUtils::SdkTargetUtils(QObject *parent) :
    QObject(parent),
    m_remoteProcessRunner(new QSsh::SshRemoteProcessRunner(this)),
    m_command(None),
    m_state(Inactive)
{
    connect(m_remoteProcessRunner, SIGNAL(connectionError()), SLOT(onConnectionError()));
    connect(m_remoteProcessRunner, SIGNAL(processStarted()), SLOT(onProcessStarted()));
    connect(m_remoteProcessRunner, SIGNAL(readyReadStandardOutput()),
            SLOT(onReadReadyStandardOutput()));
    connect(m_remoteProcessRunner, SIGNAL(readyReadStandardError()),
            SLOT(onReadReadyStandardError()));
    connect(m_remoteProcessRunner, SIGNAL(processClosed(int)), SLOT(onProcessClosed(int)));
}

void SdkTargetUtils::setSshConnectionParameters(const QSsh::SshConnectionParameters params)
{
    m_sshParameters = params;
}

bool SdkTargetUtils::checkInstalledTargets(const QString &sdkName)
{
    if (m_remoteProcessRunner->isProcessRunning())
        return false;
    if (m_state != Inactive)
        return false;
    m_state = Active;
    m_sdkName = sdkName;
    m_output.clear();
    m_command = FetchTargets;
    m_remoteProcessRunner->run("sdk-manage --target --list", m_sshParameters);
    return true;
}

void SdkTargetUtils::updateQtVersions(MerSdk &sdk, const QStringList &targets)
{
    const QString sdkToolsDir(MerSdkManager::instance()->sdkToolsDirectory()
                              + sdk.virtualMachineName() + QLatin1Char('/'));

    QHash<QString, int> updatedQtVersions;
    QStringList targetList = targets;

    QtSupport::QtVersionManager *qvm = QtSupport::QtVersionManager::instance();
    // remove old versions
    QHashIterator<QString, int> i(sdk.qtVersions());
    while (i.hasNext()) {
        i.next();
        QString t = i.key();
        if (targetList.contains(t)) {
            targetList.removeOne(t);
            updatedQtVersions.insert(i.key(), i.value());
            continue;
        }
        QtSupport::BaseQtVersion *qtv = qvm->version(i.value());
        if (qtv)
            qvm->removeVersion(qtv);
    }

    // add new versions
    foreach (const QString &t, targetList) {
        const QString vmName = sdk.virtualMachineName();
        MerQtVersion *qtv = createQtVersion(vmName, sdkToolsDir + t);
        if (qtv) {
            qtv->setDisplayName(
                        QString::fromLocal8Bit("Qt %1 in %2 %3").arg(qtv->qtVersionString(),
                                                                     vmName, t));
            qtv->setVirtualMachineName(vmName);
            qtv->setTargetName(t);
            qvm->addVersion(qtv);
            updatedQtVersions.insert(t, qtv->uniqueId());
        }
    }

    sdk.setQtVersions(updatedQtVersions);
}

void SdkTargetUtils::onReadReadyStandardOutput()
{
    const QString output = QString::fromLocal8Bit(m_remoteProcessRunner->readAllStandardOutput());
    m_output.append(output);
    emit processOutput(output);
}

void SdkTargetUtils::onReadReadyStandardError()
{
    const QString output = QString::fromLocal8Bit(m_remoteProcessRunner->readAllStandardError());
    emit processOutput(output);
}

void SdkTargetUtils::onProcessClosed(int exitCode)
{
    m_state = Inactive;
    if (m_command == FetchTargets) {
        QRegExp rx(QLatin1String("(.*)\\n"));
        rx.setMinimal(true);
        int pos = 0;
        QStringList targets;
        while ((pos = rx.indexIn(m_output, pos)) != -1) {
            targets.append(rx.cap(1));
            pos += rx.matchedLength();
        }
        emit installedTargets(m_sdkName, targets);
    }
    QTC_ASSERT(exitCode == QSsh::SshRemoteProcess::NormalExit, return);
}

MerQtVersion *SdkTargetUtils::createQtVersion(const QString &sdkName, const QString &targetDirectory)
{
    using namespace QtSupport;
    Utils::FileName qmake =
            Utils::FileName::fromString(targetDirectory + QLatin1Char('/') +
                                        QLatin1String(Constants::MER_WRAPPER_QMAKE));
    QtVersionManager *qtvm = QtSupport::QtVersionManager::instance();
    // Is there a qtversion present for this qmake?
    BaseQtVersion *qtv = qtvm->qtVersionForQMakeBinary(qmake);
    if (qtv && !qtv->isValid()) {
        qtvm->removeVersion(qtv);
        qtv = 0;
    }
    if (!qtv) {
        qtv = QtVersionFactory::createQtVersionFromQMakePath(qmake, true, targetDirectory);
    }

    QTC_ASSERT(qtv->type() == QLatin1String(Constants::MER_QT), return 0);
    return static_cast<MerQtVersion *>(qtv);
}

void SdkTargetUtils::onConnectionError()
{
    m_state = Inactive;
    emit connectionError();
}

void SdkTargetUtils::onProcessStarted()
{
    // Do nothing
}

} // Internal
} // Mer
