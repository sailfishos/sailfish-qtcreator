/****************************************************************************
**
** Copyright (C) 2012-2015,2017,2019 Jolla Ltd.
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

#include "merdevicexmlparser.h"

#include <utils/fileutils.h>

#include <QAbstractMessageHandler>
#include <QAbstractXmlReceiver>
#include <QFileInfo>
#include <QStack>
#include <QXmlQuery>
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#include <QXmlStreamWriter>

using namespace Utils;

//Parser for:
//https://wiki.merproject.org/wiki/SDK_on_VirtualBox/Design

//<devices>
// <engine name="Sailfish OS Build Engine" type="vbox">
//  <subnet>10.220.220</subnet
// </engine>
// <device name="Nemo N9" type="real">
//  <ip>192.168.0.12</ip>
//  <sshkeypath></sshkeypath>
// </device>
// <device name="Sailfish OS Emulator" type="vbox">
//  <index>2</index>
//  <subnet>10.220.220</subnet>
//  <sshkeypath></sshkeypath>
//  <mac>08:00:27:7C:A1:AF</mac>
// </device>
//</devices>

const char DEVICE[] = "device";
const char DEVICES[] = "devices";
const char NAME[] = "name";
const char TYPE[] = "type";
const char ENGINE[] = "engine";
const char IP[] = "ip";
const char SSH_PATH[] = "sshkeypath";
const char SUBNET[] = "subnet";
const char MAC[] = "mac";
const char INDEX[] = "index";
const char SSH_PORT[] = "sshport";

namespace Mer {

void MerDeviceData::clear()
{
    m_name.clear();
    m_type.clear();
    m_ip.clear();
    m_subNet.clear();
    m_mac.clear();
    m_sshKeyPath.clear();
}

void MerEngineData::clear()
{
    m_name.clear();
    m_type.clear();
    m_subNet.clear();
}

class MerDevicesXmlWriterPrivate
{
public:
    MerDevicesXmlWriterPrivate(const QString &fileName) : fileSaver(fileName, QIODevice::WriteOnly) {}

    FileSaver fileSaver;
};

MerDevicesXmlWriter::MerDevicesXmlWriter(const QString &fileName,
                                         const QList<MerDeviceData> &devices,  const MerEngineData &engine, QObject *parent)
    : QObject(parent),
      d(new MerDevicesXmlWriterPrivate(fileName))
{
    QByteArray data;
    QXmlStreamWriter writer(&data);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(QLatin1String(DEVICES));
    writer.writeStartElement(QLatin1String(ENGINE));
    writer.writeAttribute(QLatin1String(NAME), engine.m_name);
    writer.writeAttribute(QLatin1String(TYPE), engine.m_type);
    writer.writeStartElement(QLatin1String(SUBNET));
    writer.writeCharacters(engine.m_subNet);
    writer.writeEndElement(); // subnet
    writer.writeEndElement(); // eninge
    foreach (const MerDeviceData &data, devices) {
        writer.writeStartElement(QLatin1String(DEVICE));
        writer.writeAttribute(QLatin1String(NAME), data.m_name);
        writer.writeAttribute(QLatin1String(TYPE), data.m_type);
        if(!data.m_index.isEmpty()) {
            writer.writeStartElement(QLatin1String(INDEX));
            writer.writeCharacters(data.m_index);
            writer.writeEndElement(); // index
        }
        if(!data.m_ip.isEmpty()) {
            writer.writeStartElement(QLatin1String(IP));
            writer.writeCharacters(data.m_ip);
            writer.writeEndElement(); // ip
        }
        if(!data.m_subNet.isEmpty()) {
            writer.writeStartElement(QLatin1String(SUBNET));
            writer.writeCharacters(data.m_subNet);
            writer.writeEndElement(); // subnet
        }
        if(!data.m_sshKeyPath.isEmpty()) {
            writer.writeStartElement(QLatin1String(SSH_PATH));
            writer.writeCharacters(data.m_sshKeyPath);
            writer.writeEndElement(); // ssh
        }
        if(!data.m_mac.isEmpty()) {
            writer.writeStartElement(QLatin1String(MAC));
            writer.writeCharacters(data.m_mac);
            writer.writeEndElement(); // mac
        }
        if(!data.m_sshPort.isEmpty()) {
            writer.writeStartElement(QLatin1String(SSH_PORT));
            writer.writeCharacters(data.m_sshPort);
            writer.writeEndElement(); // sshport
        }
        writer.writeEndElement(); // device
    }
    {
        // This is a hack. This hack allows external modifications to the xml file for devices
        // Only devices that are of type=vbox are overwritten.
        // REMOVE THESE LINES
        // STARTS HERE
        FileReader fileReader;
        if (fileReader.fetch(fileName)) {
            QXmlStreamReader xmlReader(fileReader.data());
            while (!xmlReader.atEnd()) {
                xmlReader.readNext();
                if (xmlReader.isStartElement() && xmlReader.name() == QLatin1String(DEVICE)) {
                    QXmlStreamAttributes attributes = xmlReader.attributes();
                    if (attributes.value(QLatin1String(TYPE)) == QLatin1String("custom")) {
                        writer.writeStartElement(QLatin1String(DEVICE));
                        writer.writeAttributes(attributes);
                        while (xmlReader.readNext()) {
                            if (xmlReader.isStartElement()) {
                                writer.writeStartElement(xmlReader.name().toString());
                                attributes = xmlReader.attributes();
                                writer.writeAttributes(attributes);
                            } else if (xmlReader.isEndElement()) {
                                writer.writeEndElement();
                                if (xmlReader.name() == QLatin1String(DEVICE))
                                    break;
                            } else if (xmlReader.isCharacters()) {
                                writer.writeCharacters(xmlReader.text().toString());
                            }
                        }
                    }
                }
            }
        }
        // ENDS HERE
    }
    writer.writeEndElement(); // devices
    writer.writeEndDocument();
    d->fileSaver.write(data);
    d->fileSaver.finalize();
}

MerDevicesXmlWriter::~MerDevicesXmlWriter()
{
    delete d;
}

bool MerDevicesXmlWriter::hasError() const
{
    return d->fileSaver.hasError();
}

QString MerDevicesXmlWriter::errorString() const
{
    return d->fileSaver.errorString();
}

} // Mer
