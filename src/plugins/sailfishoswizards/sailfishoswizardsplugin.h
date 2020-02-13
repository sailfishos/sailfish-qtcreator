/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef SAILFISHOSWIZARDS_H
#define SAILFISHOSWIZARDS_H

#include "sailfishoswizards_global.h"

#include <extensionsystem/iplugin.h>

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief SailfishOSWizardsPlugin class is a plugin that registers new pages types
 * for JSON-based wizards.
 */
class SailfishOSWizardsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SailfishOSWizards.json")

public:
    SailfishOSWizardsPlugin();
    ~SailfishOSWizardsPlugin() Q_DECL_OVERRIDE;

    bool initialize(const QStringList &arguments, QString *errorString) Q_DECL_OVERRIDE;
    void extensionsInitialized() Q_DECL_OVERRIDE;
    ShutdownFlag aboutToShutdown() Q_DECL_OVERRIDE;
};

} // namespace Internal
} // namespace SailfishOSWizards

#endif // SAILFISHOSWIZARDS_H
