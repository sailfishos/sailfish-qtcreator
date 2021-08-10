/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "splashscreenwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

namespace {
static Q_LOGGING_CATEGORY(androidManifestEditorLog, "qtc.android.splashScreenWidget", QtWarningMsg)
const auto fileDialogImageFiles = QStringLiteral("%1 (*.png *.jpg)").arg(QWidget::tr("Images"));
QString manifestDir(TextEditor::TextEditorWidget *textEditorWidget)
{
    // Get the manifest file's directory from its filepath.
    return textEditorWidget->textDocument()->filePath().toFileInfo().absolutePath();
}
}

SplashScreenWidget::SplashScreenWidget(QWidget *parent) : QWidget(parent)
{
}

SplashScreenWidget::SplashScreenWidget(
        QWidget *parent,
        const QSize &size, const QSize &screenSize,
        const QString &title, const QString &tooltip,
        const QString &imagePath,
        int scalingRatio, int maxScalingRatio,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QWidget(parent),
      m_textEditorWidget(textEditorWidget),
      m_scalingRatio(scalingRatio),
      m_maxScalingRatio(maxScalingRatio),
      m_imagePath(imagePath),
      m_screenSize(screenSize)
{
    auto splashLayout = new QVBoxLayout(this);
    auto splashTitle = new QLabel(title, this);
    auto splashButtonLayout = new QGridLayout();
    m_splashScreenButton = new SplashScreenButton(this);
    m_splashScreenButton->setMinimumSize(size);
    m_splashScreenButton->setMaximumSize(size);
    m_splashScreenButton->setToolTip(tooltip);
    QSize clearAndWarningSize(16, 16);
    QToolButton *clearButton = nullptr;
    if (textEditorWidget) {
        clearButton = new QToolButton(this);
        clearButton->setMinimumSize(clearAndWarningSize);
        clearButton->setMaximumSize(clearAndWarningSize);
        clearButton->setIcon(Utils::Icons::CLOSE_FOREGROUND.icon());
    }
    if (textEditorWidget) {
        m_scaleWarningLabel = new QLabel(this);
        m_scaleWarningLabel->setMinimumSize(clearAndWarningSize);
        m_scaleWarningLabel->setMaximumSize(clearAndWarningSize);
        m_scaleWarningLabel->setPixmap(Utils::Icons::WARNING.icon().pixmap(clearAndWarningSize));
        m_scaleWarningLabel->setToolTip(tr("Icon scaled up."));
        m_scaleWarningLabel->setVisible(false);
    }
    auto label = new QLabel(tr("Click to select..."), parent);
    splashLayout->addWidget(splashTitle);
    splashLayout->setAlignment(splashTitle, Qt::AlignHCenter);
    splashButtonLayout->setColumnMinimumWidth(0, 16);
    splashButtonLayout->addWidget(m_splashScreenButton, 0, 1, 1, 3);
    splashButtonLayout->setAlignment(m_splashScreenButton, Qt::AlignVCenter);
    if (textEditorWidget) {
        splashButtonLayout->addWidget(clearButton, 0, 4, 1, 1);
        splashButtonLayout->setAlignment(clearButton, Qt::AlignTop);
    }
    if (textEditorWidget) {
        splashButtonLayout->addWidget(m_scaleWarningLabel, 0, 0, 1, 1);
        splashButtonLayout->setAlignment(m_scaleWarningLabel, Qt::AlignTop);
    }
    splashLayout->addLayout(splashButtonLayout);
    splashLayout->setAlignment(splashButtonLayout, Qt::AlignHCenter);
    splashLayout->addWidget(label);
    splashLayout->setAlignment(label, Qt::AlignHCenter);
    this->setLayout(splashLayout);
    connect(m_splashScreenButton, &QAbstractButton::clicked,
            this, &SplashScreenWidget::selectImage);
    if (clearButton)
        connect(clearButton, &QAbstractButton::clicked,
                this, &SplashScreenWidget::removeImage);
    m_imageSelectionText = tooltip;
}

SplashScreenWidget::SplashScreenButton::SplashScreenButton(SplashScreenWidget *parent)
    : QToolButton(parent),
      m_parentWidget(parent)
{
}

void SplashScreenWidget::SplashScreenButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setPen(QPen(Qt::gray, 1));
    painter.setBrush(QBrush(m_parentWidget->m_backgroundColor));
    painter.drawRect(0, 0, width()-1, height()-1);
    if (!m_parentWidget->m_image.isNull())
        painter.drawImage(m_parentWidget->m_imagePosition, m_parentWidget->m_image);
}

void SplashScreenWidget::setBackgroundColor(const QColor &backgroundColor)
{
    m_backgroundColor = backgroundColor;
    m_splashScreenButton->update();
    emit imageChanged();
}

void SplashScreenWidget::showImageFullScreen(bool fullScreen)
{
    m_showImageFullScreen = fullScreen;
    loadImage();
}

void SplashScreenWidget::setImageFromPath(const QString &imagePath, bool resize)
{
    if (!m_textEditorWidget)
        return;
    const QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_imagePath + m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path is empty, cannot set image.";
        return;
    }
    QImage image = QImage(imagePath);
    if (image.isNull()) {
        qCDebug(androidManifestEditorLog) << "Image '" << imagePath << "' not found or invalid format.";
        return;
    }
    QDir dir;
    if (!dir.mkpath(QFileInfo(targetPath).absolutePath())) {
        qCDebug(androidManifestEditorLog) << "Cannot create image target path.";
        return;
    }
    if (resize == true && m_scalingRatio < m_maxScalingRatio) {
        const QSize size = QSize((float(image.width()) / float(m_maxScalingRatio)) * float(m_scalingRatio),
                                 (float(image.height()) / float(m_maxScalingRatio)) * float(m_scalingRatio));
        image = image.scaled(size);
    }
    QFile file(targetPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        image.save(&file, "PNG");
        file.close();
        loadImage();
        emit imageChanged();
    }
    else {
        qCDebug(androidManifestEditorLog) << "Cannot save image.";
    }
}

void SplashScreenWidget::selectImage()
{
    const QString file = QFileDialog::getOpenFileName(this, m_imageSelectionText,
                                                      QDir::homePath(), fileDialogImageFiles);
    if (file.isEmpty())
        return;
    setImageFromPath(file, false);
    emit imageChanged();
}

void SplashScreenWidget::removeImage()
{
    const QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_imagePath + m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path empty, cannot remove image.";
        return;
    }
    QFile::remove(targetPath);
    m_image = QImage();
    m_splashScreenButton->update();
    setScaleWarningLabelVisible(false);
}

void SplashScreenWidget::clearImage()
{
    removeImage();
    emit imageChanged();
}

void SplashScreenWidget::loadImage()
{
    if (m_imageFileName.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image name not set, cannot load image.";
        return;
    }
    const QString baseDir = manifestDir(m_textEditorWidget);
    const QString targetPath = baseDir + m_imagePath + m_imageFileName;
    if (targetPath.isEmpty()) {
        qCDebug(androidManifestEditorLog) << "Image target path empty, cannot load image.";
        return;
    }
    QImage image = QImage(targetPath);
    if (image.isNull()) {
        qCDebug(androidManifestEditorLog) << "Cannot load image.";
        return;
    }
    if (m_showImageFullScreen) {
        m_image = image.scaled(m_splashScreenButton->size());
        m_imagePosition = QPoint(0,0);
    }
    else {
        QSize scaledSize = QSize((m_splashScreenButton->width() * image.width()) / m_screenSize.width(),
                                 (m_splashScreenButton->height() * image.height()) / m_screenSize.height());
        m_image = image.scaled(scaledSize, Qt::KeepAspectRatio);
        m_imagePosition = QPoint((m_splashScreenButton->width() - m_image.width()) / 2,
                                 (m_splashScreenButton->height() - m_image.height()) / 2);
    }
    m_splashScreenButton->update();
}

bool SplashScreenWidget::hasImage() const
{
    return !m_image.isNull();
}

void SplashScreenWidget::setScaleWarningLabelVisible(bool visible)
{
    if (m_scaleWarningLabel)
        m_scaleWarningLabel->setVisible(visible);
}

void SplashScreenWidget::setImageFileName(const QString &imageFileName)
{
    m_imageFileName = imageFileName;
}

} // namespace Internal
} // namespace Android
