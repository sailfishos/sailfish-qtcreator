/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#include "mersdkselectiondialog.h"
#include "ui_mersdkselectiondialog.h"

#include "merconnection.h"
#include "mervirtualboxmanager.h"

#include <QListWidgetItem>
#include <QPushButton>

namespace Mer {
namespace Internal {

MerSdkSelectionDialog::MerSdkSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::MerSdkSelectionDialog)
{
    m_ui->setupUi(this);

    const QSet<QString> usedVMs = MerConnection::usedVirtualMachines().toSet();
    const QStringList registeredVMs = MerVirtualBoxManager::fetchRegisteredVirtualMachines();
    foreach (const QString &vm, registeredVMs) {
        // add only unused machines
        if (!usedVMs.contains(vm)) {
            new QListWidgetItem(vm, m_ui->virtualMachineListWidget);
        }
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Add"));

    connect(m_ui->virtualMachineListWidget, &QListWidget::itemSelectionChanged,
            this, &MerSdkSelectionDialog::handleItemSelectionChanged);
    connect(m_ui->virtualMachineListWidget, &QListWidget::itemDoubleClicked,
            this, &MerSdkSelectionDialog::handleItemDoubleClicked);
    handleItemSelectionChanged();

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted,
            this, &MerSdkSelectionDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected,
            this, &MerSdkSelectionDialog::reject);
}

MerSdkSelectionDialog::~MerSdkSelectionDialog()
{
    delete m_ui;
}

void MerSdkSelectionDialog::handleItemSelectionChanged()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !m_ui->virtualMachineListWidget->selectedItems().isEmpty());
}

void MerSdkSelectionDialog::handleItemDoubleClicked()
{
    accept();
}

QString MerSdkSelectionDialog::selectedSdkName() const
{
    QList<QListWidgetItem *> selected = m_ui->virtualMachineListWidget->selectedItems();
    if (selected.isEmpty())
        return QString();
    return selected.at(0)->data(Qt::DisplayRole).toString();
}

} // Internal
} // Mer
