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

#include <QDialog>

namespace Core { class Id; }
namespace ProjectExplorer { class Kit; }
namespace Utils { class FilePath; }

namespace Debugger {
namespace Internal {

class AttachCoreDialogPrivate;

class AttachCoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachCoreDialog(QWidget *parent);
    ~AttachCoreDialog() override;

    int exec() override;

    Utils::FilePath symbolFile() const;
    QString localCoreFile() const;
    QString remoteCoreFile() const;
    QString overrideStartScript() const;
    bool useLocalCoreFile() const;
    bool forcesLocalCoreFile() const;
    bool isLocalKit() const;

    // For persistance.
    ProjectExplorer::Kit *kit() const;
    void setSymbolFile(const QString &symbolFileName);
    void setLocalCoreFile(const QString &core);
    void setRemoteCoreFile(const QString &core);
    void setOverrideStartScript(const QString &scriptName);
    void setKitId(Core::Id id);
    void setForceLocalCoreFile(bool on);

private:
    void changed();
    void coreFileChanged(const QString &core);
    void selectRemoteCoreFile();

    AttachCoreDialogPrivate *d;
};


} // namespace Debugger
} // namespace Internal
