/****************************************************************************
**
** Copyright (C) 2012-2016,2018-2019 Jolla Ltd.
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

#include "targetsxmlreader_p.h"

#include "buildengine_p.h"

#include <utils/fileutils.h>

#include <QAbstractMessageHandler>
#include <QAbstractXmlReceiver>
#include <QDir>
#include <QFileInfo>
#include <QStack>
#include <QXmlQuery>
#include <QXmlSchema>
#include <QXmlSchemaValidator>

using namespace Utils;

namespace Sfdk {

namespace {

const char TARGET[] = "target";
const char TARGETS[] = "targets";
const char OUTPUT[] = "output";
const char NAME[] = "name";
const char VERSION[] = "version";
const char GCCDUMPMACHINE[] = "GccDumpMachine";
const char GCCDUMPMACROS[] = "GccDumpMacros";
const char GCCDUMPINCLUDES[] = "GccDumpIncludes";
const char QMAKEQUERY[] = "QmakeQuery";
const char RPMVALIDATIONSUITES[] = "RpmValidationSuites";

class MessageHandler : public QAbstractMessageHandler
{
public:
    QString errorString() const
    {
        return QObject::tr("Error %1:%2:%3 %4").arg(m_identifier.toString())
                .arg(m_sourceLocation.line())
                .arg(m_sourceLocation.column())
                .arg(m_description);
    }

protected:
    void handleMessage(QtMsgType /*type*/, const QString &description,
                       const QUrl &identifier, const QSourceLocation &sourceLocation) override
    {
        m_description = description;
        m_identifier = identifier;
        m_sourceLocation = sourceLocation;
    }

private:
    QString m_description;
    QSourceLocation m_sourceLocation;
    QUrl m_identifier;
};

class XmlReceiver : public QAbstractXmlReceiver
{
public:
    XmlReceiver(const QXmlNamePool &namePool) : m_namePool(namePool), m_version(-1) {}

    int version() const
    {
        return m_version;
    }

    QList<BuildTargetDump> targets() const
    {
        return m_targets;
    }

protected:
    void startElement(const QXmlName &name) override
    {
        const QString element = name.localName(m_namePool);
        if (element == QLatin1String(TARGET))
            m_currentTarget = {};
        m_currentElementStack.push(element);
    }

    void endElement() override
    {
        if (m_currentElementStack.pop() == QLatin1String(TARGET))
            m_targets.append(m_currentTarget);
    }

    void attribute(const QXmlName &name,
                   const QStringRef &value) override
    {
        const QString attributeName = name.localName(m_namePool);
        const QString attributeValue = value.toString();
        const QString element = m_currentElementStack.top();
        if (element == QLatin1String(TARGETS) && attributeName == QLatin1String(VERSION))
            m_version = attributeValue.toInt();
        else if (element == QLatin1String(TARGET) && attributeName == QLatin1String(NAME))
            m_currentTarget.name = attributeValue;
        else if (element == QLatin1String(OUTPUT) && attributeName == QLatin1String(NAME))
            m_attributeValue = attributeValue;
        else
            m_attributeValue.clear();
    }

    void characters(const QStringRef &value) override
    {
        if (m_currentElementStack.top() == QLatin1String(OUTPUT)) {
            if (m_attributeValue == QLatin1String(GCCDUMPMACHINE))
                m_currentTarget.gccDumpMachine = value.toString();
            else if (m_attributeValue == QLatin1String(GCCDUMPMACROS))
                m_currentTarget.gccDumpMacros = value.toString();
            else if (m_attributeValue == QLatin1String(GCCDUMPINCLUDES))
                m_currentTarget.gccDumpIncludes = value.toString();
            else if (m_attributeValue == QLatin1String(QMAKEQUERY))
                m_currentTarget.qmakeQuery = value.toString();
            else if (m_attributeValue == QLatin1String(RPMVALIDATIONSUITES))
                m_currentTarget.rpmValidationSuites = value.toString();
        }
    }

    void comment(const QString &/*value*/) override
    {
    }

    void startDocument() override
    {
    }

    void endDocument() override
    {
    }

    void processingInstruction(const QXmlName &/*target*/, const QString &/*value*/) override
    {
    }

    void atomicValue(const QVariant &/*value*/) override
    {
    }

    void namespaceBinding(const QXmlName &/*name*/) override
    {
    }

    void startOfSequence() override
    {
    }

    void endOfSequence() override
    {
    }

private:
    QXmlNamePool m_namePool;
    QStack<QString> m_currentElementStack;
    QList<BuildTargetDump> m_targets;
    BuildTargetDump m_currentTarget;
    QString m_attributeValue;
    int m_version;
};

#ifdef Q_OS_MAC
#  define SHARE_PATH "Resources"
#else
#  define SHARE_PATH "share/qtcreator"
#endif

#define LIBEXEC_PREFIX "/libexec/"

static QString applicationDirPath()
{
    return QCoreApplication::applicationDirPath();
}

static inline QDir sharedDir()
{
    QDir retv = QDir(applicationDirPath());

    // cd up to installation prefix
#ifdef Q_OS_LINUX
    // This has to be decided at runtime since the app can be either Qt Creator
    // itself (installed under bin/) or a tool like sdktool (installed under
    // libexec/qtcreator)
    if (retv.absolutePath().contains(QLatin1String(LIBEXEC_PREFIX)))
        retv.cdUp();
#endif
    retv.cdUp();

    retv.cd(QLatin1String(SHARE_PATH));

    return retv;
}

} // namespace anonymous

class TargetsXmlReaderPrivate
{
public:
    TargetsXmlReaderPrivate()
        : receiver(new XmlReceiver(query.namePool())),
          error(false)
    {}

    ~TargetsXmlReaderPrivate()
    {
        delete receiver;
    }

    QXmlQuery query;
    XmlReceiver *receiver;
    MessageHandler messageHandler;
    bool error;
    QString errorString;
};

TargetsXmlReader::TargetsXmlReader(const QString &fileName, QObject *parent)
    : QObject(parent),
      d_ptr(std::make_unique<TargetsXmlReaderPrivate>())
{
    Q_D(TargetsXmlReader);

    FileReader reader;
    d->error = !reader.fetch(fileName, QIODevice::ReadOnly);
    if (d->error) {
        d->errorString = reader.errorString();
        return;
    }

    QXmlSchema schema;
    schema.setMessageHandler(&d->messageHandler);

    FileReader schemeReader;
    d->error = !schemeReader.fetch(sharedDir().filePath(QLatin1String("mer/targets.xsd")),
            QIODevice::ReadOnly);
    if (d->error) {
        d->errorString = schemeReader.errorString();
        return;
    }
    schema.load(schemeReader.data());
    d->error = !schema.isValid();
    if (d->error) {
        d->errorString = d->messageHandler.errorString();
        return;
    }

    QXmlSchemaValidator validator(schema);
    validator.setMessageHandler(&d->messageHandler);
    d->error = !validator.validate(reader.data());
    if (d->error) {
        d->errorString = d->messageHandler.errorString();
        return;
    }

    QUrl docfile = QUrl::fromLocalFile(fileName);
    d->query.setQuery(QString::fromLatin1("doc('%1')").arg(docfile.toString()));
    d->query.setMessageHandler(&d->messageHandler);
    d->error = !d->query.isValid();
    if (d->error) {
        d->errorString = d->messageHandler.errorString();
        return;
    }

    d->error = !d->query.evaluateTo(d->receiver);
    if (d->error)
        d->errorString = d->messageHandler.errorString();
}

TargetsXmlReader::~TargetsXmlReader() = default;

bool TargetsXmlReader::hasError() const
{
    return d_func()->error;
}

QString TargetsXmlReader::errorString() const
{
    return d_func()->errorString;
}

QList<BuildTargetDump> TargetsXmlReader::targets() const
{
    return d_func()->receiver->targets();
}

int TargetsXmlReader::version() const
{
    return d_func()->receiver->version();
}

} // namespace Sfdk
