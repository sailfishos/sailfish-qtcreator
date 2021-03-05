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

#include "branchadddialog.h"
#include "branchmodel.h"
#include "ui_branchadddialog.h"
#include "gitplugin.h"

#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>

#include <QPushButton>
#include <QRegularExpression>
#include <QValidator>

namespace Git {
namespace Internal {

/*!
 * \brief The BranchNameValidator class validates the corresponding string as
 * a valid Git branch name.
 *
 * The class does this by a couple of rules that are applied on the string.
 *
 */
class BranchNameValidator : public QValidator
{
public:
    BranchNameValidator(const QStringList &localBranches, QObject *parent = nullptr) :
        QValidator(parent),
        m_invalidChars('(' + GitPlugin::invalidBranchAndRemoteNamePattern() + ")+"),
        m_localBranches(localBranches)
    {
    }

    State validate(QString &input, int &pos) const override
    {
        Q_UNUSED(pos)

        if (input.isEmpty())
            return Intermediate;

        input.replace(m_invalidChars, "_");

        // "Intermediate" patterns, may change to Acceptable when user edits further:

        if (input.endsWith(".lock")) //..may not end with ".lock"
            return Intermediate;

        if (input.endsWith('.')) // no dot at the end (but allowed in the middle)
            return Intermediate;

        if (input.endsWith('/')) // no slash at the end (but allowed in the middle)
            return Intermediate;

        if (m_localBranches.contains(input, Utils::HostOsInfo::isWindowsHost()
                                     ? Qt::CaseInsensitive : Qt::CaseSensitive)) {
            return Intermediate;
        }

        // is a valid branch name
        return Acceptable;
    }

private:
    const QRegularExpression m_invalidChars;
    QStringList m_localBranches;
};

BranchValidationDelegate::BranchValidationDelegate(QWidget *parent, BranchModel *model)
    : QItemDelegate(parent)
    , m_model(model)
{
}

QWidget *BranchValidationDelegate::createEditor(QWidget *parent,
                                                const QStyleOptionViewItem & /*option*/,
                                                const QModelIndex & /*index*/) const
{
    auto lineEdit = new Utils::FancyLineEdit(parent);
    BranchNameValidator *validator = new BranchNameValidator(m_model->localBranchNames(), lineEdit);
    lineEdit->setValidator(validator);
    return lineEdit;
}

BranchAddDialog::BranchAddDialog(const QStringList &localBranches, Type type, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchAddDialog)
{
    m_ui->setupUi(this);
    m_ui->trackingCheckBox->setVisible(false);
    setCheckoutVisible(false);

    switch (type) {
    case BranchAddDialog::AddBranch:
        setWindowTitle(tr("Add Branch"));
        break;
    case BranchAddDialog::RenameBranch:
        setWindowTitle(tr("Rename Branch"));
        break;
    case BranchAddDialog::AddTag:
        setWindowTitle(tr("Add Tag"));
        m_ui->branchNameLabel->setText(tr("Tag name:"));
        break;
    case BranchAddDialog::RenameTag:
        setWindowTitle(tr("Rename Tag"));
        m_ui->branchNameLabel->setText(tr("Tag name:"));
        break;
    }

    m_ui->branchNameEdit->setValidator(new BranchNameValidator(localBranches, this));
    connect(m_ui->branchNameEdit, &QLineEdit::textChanged, this, &BranchAddDialog::updateButtonStatus);
}

BranchAddDialog::~BranchAddDialog()
{
    delete m_ui;
}

void BranchAddDialog::setBranchName(const QString &n)
{
    m_ui->branchNameEdit->setText(n);
    m_ui->branchNameEdit->selectAll();
}

QString BranchAddDialog::branchName() const
{
    return m_ui->branchNameEdit->text();
}

void BranchAddDialog::setTrackedBranchName(const QString &name, bool remote)
{
    if (name.isEmpty()) {
        m_ui->trackingCheckBox->setVisible(false);
        m_ui->trackingCheckBox->setChecked(false);
    } else {
        m_ui->trackingCheckBox->setText(remote ? tr("Track remote branch \"%1\"").arg(name) :
                                                 tr("Track local branch \"%1\"").arg(name));
        m_ui->trackingCheckBox->setVisible(true);
        m_ui->trackingCheckBox->setChecked(remote);
    }
}

bool BranchAddDialog::track() const
{
    return m_ui->trackingCheckBox->isChecked();
}

void BranchAddDialog::setCheckoutVisible(bool visible)
{
    m_ui->checkoutCheckBox->setVisible(visible);
    m_ui->checkoutCheckBox->setChecked(visible);
}

bool BranchAddDialog::checkout() const
{
    return m_ui->checkoutCheckBox->isChecked();
}

/*! Updates the ok button enabled state of the dialog according to the validity of the branch name. */
void BranchAddDialog::updateButtonStatus()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_ui->branchNameEdit->hasAcceptableInput());
}

} // namespace Internal
} // namespace Git
