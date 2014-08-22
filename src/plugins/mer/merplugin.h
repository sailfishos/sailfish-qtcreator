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

#ifndef MERPLUGIN_H
#define MERPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <QMap>

namespace Mer {
namespace Internal {

class MerConnection;

class MerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "plugins/mer/Mer.json")

public:
    MerPlugin();
    ~MerPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private slots:
    void handlePromptClosed(const QString& vm, bool accepted);
    void handleConnectionStateChanged();

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
