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

#include "splashscreencontainerwidget.h"
#include "splashscreenwidget.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace Android {
namespace Internal {

namespace {
const QString extraExtraExtraHighDpiImagePath = QLatin1String("/res/drawable-xxxhdpi/");
const QString extraExtraHighDpiImagePath = QLatin1String("/res/drawable-xxhdpi/");
const QString extraHighDpiImagePath = QLatin1String("/res/drawable-xhdpi/");
const QString highDpiImagePath = QLatin1String("/res/drawable-hdpi/");
const QString mediumDpiImagePath = QLatin1String("/res/drawable-mdpi/");
const QString lowDpiImagePath = QLatin1String("/res/drawable-ldpi/");
const QString splashscreenName = QLatin1String("splashscreen");
const QString splashscreenPortraitName = QLatin1String("splashscreen_port");
const QString splashscreenLandscapeName = QLatin1String("splashscreen_land");
const QString splashscreenFileName = QLatin1String("logo");
const QString splashscreenPortraitFileName = QLatin1String("logo_port");
const QString splashscreenLandscapeFileName = QLatin1String("logo_land");
const QString imageSuffix = QLatin1String(".png");
const QString fileDialogImageFiles = QString(QWidget::tr("Images (*.png *.jpg)"));
const QSize lowDpiImageSize{200, 320};
const QSize mediumDpiImageSize{320, 480};
const QSize highDpiImageSize{480, 800};
const QSize extraHighDpiImageSize{720, 1280};
const QSize extraExtraHighDpiImageSize{960, 1600};
const QSize extraExtraExtraHighDpiImageSize{1280, 1920};
const QSize displaySize{48, 72};
const QSize landscapeDisplaySize{72, 48};
// https://developer.android.com/training/multiscreen/screendensities#TaskProvideAltBmp
const int extraExtraExtraHighDpiScalingRatio = 16;
const int extraExtraHighDpiScalingRatio = 12;
const int extraHighDpiScalingRatio = 8;
const int highDpiScalingRatio = 6;
const int mediumDpiScalingRatio = 4;
const int lowDpiScalingRatio = 3;

QString manifestDir(TextEditor::TextEditorWidget *textEditorWidget)
{
    // Get the manifest file's directory from its filepath.
    return textEditorWidget->textDocument()->filePath().toFileInfo().absolutePath();
}

}

static SplashScreenWidget *addWidgetToPage(QWidget *page,
                                           const QSize &size, const QSize &screenSize,
                                           const QString &title, const QString &tooltip,
                                           TextEditor::TextEditorWidget *textEditorWidget,
                                           const QString &splashScreenPath,
                                           int scalingRatio, int maxScalingRatio,
                                           QHBoxLayout *pageLayout,
                                           QVector<SplashScreenWidget *> &widgetContainer)
{
    auto splashScreenWidget = new SplashScreenWidget(page,
                                                     size,
                                                     screenSize,
                                                     title,
                                                     tooltip,
                                                     splashScreenPath,
                                                     scalingRatio,
                                                     maxScalingRatio,
                                                     textEditorWidget);
    pageLayout->addWidget(splashScreenWidget);
    widgetContainer.push_back(splashScreenWidget);
    return splashScreenWidget;
}

static QWidget *createPage(TextEditor::TextEditorWidget *textEditorWidget,
                           QVector<SplashScreenWidget *> &widgetContainer,
                           QVector<SplashScreenWidget *> &portraitWidgetContainer,
                           QVector<SplashScreenWidget *> &landscapeWidgetContainer,
                           int scalingRatio,
                           const QSize &size,
                           const QSize &portraitSize,
                           const QSize &landscapeSize,
                           const QString &path)
{
    auto sizeToStr = [](const QSize &size) { return QString(" (%1x%2)").arg(size.width()).arg(size.height()); };
    QWidget *page = new QWidget();
    auto pageLayout = new QHBoxLayout(page);
    auto genericWidget= addWidgetToPage(page,
                                        displaySize, size,
                                        SplashScreenContainerWidget::tr("Splash screen"),
                                        SplashScreenContainerWidget::tr("Select splash screen image")
                                        + sizeToStr(size),
                                        textEditorWidget,
                                        path,
                                        scalingRatio, extraExtraExtraHighDpiScalingRatio,
                                        pageLayout,
                                        widgetContainer);

    auto portraitWidget = addWidgetToPage(page,
                                          displaySize, portraitSize,
                                          SplashScreenContainerWidget::tr("Portrait splash screen"),
                                          SplashScreenContainerWidget::tr("Select portrait splash screen image")
                                          + sizeToStr(portraitSize),
                                          textEditorWidget,
                                          path,
                                          scalingRatio, extraExtraExtraHighDpiScalingRatio,
                                          pageLayout,
                                          portraitWidgetContainer);

    auto landscapeWidget = addWidgetToPage(page,
                                           landscapeDisplaySize, landscapeSize,
                                           SplashScreenContainerWidget::tr("Landscape splash screen"),
                                           SplashScreenContainerWidget::tr("Select landscape splash screen image")
                                           + sizeToStr(landscapeSize),
                                           textEditorWidget,
                                           path,
                                           scalingRatio, extraExtraExtraHighDpiScalingRatio,
                                           pageLayout,
                                           landscapeWidgetContainer);

    auto clearButton = new QToolButton(page);
    clearButton->setText(SplashScreenContainerWidget::tr("Clear All"));
    pageLayout->addWidget(clearButton);
    pageLayout->setAlignment(clearButton, Qt::AlignVCenter);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       genericWidget, &SplashScreenWidget::clearImage);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       portraitWidget, &SplashScreenWidget::clearImage);
    SplashScreenContainerWidget::connect(clearButton, &QAbstractButton::clicked,
                                       landscapeWidget, &SplashScreenWidget::clearImage);
    return page;
}


SplashScreenContainerWidget::SplashScreenContainerWidget(
        QWidget *parent,
        TextEditor::TextEditorWidget *textEditorWidget)
    : QStackedWidget(parent),
      m_textEditorWidget(textEditorWidget)
{
    auto noSplashscreenWidget = new QWidget(this);
    auto splashscreenWidget = new QWidget(this);
    auto layout = new QHBoxLayout(this);
    auto settingsLayout = new QVBoxLayout(this);
    auto noSplashscreenLayout = new QVBoxLayout(this);
    auto formLayout = new QFormLayout(this);
    QTabWidget *tab = new QTabWidget(this);

    m_stickyCheck = new QCheckBox(this);
    m_stickyCheck->setToolTip(tr("A non-sticky splash screen is hidden automatically when an activity is drawn.\n"
                                 "To hide a sticky splash screen, invoke QtAndroid::hideSplashScreen()."));
    formLayout->addRow(tr("Sticky splash screen:"), m_stickyCheck);

    m_imageShowMode = new QComboBox(this);
    formLayout->addRow(tr("Image show mode:"), m_imageShowMode);
    const QList<QStringList> imageShowModeMethodsMap = {
        {"center", "Place the object in the center of the screen in both the vertical and horizontal axis,\n"
                   "not changing its size."},
        {"fill", "Grow the horizontal and vertical size of the image if needed so it completely fills its screen."}};
    for (int i = 0; i < imageShowModeMethodsMap.size(); ++i) {
        m_imageShowMode->addItem(imageShowModeMethodsMap.at(i).first());
        m_imageShowMode->setItemData(i, imageShowModeMethodsMap.at(i).at(1), Qt::ToolTipRole);
    }

    m_backgroundColor = new QToolButton(this);
    m_backgroundColor->setToolTip(tr("Background color of the splash screen."));
    formLayout->addRow(tr("Background color:"), m_backgroundColor);

    m_masterImage = new QToolButton(this);
    m_masterImage->setToolTip(tr("Select master image to use."));
    m_masterImage->setIcon(QIcon::fromTheme(QLatin1String("document-open"), Utils::Icons::OPENFILE.icon()));
    formLayout->addRow(tr("Master image:"), m_masterImage);

    m_portraitMasterImage = new QToolButton(this);
    m_portraitMasterImage->setToolTip(tr("Select portrait master image to use."));
    m_portraitMasterImage->setIcon(QIcon::fromTheme(QLatin1String("document-open"), Utils::Icons::OPENFILE.icon()));
    formLayout->addRow(tr("Portrait master image:"), m_portraitMasterImage);

    m_landscapeMasterImage = new QToolButton(this);
    m_landscapeMasterImage->setToolTip(tr("Select landscape master image to use."));
    m_landscapeMasterImage->setIcon(QIcon::fromTheme(QLatin1String("document-open"), Utils::Icons::OPENFILE.icon()));
    formLayout->addRow(tr("Landscape master image:"), m_landscapeMasterImage);

    auto clearAllButton = new QToolButton(this);
    clearAllButton->setText(SplashScreenContainerWidget::tr("Clear All"));

    auto ldpiPage = createPage(textEditorWidget,
                               m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                               lowDpiScalingRatio,
                               lowDpiImageSize,
                               lowDpiImageSize,
                               lowDpiImageSize.transposed(),
                               lowDpiImagePath);
    tab->addTab(ldpiPage, tr("LDPI"));
    auto mdpiPage = createPage(textEditorWidget,
                               m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                               mediumDpiScalingRatio,
                               mediumDpiImageSize,
                               mediumDpiImageSize,
                               mediumDpiImageSize.transposed(),
                               mediumDpiImagePath);
    tab->addTab(mdpiPage, tr("MDPI"));
    auto hdpiPage = createPage(textEditorWidget,
                               m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                               highDpiScalingRatio,
                               highDpiImageSize,
                               highDpiImageSize,
                               highDpiImageSize.transposed(),
                               highDpiImagePath);
    tab->addTab(hdpiPage, tr("HDPI"));
    auto xHdpiPage = createPage(textEditorWidget,
                                m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                                extraHighDpiScalingRatio,
                                extraHighDpiImageSize,
                                extraHighDpiImageSize,
                                extraHighDpiImageSize.transposed(),
                                extraHighDpiImagePath);
    tab->addTab(xHdpiPage, tr("XHDPI"));
    auto xxHdpiPage = createPage(textEditorWidget,
                                 m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                                 extraExtraHighDpiScalingRatio,
                                 extraExtraHighDpiImageSize,
                                 extraExtraHighDpiImageSize,
                                 extraExtraHighDpiImageSize.transposed(),
                                 extraExtraHighDpiImagePath);
    tab->addTab(xxHdpiPage, tr("XXHDPI"));
    auto xxxHdpiPage = createPage(textEditorWidget,
                                  m_imageWidgets, m_portraitImageWidgets, m_landscapeImageWidgets,
                                  extraExtraExtraHighDpiScalingRatio,
                                  extraExtraExtraHighDpiImageSize,
                                  extraExtraExtraHighDpiImageSize,
                                  extraExtraExtraHighDpiImageSize.transposed(),
                                  extraExtraExtraHighDpiImagePath);
    tab->addTab(xxxHdpiPage, tr("XXXHDPI"));
    formLayout->setContentsMargins(10, 10, 10, 10);
    formLayout->setSpacing(10);
    settingsLayout->addLayout(formLayout);
    settingsLayout->addWidget(clearAllButton);
    settingsLayout->setAlignment(clearAllButton, Qt::AlignHCenter);
    layout->addLayout(settingsLayout);
    layout->addWidget(tab);
    splashscreenWidget->setLayout(layout);
    addWidget(splashscreenWidget);
    setBackgroundColor(Qt::white);

    auto warningLabel = new QLabel(this);
    warningLabel->setAlignment(Qt::AlignHCenter);
    warningLabel->setText(tr("An image is used for the splashscreen. Qt Creator manages\n"
                             "splashscreen by using a different method which requires changing\n"
                             "the manifest file by overriding your settings. Allow override?"));
    m_convertSplashscreen = new QToolButton(this);
    m_convertSplashscreen->setText(tr("Convert"));
    noSplashscreenLayout->addStretch();
    noSplashscreenLayout->addWidget(warningLabel);
    noSplashscreenLayout->addWidget(m_convertSplashscreen);
    noSplashscreenLayout->addStretch();
    noSplashscreenLayout->setSpacing(10);
    noSplashscreenLayout->setAlignment(warningLabel, Qt::AlignHCenter);
    noSplashscreenLayout->setAlignment(m_convertSplashscreen, Qt::AlignHCenter);
    noSplashscreenWidget->setLayout(noSplashscreenLayout);
    addWidget(noSplashscreenWidget);

    for (auto &&imageWidget : m_imageWidgets)
        imageWidget->setImageFileName(splashscreenFileName + imageSuffix);
    for (auto &&imageWidget : m_portraitImageWidgets)
        imageWidget->setImageFileName(splashscreenPortraitFileName + imageSuffix);
    for (auto &&imageWidget : m_landscapeImageWidgets)
        imageWidget->setImageFileName(splashscreenLandscapeFileName + imageSuffix);

    for (auto &&imageWidget : m_imageWidgets) {
        connect(imageWidget, &SplashScreenWidget::imageChanged, [this]() {
            createSplashscreenThemes();
            emit splashScreensModified();
        });
    }
    for (auto &&imageWidget : m_portraitImageWidgets) {
        connect(imageWidget, &SplashScreenWidget::imageChanged, [this]() {
            createSplashscreenThemes();
            emit splashScreensModified();
        });
    }
    for (auto &&imageWidget : m_landscapeImageWidgets) {
        connect(imageWidget, &SplashScreenWidget::imageChanged, [this]() {
            createSplashscreenThemes();
            emit splashScreensModified();
        });
    }
    connect(m_stickyCheck, &QCheckBox::stateChanged, [this](int state) {
        bool old = m_splashScreenSticky;
        m_splashScreenSticky = (state == Qt::Checked);
        if (old != m_splashScreenSticky)
            emit splashScreensModified();
    });
    connect(m_backgroundColor, &QToolButton::clicked, [this]() {
        const QColor color = QColorDialog::getColor(m_splashScreenBackgroundColor,
                                                    this,
                                                    tr("Select background color"));
        if (color.isValid()) {
            setBackgroundColor(color);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_masterImage, &QToolButton::clicked, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, tr("Select master image"),
                                                    QDir::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_imageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_portraitMasterImage, &QToolButton::clicked, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, tr("Select portrait master image"),
                                                    QDir::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_portraitImageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_landscapeMasterImage, &QToolButton::clicked, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, tr("Select landscape master image"),
                                                    QDir::homePath(), fileDialogImageFiles);
        if (!file.isEmpty()) {
            for (auto &&imageWidget : m_landscapeImageWidgets)
                imageWidget->setImageFromPath(file);
            createSplashscreenThemes();
            emit splashScreensModified();
        }
    });
    connect(m_imageShowMode, &QComboBox::currentTextChanged, [this](const QString &mode) {
        setImageShowMode(mode);
        createSplashscreenThemes();
        emit splashScreensModified();
    });
    connect(clearAllButton, &QToolButton::clicked, [this]() {
        clearAll();
        emit splashScreensModified();
    });
    connect(m_convertSplashscreen, &QToolButton::clicked, [this]() {
        clearAll();
        setCurrentIndex(0);
        emit splashScreensModified();
    });
}

void SplashScreenContainerWidget::loadImages()
{
    if (isSplashscreenEnabled()) {
        for (auto &&imageWidget : m_imageWidgets) {
            imageWidget->loadImage();
        }
        loadSplashscreenDrawParams(splashscreenName);
        for (auto &&imageWidget : m_portraitImageWidgets) {
            imageWidget->loadImage();
        }
        loadSplashscreenDrawParams(splashscreenPortraitName);
        for (auto &&imageWidget : m_landscapeImageWidgets) {
            imageWidget->loadImage();
        }
        loadSplashscreenDrawParams(splashscreenLandscapeName);
    }
}

void SplashScreenContainerWidget::loadSplashscreenDrawParams(const QString &name)
{
    QFile file(QLatin1String("%1/res/drawable/%2.xml").arg(manifestDir(m_textEditorWidget)).arg(name));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QXmlStreamReader reader(&file);
        reader.setNamespaceProcessing(false);
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.hasError()) {
                // This should not happen
                return;
            } else {
                if (reader.name() == QLatin1String("solid")) {
                    const QXmlStreamAttributes attributes = reader.attributes();
                    const auto color = attributes.value(QLatin1String("android:color"));
                    if (!color.isEmpty())
                        setBackgroundColor(QColor(color));
                }
                else if (reader.name() == QLatin1String("bitmap")) {
                    const QXmlStreamAttributes attributes = reader.attributes();
                    const auto showMode = attributes.value(QLatin1String("android:gravity"));
                    if (!showMode.isEmpty())
                        setImageShowMode(showMode.toString());
                }
            }
        }
    }
}

void SplashScreenContainerWidget::clearAll()
{
    for (auto &&imageWidget : m_imageWidgets) {
        imageWidget->clearImage();
    }
    for (auto &&imageWidget : m_portraitImageWidgets) {
        imageWidget->clearImage();
    }
    for (auto &&imageWidget : m_landscapeImageWidgets) {
        imageWidget->clearImage();
    }
    setBackgroundColor(Qt::white);
    createSplashscreenThemes();
}

bool SplashScreenContainerWidget::hasImages() const
{
    for (auto &&imageWidget : m_imageWidgets) {
        if (imageWidget->hasImage())
            return true;
    }
    return false;
}

bool SplashScreenContainerWidget::hasPortraitImages() const
{
    for (auto &&imageWidget : m_portraitImageWidgets) {
        if (imageWidget->hasImage())
            return true;
    }
    return false;
}

bool SplashScreenContainerWidget::hasLandscapeImages() const
{
    for (auto &&imageWidget : m_landscapeImageWidgets) {
        if (imageWidget->hasImage())
            return true;
    }
    return false;
}

bool SplashScreenContainerWidget::isSticky() const
{
    return m_splashScreenSticky;
}

void SplashScreenContainerWidget::setSticky(bool sticky)
{
    m_splashScreenSticky = sticky;
    m_stickyCheck->setCheckState(m_splashScreenSticky ? Qt::Checked : Qt::Unchecked);
}

void SplashScreenContainerWidget::setBackgroundColor(const QColor &color)
{
    if (color != m_splashScreenBackgroundColor) {
        m_backgroundColor->setStyleSheet(QString("QToolButton {background-color: %1; border: 1px solid gray;}").arg(color.name()));

        for (auto &&imageWidget : m_imageWidgets)
            imageWidget->setBackgroundColor(color);
        for (auto &&imageWidget : m_portraitImageWidgets)
            imageWidget->setBackgroundColor(color);
        for (auto &&imageWidget : m_landscapeImageWidgets)
            imageWidget->setBackgroundColor(color);

        m_splashScreenBackgroundColor = color;
    }
}

void SplashScreenContainerWidget::setImageShowMode(const QString &mode)
{
    bool imageFullScreen;

    if (mode == "center")
        imageFullScreen = false;
    else if (mode == "fill")
        imageFullScreen = true;
    else
        return;

    for (auto &&imageWidget : m_imageWidgets)
        imageWidget->showImageFullScreen(imageFullScreen);
    for (auto &&imageWidget : m_portraitImageWidgets)
        imageWidget->showImageFullScreen(imageFullScreen);
    for (auto &&imageWidget : m_landscapeImageWidgets)
        imageWidget->showImageFullScreen(imageFullScreen);

    m_imageShowMode->setCurrentText(mode);
}

void SplashScreenContainerWidget::createSplashscreenThemes()
{
    const QString baseDir = manifestDir(m_textEditorWidget);
    const QStringList splashscreenThemeFiles = {"/res/values/splashscreentheme.xml",
                                                "/res/values-port/splashscreentheme.xml",
                                                "/res/values-land/splashscreentheme.xml"};
    const QStringList splashscreenDrawableFiles = {QString("/res/drawable/%1.xml").arg(splashscreenName),
                                                   QString("/res/drawable/%1.xml").arg(splashscreenPortraitName),
                                                   QString("/res/drawable/%1.xml").arg(splashscreenLandscapeName)};
    QStringList splashscreens[3];

    if (hasImages())
        splashscreens[0] << splashscreenName << splashscreenFileName;
    if (hasPortraitImages())
        splashscreens[1] << splashscreenPortraitName << splashscreenPortraitFileName;
    if (hasLandscapeImages())
        splashscreens[2] << splashscreenLandscapeName << splashscreenLandscapeFileName;

    for (int i = 0; i < 3; i++) {
        if (!splashscreens[i].isEmpty()) {
            QDir dir;
            QFile themeFile(baseDir + splashscreenThemeFiles[i]);
            dir.mkpath(QFileInfo(themeFile).absolutePath());
            if (themeFile.open(QFile::WriteOnly | QFile::Truncate)) {
                QXmlStreamWriter stream(&themeFile);
                stream.setAutoFormatting(true);
                stream.writeStartDocument();
                stream.writeStartElement("resources");
                stream.writeStartElement("style");
                stream.writeAttribute("name", "splashScreenTheme");
                stream.writeStartElement("item");
                stream.writeAttribute("name", "android:windowBackground");
                stream.writeCharacters(QLatin1String("@drawable/%1").arg(splashscreens[i].at(0)));
                stream.writeEndElement(); // item
                stream.writeEndElement(); // style
                stream.writeEndElement(); // resources
                stream.writeEndDocument();
                themeFile.close();
            }
            QFile drawableFile(baseDir + splashscreenDrawableFiles[i]);
            dir.mkpath(QFileInfo(drawableFile).absolutePath());
            if (drawableFile.open(QFile::WriteOnly | QFile::Truncate)) {
                QXmlStreamWriter stream(&drawableFile);
                stream.setAutoFormatting(true);
                stream.writeStartDocument();
                stream.writeStartElement("layer-list");
                stream.writeAttribute("xmlns:android", "http://schemas.android.com/apk/res/android");
                stream.writeStartElement("item");
                stream.writeStartElement("shape");
                stream.writeAttribute("android:shape", "rectangle");
                stream.writeEmptyElement("solid");
                stream.writeAttribute("android:color", m_splashScreenBackgroundColor.name());
                stream.writeEndElement(); // shape
                stream.writeEndElement(); // item
                stream.writeStartElement("item");
                stream.writeEmptyElement("bitmap");
                stream.writeAttribute("android:src", QLatin1String("@drawable/%1").arg(splashscreens[i].at(1)));
                stream.writeAttribute("android:gravity", m_imageShowMode->currentText());
                stream.writeEndElement(); // item
                stream.writeEndElement(); // layer-list
                stream.writeEndDocument();
                drawableFile.close();
            }
        }
        else {
            QFile::remove(baseDir + splashscreenThemeFiles[i]);
            QFile::remove(baseDir + splashscreenDrawableFiles[i]);
        }
    }
}

void SplashScreenContainerWidget::checkSplashscreenImage(const QString &name)
{
    if (isSplashscreenEnabled()) {
        const QString baseDir = manifestDir(m_textEditorWidget);
        const QStringList paths = {extraExtraExtraHighDpiImagePath,
                                   extraExtraHighDpiImagePath,
                                   extraHighDpiImagePath,
                                   highDpiImagePath,
                                   mediumDpiImagePath,
                                   lowDpiImagePath};

        for (const QString &path : paths) {
            const QString filePath(baseDir + path + name);
            if (QFile::exists(filePath + ".png")
             || QFile::exists(filePath + ".jpg")) {
                setCurrentIndex(1);
                break;
            }
        }
    }
}

bool SplashScreenContainerWidget::isSplashscreenEnabled()
{
    return (currentIndex() == 0);
}

QString SplashScreenContainerWidget::imageName() const
{
    return splashscreenName;
}

QString SplashScreenContainerWidget::portraitImageName() const
{
    return splashscreenPortraitName;
}

QString SplashScreenContainerWidget::landscapeImageName() const
{
    return splashscreenLandscapeName;
}

} // namespace Internal
} // namespace Android
