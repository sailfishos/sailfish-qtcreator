/****************************************************************************
**
** Copyright (C) 2021 Open Mobile Platform LLC.
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

#pragma once

#include "asynchronous.h"
#include "utils.h"
#include <utils/optional.h>
#include <utils/fileutils.h>

namespace Sfdk {

class SFDK_EXPORT SigningUtils : public QObject
{
    Q_OBJECT
public:
    explicit SigningUtils(QObject *parent = nullptr);
    virtual ~SigningUtils();

    static void listSecretKeys(const QObject *context,
            const Functor<bool, const QList<GpgKeyInfo> &, QString> &functor);
    static void exportSecretKey(const QString &id, const Utils::FilePath &passphraseFile,
            const Utils::FilePath &outputDir, const QObject *context, const Functor<bool, QString> &functor);
    static void exportPublicKey(const QString &id, const QObject *context,
            const Functor<bool, const QByteArray &, QString> &functor);
    static bool isGpgAvailable(QString *errorString);
private:
    static QList<GpgKeyInfo> gpgKeyInfoListFromOutput(const QString &output);
    static void verifyPassphrase(const QString &id, const QString &passphraseFile,
            const QObject *context, const Functor<bool, QString> &functor);
};

} // namespace Sfdk
