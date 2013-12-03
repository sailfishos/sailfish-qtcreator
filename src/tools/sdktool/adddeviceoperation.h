/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef ADDDEVICEOPERATION_H
#define ADDDEVICEOPERATION_H

#include "operation.h"

class AddDeviceOperation : public Operation
{
public:
    AddDeviceOperation();

public:
    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    //TODO:
    bool test() const { return false; }
#endif
    static QVariantMap initializeDevices();
    static QVariantMap addDevice(const QVariantMap &map,
                                 const QByteArray &internalId,
                                 const QString &displayName,
                                 const QString &type,
                                 int origin,
                                 bool sdkProvided,
                                 int machineType,
                                 const QString &host,
                                 int port,
                                 const QString &userName,
                                 int autheticationType,
                                 const QString &password,
                                 const QString &privateKeyFil,
                                 int timeout,
                                 const QString &freePorts,
                                 int version,
                                 const QString &virtualMachine = QString(),
                                 const QString &merMac = QString(),
                                 const QString &merSubnet = QString(),
                                 const QString &sharedSshPath = QString(),
                                 const QString &sharedConfigPath = QString());
private:    
    static QString param(const QString &text);

private:
    QByteArray m_internalId;
    QString m_displayName;
    QString m_type;
    int m_origin;
    bool m_sdkProvided;
    QString m_host;
    int m_port;
    QString m_userName;
    int m_autheticationType;
    QString m_password;
    QString m_privateKeyFile;
    int m_timeout;
    QString m_freePorts;
    int m_machineType;
    int m_version;
    QString m_merVirtualMachine;
    QString m_merMac;
    QString m_merSubnet;
    QString m_merSharedSshPath;
    QString m_merSharedConfigPath;
};

#endif // ADDDEVICEOPERATION_H
