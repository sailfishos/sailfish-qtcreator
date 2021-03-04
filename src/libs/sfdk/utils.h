/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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
#include "sfdkglobal.h"

#include <QJsonDocument>
#include <QMetaType>

namespace Sfdk {

enum class TextStyle
{
    Pretty,
    CamelCase,
    SnakeCase
};

SFDK_EXPORT bool isPortOccupied(quint16 port);

class SFDK_EXPORT GpgKeyInfo
{
public:
    QString name;
    QString fingerprint;

    QString toString() const;
    static GpgKeyInfo fromString(const QString &string);

    bool isValid() const;
    bool operator==(const GpgKeyInfo &other) const;
};

SFDK_EXPORT bool isGpgAvailable(QString *errorString);
SFDK_EXPORT void availableGpgKeys(const QObject *context,
        const Functor<bool, const QList<GpgKeyInfo> &, QString> &functor);

} // namespace Sfdk

Q_DECLARE_METATYPE(Sfdk::GpgKeyInfo);
