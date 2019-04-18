/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include "desktopwizardfactory.h"
#include "desktopeditorfactory.h"
#include "cppclasswizardfactory.h"
#include "qmlwizardfactory.h"
#include "unittestwizardfactory.h"

#include "sailfishwizardsplugin.h"
#include "sailfishwizardsconstants.h"
#include "common.h"

#include <QAction>
#include <QMessageBox>

namespace SailfishWizards {
namespace Internal {

using Core::IEditorFactory;
using Core::IWizardFactory;
using Core::Command;
using Core::ActionManager;
using Core::ActionContainer;

/*!
 * \brief Constructor of the plugin main class.
 */
SailfishWizardsPlugin::SailfishWizardsPlugin()
{

}

/*!
 * \brief Destructor of the plugin main class.
 */
SailfishWizardsPlugin::~SailfishWizardsPlugin()
{
    for(IEditorFactory *editorFactory : m_editorFactoriesPool)
        ExtensionSystem::PluginManager::removeObject(editorFactory);
    qDeleteAll(m_editorFactoriesPool);
}

/*!
 * \brief Registers wizard and editor factories.
 * Returns \c true if the process finished successfully.
 * \param arguments List of arguments.
 * \param errorString Outer parameter for recording error messages.
 */
bool SailfishWizardsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    IWizardFactory::registerFactoryCreator([]() -> QList<IWizardFactory *> {
        QList<IWizardFactory *> result;
        result << new DesktopWizardFactory();
        result << new CppClassWizardFactory();
        result << new QmlWizardFactory();
        result << new UnitTestWizardFactory();
        return result;
    });
    m_editorFactoriesPool.append(new DesktopEditorFactory);
    for(IEditorFactory *editorFactory : m_editorFactoriesPool)
        ExtensionSystem::PluginManager::addObject(editorFactory);
    return true;
}

/*!
 * \brief Does operations after all extensions including this plugin are initialized.
 */
void SailfishWizardsPlugin::extensionsInitialized()
{

}

/*!
 * \brief Does preparations before plugin shutdown.
 * Returns the shutdown flag associated with the plugin shutdown mode.
 */
ExtensionSystem::IPlugin::ShutdownFlag SailfishWizardsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace SailfishWizards
