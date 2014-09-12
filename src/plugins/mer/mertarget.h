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

class MerTarget
{
public:
    MerTarget(MerSdk *sdk);
    virtual ~MerTarget();
    QString name() const;
    QString targetPath() const;

    void setName(const QString &name);
    void setQmakeQuery(const QString &qmakeQuery);
    void setGccDumpMachine(const QString &gccMachineDump);
    void setDefaultGdb(const QString &name);

    bool fromMap(const QVariantMap &data);
    QVariantMap toMap() const;
    ProjectExplorer::Kit* createKit() const;
    MerQtVersion* createQtVersion() const;
    MerToolChain* createToolChain() const;
    bool createScripts() const;
    void deleteScripts() const;
    bool isValid() const;
    bool operator==(const MerTarget &other) const;

private:
    bool createScript(const QString &targetPath, int scriptIndex) const;
    bool createCacheFile(const QString &fileName, const QString &content) const;

private:
    MerSdk *m_sdk;
    QString m_name;
    QString m_qmakeQuery;
    QString m_gccMachineDump;
    QString m_defaultGdb;
};

}
}

#endif // MERTARGET_H
