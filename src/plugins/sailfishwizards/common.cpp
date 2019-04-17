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

#include "common.h"
#include <utils/templateengine.h>
#include <QDir>
#include <QFile>
#include <QTextStream>

const QStringList iconResolutions = {"86x86", "108x108", "128x128", "172x172"};

namespace SailfishWizards {
namespace Internal {
namespace Common {

/*!
 * \brief Fills in the \a fields according to the template found by the \a templateName in the plugin resources.
 * Returns the file content built by template and \a fields information.
 * \param templateName Name of the template from the plugin resources.
 * \param fields Contains the names of template variables with the associated values.
 * \param errorMessage The output parameter in which the error message is stored.
 */
QString processTemplate(const QString &templateName, const QHash<QString, QString> &fields,
                        QString *errorMessage)
{
    const QString templatePath = QString(TEMPLATE_PATH_SPEC).arg(templateName);
    QFile file(templatePath);
    if (!file.open(QIODevice::ReadOnly)) {
        *errorMessage = QString("Unable to find template: '%1'").arg(templatePath);
        return QString();
    }
    QTextStream templateText(&file);

    Utils::MacroExpander expander;
    expander.registerExtraResolver([&fields](QString field, QString * result) -> bool {
        if (!fields.contains(field)) {
            return false;
        }
        *result = fields.value(field);
        return true;
    });

    return Utils::TemplateEngine::processText(&expander, templateText.readAll(), errorMessage);
}

/*!
 * \brief Returns the path to .pro file from dir .
 */
QString getProFilePath(const QString &applicationDir)
{
   QDir projectDir(applicationDir);
   QFileInfoList dirContent = projectDir.entryInfoList(QStringList() << "*.pro", QDir::Files);

   while (dirContent.isEmpty() && projectDir != QDir::root()) {
       projectDir.cd("..");
       dirContent = projectDir.entryInfoList(QStringList() << "*.pro", QDir::Files);
   }

   if (dirContent.count() == 1) {
       return dirContent.first().absoluteFilePath();
   } else {
       for (const QFileInfo &proFile : dirContent) {
           if (proFile.fileName() == projectDir.dirName())
               return proFile.absoluteFilePath();
       }
   }
   return QString();
}

QStringList getStandartIconResolutions()
{
    return iconResolutions;
}

} // namespace Common
} // namespace Internal
} // namespace SailfishWizards
