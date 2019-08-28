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

    void start() override;
    void stop() override;
    void isRunning(QObject *context, std::function<void(bool,bool)> slot) const override;
    bool isHeadlessEffectively() const override;

protected:
    void prepareForNameChange() override;

private:
    void onNameChanged();

private:
    static QMap<QString, int> s_usedVmNames;
};

} // namespace Sfdk
