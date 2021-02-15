/****************************************************************************
**
** Copyright (C) 2020 Jolla Ltd.
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

#include <QHash>
#include <QObject>
#include <QString>

#include <memory>

namespace Sfdk {

class DBusManager
{
public:
    using Ptr = std::shared_ptr<DBusManager>;

    struct PrivateConstructorTag;
    DBusManager(quint16 busPort, const QString &connectionName, const PrivateConstructorTag &);
    ~DBusManager();

    static Ptr get(quint16 busPort, const QString &connectionName);

    QString serviceName() const { return m_serviceName; }

private:
    static QHash<QString, std::weak_ptr<DBusManager>> s_instances;
    quint16 m_busPort;
    QString m_connectionName;
    QString m_serviceName;
    std::unique_ptr<QObject> m_configuredDeviceObject;
};

} // namespace Sfdk
