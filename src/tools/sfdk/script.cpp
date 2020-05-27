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

#include <utils/optional.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace Sfdk;

namespace Sfdk {

namespace {
const char JS_CONFIGURATION_EXTENSION_NAME[] = "configuration";
const char JS_MODULE_EXTENSION_NAME[] = "module";
const char JS_UTILS_EXTENSION_NAME[] = "utils";
} // namespace anonymous

class JSUtilsExtension : public QObject
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

class JSConfigurationExtension : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    Q_INVOKABLE bool isOptionSet(const QString &optionName) const
    {
        return optionEffectiveState(optionName).has_value();
    }

    Q_INVOKABLE QString optionArgument(const QString &optionName) const
    {
        QTC_ASSERT(isOptionSet(optionName), return {});
        return optionEffectiveState(optionName)->argument();
    }

private:
    Utils::optional<OptionEffectiveOccurence> optionEffectiveState(const QString &optionName) const
    {
        const Option *const option = Dispatcher::option(optionName);
        QTC_ASSERT(option, return {});
        return Configuration::effectiveState(option);
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

    globalObject().setProperty(JS_UTILS_EXTENSION_NAME,
            newQObject(new JSUtilsExtension));
    globalObject().setProperty(JS_CONFIGURATION_EXTENSION_NAME,
            newQObject(new JSConfigurationExtension));
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

QJSValue JSEngine::call(const QString &functionName, const QJSValueList &args,
        const Module *context, TypeValidator returnTypeValidator)
{
    QJSValue function = evaluate(functionName, context);

    if (function.isError()) {
        const QString errorFileName = function.property("fileName").toString();
        const int errorLineNumber = function.property("lineNumber").toInt();
        qCCritical(sfdk) << "Error dereferencing" << functionName
            << "in the context of" << context->fileName << "module:"
            << errorFileName << ":" << errorLineNumber << ":" << function.toString();
        return newErrorObject(QJSValue::GenericError, tr("Internal error"));
    }

    if (!function.isCallable()) {
        qCCritical(sfdk) << "Error dereferencing" << functionName
            << "in the context of" << context->fileName << "module:"
            << "The result is not callable";
        return newErrorObject(QJSValue::GenericError, tr("Internal error"));
    }

    const QJSValue result = function.call(args);

    if (result.isError()) {
        const QString errorFileName = function.property("fileName").toString();
        const int errorLineNumber = function.property("lineNumber").toInt();
        qCCritical(sfdk) << "Error calling" << functionName
            << "in the context of" << context->fileName << "module:"
            << errorFileName << ":" << errorLineNumber << ":" << result.toString();
        return result;
    }

    QString errorString;
    if (returnTypeValidator && !returnTypeValidator(result, &errorString)) {
        qCCritical(sfdk) << "Error calling" << functionName
            << "in the context of" << context->fileName << "module:"
            << "Unexpected return value:" << errorString;
        return newErrorObject(QJSValue::GenericError, tr("Internal error"));
    }

    return result;
}

#include "script.moc"
