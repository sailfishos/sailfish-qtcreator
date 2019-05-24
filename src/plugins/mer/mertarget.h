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

#ifndef MERTARGET_H
#define MERTARGET_H

#include <QVariantMap>

#include <projectexplorer/toolchain.h>

namespace ProjectExplorer {
 class Kit;
}

namespace Utils {
 class FileName;
}

namespace Mer {
namespace Internal {

class MerSdk;
class MerQtVersion;
class MerToolChain;

class MerRpmValidationSuiteData
{
public:
    bool operator==(const MerRpmValidationSuiteData &other) const;

    QString id;
    QString name;
    QString website;
    bool essential;
};

class MerTarget
{
public:
    MerTarget();
    MerTarget(MerSdk *sdk);
    virtual ~MerTarget();
    QString name() const;
#ifdef MER_LIBRARY
    QString targetPath() const;
#endif // MER_LIBRARY

    void setName(const QString &name);
    void setQmakeQuery(const QString &qmakeQuery);
    void setGccDumpMachine(const QString &gccMachineDump);
    void setGccDumpMacros(const QString &gccMacrosDump);
    void setGccDumpIncludes(const QString &gccIncludesDump);
    void setRpmValidationSuites(const QString &rpmValidationSuites);
    void setDefaultGdb(const QString &name);

    QList<MerRpmValidationSuiteData> rpmValidationSuites() const;

    bool fromMap(const QVariantMap &data);
    QVariantMap toMap() const;

#ifdef MER_LIBRARY
    ProjectExplorer::Kit *kit() const;

    bool finalizeKitCreation(ProjectExplorer::Kit *k) const;
    void ensureDebuggerIsSet(ProjectExplorer::Kit *k) const;
    MerQtVersion* createQtVersion() const;
    MerToolChain* createToolChain(Core::Id l) const;
    bool createScripts() const;
    void deleteScripts() const;
#endif // MER_LIBRARY

    bool isValid() const;
    bool operator==(const MerTarget &other) const;

private:
#ifdef MER_LIBRARY
    bool createScript(const QString &targetPath, int scriptIndex) const;
    bool createCacheFile(const QString &fileName, const QString &content) const;
    bool createPkgConfigWrapper(const QString &targetPath) const;
#endif // MER_LIBRARY

    static QList<MerRpmValidationSuiteData> rpmValidationSuitesFromString(const QString &string, bool *ok);
    static QString rpmValidationSuitesToString(const QList<MerRpmValidationSuiteData> &suites);

private:
    MerSdk *m_sdk = nullptr;
    QString m_name;
    QString m_qmakeQuery;
    QString m_gccMachineDump;
    QString m_gccMacrosDump;
    QString m_gccIncludesDump;
    QList<MerRpmValidationSuiteData> m_rpmValidationSuites;
    bool m_rpmValidationSuitesIsValid = false;
    QString m_defaultGdb;
};

}
}

#endif // MERTARGET_H
