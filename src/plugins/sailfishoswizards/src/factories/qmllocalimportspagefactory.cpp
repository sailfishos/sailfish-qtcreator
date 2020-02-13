/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "factories/qmllocalimportspagefactory.h"

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <utils/qtcassert.h>

#include "pages/qmllocalimportspage.h"

namespace SailfishOSWizards {
namespace Internal {

QmlLocalImportsPageFactory::QmlLocalImportsPageFactory()
{
    setTypeIdsSuffix(QLatin1String("QmlLocalImports"));
}

Utils::WizardPage *QmlLocalImportsPageFactory::create(JsonWizard *wizard, Core::Id typeId,
                                                      const QVariant &data)
{
    Q_UNUSED(wizard)
    Q_UNUSED(data)
    QTC_ASSERT(canCreate(typeId), return nullptr);
    return new QmlLocalImportsPage;
}

bool QmlLocalImportsPageFactory::validateData(Core::Id typeId, const QVariant &data,
                                              QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);
    if (!data.isNull() && (data.type() != QVariant::Map || !data.toMap().isEmpty())) {
        *errorMessage = QCoreApplication::translate(
                    "ProjectExplorer::JsonWizard",
                    "\"data\" for a \"QmlLocalImports\" page needs to be unset or an empty object.");
        return false;
    }
    return true;
}

}
}
