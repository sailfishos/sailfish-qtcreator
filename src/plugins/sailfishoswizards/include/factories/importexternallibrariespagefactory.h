/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef IMPORTEXTERNALLIBRARIESPAGEFACTORY_H
#define IMPORTEXTERNALLIBRARIESPAGEFACTORY_H

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

namespace SailfishOSWizards {
namespace Internal {

using namespace ProjectExplorer;

/*!
 * \brief ImportExternalLibrariesPageFactory class is a factory for creating and registering
 * the 'ExternalLibraries' page type for JSON-based wizards.
 */
class ImportExternalLibrariesPageFactory : public JsonWizardPageFactory
{

public:
    ImportExternalLibrariesPageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Core::Id typeId,
                              const QVariant &data) Q_DECL_OVERRIDE;
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) Q_DECL_OVERRIDE;
};

}
}

#endif // IMPORTEXTERNALLIBRARIESPAGEFACTORY_H
