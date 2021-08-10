/****************************************************************************
**
** Copyright (C) 2019 Jochen Seemann
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

#include "../itestframework.h"

#include "catchtestsettings.h"
#include "catchtestsettingspage.h"

namespace Autotest {
namespace Internal {

class CatchFramework : public ITestFramework
{
public:
    CatchFramework() : ITestFramework(true) {}

    const char *name() const override;
    unsigned priority() const override;

protected:
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;

private:
    ITestSettings * testSettings() override { return &m_settings; }
    CatchTestSettings m_settings;
    CatchTestSettingsPage m_settingsPage{&m_settings, settingsId()};
};

} // namespace Internal
} // namespace Autotest
