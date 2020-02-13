/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef QMLLOCALIMPORTSPAGEFACTORY_H
#define QMLLOCALIMPORTSPAGEFACTORY_H

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

namespace SailfishOSWizards {
namespace Internal {

using namespace ProjectExplorer;

/*!
 * \brief QmlLocalImportsPageFactory class is a factory for creating and registering
 * the 'QmlLocalImports' page type for JSON-based wizards.
 */
class QmlLocalImportsPageFactory : public JsonWizardPageFactory
{

public:
    QmlLocalImportsPageFactory();

    Utils::WizardPage *create(JsonWizard *wizard, Core::Id typeId, const QVariant &data) Q_DECL_OVERRIDE;
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) Q_DECL_OVERRIDE;
};

}
}

#endif // QMLLOCALIMPORTSPAGEFACTORY_H
