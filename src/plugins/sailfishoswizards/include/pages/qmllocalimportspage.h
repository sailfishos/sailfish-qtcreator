/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef QMLLOCALIMPORTSPAGE_H
#define QMLLOCALIMPORTSPAGE_H

#include <utils/wizardpage.h>
#include <projectexplorer/jsonwizard/jsonwizard.h>

#include "ui_qmllocalimportspage.h"

namespace SailfishOSWizards {
namespace Internal {

using namespace ProjectExplorer;

/*!
 * brief QmlLocalImportsPage class is a page for adding local imports for the QML-file.
 * The page is used on the wizard for creating new QML-file.
 */
class QmlLocalImportsPage : public Utils::WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString localImports READ localImports)

public:
    explicit QmlLocalImportsPage();

    void initializePage() Q_DECL_OVERRIDE;

    QString localImports() const;

private:
    JsonWizard *m_wizard;
    QString m_path;
    QStringList m_importsList;
    Ui::QmlLocalImportsPage m_pageUi;

    void openFileDialog();
    void openQmldirDialog();
    void openDirDialog();

    void addImports(QStringList localImports);
    void addImport(const QString &import);
    void removeSelectedImport();
};

}
}

#endif // QMLLOCALIMPORTSPAGE_H
