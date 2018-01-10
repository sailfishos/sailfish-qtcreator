/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "androidrunconfiguration.h"

#include "projectexplorer/runconfiguration.h"
#include "utils/detailswidget.h"

#include <QStringListModel>

#include <memory>

namespace Android {
namespace Internal {

class AdbCommandsWidget;
namespace Ui {
    class AndroidRunConfigurationWidget;
}

class AndroidRunConfigurationWidget : public Utils::DetailsWidget
{
    Q_OBJECT
public:
    AndroidRunConfigurationWidget(QWidget *parent = nullptr);
    ~AndroidRunConfigurationWidget();

    void setAmStartArgs(const QStringList &args);
    void setPreStartShellCommands(const QStringList &cmdList);
    void setPostFinishShellCommands(const QStringList &cmdList);

signals:
    void amStartArgsChanged(QStringList args);
    void preStartCmdsChanged(const QStringList &cmdList);
    void postFinishCmdsChanged(const QStringList &cmdList);

private:
    void addPreStartCommand(const QString &command);
    void addPostFinishCommand(const QString &command);

private:
    std::unique_ptr<Ui::AndroidRunConfigurationWidget> m_ui;
    AdbCommandsWidget *m_preStartCmdsWidget = nullptr;
    AdbCommandsWidget *m_postEndCmdsWidget = nullptr;
};

} // namespace Internal
} // namespace Android

