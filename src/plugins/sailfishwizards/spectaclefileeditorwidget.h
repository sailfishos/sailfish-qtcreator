/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include <QTabWidget>
#include <QMessageBox>
#include "spectaclefilewizardpages.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief This class defines spectacle file editor's widget and user interface.
 * \sa QTabWidget
 */
class SpectacleFileEditorWidget : public QTabWidget
{
    Q_OBJECT
public:
    SpectacleFileEditorWidget(QWidget *parent = nullptr);
    QString content();
    void setContent(const QByteArray &content);
    void clearDependencies();

private:
    SpectacleFileOptionsPage *m_optionsPage;
    SpectacleFileDependenciesPage *m_dependenciesExpertPage;
    SpectacleFileProjectTypePage *m_dependenciesPage;
    QList<DependencyListModel *> *m_dependencyLists;
    QString m_fileContent;
    QMessageBox *m_warningBox;

signals:
    void contentModified();

private slots:
    void setChecks();
    void showWarning(int index);
};

} // namespace Internal
} // namespace SailfishWizards
