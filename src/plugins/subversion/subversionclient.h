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

#include <vcsbase/vcsbaseclient.h>

#include <utils/fileutils.h>

namespace VcsBase { class VcsCommand; }

namespace Subversion {
namespace Internal {

class SubversionDiffEditorController;
class SubversionSettings;

class SubversionClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT

public:
    SubversionClient(SubversionSettings *settings);

    bool doCommit(const QString &repositoryRoot,
                  const QStringList &files,
                  const QString &commitMessageFile,
                  const QStringList &extraOptions = QStringList()) const;
    void commit(const QString &repositoryRoot,
                const QStringList &files,
                const QString &commitMessageFile,
                const QStringList &extraOptions = QStringList()) override;

    void diff(const QString &workingDirectory, const QStringList &files,
              const QStringList &extraOptions) override;

    void log(const QString &workingDir,
             const QStringList &files = QStringList(),
             const QStringList &extraOptions = QStringList(),
             bool enableAnnotationContextMenu = false) override;

    void describe(const QString &workingDirectory, int changeNumber, const QString &title);

    // Add authorization options to the command line arguments.
    static QStringList addAuthenticationOptions(const VcsBase::VcsBaseClientSettings &settings);

    QString synchronousTopic(const QString &repository) const;

    static QString escapeFile(const QString &file);
    static QStringList escapeFiles(const QStringList &files);

protected:
    Core::Id vcsEditorKind(VcsCommandTag cmd) const override;

private:
    SubversionDiffEditorController *findOrCreateDiffEditor(const QString &documentId, const QString &source,
                                           const QString &title, const QString &workingDirectory);

    mutable Utils::FilePath m_svnVersionBinary;
    mutable QString m_svnVersion;
};

} // namespace Internal
} // namespace Subversion
