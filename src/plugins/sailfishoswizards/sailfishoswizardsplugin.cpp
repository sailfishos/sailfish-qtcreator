/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "sailfishoswizardsplugin.h"
#include "sailfishoswizardsconstants.h"

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include "factories/qmllocalimportspagefactory.h"
#include "factories/importexternallibrariespagefactory.h"

namespace SailfishOSWizards {
namespace Internal {

SailfishOSWizardsPlugin::SailfishOSWizardsPlugin()
{
}

SailfishOSWizardsPlugin::~SailfishOSWizardsPlugin()
{
}

/*!
 * \brief Initializes the plugin.
 * This method registers the pages factories for using the pages inside the JSON-based wizards.
 */
bool SailfishOSWizardsPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
    JsonWizardFactory::registerPageFactory(new QmlLocalImportsPageFactory);
    JsonWizardFactory::registerPageFactory(new ImportExternalLibrariesPageFactory);
    return true;
}

void SailfishOSWizardsPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag SailfishOSWizardsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace SailfishOSWizardFields
