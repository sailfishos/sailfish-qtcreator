/****************************************************************************
**
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

#pragma once

#include "sfdkglobal.h"

#include "asynchronous.h"

#include <utils/fileutils.h>

#include <QSize>

namespace Utils {
class PortList;
}

namespace Sfdk {

class VirtualMachine;
class UserSettings;

class EmulatorManager;

class SFDK_EXPORT DeviceModelData
{
public:
    bool isNull() const;
    bool isValid() const;

    bool operator==(const DeviceModelData &other) const;
    bool operator!=(const DeviceModelData &other) const;

    QString name;
    QSize displayResolution;
    QSize displaySize;
    QString dconf;
    bool autodetected{false};
};

class EmulatorPrivate;
class SFDK_EXPORT Emulator : public QObject
{
    Q_OBJECT

public:
    struct PrivateConstructorTag;
    Emulator(QObject *parent, const PrivateConstructorTag &);
    ~Emulator() override;

    QUrl uri() const;
    QString name() const;

    VirtualMachine *virtualMachine() const;

    bool isAutodetected() const;

    Utils::FileName sharedConfigPath() const;
    Utils::FileName sharedSshPath() const;

    quint16 sshPort() const;
    void setSshPort(quint16 sshPort, const QObject *context, const Functor<bool> &functor);

    Utils::PortList freePorts() const;
    void setFreePorts(const Utils::PortList &freePorts, const QObject *context,
            const Functor<bool> &functor);

    Utils::PortList qmlLivePorts() const;
    void setQmlLivePorts(const Utils::PortList &qmlLivePorts, const QObject *context,
            const Functor<bool> &functor);

    QString factorySnapshot() const;
    void setFactorySnapshot(const QString &snapshotName);
    void restoreFactoryState(const QObject *context, const Functor<bool> &functor);

    DeviceModelData deviceModel() const;
    Qt::Orientation orientation() const;
    bool isViewScaled() const;
    void setDisplayProperties(const DeviceModelData &deviceModel, Qt::Orientation orientation,
            bool viewScaled, const QObject *context, const Functor<bool> &functor);

signals:
    void sharedConfigPathChanged(const Utils::FileName &sharedConfigPath);
    void sharedSshPathChanged(const Utils::FileName &sharedSshPath);
    void sshPortChanged(quint16 sshPort);
    void freePortsChanged();
    void qmlLivePortsChanged();
    void factorySnapshotChanged(const QString &snapshotName);
    void deviceModelNameChanged(const QString &deviceModelName);
    void deviceModelChanged();
    void orientationChanged(Qt::Orientation orientation);
    void viewScaledChanged(bool viewScaled);

private:
    std::unique_ptr<EmulatorPrivate> d_ptr;
    Q_DISABLE_COPY(Emulator)
    Q_DECLARE_PRIVATE(Emulator)
};

} // namespace Sfdk
