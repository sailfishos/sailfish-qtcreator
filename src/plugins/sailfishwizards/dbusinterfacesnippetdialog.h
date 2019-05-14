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

#include "ui_dbusinterfacesnippetpage.h"
#include "dbusservicestablemodel.h"
#include "dbusservice.h"
#include <QDialog>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief DBusInterfaceSnippetDialog class allows to connect
 * the registered dbus service in application in c ++ or qml files.
 * \sa QDialog
 */
class DBusInterfaceSnippetDialog : public QDialog
{
    Q_OBJECT
public:
    DBusInterfaceSnippetDialog(QWidget *parent = nullptr);
    ~DBusInterfaceSnippetDialog();
    void validateInput(bool condition, QWidget *widget,
                       const QString &errorMessage) const;
    QList<QWidget *> getWidgets();
    void updateHintsWithout(QWidget *widget);
    void loadServicesFromJson();
    void removeServicesWithWrongPaths(DBusServicesTableModel *firstModel,
                                      DBusServicesTableModel *secondModel);
private:
    Ui::DBusInterfaceSnippetPage m_pageUi;
    QList<DBusService> m_services;
    QList<DBusService> m_servicesToPrint;
    QList<bool> m_checks;
    QList<DBusService> m_signalsToPrint;
    QList<bool> m_signalsChecks;
    QStringList m_busTypes;
    QString m_suffix;
    QStringList m_sessionServices = {""};
    QStringList m_systemServices = {""};
    DBusServicesTableModel *m_tableModel;
    DBusServicesTableModel *m_tableSignalsModel;
    QString m_methodsSelectingHint;
    QString m_signalsSelectingHint;
    QString generateCppSnippet();
    QString generateQmlSnippet();
private slots:
    void updateListOfMethods();
    void updateServicePathsOrMethods();
    void updateInputFields();
    void updateServices();
    void updateInterfaces();
    void updateMethods();
    void enableInsert();
    void insert();
    void generateHint();
    void turnCheckOn(const QModelIndex &index);
signals:
    void inputChanged();
};

} // namespace Internal
} // namespace SailfishWizards
