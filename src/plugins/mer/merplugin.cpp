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
#include "merconnectionmanager.h"
#include "mervirtualboxmanager.h"
#include "meryamlupdater.h"
#include "merbuildconfigurationprojectpathenvironmentvariablesetter.h"
#include "yamleditorfactory.h"
#include "jollawelcomepage.h"
#include "mermode.h"
#include "merconnectionprompt.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtPlugin>
#include <QMessageBox>
#include <QTimer>

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
    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/mer/YAMLEditor.mimetypes.xml"), errorString))
        return false;

    MerSdkManager::verbose = arguments.count(QLatin1String("-mer-verbose"));

    addAutoReleasedObject(new MerSdkManager);
    addAutoReleasedObject(new MerVirtualBoxManager);
    addAutoReleasedObject(new MerConnectionManager);
    addAutoReleasedObject(new MerOptionsPage);
    addAutoReleasedObject(new MerDeviceFactory);
    addAutoReleasedObject(new MerQtVersionFactory);
    addAutoReleasedObject(new MerToolChainFactory);
    addAutoReleasedObject(new MerDeployConfigurationFactory);
    addAutoReleasedObject(new MerRunConfigurationFactory);
    addAutoReleasedObject(new MerRunControlFactory);
    addAutoReleasedObject(new MerDeployStepFactory);
    addAutoReleasedObject(new MerYamlUpdater);
    addAutoReleasedObject(new MerBuildConfigurationProjectPathEnvironmentVariableSetter);
    addAutoReleasedObject(new YamlEditorFactory);

    addAutoReleasedObject(new JollaWelcomePage);
    addAutoReleasedObject(new MerMode);

    return true;
}

void MerPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    MerConnectionPrompt *promtConnection = new MerConnectionPrompt(QString(), this);
    promtConnection->prompt(MerConnectionPrompt::Close);
    QTimer::singleShot(3000, this, SIGNAL(asynchronousShutdownFinished()));

    return AsynchronousShutdown;
}

} // Internal
} // Mer

Q_EXPORT_PLUGIN2(Mer, Mer::Internal::MerPlugin)
