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
#include "desktopwizardpages.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief This class defines desktop editor's widget and user interface.
 * \sa QTabWidget
 */
class DesktopEditorWidget : public QTabWidget
{
    Q_OBJECT
public:
    DesktopEditorWidget(QWidget *parent = nullptr);
    ~DesktopEditorWidget();
    QString content() const;
    void setContent(const QByteArray &content, const QString &applicationPath, const QStringList &resolutions);
    void clearTranslations();
    void setIconValue(const QString &applicationPath, int iconsIndex);
    bool proceedEmptyIcon();
    bool validateIcons();
    /*!
     * \brief Returns \c true if icon page is in the manual mode, otherwise returns \c false.
     */
    bool isManualChecked() const
    {
        return m_iconPage->Ui().manualInputCheck->isChecked();
    }

    void reloadIconPage(const QString &applicationPath, int iconsIndex);
    QHash<QString, QString> iconPaths() const;

private:
    DesktopSettingPage *m_settingPage;
    DesktopIconPage *m_iconPage;
    TranslationTableModel *m_tableModel;
    QString m_fileContent;
    bool eventFilter(QObject *target, QEvent *event);

signals:
    void contentModified();
};

} // namespace Internal
} // namespace SailfishWizards
