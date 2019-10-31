/****************************************************************************
**
** Copyright (C) 2012-2014,2017-2018 Jolla Ltd.
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

#ifndef MERDEVICEDEBUGSUPPORT_H
#define MERDEVICEDEBUGSUPPORT_H

#include <debugger/debuggerruncontrol.h>

namespace ProjectExplorer {
    class RunControl;
}

namespace Mer {
namespace Internal {

// Based on RemoteLinux::LinuxDeviceDebugSupport
class MerDeviceDebugSupport : public Debugger::DebuggerRunTool
{
public:
    MerDeviceDebugSupport(ProjectExplorer::RunControl *runControl);

protected:
    void start() override;
};

} // namespace Internal
} // namespace Mer

#endif // MERDEVICEDEBUGSUPPORT_H
