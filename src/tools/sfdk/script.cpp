/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "script.h"

#include "dispatch.h"
#include "sfdkglobal.h"

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/sdk.h>

#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace Sfdk;

namespace Sfdk {

namespace {
const char JS_EXTENSION_NAME[] = "utils";
const char JS_MODULE_EXTENSION_NAME[] = "module";
} // namespace anonymous

class JSExtension : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE bool isDevice(const QString &deviceName) const
    {
        for (Device *const device : Sdk::devices()) {
            if (device->name() == deviceName)
                return true;
        }
        return false;
    }

    Q_INVOKABLE bool isBuildTarget(const QString &buildTargetName) const
    {
        if (!SdkManager::hasEngine()) {
            qCWarning(sfdk).noquote() << SdkManager::noEngineFoundMessage();
            return false;
        }

        return SdkManager::engine()->buildTargetNames().contains(buildTargetName);
    }

    Q_INVOKABLE bool exists(const QString &fileName) const
    {
        return QFileInfo::exists(fileName);
    }

    Q_INVOKABLE bool isDirectory(const QString &fileName) const
    {
        return QFileInfo(fileName).isDir();
    }

    Q_INVOKABLE bool isFile(const QString &fileName) const
    {
        return QFileInfo(fileName).isFile();
    }
};

} // namespace Sfdk

/*!
 * \class JSEngine
 */

JSEngine::JSEngine(QObject *parent)
    : QJSEngine(parent)
{
    installExtensions(QJSEngine::TranslationExtension | QJSEngine::ConsoleExtension);

    const QJSValue extension = newQObject(new JSExtension);
    globalObject().setProperty(JS_EXTENSION_NAME, extension);
}

QJSValue JSEngine::evaluate(const QString &program, const Module *context)
{
    QTC_CHECK(context->fileName.endsWith(".json"));
    const QString moduleExtensionFileName = context->fileName.chopped(5) + ".mjs";

    if (QFileInfo(moduleExtensionFileName).exists()) {
        const QJSValue moduleExtension = importModule(moduleExtensionFileName);
        if (moduleExtension.isError())
            return moduleExtension;
        globalObject().setProperty(JS_MODULE_EXTENSION_NAME, moduleExtension);
    } else {
        const bool deleteOk = globalObject().deleteProperty(JS_MODULE_EXTENSION_NAME);
        QTC_CHECK(deleteOk);
    }

    return QJSEngine::evaluate(program);
}

#include "script.moc"
