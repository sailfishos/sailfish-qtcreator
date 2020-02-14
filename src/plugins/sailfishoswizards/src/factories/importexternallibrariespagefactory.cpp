/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "factories/importexternallibrariespagefactory.h"

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <utils/qtcassert.h>

#include "pages/importexternallibrariespage.h"

namespace SailfishOSWizards {
namespace Internal {

ImportExternalLibrariesPageFactory::ImportExternalLibrariesPageFactory()
{
    setTypeIdsSuffix(QLatin1String("ImportExternalLibraries"));
}

Utils::WizardPage *ImportExternalLibrariesPageFactory::create(ProjectExplorer::JsonWizard *wizard,
                                                              Core::Id typeId, const QVariant &data)
{
    Q_UNUSED(wizard)
    Q_UNUSED(data)
    QTC_ASSERT(canCreate(typeId), return nullptr);
    return new ImportExternalLibrariesPage;
}

bool ImportExternalLibrariesPageFactory::validateData(Core::Id typeId, const QVariant &data,
                                                      QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map || !data.toMap().isEmpty())) {
        *errorMessage = QCoreApplication::translate(
                            "ProjectExplorer::JsonWizard",
                            "\"data\" for a \"ImportExternalLibraries\" page needs to be unset or an empty object.");
        return false;
    }
    return true;
}

}
}
