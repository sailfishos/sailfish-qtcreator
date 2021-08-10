/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"

#include <coreplugin/icore.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/hostosinfo.h>

#include <QSettings>

#ifdef WITH_TESTS
#   include <QTest>
#   include "webassemblyplugin.h"
#endif // WITH_TESTS

using namespace Utils;

namespace WebAssembly {
namespace Internal {

using EmSdkEnvCache = QCache<QString, QByteArray>;
Q_GLOBAL_STATIC_WITH_ARGS(EmSdkEnvCache, emSdkEnvCache, (10))
using EmSdkVersionCache = QCache<QString, QVersionNumber>;
Q_GLOBAL_STATIC_WITH_ARGS(EmSdkVersionCache, emSdkVersionCache, (10))

static QByteArray emSdkEnvOutput(const FilePath &sdkRoot)
{
    const QString cacheKey = sdkRoot.toString();
    if (!emSdkEnvCache()->contains(cacheKey)) {
        const QString scriptFile = sdkRoot.pathAppended(QLatin1String("emsdk_env") +
                                        (HostOsInfo::isWindowsHost() ? ".bat" : ".sh")).toString();
        QtcProcess emSdkEnv;
        if (HostOsInfo::isWindowsHost()) {
            emSdkEnv.setCommand(CommandLine(scriptFile));
        } else {
            // File needs to be source'd, not executed.
            emSdkEnv.setCommand({"bash", {"-c", ". " + scriptFile}});
        }
        emSdkEnv.start();
        if (!emSdkEnv.waitForFinished())
            return {};
        emSdkEnvCache()->insert(cacheKey, new QByteArray(emSdkEnv.readAllStandardError()));
    }
    return *emSdkEnvCache()->object(cacheKey);
}

static void parseEmSdkEnvOutputAndAddToEnv(const QByteArray &output, Environment &env)
{
    const QStringList lines = QString::fromLocal8Bit(output).split('\n');

    for (const QString &line : lines) {
        const QStringList prependParts = line.trimmed().split(" += ");
        if (prependParts.count() == 2)
            env.prependOrSetPath(prependParts.last());

        const QStringList setParts = line.trimmed().split(" = ");
        if (setParts.count() == 2) {
            if (setParts.first() != "PATH") // Path was already extended above
                env.set(setParts.first(), setParts.last());
            continue;
        }
    }
}

bool WebAssemblyEmSdk::isValid(const FilePath &sdkRoot)
{
    return !version(sdkRoot).isNull();
}

void WebAssemblyEmSdk::addToEnvironment(const FilePath &sdkRoot, Environment &env)
{
    if (sdkRoot.exists())
        parseEmSdkEnvOutputAndAddToEnv(emSdkEnvOutput(sdkRoot), env);
}

QVersionNumber WebAssemblyEmSdk::version(const FilePath &sdkRoot)
{
    if (!sdkRoot.exists())
        return {};
    const QString cacheKey = sdkRoot.toString();
    if (!emSdkVersionCache()->contains(cacheKey)) {
        Environment env = Environment::systemEnvironment();
        WebAssemblyEmSdk::addToEnvironment(sdkRoot, env);
        const QString scriptFile =
                QLatin1String("emcc") + QLatin1String(HostOsInfo::isWindowsHost() ? ".bat" : "");
        const CommandLine command(env.searchInPath(scriptFile), {"-dumpversion"});
        QtcProcess emcc;
        emcc.setCommand(command);
        emcc.setEnvironment(env);
        emcc.start();
        if (!emcc.waitForFinished())
            return {};
        const QString version = QLatin1String(emcc.readAllStandardOutput());
        emSdkVersionCache()->insert(cacheKey,
                                    new QVersionNumber(QVersionNumber::fromString(version)));
    }
    return *emSdkVersionCache()->object(cacheKey);
}

void WebAssemblyEmSdk::registerEmSdk(const FilePath &sdkRoot)
{
    QSettings *s = Core::ICore::settings();
    s->setValue(QLatin1String(Constants::SETTINGS_GROUP) + '/'
                + QLatin1String(Constants::SETTINGS_KEY_EMSDK), sdkRoot.toString());
}

FilePath WebAssemblyEmSdk::registeredEmSdk()
{
    QSettings *s = Core::ICore::settings();
    const QString path = s->value(QLatin1String(Constants::SETTINGS_GROUP) + '/'
                                  + QLatin1String(Constants::SETTINGS_KEY_EMSDK)).toString();
    return FilePath::fromUserInput(path);
}

void WebAssemblyEmSdk::clearCaches()
{
    emSdkEnvCache()->clear();
    emSdkVersionCache()->clear();
}

// Unit tests:
#ifdef WITH_TESTS
void WebAssemblyPlugin::testEmSdkEnvParsing()
{
    // Output of "emsdk_env"
    const QByteArray emSdkEnvOutput = HostOsInfo::isWindowsHost() ?
                R"(
Adding directories to PATH:
PATH += C:\Users\user\dev\emsdk
PATH += C:\Users\user\dev\emsdk\upstream\emscripten
PATH += C:\Users\user\dev\emsdk\node\12.18.1_64bit\bin
PATH += C:\Users\user\dev\emsdk\python\3.7.4-pywin32_64bit
PATH += C:\Users\user\dev\emsdk\java\8.152_64bit\bin

Setting environment variables:
PATH = C:\Users\user\dev\emsdk;C:\Users\user\dev\emsdk\upstream\emscripten;C:\Users\user\dev\emsdk\node\12.18.1_64bit\bin;C:\Users\user\dev\emsdk\python\3.7.4-pywin32_64bit;C:\Users\user\dev\emsdk\java\8.152_64bit\bin;...other_stuff...
EMSDK = C:/Users/user/dev/emsdk
EM_CONFIG = C:\Users\user\dev\emsdk\.emscripten
EM_CACHE = C:/Users/user/dev/emsdk/upstream/emscripten\cache
EMSDK_NODE = C:\Users\user\dev\emsdk\node\12.18.1_64bit\bin\node.exe
EMSDK_PYTHON = C:\Users\user\dev\emsdk\python\3.7.4-pywin32_64bit\python.exe
JAVA_HOME = C:\Users\user\dev\emsdk\java\8.152_64bit
                )" : R"(
Adding directories to PATH:
PATH += /home/user/dev/emsdk
PATH += /home/user/dev/emsdk/upstream/emscripten
PATH += /home/user/dev/emsdk/node/12.18.1_64bit/bin

Setting environment variables:
PATH = /home/user/dev/emsdk:/home/user/dev/emsdk/upstream/emscripten:/home/user/dev/emsdk/node/12.18.1_64bit/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
EMSDK = /home/user/dev/emsdk
EM_CONFIG = /home/user/dev/emsdk/.emscripten
EM_CACHE = /home/user/dev/emsdk/upstream/emscripten/cache
EMSDK_NODE = /home/user/dev/emsdk/node/12.18.1_64bit/bin/node
               )";
    Environment env;
    parseEmSdkEnvOutputAndAddToEnv(emSdkEnvOutput, env);

    if (HostOsInfo::isWindowsHost()) {
        QVERIFY(env.path().count() == 5);
        QCOMPARE(env.value("EMSDK"), "C:/Users/user/dev/emsdk");
        QCOMPARE(env.value("EM_CONFIG"), "C:\\Users\\user\\dev\\emsdk\\.emscripten");
    } else {
        QVERIFY(env.path().count() == 3);
        QCOMPARE(env.value("EMSDK"), "/home/user/dev/emsdk");
        QCOMPARE(env.value("EM_CONFIG"), "/home/user/dev/emsdk/.emscripten");
    }
}
#endif // WITH_TESTS

} // namespace Internal
} // namespace WebAssembly
