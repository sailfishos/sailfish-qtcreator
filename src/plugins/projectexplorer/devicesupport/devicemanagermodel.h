/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "../projectexplorer_export.h"
#include "idevice.h"

#include <QAbstractListModel>

#include <memory>

namespace ProjectExplorer {
namespace Internal { class DeviceManagerModelPrivate; }
class IDevice;
class DeviceManager;

class PROJECTEXPLORER_EXPORT DeviceManagerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DeviceManagerModel(const DeviceManager *deviceManager, QObject *parent = nullptr);
    ~DeviceManagerModel() override;

    void setFilter(const QList<Utils::Id> &filter);
    void setTypeFilter(Utils::Id type);

    IDevice::ConstPtr device(int pos) const;
    Utils::Id deviceId(int pos) const;
    int indexOf(IDevice::ConstPtr dev) const;
    int indexForId(Utils::Id id) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void updateDevice(Utils::Id id);

private:
    void handleDeviceAdded(Utils::Id id);
    void handleDeviceRemoved(Utils::Id id);
    void handleDeviceUpdated(Utils::Id id);
    void handleDeviceListChanged();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool matchesTypeFilter(const IDevice::ConstPtr &dev) const;

    const std::unique_ptr<Internal::DeviceManagerModelPrivate> d;
};

} // namespace ProjectExplorer
