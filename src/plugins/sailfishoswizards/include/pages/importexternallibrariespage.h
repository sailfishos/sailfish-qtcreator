/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef IMPORTEXTERNALLIBRARIESPAGE_H
#define IMPORTEXTERNALLIBRARIESPAGE_H

#include <utils/wizardpage.h>
#include <projectexplorer/jsonwizard/jsonwizard.h>

#include "models/externallibrarylistmodel.h"
#include "ui_importexternallibrariespage.h"
#include "ui_selectitemsdialog.h"

namespace SailfishOSWizards {
namespace Internal {

using namespace ProjectExplorer;

/*!
 * \brief ImportExternalLibrariesPage class is a page to import standard external libraries
 * for the created project or file.
 */
class ImportExternalLibrariesPage : public Utils::WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString qmlLibrariesImports READ qmlLibrariesImports)

public:
    explicit ImportExternalLibrariesPage();

    QString qmlLibrariesImports() const;

private:
    Ui::ImportExternalLibrariesPage m_pageUi;
    Ui::SelectItemsDialog m_selectLibrariesDialogUi;
    QDialog m_selectLibrariesDialog;
    ExternalLibraryListModel *m_allLibraries;
    ExternalLibraryListModel *m_selectedLibraries;

    void openSelectLibrariesDialog();
    void removeSelectedLibraryFromList();
    void addSelectedLibrary(const QModelIndex &index);
    void addMultimpleSelectedLibraries();
    void addExternalLibrary(const int index);
};

}
}

#endif // IMPORTEXTERNALLIBRARIESPAGE_H
