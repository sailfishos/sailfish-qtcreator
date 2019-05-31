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

namespace QmlJSEditor {
namespace Internal {

namespace Ui { class ComponentNameDialog; }

class ComponentNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComponentNameDialog(QWidget *parent = nullptr);
    ~ComponentNameDialog() override;

    static bool go(QString *proposedName, QString *proposedPath, QString *proposedSuffix,
                   const QStringList &properties, const QStringList &sourcePreview, const QString &oldFileName,
                   QStringList *result,
                   QWidget *parent = nullptr);

    void setProperties(const QStringList &properties);

    QStringList propertiesToKeep() const;

    void generateCodePreview();

    void validate();

protected:
    QString isValid() const;

private:
    Ui::ComponentNameDialog *ui;
    QStringList m_sourcePreview;
};

} // namespace Internal
} // namespace QmlJSEditor
