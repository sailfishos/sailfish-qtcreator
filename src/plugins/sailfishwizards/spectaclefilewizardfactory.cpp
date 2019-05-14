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

#include "spectaclefilewizardfactory.h"
#include "spectaclefilewizardpages.h"
#include "sailfishwizardsconstants.h"
#include "common.h"

#include <QTextCodec>
#include <QFile>
#include <QDebug>
#include <QScreen>
#include <QJsonDocument>
#include <QJsonObject>

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief Constructs, sets the parameters of the wizard to create a spectacle yaml file.
 */
SpectacleFileWizardFactory::SpectacleFileWizardFactory()
{
    setId("SpectacleFileWizard");
    setIconText("Spectacle");
    setDisplayName(tr("Spectacle yaml-file"));
    setDescription(tr("The wizard allows to create the Spectacle configuration file in YAML format."));
    setCategory("Sailfish");
    setDisplayCategory("Sailfish");
    setFlags(IWizardFactory::WizardFlag::PlatformIndependent);
}

/*!
 * \brief Platform availability.
 * Returns \c true if the platform is available, otherwise \c false.
 * \param platformId platform identifier.
 */
bool SpectacleFileWizardFactory::isAvailable(Id platformId) const
{
    Q_UNUSED(platformId);
    return true;
}

/*!
 * \brief Creates \c BaseFileWizard and sets the wizard pages.
 * Returns the \c BaseFileWizard with the specified pages.
 * \param parent The parent object instance.
 * \param parameters Contains an accessible set of parameters for the wizard.
 */
BaseFileWizard *SpectacleFileWizardFactory::create(QWidget *parent,
                                                   const WizardDialogParameters &parameters) const
{
    BaseFileWizard *result = new BaseFileWizard(this, parameters.extraValues(), parent);
    QList<DependencyListModel *> *wizardLists = new QList<DependencyListModel *>({new DependencyListModel, new DependencyListModel, new DependencyListModel});
    result->addPage(new SpectacleFileSelectionPage(parameters.defaultPath()));
    result->addPage(new SpectacleFileOptionsPage(parameters.defaultPath()));
    result->addPage(new SpectacleFileProjectTypePage(wizardLists));
    result->addPage(new SpectacleFileDependenciesPage(wizardLists));
    QListIterator<QWizardPage *> extra_pages(result->extensionPages());
    while (extra_pages.hasNext()) {
        result->addPage(extra_pages.next());
    }
    QRect window = QGuiApplication::primaryScreen()->geometry();
    result->setMinimumSize(static_cast<int>(window.width() * 0.5),
                           static_cast<int>(window.height() * 0.5));
    result->setWindowTitle(displayName());
    return result;
}

/*!
 * \brief Sets the files to create.
 * Returns a set of files to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFiles SpectacleFileWizardFactory::generateFiles(const QWizard *wizard,
                                                         QString *errorMessage) const
{
    GeneratedFiles files;

    errorMessage->clear();
    GeneratedFile specConfigFile = generateFile(wizard, errorMessage);
    if (!errorMessage->isEmpty()) {
        return files;
    }
    files << specConfigFile;
    return files;
}

/*!
 * \brief Returns mime type of the yaml file.
 * \return
 */
QString SpectacleFileWizardFactory::yamlSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::YAML_MIMETYPE);
}

/*!
 * \brief Fills the template for the yaml file.
 * Returns a set of yaml file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFile SpectacleFileWizardFactory::generateFile(const QWizard *wizard,
                                                       QString *errorMessage) const
{
    GeneratedFile spectacleFile;
    QHash <QString, QString> values;
    values.insert("configname", wizard->field("fileName").toString());
    values.insert("summary", wizard->field("summary").toString());
    values.insert("description", wizard->field("description").toString().replace("\n",
                                                                                 "\n  ").trimmed());
    values.insert("URL", wizard->field("URL").toString());
    values.insert("configversion", wizard->field("version").toString());
    values.insert("license", wizard->field("licenseEdit").toString());
    QString strRequirements[3];
    SpectacleFileDependenciesPage *dependencyPage = static_cast<SpectacleFileDependenciesPage *>
                                                    (wizard->page(PageId::DEPENDENCIES));
    QList<QList<DependencyRecord>> wizardLists = {dependencyPage->dependencies(PKG_BR), dependencyPage->dependencies(PKG_CONFIG_BR), dependencyPage->dependencies(REQUIRES)};
    if (!wizardLists.isEmpty()) {
        for (int i = 0; i < wizardLists.size(); i++)
            for (int j = 0; j < wizardLists[i].size(); j++)
                strRequirements[i] += "  - " + wizardLists[i].value(j).toString() + "\n";
    }
    values.insert("pkgbr", strRequirements[PKG_BR]);
    values.insert("pkgconfigbr", strRequirements[PKG_CONFIG_BR]);
    values.insert("requires", strRequirements[REQUIRES]);
    QString fileContents = Common::processTemplate("spectacleconfig.yaml", values, errorMessage);
    if (!errorMessage->isEmpty()) {
        return spectacleFile;
    }
    QString filePath = buildFileName(wizard->field("filePath").toString(),
                                     wizard->field("fileName").toString(), yamlSuffix());
    spectacleFile.setPath(filePath);
    spectacleFile.setContents(fileContents);
    return spectacleFile;
}

} // namespace Internal
} // namespace SailfishWizards
