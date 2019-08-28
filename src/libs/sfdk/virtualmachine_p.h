/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "virtualmachine.h"

#include "vmconnection_p.h"

#include <ssh/sshconnection.h>

namespace Sfdk {

class VirtualMachinePrivate
{
    Q_DECLARE_PUBLIC(VirtualMachine)

public:
    VirtualMachinePrivate(VirtualMachine *q) : q_ptr(q) {}

    static VirtualMachinePrivate *get(VirtualMachine *q) { return q->d_func(); }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void isRunning(QObject *context, std::function<void(bool,bool)> slot) const = 0;
    virtual bool isHeadlessEffectively() const = 0;

    VirtualMachine::ConnectionUi *connectionUi() const { return connectionUi_.get(); }

protected:
    virtual void prepareForNameChange() {};

    VirtualMachine *const q_ptr;

private:
    QString name;
    std::unique_ptr<VirtualMachine::ConnectionUi> connectionUi_;
    std::unique_ptr<VmConnection> connection;
    QSsh::SshConnectionParameters sshParameters;
    bool headless = false;
    bool autoConnectEnabled = true;
};

} // namespace Sfdk
