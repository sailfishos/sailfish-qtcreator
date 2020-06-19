/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <projectexplorer/toolchain.h>

#include "builddirreader.h"
#include "cmakeprocess.h"

#include <QRegularExpression>

namespace Utils { class QtcProcess; }

namespace CMakeProjectManager {
namespace Internal {

class TeaLeafReader final : public BuildDirReader
{
    Q_OBJECT

public:
    TeaLeafReader();
    ~TeaLeafReader() final;

    void setParameters(const BuildDirParameters &p) final;

    bool isCompatible(const BuildDirParameters &p) final;
    void resetData() final;
    void parse(bool forceCMakeRun, bool forceConfiguration) final;
    void stop() final;

    bool isParsing() const final;

    QSet<Utils::FilePath> projectFilesToWatch() const final;
    QList<CMakeBuildTarget> takeBuildTargets(QString &errorMessage) final;
    CMakeConfig takeParsedConfiguration(QString &errorMessage) final;
    std::unique_ptr<CMakeProjectNode> generateProjectTree(
        const QList<const ProjectExplorer::FileNode *> &allFiles, QString &errorMessage) final;
    ProjectExplorer::RawProjectParts createRawProjectParts(QString &errorMessage) final;

private:
    void extractData();

    void startCMake(const QStringList &configurationArguments);

    void cmakeFinished(int code, QProcess::ExitStatus status);

    QStringList getFlagsFor(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache, Core::Id lang) const;
    bool extractFlagsFromMake(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache, Core::Id lang) const;
    bool extractFlagsFromNinja(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache, Core::Id lang) const;

    // Process data:
    std::unique_ptr<CMakeProcess> m_cmakeProcess;

    QSet<Utils::FilePath> m_cmakeFiles;
    QString m_projectName;
    QList<CMakeBuildTarget> m_buildTargets;
    std::vector<std::unique_ptr<ProjectExplorer::FileNode>> m_files;

    // RegExps for function-like macrosses names fixups
    QRegularExpression m_macroFixupRe1;
    QRegularExpression m_macroFixupRe2;
    QRegularExpression m_macroFixupRe3;

    friend class CMakeFile;
};

} // namespace Internal
} // namespace CMakeProjectManager
