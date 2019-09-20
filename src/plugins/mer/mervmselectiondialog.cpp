/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "mervmselectiondialog.h"
#include "ui_mervmselectiondialog.h"

#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <utils/qtcassert.h>

#include <QListWidgetItem>
#include <QPushButton>

using namespace Sfdk;

namespace Mer {
namespace Internal {

MerVmSelectionDialog::MerVmSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::MerVmSelectionDialog)
{
    m_ui->setupUi(this);

    QList<VirtualMachineDescriptor> unusedVms;
    bool ok;
    execAsynchronous(std::tie(unusedVms, ok), Sdk::unusedVirtualMachines);
    QTC_CHECK(ok);
    for (const VirtualMachineDescriptor &vm : unusedVms) {
        auto item = new QListWidgetItem(vm.name, m_ui->virtualMachineListWidget);
        item->setData(Qt::UserRole, vm.uri);
    }

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Add"));

    connect(m_ui->virtualMachineListWidget, &QListWidget::itemSelectionChanged,
            this, &MerVmSelectionDialog::handleItemSelectionChanged);
    connect(m_ui->virtualMachineListWidget, &QListWidget::itemDoubleClicked,
            this, &MerVmSelectionDialog::handleItemDoubleClicked);
    handleItemSelectionChanged();

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted,
            this, &MerVmSelectionDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected,
            this, &MerVmSelectionDialog::reject);
}

MerVmSelectionDialog::~MerVmSelectionDialog()
{
    delete m_ui;
}

void MerVmSelectionDialog::handleItemSelectionChanged()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !m_ui->virtualMachineListWidget->selectedItems().isEmpty());
}

void MerVmSelectionDialog::handleItemDoubleClicked()
{
    accept();
}

QUrl MerVmSelectionDialog::selectedVmUri() const
{
    QList<QListWidgetItem *> selected = m_ui->virtualMachineListWidget->selectedItems();
    if (selected.isEmpty())
        return {};
    return selected.at(0)->data(Qt::UserRole).toUrl();
}

} // Internal
} // Mer
