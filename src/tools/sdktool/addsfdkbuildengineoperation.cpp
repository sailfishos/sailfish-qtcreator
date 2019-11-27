/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013-2014,2017,2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addsfdkbuildengineoperation.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include "sfdkutils.h"

#include "../../libs/sfdk/sfdkconstants.h"

#include <QDateTime>
#include <QDir>

#include <iostream>

// FIXME most of these should be possible to detect at first run
const char MER_PARAM_VM_URI[] = "--vm-uri";
const char MER_PARAM_AUTODETECTED[] = "--autodetected";
const char MER_PARAM_SHARED_HOME[] = "--shared-home";
const char MER_PARAM_SHARED_TARGETS[] = "--shared-targets";
const char MER_PARAM_SHARED_SSH[] = "--shared-ssh";
const char MER_PARAM_SHARED_SRC[] = "--shared-src";
const char MER_PARAM_SHARED_CONFIG[] = "--shared-config";
const char MER_PARAM_HOST[] = "--host";
const char MER_PARAM_USERNAME[] = "--username";
const char MER_PARAM_PRIVATE_KEY_FILE[] = "--private-key-file";
const char MER_PARAM_SSH_PORT[] = "--ssh-port";
const char MER_PARAM_WWW_PORT[] = "--www-port";
const char MER_PARAM_HEADLESS[] = "--headless";
const char MER_PARAM_INSTALLDIR[] = "--installdir";

using namespace Utils;
namespace C = Sfdk::Constants;

AddSfdkBuildEngineOperation::AddSfdkBuildEngineOperation()
{
}

QString AddSfdkBuildEngineOperation::name() const
{
    return QLatin1String("addSfdkBuildEngine");
}

QString AddSfdkBuildEngineOperation::helpText() const
{
    return QLatin1String("add an Sfdk build engine");
}

QString AddSfdkBuildEngineOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + QLatin1String(MER_PARAM_INSTALLDIR) + QLatin1String(" <DIR>            SDK installation directory (required).\n")
         + indent + QLatin1String(MER_PARAM_VM_URI) + QLatin1String(" <URI>                virtual machine URI (required).\n")
         + indent + QLatin1String(MER_PARAM_AUTODETECTED) + QLatin1String(" <BOOL>         is build engine autodetected.\n")
         + indent + QLatin1String(MER_PARAM_SHARED_HOME) + QLatin1String(" <PATH>          shared \"home\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_TARGETS) + QLatin1String(" <PATH>       shared \"targets\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_SSH) + QLatin1String(" <PATH>           shared \"ssh\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_SRC) + QLatin1String(" <PATH>           shared \"src\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_CONFIG) + QLatin1String(" <PATH>        shared \"config\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_HOST) + QLatin1String(" <NAME>                 ssh hostname (required).\n")
         + indent + QLatin1String(MER_PARAM_USERNAME) + QLatin1String(" <NAME>             ssh username (required).\n")
         + indent + QLatin1String(MER_PARAM_PRIVATE_KEY_FILE) + QLatin1String(" <FILE>     ssh private key file (required).\n")
         + indent + QLatin1String(MER_PARAM_SSH_PORT) + QLatin1String(" <NUMBER>           ssh port (required).\n")
         + indent + QLatin1String(MER_PARAM_WWW_PORT) + QLatin1String(" <NUMBER>           www port (required).\n")
         + indent + QLatin1String(MER_PARAM_HEADLESS) + QLatin1String("                    set headless mode.\n");
}

bool AddSfdkBuildEngineOperation::setArguments(const QStringList &args)
{

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String(MER_PARAM_INSTALLDIR)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_installDir = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VM_URI)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_vmUri = QUrl(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_AUTODETECTED)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_autodetected = next == QLatin1String("true");
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_HOME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedHomePath = QDir::fromNativeSeparators(next);
            continue;
        }


        if (current == QLatin1String(MER_PARAM_SHARED_TARGETS)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedTargetsPath = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_SSH)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedSshPath = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_SRC)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedSrcPath = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_CONFIG)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedConfigPath = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HOST)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_host = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_USERNAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_userName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_PRIVATE_KEY_FILE)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_privateKeyFile = QDir::fromNativeSeparators(next);
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SSH_PORT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sshPort = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_WWW_PORT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_wwwPort = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HEADLESS)) {
            m_headless = true;
            continue;
        }
    }

    const char MISSING[] = " parameter missing.";
    bool error = false;
    if (m_installDir.isEmpty()) {
        std::cerr << MER_PARAM_INSTALLDIR << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_vmUri.isEmpty()) {
        std::cerr << MER_PARAM_VM_URI << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedHomePath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_HOME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedTargetsPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_TARGETS << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedSshPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_SSH << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedSrcPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_SRC << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedConfigPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_CONFIG << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_host.isEmpty()) {
        std::cerr << MER_PARAM_HOST << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_userName.isEmpty()) {
        std::cerr << MER_PARAM_USERNAME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_privateKeyFile.isEmpty()) {
        std::cerr << MER_PARAM_PRIVATE_KEY_FILE << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sshPort == 0) {
        std::cerr << MER_PARAM_SSH_PORT << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_wwwPort == 0) {
        std::cerr << MER_PARAM_WWW_PORT << MISSING << std::endl << std::endl;
        error = true;
    }

    return !error;
}

int AddSfdkBuildEngineOperation::execute() const
{
    useSfdkSettingsPath();

    QVariantMap map = load(QLatin1String("SfdkBuildEngines"));
    if (map.isEmpty())
        map = initializeBuildEngines(1, m_installDir);

    const QVariantMap result = addBuildEngine(map, m_vmUri, QDateTime::currentDateTime(),
            m_autodetected, m_sharedHomePath, m_sharedTargetsPath, m_sharedSshPath, m_sharedSrcPath,
            m_sharedConfigPath, m_host, m_userName, m_privateKeyFile, m_sshPort, m_wwwPort,
            m_headless);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("SfdkBuildEngines")) ? 0 : 3;
}

QVariantMap AddSfdkBuildEngineOperation::initializeBuildEngines(int version,
        const QString &installDir)
{
    QVariantMap map;
    map.insert(QLatin1String(C::BUILD_ENGINES_VERSION_KEY), version);
    map.insert(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY), installDir);
    map.insert(QLatin1String(C::BUILD_ENGINES_COUNT_KEY), 0);
    return map;
}

QVariantMap AddSfdkBuildEngineOperation::addBuildEngine(const QVariantMap &map,
                                          const QUrl &vmUri,
                                          const QDateTime &creationTime,
                                          bool autodetected,
                                          const QString &sharedHomePath,
                                          const QString &sharedTargetsPath,
                                          const QString &sharedSshPath,
                                          const QString &sharedSrcPath,
                                          const QString &sharedConfigPath,
                                          const QString &host,
                                          const QString &userName,
                                          const QString &privateKeyFile,
                                          quint16 sshPort,
                                          quint16 wwwPort,
                                          bool headless)
{
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(vmUri));
    bool hasTarget = false;
    foreach (const QString &t, valueKeys) {
        if (t.endsWith(QLatin1Char('/') + QLatin1String(C::BUILD_ENGINE_VM_URI))) {
            hasTarget = true;
            break;
        }
    }
    if (hasTarget) {
        std::cerr << "Error: VM " << qPrintable(vmUri.toString()) << " already defined as a build engine." << std::endl;
        return QVariantMap();
    }

    bool ok;
    const int count = GetOperation::get(map, QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Invalid build engines count, file seems corrupted." << std::endl;
        return QVariantMap();
    }

    const QString engine = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(count);

    // remove old count
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, QStringList()
            << QLatin1String(C::BUILD_ENGINES_COUNT_KEY));

    KeyValuePairList data;
    auto addPrefix = [&](const QString &key, const QVariant &value) {
        return KeyValuePair(QStringList{engine, key}, value);
    };
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_VM_URI), QVariant(vmUri));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_CREATION_TIME), QVariant(creationTime));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_AUTODETECTED), QVariant(autodetected));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_SHARED_HOME), QVariant(sharedHomePath));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_SHARED_TARGET), QVariant(sharedTargetsPath));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_SHARED_SSH), QVariant(sharedSshPath));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_SHARED_SRC), QVariant(sharedSrcPath));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_SHARED_CONFIG), QVariant(sharedConfigPath));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_HOST), QVariant(host));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_USER_NAME), QVariant(userName));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_PRIVATE_KEY_FILE), QVariant(privateKeyFile));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_SSH_PORT), QVariant(sshPort));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_WWW_PORT), QVariant(wwwPort));
    data << addPrefix(QLatin1String(C::BUILD_ENGINE_HEADLESS), QVariant(headless));
    data << KeyValuePair(QLatin1String(C::BUILD_ENGINES_COUNT_KEY), QVariant(count + 1));

    return AddKeysOperation::addKeys(cleaned, data);
}

#ifdef WITH_TESTS
bool AddSfdkBuildEngineOperation::test() const
{
    QVariantMap map = initializeBuildEngines(2, QLatin1String("/dir"));

    if (map.count() != 3
            || !map.contains(QLatin1String(C::BUILD_ENGINES_VERSION_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir")
            || !map.contains(QLatin1String(C::BUILD_ENGINES_COUNT_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt() != 0)
        return false;

    const auto now = QDateTime::currentDateTime();

    map = addBuildEngine(map, QUrl("sfdkvm:VirtualBox#testBuildEngine"), now, true,
                 QLatin1String("/test/sharedHomePath"),
                 QLatin1String("/test/sharedTargetPath"),
                 QLatin1String("/test/sharedSshPath"),
                 QLatin1String("/test/sharedSrcPath"),
                 QLatin1String("/test/sharedConfigPath"),
                 QLatin1String("host"),
                 QLatin1String("user"),
                 QLatin1String("/test/privateKey"),22,80,false);

    const QString sdk = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(0);

    if (map.count() != 4
            || !map.contains(QLatin1String(C::BUILD_ENGINES_VERSION_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir")
            || !map.contains(QLatin1String(C::BUILD_ENGINES_COUNT_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt() != 1
            || !map.contains(sdk))
        return false;

    QVariantMap sdkMap= map.value(sdk).toMap();
    if (sdkMap.count() != 14
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_VM_URI))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_VM_URI)).toString() != QLatin1String("sfdkvm:VirtualBox#testBuildEngine")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_CREATION_TIME))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_CREATION_TIME)).toDateTime() != now
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_AUTODETECTED))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_AUTODETECTED)).toBool() != true
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_SHARED_HOME))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_SHARED_HOME)).toString() != QLatin1String("/test/sharedHomePath")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_SHARED_TARGET))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_SHARED_TARGET)).toString() != QLatin1String("/test/sharedTargetPath")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_SHARED_SSH))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_SHARED_SSH)).toString() != QLatin1String("/test/sharedSshPath")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_SHARED_SRC))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_SHARED_SRC)).toString() != QLatin1String("/test/sharedSrcPath")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_SHARED_CONFIG))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_SHARED_CONFIG)).toString() != QLatin1String("/test/sharedConfigPath")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_HOST))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_HOST)).toString() != QLatin1String("host")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_USER_NAME))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_USER_NAME)).toString() != QLatin1String("user")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_PRIVATE_KEY_FILE))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_PRIVATE_KEY_FILE)).toString() != QLatin1String("/test/privateKey")
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_SSH_PORT))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_SSH_PORT)).toInt() != 22
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_WWW_PORT))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_WWW_PORT)).toInt() != 80
            || !sdkMap.contains(QLatin1String(C::BUILD_ENGINE_HEADLESS))
            || sdkMap.value(QLatin1String(C::BUILD_ENGINE_HEADLESS)).toBool() != false)
        return false;

    return true;
}
#endif
