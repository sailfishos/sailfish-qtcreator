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

#include "mermode.h"

#include "merconstants.h"
#include "mericons.h"
#include "mermanagementwebview.h"

#include <coreplugin/modemanager.h>
#include <utils/icon.h>

#include <QDebug>

using namespace Core;

namespace Mer {
namespace Internal {

MerMode::MerMode()
{
    setWidget(new MerManagementWebView);
    setContext(Context("Mer.MerMode"));
    setDisplayName(tr("Sailfish OS"));
    setIcon(Utils::Icon::modeIcon(Icons::MER_MODE_ICON_CLASSIC,
                                Icons::MER_MODE_ICON_FLAT, Icons::MER_MODE_ICON_FLAT_ACTIVE));
    setPriority(80); // between "Projects" and "Analyze" modes
    setId("Mer.MerMode");
    setContextHelpId(QString());
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &MerMode::handleUpdateContext);
}

void MerMode::handleUpdateContext(Id newMode, Id oldMode)
{
    MerManagementWebView* view = qobject_cast<MerManagementWebView*>(widget());
    if (view && newMode == id()) {
        view->setAutoFailReload(true);
    } else if (view && oldMode == id()) {
        view->setAutoFailReload(false);
    }
}

} // namespace Internal
} // namespace Mer
