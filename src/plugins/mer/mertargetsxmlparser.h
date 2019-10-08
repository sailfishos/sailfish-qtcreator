/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#ifndef MERTARGETSXMLPARSER_H
#define MERTARGETSXMLPARSER_H

#include <QMetaType>
#include <QObject>

namespace Utils { class FileName; }

namespace Mer {

class MerTargetData;
class MerTargetsXmlReaderPrivate;
class MerTargetsXmlReader : public QObject
{
public:
    MerTargetsXmlReader(const QString &fileName, QObject *parent = 0);
    ~MerTargetsXmlReader() override;

    bool hasError() const;
    QString errorString() const;

    QList<MerTargetData> targetData() const;
    int version() const;

private:
    MerTargetsXmlReaderPrivate *d;
};

class MerTargetsXmlWriterPrivate;
class MerTargetsXmlWriter : public QObject
{
public:
    MerTargetsXmlWriter(const QString &fileName, int version,
                        const QList<MerTargetData> &targetData, QObject *parent = 0);
    ~MerTargetsXmlWriter() override;

    bool hasError() const;
    QString errorString() const;

private:
    MerTargetsXmlWriterPrivate *d;
};

class MerTargetData
{
public:
    void clear();

    QString name;
    QString gccDumpMachine;
    QString gccDumpMacros;
    QString gccDumpIncludes;
    QString qmakeQuery;
    QString rpmValidationSuites;
};

} // Mer

Q_DECLARE_METATYPE(Mer::MerTargetData)

#endif // MERTARGETSXMLPARSER_H
