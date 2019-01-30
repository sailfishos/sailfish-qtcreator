/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#ifndef MERDEVICEXMLPARSER_H
#define MERDEVICEXMLPARSER_H

#include <QMetaType>
#include <QObject>

namespace Utils { class FileName; }

namespace Mer {

class MerDeviceData;
class MerEngineData;
class MerDevicesXmlReaderPrivate;
class MerDevicesXmlReader : public QObject
{
public:
    MerDevicesXmlReader(const QString &fileName, QObject *parent = 0);
    ~MerDevicesXmlReader() override;

    bool hasError() const;
    QString errorString() const;

    QList<MerDeviceData> deviceData() const;
    MerEngineData engineData() const;

private:
    MerDevicesXmlReaderPrivate *d;

};

class MerDevicesXmlWriterPrivate;
class MerDevicesXmlWriter : public QObject
{
public:
    MerDevicesXmlWriter(const QString &fileName, const QList<MerDeviceData> &deviceData, const MerEngineData &engineData, QObject *parent = 0);
    ~MerDevicesXmlWriter() override;

    bool hasError() const;
    QString errorString() const;
private:
    MerDevicesXmlWriterPrivate *d;

};

class MerDeviceData
{
public:
    void clear();

    QString m_ip;
    QString m_sshKeyPath;
    QString m_mac;
    QString m_subNet;
    QString m_name;
    QString m_type;
    QString m_index;
    QString m_sshPort;
};

class MerEngineData
{
public:
    void clear();
    QString m_name;
    QString m_type;
    QString m_subNet;
};

} // Mer

Q_DECLARE_METATYPE(Mer::MerDeviceData)

#endif
