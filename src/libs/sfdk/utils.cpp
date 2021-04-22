
/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "utils_p.h"

#include "asynchronous_p.h"
#include "sfdkconstants.h"
#include "signingutils_p.h"

#include <utils/algorithm.h>
#include <utils/portlist.h>

#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTimer>

#include <algorithm>

using namespace Utils;

namespace Sfdk {

QList<Utils::Port> toList(const Utils::PortList &portList)
{
    PortList portList_(portList);

    using Dummy = int;
    QMap<Utils::Port, Dummy> ports;

    while (portList_.hasMore()) {
        QTC_ASSERT(ports.count() < Constants::MAX_PORT_LIST_PORTS, return ports.keys());
        ports.insert(portList_.getNext(), Dummy());
    }

    return ports.keys();
}

Utils::PortList toPortList(const QList<Utils::Port> &ports)
{
    auto uniqueSort = [](auto &container) {
        Utils::sort(container);
        auto last = std::unique(std::begin(container), std::end(container));
        container.erase(last, std::end(container));
    };

    QList<Port> ports_ = ports;
    uniqueSort(ports_);

    if (ports_.isEmpty())
        return {};

    PortList portList;

    Port previous = ports_.takeFirst();
    Port rangeStart = previous;
    while (!ports_.isEmpty()) {
        Port port = ports_.takeFirst();
        if (port.number() - previous.number() > 1) {
            portList.addRange(rangeStart, previous);
            rangeStart = port;
        }
        previous = port;
    }
    portList.addRange(rangeStart, previous);

    return portList;
}

Utils::PortList toPortList(const QList<quint16> &portList)
{
    return toPortList(Utils::transform(portList, [](quint16 port) { return Port(port); }));
}

QString separator(TextStyle textStyle)
{
    switch (textStyle) {
    case TextStyle::Pretty:
        return {QChar::Nbsp};
    case TextStyle::CamelCase:
        return {};
    case TextStyle::SnakeCase:
        return {'_'};
    }
    return {};
}

bool isPortOccupied(quint16 port)
{
    return !QTcpServer().listen(QHostAddress::LocalHost, port);
}

QString GpgKeyInfo::toString() const
{
    if (isValid())
        return QString(name + " [" + fingerprint + "]");
    return {};
}

GpgKeyInfo GpgKeyInfo::fromString(const QString &string)
{
    QRegularExpression re("^(.*) \\[(.*)\\]$");
    QRegularExpressionMatch match = re.match(string);
    return {match.captured(1), match.captured(2)};
}

bool GpgKeyInfo::isValid() const
{
    return !name.isEmpty() && !fingerprint.isEmpty();
}

bool GpgKeyInfo::operator==(const GpgKeyInfo &other) const
{
    return name == other.name && fingerprint == other.fingerprint;
}

bool isGpgAvailable(QString *errorString)
{
    return SigningUtils::isGpgAvailable(errorString);
}

void availableGpgKeys(const QObject *context,
        const Functor<bool, const QList<GpgKeyInfo> &, QString> &functor)
{
    QString errorString;
    QTC_ASSERT(isGpgAvailable(&errorString), {
        BatchComposer::enqueueCheckPoint(context,
                std::bind(functor, false, QList<GpgKeyInfo>{}, errorString));
        return;
    });

    SigningUtils::listSecretKeys(context, functor);
}

} // namespace Sfdk
