/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#include "merplugin.h"
#include "merdevicefactory.h"
#include "merqtversionfactory.h"
#include "mertoolchainfactory.h"
#include "merdeployconfigurationfactory.h"
#include "merrunconfigurationfactory.h"
#include "merruncontrolfactory.h"
#include "meroptionspage.h"
#include "merdeploystepfactory.h"
#include "mersdkmanager.h"

#include "jollawelcomepage.h"

#include <QtPlugin>

namespace Mer {
namespace Internal {

MerPlugin::MerPlugin()
{
}

MerPlugin::~MerPlugin()
{
}

bool MerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    MerSdkManager::instance();
    addAutoReleasedObject(new MerOptionsPage);
    addAutoReleasedObject(new MerDeviceFactory);
    addAutoReleasedObject(new MerQtVersionFactory);
    addAutoReleasedObject(new MerToolChainFactory);
    addAutoReleasedObject(new MerDeployConfigurationFactory);
    addAutoReleasedObject(new MerRunConfigurationFactory);
    addAutoReleasedObject(new MerRunControlFactory);
    addAutoReleasedObject(new MerDeployStepFactory);

    addAutoReleasedObject(new JollaWelcomePage);

    return true;
}

void MerPlugin::extensionsInitialized()
{
}

} // Internal
} // Mer

Q_EXPORT_PLUGIN2(Mer, Mer::Internal::MerPlugin)
