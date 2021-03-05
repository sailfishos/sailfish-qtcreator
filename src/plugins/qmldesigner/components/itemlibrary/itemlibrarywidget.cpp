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

#include "itemlibrarywidget.h"

#include "customfilesystemmodel.h"
#include "itemlibraryassetimportdialog.h"
#include "itemlibraryiconimageprovider.h"

#include <theme.h>

#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <itemlibraryimageprovider.h>
#include <itemlibraryinfo.h>
#include <itemlibrarymodel.h>
#include <metainfo.h>
#include <model.h>
#include <previewtooltip/previewtooltipbackend.h>
#include <rewritingexception.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/flowlayout.h>
#include <utils/fileutils.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#ifdef IMPORT_QUICK3D_ASSETS
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#endif

#include <QApplication>
#include <QDrag>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QGridLayout>
#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include <QQmlContext>
#include <QQuickItem>

namespace QmlDesigner {

static QString propertyEditorResourcesPath() {
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/propertyEditorQmlSources");
}

ItemLibraryWidget::ItemLibraryWidget(ImageCache &imageCache)
    : m_itemIconSize(24, 24)
    , m_itemViewQuickWidget(new QQuickWidget(this))
    , m_resourcesView(new ItemLibraryResourceView(this))
    , m_importTagsWidget(new QWidget(this))
    , m_addResourcesWidget(new QWidget(this))
    , m_imageCache{imageCache}
    , m_filterFlag(QtBasic)
{
    m_compressionTimer.setInterval(200);
    m_compressionTimer.setSingleShot(true);
    ItemLibraryModel::registerQmlTypes();

    setWindowTitle(tr("Library", "Title of library view"));

    /* create Items view and its model */
    m_itemViewQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_itemViewQuickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_itemLibraryModel = new ItemLibraryModel(this);

    m_itemViewQuickWidget->rootContext()->setContextProperties(QVector<QQmlContext::PropertyPair>{
        {{"itemLibraryModel"}, QVariant::fromValue(m_itemLibraryModel.data())},
        {{"itemLibraryIconWidth"}, m_itemIconSize.width()},
        {{"itemLibraryIconHeight"}, m_itemIconSize.height()},
        {{"rootView"}, QVariant::fromValue(this)},
        {{"highlightColor"}, Utils::StyleHelper::notTooBrightHighlightColor()},
    });

    m_previewTooltipBackend = std::make_unique<PreviewTooltipBackend>(m_imageCache);
    m_itemViewQuickWidget->rootContext()->setContextProperty("tooltipBackend",
                                                             m_previewTooltipBackend.get());

    m_itemViewQuickWidget->setClearColor(
        Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));

    /* create Resources view and its model */
    m_resourcesFileSystemModel = new CustomFileSystemModel(this);
    m_resourcesView->setModel(m_resourcesFileSystemModel.data());

    /* create image provider for loading item icons */
    m_itemViewQuickWidget->engine()->addImageProvider(QStringLiteral("qmldesigner_itemlibrary"), new Internal::ItemLibraryImageProvider);

    Theme::setupTheme(m_itemViewQuickWidget->engine());

    /* other widgets */
    auto tabBar = new QTabBar(this);
    tabBar->addTab(tr("QML Types", "Title of library QML types view"));
    tabBar->addTab(tr("Assets", "Title of library assets view"));
    tabBar->addTab(tr("QML Imports", "Title of QML imports view"));
    tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(tabBar, &QTabBar::currentChanged, this, &ItemLibraryWidget::setCurrentIndexOfStackedWidget);
    connect(tabBar, &QTabBar::currentChanged, this, &ItemLibraryWidget::updateSearch);

    m_filterLineEdit = new Utils::FancyLineEdit(this);
    m_filterLineEdit->setObjectName(QStringLiteral("itemLibrarySearchInput"));
    m_filterLineEdit->setPlaceholderText(tr("<Filter>", "Library search input hint text"));
    m_filterLineEdit->setDragEnabled(false);
    m_filterLineEdit->setMinimumWidth(75);
    m_filterLineEdit->setTextMargins(0, 0, 20, 0);
    m_filterLineEdit->setFiltering(true);
    QWidget *lineEditFrame = new QWidget(this);
    lineEditFrame->setObjectName(QStringLiteral("itemLibrarySearchInputFrame"));
    auto lineEditLayout = new QGridLayout(lineEditFrame);
    lineEditLayout->setContentsMargins(2, 2, 2, 2);
    lineEditLayout->setSpacing(0);
    lineEditLayout->addItem(new QSpacerItem(5, 3, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 0, 1, 3);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0);
    lineEditLayout->addWidget(m_filterLineEdit.data(), 1, 1, 1, 1);
    lineEditLayout->addItem(new QSpacerItem(5, 5, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 2);
    connect(m_filterLineEdit.data(), &Utils::FancyLineEdit::filterChanged, this, &ItemLibraryWidget::setSearchFilter);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(m_itemViewQuickWidget.data());
    m_stackedWidget->addWidget(m_resourcesView.data());
    m_stackedWidget->setMinimumHeight(30);
    m_stackedWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    QWidget *spacer = new QWidget(this);
    spacer->setObjectName(QStringLiteral("itemLibrarySearchInputSpacer"));
    spacer->setFixedHeight(4);

    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tabBar, 0, 0, 1, 1);
    layout->addWidget(spacer, 1, 0);
    layout->addWidget(lineEditFrame, 2, 0, 1, 1);
    layout->addWidget(m_importTagsWidget.data(), 3, 0, 1, 1);
    layout->addWidget(m_addResourcesWidget.data(), 4, 0, 1, 1);
    layout->addWidget(m_stackedWidget.data(), 5, 0, 1, 1);

    setSearchFilter(QString());

    /* style sheets */
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css")))));
    m_resourcesView->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F5), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &ItemLibraryWidget::reloadQmlSource);

    connect(&m_compressionTimer, &QTimer::timeout, this, &ItemLibraryWidget::updateModel);

    auto flowLayout = new Utils::FlowLayout(m_importTagsWidget.data());
    flowLayout->setContentsMargins(4, 4, 4, 4);

    m_addResourcesWidget->setVisible(false);
    flowLayout = new Utils::FlowLayout(m_addResourcesWidget.data());
    flowLayout->setContentsMargins(4, 4, 4, 4);
    auto button = new QToolButton(m_addResourcesWidget.data());
    auto font = button->font();
    font.setPixelSize(Theme::instance()->smallFontPixelSize());
    button->setFont(font);
    button->setIcon(Utils::Icons::PLUS.icon());
    button->setText(tr("Add New Assets..."));
    button->setToolTip(tr("Add new assets to project."));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    flowLayout->addWidget(button);
    connect(button, &QToolButton::clicked, this, &ItemLibraryWidget::addResources);

#ifdef IMPORT_QUICK3D_ASSETS
    DesignerActionManager *actionManager =
             &QmlDesignerPlugin::instance()->viewManager().designerActionManager();

    auto handle3DModel = [](const QStringList &fileNames, const QString &defaultDir) -> bool {
        auto importDlg = new ItemLibraryAssetImportDialog(fileNames, defaultDir, Core::ICore::mainWindow());
        importDlg->show();
        return true;
    };

    auto add3DHandler = [&](const QString &category, const QString &ext) {
        const QString filter = QStringLiteral("*.%1").arg(ext);
        actionManager->registerAddResourceHandler(
                    AddResourceHandler(category, filter, handle3DModel, 10));
    };

    QSSGAssetImportManager importManager;
    QHash<QString, QStringList> supportedExtensions = importManager.getSupportedExtensions();

    // All things importable by QSSGAssetImportManager are considered to be in the same category
    // so we don't get multiple separate import dialogs when different file types are imported.
    const QString category = tr("3D Assets");

    // Skip if 3D asset handlers have already been added
    const QList<AddResourceHandler> handlers = actionManager->addResourceHandler();
    bool categoryAlreadyAdded = false;
    for (const auto &handler : handlers) {
        if (handler.category == category) {
            categoryAlreadyAdded = true;
            break;
        }
    }

    if (!categoryAlreadyAdded) {
        const auto groups = supportedExtensions.keys();
        for (const auto &group : groups) {
            const auto extensions = supportedExtensions[group];
            for (const auto &ext : extensions)
                add3DHandler(category, ext);
        }
    }
#endif

    // init the first load of the QML UI elements
    reloadQmlSource();
}

ItemLibraryWidget::~ItemLibraryWidget() = default;

void ItemLibraryWidget::setItemLibraryInfo(ItemLibraryInfo *itemLibraryInfo)
{
    if (m_itemLibraryInfo.data() == itemLibraryInfo)
        return;

    if (m_itemLibraryInfo) {
        disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                   this, &ItemLibraryWidget::delayedUpdateModel);
        disconnect(m_itemLibraryInfo.data(), &ItemLibraryInfo::importTagsChanged,
                   this, &ItemLibraryWidget::delayedUpdateModel);
    }
    m_itemLibraryInfo = itemLibraryInfo;
    if (itemLibraryInfo) {
        connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::entriesChanged,
                this, &ItemLibraryWidget::delayedUpdateModel);
        connect(m_itemLibraryInfo.data(), &ItemLibraryInfo::importTagsChanged,
                this, &ItemLibraryWidget::delayedUpdateModel);
    }
    delayedUpdateModel();
}

void ItemLibraryWidget::updateImports()
{
    if (m_model)
        setupImportTagWidget();
}

void ItemLibraryWidget::setImportsWidget(QWidget *importsWidget)
{
    m_stackedWidget->addWidget(importsWidget);
}

QList<QToolButton *> ItemLibraryWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;
    return buttons;
}

void ItemLibraryWidget::setSearchFilter(const QString &searchFilter)
{
    if (m_stackedWidget->currentIndex() == 0) {
        m_itemLibraryModel->setSearchText(searchFilter);
        m_itemViewQuickWidget->update();
    } else {
        QStringList nameFilterList;

        m_resourcesFileSystemModel->setSearchFilter(searchFilter);
        m_resourcesFileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
        m_resourcesView->scrollToTop();
    }
}

void ItemLibraryWidget::delayedUpdateModel()
{
    static bool disableTimer = DesignerSettings::getValue(DesignerSettingsKey::DISABLE_ITEM_LIBRARY_UPDATE_TIMER).toBool();
    if (disableTimer)
        updateModel();
    else
        m_compressionTimer.start();
}

void ItemLibraryWidget::setModel(Model *model)
{
    m_itemViewQuickWidget->engine()->removeImageProvider("itemlibrary_preview");
    m_model = model;
    if (!model)
        return;

    m_itemViewQuickWidget->engine()->addImageProvider("itemlibrary_preview",
                                                      new ItemLibraryIconImageProvider{m_imageCache});

    setItemLibraryInfo(model->metaInfo().itemLibraryInfo());
}

void ItemLibraryWidget::setCurrentIndexOfStackedWidget(int index)
{
    if (index == 2) {
        m_filterLineEdit->setVisible(false);
        m_importTagsWidget->setVisible(true);
        m_addResourcesWidget->setVisible(false);
    }
    if (index == 1) {
        m_filterLineEdit->setVisible(true);
        m_importTagsWidget->setVisible(false);
        m_addResourcesWidget->setVisible(true);
    } else {
        m_filterLineEdit->setVisible(true);
        m_importTagsWidget->setVisible(true);
        m_addResourcesWidget->setVisible(false);
    }

    m_stackedWidget->setCurrentIndex(index);
}

QString ItemLibraryWidget::qmlSourcesPath()
{
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/itemLibraryQmlSources");
}

void ItemLibraryWidget::clearSearchFilter()
{
    m_filterLineEdit->clear();
}

void ItemLibraryWidget::reloadQmlSource()
{
    QString itemLibraryQmlFilePath = qmlSourcesPath() + QStringLiteral("/ItemsView.qml");
    QTC_ASSERT(QFileInfo::exists(itemLibraryQmlFilePath), return);
    m_itemViewQuickWidget->engine()->clearComponentCache();
    m_itemViewQuickWidget->setSource(QUrl::fromLocalFile(itemLibraryQmlFilePath));
}

void ItemLibraryWidget::setupImportTagWidget()
{
    QTC_ASSERT(m_model, return);

    const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();
    const bool isQtForMCUs = mcuManager.isMCUProject();

    const QStringList imports = m_model->metaInfo().itemLibraryInfo()->showTagsForImports();

    qDeleteAll(m_importTagsWidget->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));

    auto flowLayout = m_importTagsWidget->layout();

    auto createButton = [this](const QString &import) {
        auto button = new QToolButton(m_importTagsWidget.data());
        auto font = button->font();
        font.setPixelSize(Theme::instance()->smallFontPixelSize());
        button->setFont(font);
        button->setIcon(Utils::Icons::PLUS.icon());
        button->setText(import);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setToolTip(tr("Add import %1").arg(import));
        connect(button, &QToolButton::clicked, this, [this, import]() {
            addPossibleImport(import);
        });
        return button;
    };

    if (!isQtForMCUs) {
        for (const QString &importPath : imports) {
            const Import import = Import::createLibraryImport(importPath);
            if (!m_model->hasImport(import, true, true)
                && m_model->isImportPossible(import, true, true))
                flowLayout->addWidget(createButton(importPath));
        }
    }
}

void ItemLibraryWidget::updateModel()
{
    QTC_ASSERT(m_itemLibraryModel, return);

    if (m_compressionTimer.isActive()) {
        m_updateRetry = false;
        m_compressionTimer.stop();
    }

    m_itemLibraryModel->update(m_itemLibraryInfo.data(), m_model.data());

    if (m_itemLibraryModel->rowCount() == 0 && !m_updateRetry) {
        m_updateRetry = true; // Only retry once to avoid endless loops
        m_compressionTimer.start();
    } else {
        m_updateRetry = false;
    }
    updateImports();
    updateSearch();
}

void ItemLibraryWidget::updateSearch()
{
    setSearchFilter(m_filterLineEdit->text());
}

void ItemLibraryWidget::setResourcePath(const QString &resourcePath)
{
    if (m_resourcesView->model() == m_resourcesFileSystemModel.data()) {
        m_resourcesFileSystemModel->setRootPath(resourcePath);
        m_resourcesView->setRootIndex(m_resourcesFileSystemModel->indexForPath(resourcePath));
    }
    updateSearch();
}

void ItemLibraryWidget::startDragAndDrop(QQuickItem *mouseArea, QVariant itemLibraryId)
{
    m_currentitemLibraryEntry = itemLibraryId.value<ItemLibraryEntry>();

    QMimeData *mimeData = m_itemLibraryModel->getMimeData(m_currentitemLibraryEntry);
    auto drag = new QDrag(this);

    drag->setPixmap(Utils::StyleHelper::dpiSpecificImageFile(
                        m_currentitemLibraryEntry.libraryEntryIconPath()));
    drag->setMimeData(mimeData);

    /* Workaround for bug in Qt. The release event is not delivered for Qt < 5.9 if a drag is started */
    QMouseEvent event (QEvent::MouseButtonRelease, QPoint(-1, -1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(mouseArea, &event);

    QTimer::singleShot(0, [drag]() {
        drag->exec();
        drag->deleteLater();
    });
}

void ItemLibraryWidget::setFlowMode(bool b)
{
    m_itemLibraryModel->setFlowMode(b);
}

void ItemLibraryWidget::removeImport(const QString &name)
{
    QTC_ASSERT(m_model, return);

    QList<Import> toBeRemovedImportList;
    foreach (const Import &import, m_model->imports())
        if (import.isLibraryImport() && import.url().compare(name, Qt::CaseInsensitive) == 0)
            toBeRemovedImportList.append(import);

    m_model->changeImports({}, toBeRemovedImportList);
}

void ItemLibraryWidget::addImport(const QString &name, const QString &version)
{
    QTC_ASSERT(m_model, return);
    m_model->changeImports({Import::createLibraryImport(name, version)}, {});
}

void ItemLibraryWidget::addPossibleImport(const QString &name)
{
    QTC_ASSERT(m_model, return);
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_IMPORT_ADDED_FLOWTAG
                                           + name);
    const Import import = m_model->highestPossibleImport(name);
    try {
        QList<Import> addedImports = {Import::createLibraryImport(name, import.version())};
        // Special case for adding an import for 3D asset - also add QtQuick3D import
        const QString asset3DPrefix = QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER + 1)
                + QLatin1Char('.');
        if (name.startsWith(asset3DPrefix)) {
            const QString q3Dlib = QLatin1String(Constants::QT_QUICK_3D_MODULE_NAME);
            Import q3DImport = m_model->highestPossibleImport(q3Dlib);
            if (q3DImport.url() == q3Dlib)
                addedImports.prepend(Import::createLibraryImport(q3Dlib, q3DImport.version()));
        }
        RewriterTransaction transaction
                = m_model->rewriterView()->beginRewriterTransaction(
                    QByteArrayLiteral("ItemLibraryWidget::addPossibleImport"));

        m_model->changeImports(addedImports, {});
        transaction.commit();
    }
    catch (const RewritingException &e) {
        e.showException();
    }
    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
}

void ItemLibraryWidget::addResources()
{
    auto document = QmlDesignerPlugin::instance()->currentDesignDocument();

    QTC_ASSERT(document, return);

    QList<AddResourceHandler> handlers = QmlDesignerPlugin::instance()->viewManager().designerActionManager().addResourceHandler();

    QMultiMap<QString, QString> map;
    for (const AddResourceHandler &handler : handlers) {
        map.insert(handler.category, handler.filter);
    }

    QMap<QString, QString> reverseMap;
    for (const AddResourceHandler &handler : handlers) {
        reverseMap.insert(handler.filter, handler.category);
    }

    QMap<QString, int> priorities;
    for (const AddResourceHandler &handler : handlers) {
        priorities.insert(handler.category, handler.piority);
    }

    QStringList sortedKeys = map.uniqueKeys();
    Utils::sort(sortedKeys, [&priorities](const QString &first,
                const QString &second){
        return priorities.value(first) < priorities.value(second);
    });

    QStringList filters;

    for (const QString &key : sortedKeys) {
        QString str = key + " (";
        str.append(map.values(key).join(" "));
        str.append(")");
        filters.append(str);
    }

    filters.prepend(tr("All Files (%1)").arg(map.values().join(" ")));

    static QString lastDir;
    const QString currentDir = lastDir.isEmpty() ? document->fileName().parentDir().toString() : lastDir;

    const auto fileNames = QFileDialog::getOpenFileNames(Core::ICore::dialogParent(),
                                                         tr("Add Assets"),
                                                         currentDir,
                                                         filters.join(";;"));

    if (!fileNames.isEmpty())
        lastDir = QFileInfo(fileNames.first()).absolutePath();

    QMultiMap<QString, QString> partitionedFileNames;

    for (const QString &fileName : fileNames) {
        const QString suffix = "*." + QFileInfo(fileName).suffix().toLower();
        const QString category = reverseMap.value(suffix);
        partitionedFileNames.insert(category, fileName);
    }

    for (const QString &category : partitionedFileNames.uniqueKeys()) {
         for (const AddResourceHandler &handler : handlers) {
             QStringList fileNames = partitionedFileNames.values(category);
             if (handler.category == category) {
                 QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_RESOURCE_IMPORTED + category);
                 if (!handler.operation(fileNames, document->fileName().parentDir().toString()))
                     Core::AsynchronousMessageBox::warning(tr("Failed to Add Files"), tr("Could not add %1 to project.").arg(fileNames.join(" ")));
                 break;
             }
         }
    }
}
} // namespace QmlDesigner
