/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include <QString>
#include <QStringList>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Class for DBus service.
 */
class DBusService
{
public:
    /*!
     * \brief Constructor initializes class's fields.
     * \param humanizedServiceName Humanized name of service.
     * \param serviceName DBus service name.
     * \param servicePath Path of DBus service.
     * \param busType Bus type.
     * \param interface Name of DBus service interface.
     * \param method Name of interface declared method.
     */
    DBusService(const QString &humanizedServiceName = QString(), const QString &serviceName = QString(), const QString &servicePath = QString(),
                const QString &busType = QString(), const QString &interface = QString(), const QString &method = QString(), const QString &signal = QString())
        : m_humanizedServiceName(humanizedServiceName)
        , m_serviceName(serviceName)
        , m_servicePath(servicePath)
        , m_busType(busType)
        , m_interface(interface)
        , m_method(method)
        , m_signal(signal)
    {}

    /*!
     * \brief Return humanized name of service.
     */
    QString getHumanizedServiceName() const
    {
        return m_humanizedServiceName;
    }

    /*!
     * \brief Return name of DBus service.
     */
    const QString getServiceName() const
    {
        return m_serviceName;
    }

    /*!
     * \brief Return path of DBus service.
     */
    const QString getServicePath() const
    {
        return m_servicePath;
    }

    /*!
     * \brief Return bus type.
     */
    const QString getBusType() const
    {
        return m_busType;
    }

    /*!
     * \brief Return name of DBus service interface.
     */
    const QString getInterface() const
    {
        return m_interface;
    }

    /*!
     * \brief Return name of interface declared method.
     */
    const QString getMethod() const
    {
        return m_method;
    }

    /*!
     * \brief Return name of interface declared signal.
     */
    const QString getSignal() const
    {
        return m_signal;
    }

    /*!
     * \brief Return result of method comparison.
     * \param service DBus service.
     */
    bool operator==(const DBusService &service)
    {
        return (m_method == service.m_method &&
                m_interface == service.m_interface &&
                m_signal == service.m_signal);
    }
private:
    QString m_humanizedServiceName;
    QString m_serviceName;
    QString m_servicePath;
    QString m_busType;
    QString m_interface;
    QString m_method;
    QString m_signal;
};

} // namespace Internal
} // namespace SailfishWizards
