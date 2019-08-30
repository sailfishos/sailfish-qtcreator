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

#include "virtualmachine_p.h"

namespace Sfdk {

class VBoxVirtualMachinePrivate;
// FIXME internal
class SFDK_EXPORT VBoxVirtualMachine : public VirtualMachine
{
    Q_OBJECT

public:
    explicit VBoxVirtualMachine(QObject *parent = nullptr); // FIXME factory
    ~VBoxVirtualMachine() override;

    void hasPortForwarding(quint16 hostPort, const QObject *context,
            const Functor<bool, const QString &, bool> &functor) const override;
    void addPortForwarding(const QString &ruleName, const QString &protocol,
            quint16 hostPort, quint16 emulatorVmPort, const QObject *context,
            const Functor<bool> &functor) override;
    void removePortForwarding(const QString &ruleName, const QObject *context,
            const Functor<bool> &functor) override;

    static QStringList usedVirtualMachines();

private:
    Q_DISABLE_COPY(VBoxVirtualMachine)
    Q_DECLARE_PRIVATE(VBoxVirtualMachine)
};

class VBoxVirtualMachinePrivate : public VirtualMachinePrivate
{
    Q_DECLARE_PUBLIC(VBoxVirtualMachine)

public:
    using VirtualMachinePrivate::VirtualMachinePrivate;

    void start(const QObject *context, const Functor<bool> &functor) override;
    void stop(const QObject *context, const Functor<bool> &functor) override;
    void probe(const QObject *context,
            const Functor<BasicState, bool> &functor) const override;

protected:
    void prepareForNameChange() override;

private:
    void onNameChanged();

private:
    static QMap<QString, int> s_usedVmNames;
};

} // namespace Sfdk
