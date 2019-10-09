/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013-2014,2017,2019 Jolla Ltd.
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

#ifndef ADDMERSDKOPERATION_H
#define ADDMERSDKOPERATION_H
#include "operation.h"

class AddMerSdkOperation : public Operation
{


public:
    AddMerSdkOperation();
    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    bool test() const;
#endif

    static QVariantMap addSdk(const QVariantMap &map,
           const QString &sdkName,
           bool autodetected,
           const QString &sharedHomePath,
           const QString &sharedTargetsPath,
           const QString &sharedSshPath,
           const QString &sharedSrcPath,
           const QString &sharedConfigPath,
           const QString &host,
           const QString &userName,
           const QString &privateKeyFile,
           quint16 sshPort,
           quint16 wwwPort,
           bool headless,
           int memorySizeMb,
           int cpuCount,
           int vdiCapacityMb);
    static QVariantMap initializeSdks(int version);

private:
    QString m_installDir;
    QString m_name;
    int m_version;
    bool m_autoDetected;
    QString m_sharedHomePath;
    QString m_sharedTargetsPath;
    QString m_sharedSshPath;
    QString m_sharedSrcPath;
    QString m_sharedConfigPath;
    QString m_host;
    QString m_userName;
    QString m_privateKeyFile;
    quint16 m_sshPort;
    quint16 m_wwwPort;
    bool m_headless;
    int m_memorySizeMb;
    int m_cpuCount;
    int m_vdiCapacityMb;
};
#endif
