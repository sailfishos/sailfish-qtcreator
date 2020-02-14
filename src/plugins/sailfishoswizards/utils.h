/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef UTILS_H
#define UTILS_H

#include "models/externallibrarylistmodel.h"

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief Utils class is a class with utility static functions.
 */
class Utils
{

public:
    static ExternalLibraryListModel *loadExternalLibraries(const QString &modulesJsonFilePath);

private:
    static QString readFile(const QString &filePath);
    static QStringList jsonArrayToStringList(QJsonArray jsonArray, QRegExp regExp = QRegExp());
};

}
}

#endif // UTILS_H
