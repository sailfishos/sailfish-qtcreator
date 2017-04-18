/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "avddialog.h"
#include "androidconfigurations.h"

#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QToolTip>

using namespace Android;
using namespace Android::Internal;

AvdDialog::AvdDialog(int minApiLevel, const QString &targetArch, const AndroidConfig *config, QWidget *parent) :
    QDialog(parent), m_config(config), m_minApiLevel(minApiLevel),
    m_allowedNameChars(QLatin1String("[a-z|A-Z|0-9|._-]*"))
{
    m_avdDialog.setupUi(this);
    m_hideTipTimer.setInterval(2000);
    m_hideTipTimer.setSingleShot(true);

    if (targetArch.isEmpty())
        m_avdDialog.abiComboBox->addItems(QStringList()
                                        << QLatin1String("armeabi-v7a")
                                        << QLatin1String("armeabi")
                                        << QLatin1String("x86")
                                        << QLatin1String("mips"));
    else
        m_avdDialog.abiComboBox->addItems(QStringList(targetArch));

    QRegExpValidator *v = new QRegExpValidator(m_allowedNameChars, this);
    m_avdDialog.nameLineEdit->setValidator(v);
    m_avdDialog.nameLineEdit->installEventFilter(this);

    m_avdDialog.warningIcon->setPixmap(Utils::Icons::WARNING.pixmap());

    updateApiLevelComboBox();

    connect(m_avdDialog.abiComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AvdDialog::updateApiLevelComboBox);

    connect(&m_hideTipTimer, &QTimer::timeout,
            this, [](){Utils::ToolTip::hide();});
}

bool AvdDialog::isValid() const
{
    return !name().isEmpty() && !target().isEmpty() && !abi().isEmpty();
}

QString AvdDialog::target() const
{
    return m_avdDialog.targetComboBox->currentText();
}

QString AvdDialog::name() const
{
    return m_avdDialog.nameLineEdit->text();
}

QString AvdDialog::abi() const
{
    return m_avdDialog.abiComboBox->currentText();
}

int AvdDialog::sdcardSize() const
{
    return m_avdDialog.sizeSpinBox->value();
}

void AvdDialog::updateApiLevelComboBox()
{
    QList<SdkPlatform> filteredList;
    QList<SdkPlatform> platforms = m_config->sdkTargets(m_minApiLevel);
    foreach (const SdkPlatform &platform, platforms) {
        if (platform.abis.contains(abi()))
            filteredList << platform;
    }

    m_avdDialog.targetComboBox->clear();
    m_avdDialog.targetComboBox->addItems(AndroidConfig::apiLevelNamesFor(filteredList));

    if (platforms.isEmpty()) {
        m_avdDialog.warningIcon->setVisible(true);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create a new AVD. No sufficiently recent Android SDK available.\n"
                                            "Install an SDK of at least API version %1.")
                                         .arg(m_minApiLevel));
    } else if (filteredList.isEmpty()) {
        m_avdDialog.warningIcon->setVisible(true);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create a AVD for ABI %1. Install an image for it.")
                                         .arg(abi()));
    } else {
        m_avdDialog.warningIcon->setVisible(false);
        m_avdDialog.warningText->setVisible(false);
    }
}

bool AvdDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avdDialog.nameLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        const QString key = ke->text();
        if (!key.isEmpty() && !m_allowedNameChars.exactMatch(key)) {
            QPoint position = m_avdDialog.nameLineEdit->parentWidget()->mapToGlobal(m_avdDialog.nameLineEdit->geometry().bottomLeft());
            position -= Utils::ToolTip::offsetFromPosition();
            Utils::ToolTip::show(position, tr("Allowed characters are: a-z A-Z 0-9 and . _ -"), m_avdDialog.nameLineEdit);
            m_hideTipTimer.start();
        } else {
            m_hideTipTimer.stop();
            Utils::ToolTip::hide();
        }
    }
    return QDialog::eventFilter(obj, event);
}
