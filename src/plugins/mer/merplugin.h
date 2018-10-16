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

#ifndef MERPLUGIN_H
#define MERPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QMap>

namespace Mer {

class MerConnection;

namespace Internal {

class MerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "plugins/mer/SailfishOS.json")

public:
    MerPlugin();
    ~MerPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private slots:
    void handlePromptClosed(int result);
    void handleConnectionStateChanged();
    void handleLockDownFailed();

private:
    QMap<QString, MerConnection *> m_stopList;

#ifdef WITH_TESTS
    void verifyTargets(const QString &vm, QStringList expectedKits, QStringList expectedToolChains, QStringList expectedQtVersion);
    void testMerSshOutputParsers_data();
    void testMerSshOutputParsers();
    void testMerSdkManager_data();
    void testMerSdkManager();
#endif

};

} // Internal
} // Mer

#endif // MERPLUGIN_H
