/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator modul for adding dependencies to file.
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

#pragma once

#include <QString>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include "dependencymodel.h"

namespace SailfishWizards {
namespace Internal {
namespace Qmldir {
void addFileInQmldir(QString path, QString name, QString fullname, QWidget *page);
} // namespace Qmldir
namespace SailfishProjectDependencies {
void addDependencyInProFile(const QString &path, DependencyModel *selectExternalLibraryList,
                            QWidget *page);
void addDependecyInYamlFile(const QString &dir, DependencyModel *selectExternalLibraryList, QWidget *page);
void addDependecyInSpecFile(const QString &dir, DependencyModel *selectExternalLibraryList, QWidget *page);
} // namespace SailfishProjectDependencies
} // namespace Internal
} // namespace SailfishWizards
