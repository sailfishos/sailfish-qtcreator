/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "utils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTextCodec>

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief Reads a JSON_file with description of external libraries, writes the read data into
 * the ExternalLibraryListModel instance and return the instance pointer.
 * \param modulesJsonFilePath Path to the JSON-file with description of external libraries.
 * \return ExternalLibraryListModel instance pointer.
 */
ExternalLibraryListModel *Utils::loadExternalLibraries(const QString &modulesJsonFilePath)
{
    ExternalLibraryListModel *model = new ExternalLibraryListModel;
    ExternalLibrary newExternalLibrary;
    QString jsonFileString = readFile(modulesJsonFilePath);
    QJsonObject jsonObject = QJsonDocument::fromJson(jsonFileString.toUtf8()).object();
    for (QJsonObject::Iterator iter = jsonObject.begin(); iter != jsonObject.end(); ++iter) {
        newExternalLibrary.setName(iter.key());
        QJsonObject obj = iter.value().toObject();
        newExternalLibrary.setLayoutName(obj["layoutname"].toString());
        newExternalLibrary.setQtList(jsonArrayToStringList(obj["qt"].toArray()));
        newExternalLibrary.setYamlHeader(obj["yamlheader"].toString());
        newExternalLibrary.setSpecHeader(obj["specheader"].toString());
        newExternalLibrary.setDependencyList(jsonArrayToStringList(obj["dependencies"].toArray(),
                                                                   QRegExp(" ")));
        newExternalLibrary.setTypesList(jsonArrayToStringList(obj["types"].toArray()));
        model->addExternalLibrary(newExternalLibrary);
    }
    return model;
}

/*!
 * \brief Reads a file by the given path and returns its content.
 * \param filePath Path of file to read.
 * \return QString with file content.
 */
QString Utils::readFile(const QString &filePath)
{
    QFile file(filePath);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString fileContent = QTextCodec::codecForName("UTF-8")->toUnicode(file.readAll());
    file.close();
    return fileContent;
}

/*!
 * \brief Converts the given QJsonArray to QStringList.
 * \param jsonArray QJsonArray object.
 * \param regExp QRegExp object used if it is needed to split JSON strings by regular expression.
 * \return QStringList with strings of JSON data.
 */
QStringList Utils::jsonArrayToStringList(QJsonArray jsonArray, QRegExp regExp)
{
    QStringList strList;
    for (int i = 0; i < jsonArray.count(); i++) {
        QString str = jsonArray[i].toString();
        if (!regExp.isEmpty())
            str = str.split(regExp).value(0);
        strList << str;
    }
    return strList;
}

}
}
