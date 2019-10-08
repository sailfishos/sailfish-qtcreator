/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "meremulatormodedetailswidget.h"
#include "ui_meremulatormodedetailswidget.h"

namespace Mer {
namespace Internal {

namespace {

enum Mode {
    Simple,
    Advanced
};

enum Orientation {
    None = 0,
    Portrait = (1 << 0),
    Landscape = (1 << 1),
    PortraitInverted = (1 << 2),
    LandscapeInverted = (1 << 3),
    All = (Portrait | Landscape | PortraitInverted | LandscapeInverted)
};

const QString SCHEMA = QStringLiteral("[desktop/sailfish/silica]");
const QString DEFAULT_ALLOWED_ORIENTATIONS = QStringLiteral("default_allowed_orientations");
const QString THEME_ICON_SUBDIR = QStringLiteral("theme_icon_subdir");
const QString THEME_PIXEL_RATIO = QStringLiteral("theme_pixel_ratio");
const QString WEBVIEW_CUSTOMLAYOUTWIDTH_SCALINGFACTOR = QStringLiteral("webview_customlayoutwidth_scalingfactor");

} // namespace anonymous

MerEmulatorModeDetailsWidget::MerEmulatorModeDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MerEmulatorModeDetailsWidget)
{
    ui->setupUi(this);

    onModeChanged(ui->dconfWidget->currentIndex());

    connect(ui->dconfWidget, &QTabWidget::currentChanged,
            this, &MerEmulatorModeDetailsWidget::onModeChanged);
    connect(ui->dconfTextEdit, &QPlainTextEdit::textChanged, [this]() {
        emit dconfChanged(ui->dconfTextEdit->toPlainText());
    });

    connect(ui->screenResolutionHeightSpinBox, &QAbstractSpinBox::editingFinished,
            this, &MerEmulatorModeDetailsWidget::onScreenResolutionChanged);
    connect(ui->screenResolutionWidthSpinBox, &QAbstractSpinBox::editingFinished,
            this, &MerEmulatorModeDetailsWidget::onScreenResolutionChanged);
    connect(ui->screenHeightSpinBox, &QAbstractSpinBox::editingFinished,
            this, &MerEmulatorModeDetailsWidget::onScreenSizeChanged);
    connect(ui->screenWidthSpinBox, &QAbstractSpinBox::editingFinished,
            this, &MerEmulatorModeDetailsWidget::onScreenSizeChanged);
    connect(ui->nameLineEdit, &QLineEdit::textEdited,
            this, &MerEmulatorModeDetailsWidget::onDeviceModelNameChanged);
}

MerEmulatorModeDetailsWidget::~MerEmulatorModeDetailsWidget()
{
    delete ui;
}

void MerEmulatorModeDetailsWidget::updateNameLineEdit()
{
    QPalette p = ui->nameLineEdit->palette();
    QColor color = Qt::black;
    QString toolTip = tr("Rename device model");

    const QString name = ui->nameLineEdit->text();
    if (m_deviceModelName != name && m_deviceModelsNames.contains(name)) {
        color = Qt::red;
        toolTip = tr("Device model name is not unique");
    }

    p.setColor(QPalette::Text, color);
    ui->nameLineEdit->setPalette(p);
    ui->nameLineEdit->setToolTip(toolTip);
}

void MerEmulatorModeDetailsWidget::onDeviceModelNameChanged()
{
    updateNameLineEdit();

    const QString newName = ui->nameLineEdit->text();
    emit deviceModelNameChanged(newName);
}

void MerEmulatorModeDetailsWidget::onScreenResolutionChanged()
{
    const QSize size(ui->screenResolutionWidthSpinBox->value(),
                     ui->screenResolutionHeightSpinBox->value());
    emit screenResolutionChanged(size);
}

void MerEmulatorModeDetailsWidget::onScreenSizeChanged()
{
    const QSize size(ui->screenWidthSpinBox->value(),
                     ui->screenHeightSpinBox->value());
    emit screenSizeChanged(size);
}

void MerEmulatorModeDetailsWidget::setDeviceModelsNames(const QStringList &deviceModelsNames)
{
    m_deviceModelsNames = deviceModelsNames;
}

void MerEmulatorModeDetailsWidget::setCurrentDeviceModel(const MerEmulatorDeviceModel &model)
{
    QTC_ASSERT(!model.isNull(), return);

    m_deviceModelName = model.name();
    const QSize resolution = model.displayResolution();
    const QSize size = model.displaySize();

    ui->dconfTextEdit->setPlainText(model.dconf().isEmpty()
                                      ? SCHEMA + "\n"
                                      : model.dconf());
    if (ui->dconfWidget->currentIndex() == Simple)
        onAdvancedOptionsChanged();

    const bool predefined = model.isSdkProvided();
    ui->screenHeightSpinBox->setValue(size.height());
    ui->screenWidthSpinBox->setValue(size.width());
    ui->screenResolutionHeightSpinBox->setValue(resolution.height());
    ui->screenResolutionWidthSpinBox->setValue(resolution.width());
    ui->nameLineEdit->setText(model.name());
    ui->autoDetectionValueLabel->setText(model.isSdkProvided() ? tr("Yes") : tr("No"));

    ui->nameLineEdit->setDisabled(predefined);
    ui->screenResolutionWidthSpinBox->setDisabled(predefined);
    ui->screenWidthSpinBox->setDisabled(predefined);
    ui->screenResolutionHeightSpinBox->setDisabled(predefined);
    ui->screenHeightSpinBox->setDisabled(predefined);

    ui->dconfTextEdit->setReadOnly(predefined);
    ui->orientationPortraitCheckBox->setDisabled(predefined);
    ui->orientationPortraitInvertedCheckBox->setDisabled(predefined);
    ui->orientationLandscapeCheckBox->setDisabled(predefined);
    ui->orientationLandscapeInvertedCheckBox->setDisabled(predefined);
    ui->scalingFactorSpinBox->setDisabled(predefined);
    ui->pixelRatioSpinBox->setDisabled(predefined);
    ui->iconSubdirLineEdit->setDisabled(predefined);

    updateNameLineEdit();
}

void MerEmulatorModeDetailsWidget::onModeChanged(int tabIndex)
{
    if (tabIndex == Simple) {
        disconnect(ui->dconfTextEdit, &QPlainTextEdit::textChanged,
                   this, &MerEmulatorModeDetailsWidget::onAdvancedOptionsChanged);

        connect(ui->pixelRatioSpinBox, &QDoubleSpinBox::editingFinished,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        connect(ui->scalingFactorSpinBox, &QDoubleSpinBox::editingFinished,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        connect(ui->orientationPortraitCheckBox, &QAbstractButton::clicked,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        connect(ui->orientationPortraitInvertedCheckBox, &QAbstractButton::clicked,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        connect(ui->orientationLandscapeCheckBox, &QAbstractButton::clicked,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        connect(ui->orientationLandscapeInvertedCheckBox, &QAbstractButton::clicked,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        connect(ui->iconSubdirLineEdit, &QLineEdit::editingFinished,
                this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
    } else {
        disconnect(ui->pixelRatioSpinBox, &QDoubleSpinBox::editingFinished,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        disconnect(ui->scalingFactorSpinBox, &QDoubleSpinBox::editingFinished,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        disconnect(ui->orientationPortraitCheckBox, &QAbstractButton::clicked,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        disconnect(ui->orientationPortraitInvertedCheckBox, &QAbstractButton::clicked,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        disconnect(ui->orientationLandscapeCheckBox, &QAbstractButton::clicked,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        disconnect(ui->orientationLandscapeInvertedCheckBox, &QAbstractButton::clicked,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);
        disconnect(ui->iconSubdirLineEdit, &QLineEdit::editingFinished,
                   this, &MerEmulatorModeDetailsWidget::onSimpleOptionsChanged);

        connect(ui->dconfTextEdit, &QPlainTextEdit::textChanged,
                this, &MerEmulatorModeDetailsWidget::onAdvancedOptionsChanged);
    }
}

void MerEmulatorModeDetailsWidget::onAdvancedOptionsChanged()
{
    QVariant value = getTextEditValue(THEME_PIXEL_RATIO);
    ui->pixelRatioSpinBox->setValue(value.toFloat());

    value = getTextEditValue(WEBVIEW_CUSTOMLAYOUTWIDTH_SCALINGFACTOR);
    ui->scalingFactorSpinBox->setValue(value.toFloat());

    value = getTextEditValue(THEME_ICON_SUBDIR);
    ui->iconSubdirLineEdit->setText(value.toString());

    value = getTextEditValue(DEFAULT_ALLOWED_ORIENTATIONS);
    const auto orientation = static_cast<Orientation>(value.toInt());

    ui->orientationPortraitCheckBox->setChecked(orientation & Orientation::Portrait);
    ui->orientationPortraitInvertedCheckBox->setChecked(orientation & Orientation::PortraitInverted);
    ui->orientationLandscapeCheckBox->setChecked(orientation & Orientation::Landscape);
    ui->orientationLandscapeInvertedCheckBox->setChecked(orientation & Orientation::LandscapeInverted);
}

QVariant MerEmulatorModeDetailsWidget::getTextEditValue(const QString &name) const
{
    const QString text = ui->dconfTextEdit->toPlainText();
    const QRegularExpression re(QString("%1\\s*=\\s*(.*)").arg(name));
    const QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch()) {
        QString str = match.captured(1);
        if (!str.isEmpty()) {
            if (str.startsWith("'") && str.endsWith("'"))
                str = str.mid(1, str.size() - 2);
            return QVariant(str);
        }
    }

    return QVariant();
}

void MerEmulatorModeDetailsWidget::updateTextEdit(const QString &name, const QVariant &value)
{
    QString text = ui->dconfTextEdit->toPlainText();
    const QRegularExpression re(QString("%1\\s*=\\s*(.*)\\n").arg(name));
    const QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch()) {
        if (match.captured(1).isEmpty() || value == 0 || value == "''") {
            text.remove(match.capturedStart(), match.capturedLength()); // remove entire matched string
            if (text.isEmpty())
                text = SCHEMA + "\n";
        } else {
            text.replace(match.capturedStart(1), match.capturedLength(1), value.toString());
        }
    } else {
        if (value != 0 && value != "''")
            text.append(QString("%1=%2\n").arg(name).arg(value.toString()));
    }

    ui->dconfTextEdit->setPlainText(text);
}

void MerEmulatorModeDetailsWidget::onSimpleOptionsChanged()
{
    updateTextEdit(THEME_PIXEL_RATIO, ui->pixelRatioSpinBox->value());
    updateTextEdit(WEBVIEW_CUSTOMLAYOUTWIDTH_SCALINGFACTOR, ui->scalingFactorSpinBox->value());
    updateTextEdit(THEME_ICON_SUBDIR, QString("'%1'").arg(ui->iconSubdirLineEdit->text()));

    QFlags<Orientation> f(Orientation::None);
    f.setFlag(Orientation::Portrait,
              ui->orientationPortraitCheckBox->isChecked());
    f.setFlag(Orientation::PortraitInverted,
              ui->orientationPortraitInvertedCheckBox->isChecked());
    f.setFlag(Orientation::Landscape,
              ui->orientationLandscapeCheckBox->isChecked());
    f.setFlag(Orientation::LandscapeInverted,
              ui->orientationLandscapeInvertedCheckBox->isChecked());
    updateTextEdit(DEFAULT_ALLOWED_ORIENTATIONS, QVariant(f));
}

} // namespace Internal
} // namespace Mer
