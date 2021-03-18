/****************************************************************************
**
** Copyright (C) 2021 Open Mobile Platform LLC.
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

#include "signingutils_p.h"

#include "asynchronous_p.h"
#include "sdk_p.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"

#include <app/app_version.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

using namespace Utils;

namespace Sfdk {

namespace {

const char SIGNING_KEY_EXTENSION[] = ".key";

} // namespace anonymous

namespace {

QString gpgPath()
{
    // See ProjectExplorerPlugin::extensionsInitialized()
    QSettings qtcSettings{QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
                          QLatin1String(Core::Constants::IDE_CASED_ID)};
    const QString gitBinary = qtcSettings.value("Git/BinaryPath", "git").toString();
    const QStringList rawGitSearchPaths
        = qtcSettings.value("Git/Path").toString().split(':', Qt::SkipEmptyParts);

    const auto searchPathRetriever = [=] {
        Utils::FilePaths searchPaths;
        const QString libexecPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + '/'
                + RELATIVE_LIBEXEC_PATH);
        searchPaths << Utils::FilePath::fromString(libexecPath);
        if (Utils::HostOsInfo::isWindowsHost()) {
            const Utils::FilePaths gitSearchPaths
                = Utils::transform(rawGitSearchPaths, [](const QString &rawPath) {
                      return Utils::FilePath::fromString(rawPath);
                  });
            const Utils::FilePath fullGitPath = Utils::Environment::systemEnvironment()
                                                    .searchInPath(gitBinary, gitSearchPaths);
            if (!fullGitPath.isEmpty()) {
                searchPaths << fullGitPath.parentDir()
                            << fullGitPath.parentDir().parentDir() + "/usr/bin";
            }
        }
        return searchPaths;
    };

    //By default, gpg on windows is put together with git, so we look for it there
    const QList<FilePath> additionalSearchPaths = searchPathRetriever();

    const FilePath filePath = Environment::systemEnvironment()
            .searchInPath("gpg", additionalSearchPaths);
    if (!filePath.isEmpty())
        return filePath.toString();
    return {};
}

class GpgRunner : public ProcessRunner
{
    Q_OBJECT
public:
    explicit GpgRunner(const QStringList &arguments, QObject *parent = 0)
        : ProcessRunner(path(), arguments, parent)
    {}

private:
    static QString path()
    {
        QString path = gpgPath();

        if (!SdkPrivate::customGpgPath().isEmpty())
            path = SdkPrivate::customGpgPath();

        return path;
    }
};

CommandQueue *commandQueue()
{
    return Sfdk::SdkPrivate::commandQueue();
}

} // namespace anonymous

/*!
 * \class SigningUtils
 * \internal
 */

SigningUtils::SigningUtils(QObject *parent)
    : QObject(parent)
{}

SigningUtils::~SigningUtils() = default;

void SigningUtils::listSecretKeys(const QObject *context,
        const Functor<bool, const QList<GpgKeyInfo> &, QString> &functor)
{
    QStringList arguments;
    arguments.append("--with-colons");
    arguments.append("--list-secret-keys");

    auto runner = std::make_unique<GpgRunner>(arguments);

    QObject::connect(runner.get(), &GpgRunner::done, context, [=, process = runner->process()](bool ok) {
        if (!ok) {
            functor(false, {}, QString::fromUtf8(process->readAllStandardError()));
            return;
        }
        const QList<GpgKeyInfo> keysInfo = gpgKeyInfoListFromOutput(
            QString::fromUtf8(process->readAllStandardOutput().trimmed()));
        functor(true, keysInfo, {});
     });
    commandQueue()->enqueue(std::move(runner));
}

void SigningUtils::exportSecretKey(const QString &id, const Utils::FilePath &passphraseFile,
        const Utils::FilePath &outputDir, const QObject *context, const Functor<bool, QString> &functor)
{
    const QPointer<const QObject> context_{context};

    QTC_ASSERT(!id.isEmpty(),
            QTimer::singleShot(0, context, std::bind(functor, false, QString()));
            return);
    if (!passphraseFile.isEmpty() && !passphraseFile.exists()) {
        QTimer::singleShot(0, context, std::bind(
                functor, false,
                tr("Passphrase file set but doesn't exit")));
        return;
    }

    verifyPassphrase(id, passphraseFile.toString(), Sdk::instance(), [=](bool passphraseValid,
            const QString &errorString) {
        if (!passphraseValid) {
            callIf(context_, functor, false, errorString);
            return;
        }

        if (!outputDir.exists()) {
            QDir dir(outputDir.toString());
            if (!dir.mkdir(dir.absolutePath())) {
                callIf(context_, functor, false, tr("Failed to create folder for keys: %1").arg(outputDir.toString()));
                return;
            }
        }

        const QString keyFilePath = outputDir.pathAppended(id + SIGNING_KEY_EXTENSION).toString();

        QStringList arguments;
        arguments.append("--batch");
        arguments.append("--yes"); //need for overwrite key file if it exists
        arguments.append("--output");
        arguments.append(keyFilePath);
        arguments.append("--pinentry-mode=loopback");

        if (!passphraseFile.isEmpty()) {
            arguments.append("--passphrase-file");
            arguments.append(passphraseFile.toString());
        }

        arguments.append("--export-secret-keys");
        arguments.append(id);

        auto runner = std::make_unique<GpgRunner>(arguments);
        QObject::connect(runner.get(), &GpgRunner::done, context, [=, process = runner->process()](bool ok) {
             if (!ok) {
                 callIf(context_, functor, false, tr("Failed to export secret \"%1\" key: %2")
                         .arg(id)
                         .arg(QString::fromUtf8(process->readAllStandardError())));
                 return;
             }

             callIf(context_, functor, true, QString());
        });
        commandQueue()->enqueueImmediate(std::move(runner));
    });
}

void SigningUtils::exportPublicKey(const QString &id, const QObject *context,
        const Functor<bool, const QByteArray &, QString> &functor)
{
    QTC_ASSERT(!id.isEmpty(),
            QTimer::singleShot(0, context, std::bind(functor, false, QByteArray(), QString()));
            return);

    QStringList arguments;
    arguments.append("--batch");
    arguments.append("--armor");
    arguments.append("--export");
    arguments.append(id);

    auto runner = std::make_unique<GpgRunner>(arguments);

    QObject::connect(runner.get(), &GpgRunner::done, context,
            [id, functor, process = runner->process()](bool ok) {
        if (!ok) {
            functor(false, {}, tr("Failed to export public key for \"%1\": %2")
                    .arg(id, QString::fromUtf8(process->readAllStandardError())));
            return;
        }
        functor(true, process->readAllStandardOutput(), {});
    });
    commandQueue()->enqueue(std::move(runner));
}

bool SigningUtils::isGpgAvailable(QString *errorString)
{
    const FilePath gpg = FilePath::fromString(gpgPath());
    if (!gpg.exists()) {
        *errorString = tr("GPG utility not found");
        return false;
    }
    return true;
}

QList<GpgKeyInfo> SigningUtils::gpgKeyInfoListFromOutput(const QString &output)
{
    if (output.isEmpty())
        return {};

    QList<GpgKeyInfo> keys;
    QRegularExpression regex("(fpr:*(.*)):\ngrp(.*)\n(uid:u(.*))");
    QRegularExpressionMatchIterator i = regex.globalMatch(output);

    while (i.hasNext()) {
        QRegularExpressionMatch m = i.next();
        GpgKeyInfo gpgKeyInfo;
        for (int i = 1; i <= m.lastCapturedIndex(); ++i) {
            QString captured = m.captured(i);

            if (captured.startsWith("fpr", Qt::CaseInsensitive))
                gpgKeyInfo.fingerprint = m.captured(i + 1);
            if (captured.startsWith("uid", Qt::CaseInsensitive)) {
                QStringList keyInfo = m.captured(i + 1).split("::", Qt::SkipEmptyParts);
                gpgKeyInfo.name = keyInfo.at(2);
            }
        }
        keys << gpgKeyInfo;
    }
    return keys;
}

void SigningUtils::verifyPassphrase(const QString &id, const QString &passphraseFile,
        const QObject *context, const Functor<bool, QString> &functor)
{
    QStringList arguments;
    arguments.append("--batch");
    arguments.append("--status-fd");
    arguments.append("1");
    arguments.append("--sign");
    arguments.append("--local-user");
    arguments.append(id);
    arguments.append("--output");
    arguments.append("/dev/null");
    arguments.append("--pinentry-mode=loopback");;

    if (!passphraseFile.isEmpty()) {
        arguments.append("--passphrase-file");
        arguments.append(passphraseFile);
    }

    arguments.append("/dev/null");

    auto runner = std::make_unique<GpgRunner>(arguments);
    runner->setExpectedExitCodes({});
    runner->process()->setProcessChannelMode(QProcess::SeparateChannels); // be quiet

    QObject::connect(runner.get(), &GpgRunner::done, context,
            [passphraseFile, functor, process = runner->process()](bool ok) {
        if (!ok) {
            functor(false, tr("Internal error")); // crashed
            return;
        }
        if (process->exitCode() != 0) {
            QString errorString;
            if (passphraseFile.isEmpty())
                errorString = tr("The selected GPG key is passphrase protected "
                              "and no passphrase was specified.");
            else
                errorString = tr("The given passphrase is not a correct passphrase "
                              "for the selected key.");
            functor(false, errorString);
            return;
        }
        functor(true, {});
    });
    commandQueue()->enqueue(std::move(runner));
}

} // namespace Sfdk

#include "signingutils.moc"
