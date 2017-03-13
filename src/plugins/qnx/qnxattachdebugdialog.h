/****************************************************************************
**
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company
** Contact: info@kdab.com
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

#include <projectexplorer/devicesupport/deviceprocessesdialog.h>

namespace Utils { class PathChooser; }

namespace Qnx {
namespace Internal {

class QnxAttachDebugDialog : public ProjectExplorer::DeviceProcessesDialog
{
    Q_OBJECT

public:
    explicit QnxAttachDebugDialog(ProjectExplorer::KitChooser *kitChooser, QWidget *parent = 0);

    QString projectSource() const;
    QString localExecutable() const;

private:
    Utils::PathChooser *m_projectSource;
    Utils::PathChooser *m_localExecutable;
};

} // namespace Internal
} // namespace Qnx
