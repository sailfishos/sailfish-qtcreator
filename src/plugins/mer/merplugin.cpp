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
#include "jollawelcomepage.h"
#include "mermode.h"
#include "merconnectionprompt.h"
#include "meractionmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <QtPlugin>
#include <QMessageBox>
#include <QTimer>

namespace Mer {
namespace Internal {

MerPlugin::MerPlugin():
    m_wait(false)
{
}

MerPlugin::~MerPlugin()
{
}

bool MerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(errorString)

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
    addAutoReleasedObject(new MerActionManager);

    addAutoReleasedObject(new JollaWelcomePage);
    addAutoReleasedObject(new MerMode);

    return true;
}

void MerPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    m_stopList.clear();
    m_wait=false;
    QList<MerSdk*> sdks = MerSdkManager::instance()->sdks();
    foreach(const MerSdk* sdk, sdks) {
        if(sdk->isHeadless()) {
            const QString& vm = sdk->virtualMachineName();
            if(MerConnectionManager::instance()->isConnected(vm)) {
                Prompt * prompt = MerActionManager::createClosePrompt(vm);
                connect(prompt,SIGNAL(closed(QString,bool)),this,SLOT(handlePromptClosed(QString,bool)));
                m_stopList << vm;
            }
        }
    }
    if(m_stopList.isEmpty())
        return SynchronousShutdown;
    else
        return AsynchronousShutdown;
}

void MerPlugin::handlePromptClosed(const QString& vm, bool accepted)
{
     m_stopList.removeAll(vm);
    if (accepted) {
        MerConnectionManager::instance()->disconnectFrom(vm);
        m_wait=true;
    }

    if(m_stopList.isEmpty()) {
        if(m_wait){
             QTimer::singleShot(3000, this, SIGNAL(asynchronousShutdownFinished()));
        }else{
             emit asynchronousShutdownFinished();
        }

    }

}

} // Internal
} // Mer

Q_EXPORT_PLUGIN2(Mer, Mer::Internal::MerPlugin)
