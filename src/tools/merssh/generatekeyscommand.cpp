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

#include "generatekeyscommand.h"
#include <ssh/sshkeygenerator.h>
#include <QFile>

GenerateKeysCommand::GenerateKeysCommand()
{
}

QString GenerateKeysCommand::name() const
{
    return QLatin1String("merssh");
}

bool GenerateKeysCommand::parseArguments()
{
    const QStringList &args = arguments();

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (m_privateKeyFileName.isNull()) {
            m_privateKeyFileName = current;
            continue;
        }

        if (m_publicKeyFileName.isNull()) {
            m_publicKeyFileName = current;
            continue;
        }

        if (next.isNull())
            return false;
    }

    if (m_privateKeyFileName.isEmpty())
        qCritical() << "No private kit given.";
    if (m_publicKeyFileName.isEmpty())
        qCritical() << "No piblic key given.";

    return !m_privateKeyFileName.isEmpty() && !m_publicKeyFileName.isEmpty();

}

int GenerateKeysCommand::execute()
{

    if(!parseArguments())
    {
        qCritical() << "Could not parse arguments.";
        return 1;
    }

    fprintf(stdout, "%s", "Generating keys...");
    fflush(stdout);

    QSsh::SshKeyGenerator keyGen;
    if (!keyGen.generateKeys(QSsh::SshKeyGenerator::Rsa,
                             QSsh::SshKeyGenerator::OpenSsl, 2048,
                             QSsh::SshKeyGenerator::DoNotOfferEncryption)) {
        qCritical() << "Cannot generate the ssh keys.";
        return 1;
    }

    QFile privateKeyFile(m_privateKeyFileName);
    if (!privateKeyFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot write file:" << m_privateKeyFileName;
        return 1;
    }
    privateKeyFile.write(keyGen.privateKey());
    if (!QFile::setPermissions(m_privateKeyFileName, QFile::ReadOwner | QFile::WriteOwner)) {
        qCritical() << "Cannot set permissions of file:" << m_privateKeyFileName;
        return 1;
    }

    QFile publicKeyFile(m_publicKeyFileName);
    if (!publicKeyFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot write file:" << m_publicKeyFileName;
        return 1;
    }
    publicKeyFile.write(keyGen.publicKey());

    return 0;
}

bool GenerateKeysCommand::isValid() const
{
    return true;
}
