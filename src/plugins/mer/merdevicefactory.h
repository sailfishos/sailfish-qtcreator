/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#ifndef MERDEVICEFACTORY_H
#define MERDEVICEFACTORY_H

#include <projectexplorer/devicesupport/idevicefactory.h>

namespace Mer {
namespace Internal {

class MerDeviceFactory : public ProjectExplorer::IDeviceFactory
{
    Q_OBJECT

public:
    MerDeviceFactory();

    QString displayNameForId(Core::Id type) const;
    QList<Core::Id> availableCreationIds() const;
    static bool canCreate(Core::Id id);
    ProjectExplorer::IDevice::Ptr create(Core::Id id) const;
    bool canRestore(const QVariantMap &map) const;
    ProjectExplorer::IDevice::Ptr restore(const QVariantMap &map) const;
};

} // Internal
} // Mer

#endif // MERDEVICEFACTORY_H
