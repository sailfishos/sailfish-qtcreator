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
#include "assetexporter.h"
#include "assetexportpluginconstants.h"
#include "componentexporter.h"
#include "exportnotification.h"

#include "designdocument.h"
#include "nodemetainfo.h"
#include "qmldesignerplugin.h"
#include "rewriterview.h"
#include "qmlitemnode.h"
#include "qmlobjectnode.h"
#include "utils/qtcassert.h"
#include "utils/runextensions.h"
#include "variantproperty.h"
#include "projectexplorer/session.h"
#include "projectexplorer/project.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QPlainTextEdit>
#include <QWaitCondition>

#include <random>
#include <queue>

using namespace ProjectExplorer;
using namespace std;
namespace {
bool makeParentPath(const Utils::FilePath &path)
{
    QDir d;
    return d.mkpath(path.toFileInfo().absolutePath());
}

QByteArray generateHash(const QString &token) {
    static uint counter = 0;
    std::mt19937 gen(std::random_device().operator()());
    std::uniform_int_distribution<> distribution(1, 99999);
    QByteArray data = QString("%1%2%3").arg(token).arg(++counter).arg(distribution(gen)).toLatin1();
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

Q_LOGGING_CATEGORY(loggerInfo, "qtc.designer.assetExportPlugin.assetExporter", QtInfoMsg)
Q_LOGGING_CATEGORY(loggerWarn, "qtc.designer.assetExportPlugin.assetExporter", QtWarningMsg)
Q_LOGGING_CATEGORY(loggerError, "qtc.designer.assetExportPlugin.assetExporter", QtCriticalMsg)
}

namespace QmlDesigner {

class AssetDumper
{
public:
    AssetDumper();
    ~AssetDumper();

    void dumpAsset(const QPixmap &p, const Utils::FilePath &path);

    /* Keeps on dumping until all assets are dumped, then quits */
    void quitDumper();

    /* Aborts dumping */
    void abortDumper();

private:
    void addAsset(const QPixmap &p, const Utils::FilePath &path);
    void doDumping(QFutureInterface<void> &fi);
    void savePixmap(const QPixmap &p, Utils::FilePath &path) const;

    QFuture<void> m_dumpFuture;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    std::queue<std::pair<QPixmap, Utils::FilePath>> m_assets;
    std::atomic<bool> m_quitDumper;
};



AssetExporter::AssetExporter(AssetExporterView *view, ProjectExplorer::Project *project, QObject *parent) :
    QObject(parent),
    m_currentState(*this),
    m_project(project),
    m_view(view)
{
    connect(m_view, &AssetExporterView::loadingFinished, this, &AssetExporter::onQmlFileLoaded);
    connect(m_view, &AssetExporterView::loadingError, this, &AssetExporter::notifyLoadError);
}

AssetExporter::~AssetExporter()
{
    cancel();
}

void AssetExporter::exportQml(const Utils::FilePaths &qmlFiles, const Utils::FilePath &exportPath,
                              bool exportAssets, bool perComponentExport)
{
    m_perComponentExport = perComponentExport;
    ExportNotification::addInfo(tr("Export root directory: %1.\nExporting assets: %2")
                                .arg(exportPath.toUserOutput())
                                .arg(exportAssets? tr("Yes") : tr("No")));

    if (m_perComponentExport)
        ExportNotification::addInfo(tr("Each component is exported separately"));

    notifyProgress(0.0);
    m_exportFiles = qmlFiles;
    m_totalFileCount = m_exportFiles.count();
    m_components.clear();
    m_componentUuidCache.clear();
    m_exportPath = exportPath;
    m_currentState.change(ParsingState::Parsing);
    if (exportAssets)
        m_assetDumper = make_unique<AssetDumper>();
    else
        m_assetDumper.reset();

    QTimer::singleShot(0, this, &AssetExporter::beginExport);
}

void AssetExporter::beginExport()
{
    for (const Utils::FilePath &p : m_exportFiles) {
        if (m_cancelled)
            break;
        preprocessQmlFile(p);
    }

    if (!m_cancelled)
        triggerLoadNextFile();
}

void AssetExporter::cancel()
{
    if (!m_cancelled) {
        ExportNotification::addInfo(tr("Cancelling export."));
        m_assetDumper.reset();
        m_cancelled = true;
    }
}

bool AssetExporter::isBusy() const
{
    return m_currentState == AssetExporter::ParsingState::Parsing ||
            m_currentState == AssetExporter::ParsingState::ExportingAssets ||
            m_currentState == AssetExporter::ParsingState::WritingJson;
}

const QPixmap &AssetExporter::generateAsset(const ModelNode &node)
{
    static QPixmap nullPixmap;
    if (m_cancelled)
        return nullPixmap;

    const QString uuid = node.auxiliaryData(Constants::UuidAuxTag).toString();
    QTC_ASSERT(!uuid.isEmpty(), return nullPixmap);

    if (!m_assets.contains(uuid)) {
        // Generate asset.
        QmlObjectNode objectNode(node);
        QPixmap asset = objectNode.toQmlItemNode().instanceRenderPixmap();
        m_assets[uuid] = asset;
    }
    return m_assets[uuid];
}

Utils::FilePath AssetExporter::assetPath(const ModelNode &node, const Component *component,
                                         const QString &suffix) const
{
    const QString uuid = node.auxiliaryData(Constants::UuidAuxTag).toString();
    if (!component || uuid.isEmpty())
        return {};

    const Utils::FilePath assetExportDir =
            m_perComponentExport ? componentExportDir(component) : m_exportPath;
    const Utils::FilePath assetPath = assetExportDir.pathAppended("assets")
            .pathAppended(uuid + suffix + ".png");

    return assetPath;
}

void AssetExporter::exportAsset(const QPixmap &asset, const Utils::FilePath &path)
{
    if (m_cancelled || !m_assetDumper)
        return;

    m_assetDumper->dumpAsset(asset, path);
}

void AssetExporter::exportComponent(const ModelNode &rootNode)
{
    qCDebug(loggerInfo) << "Exporting component" << rootNode.id();
    m_components.push_back(make_unique<Component>(*this, rootNode));
    m_components.back()->exportComponent();
}

void AssetExporter::notifyLoadError(AssetExporterView::LoadState state)
{
    QString errorStr = tr("Unknown error.");
    switch (state) {
    case AssetExporterView::LoadState::Exausted:
        errorStr = tr("Loading file is taking too long.");
        break;
    case AssetExporterView::LoadState::QmlErrorState:
        errorStr = tr("Cannot parse. QML file has errors.");
        break;
    default:
        return;
    }
    qCDebug(loggerError) << "QML load error:" << errorStr;
    ExportNotification::addError(tr("Loading QML failed. %1").arg(errorStr));
}

void AssetExporter::notifyProgress(double value) const
{
    emit exportProgressChanged(value);
}

void AssetExporter::onQmlFileLoaded()
{
    QTC_ASSERT(m_view && m_view->model(), qCDebug(loggerError) << "Null model"; return);
    qCDebug(loggerInfo) << "Qml file load done" << m_view->model()->fileUrl();

    QmlDesigner::DesignDocument *designDocument = QmlDesigner::QmlDesignerPlugin::instance()
                                                          ->documentManager()
                                                          .currentDesignDocument();
    if (designDocument->hasQmlParseErrors()) {
        ExportNotification::addError(tr("Cannot export QML. Document \"%1\" have parsing errors.")
                                     .arg(designDocument->displayName()));
    } else {
        exportComponent(m_view->rootModelNode());
        QString error;
        if (!m_view->saveQmlFile(&error)) {
            ExportNotification::addError(tr("Error saving QML file. %1")
                                         .arg(error.isEmpty()? tr("Unknown") : error));
        }
    }
    notifyProgress((m_totalFileCount - m_exportFiles.count()) * 0.8 / m_totalFileCount);
    triggerLoadNextFile();
}

Utils::FilePath AssetExporter::componentExportDir(const Component *component) const
{
    return m_exportPath.pathAppended(component->name());
}

void AssetExporter::preprocessQmlFile(const Utils::FilePath &path)
{
    // Load the QML file and assign UUIDs to items having none.
    // Meanwhile cache the Component UUIDs as well
    std::unique_ptr<Model> model(Model::create("Item", 2, 7));
    Utils::FileReader reader;
    if (!reader.fetch(path.toString())) {
        ExportNotification::addError(tr("Cannot preprocess file: %1. Error %2")
                                     .arg(path.toString()).arg(reader.errorString()));
        return;
    }

    QPlainTextEdit textEdit;
    textEdit.setPlainText(QString::fromUtf8(reader.data()));
    NotIndentingTextEditModifier *modifier = new NotIndentingTextEditModifier(&textEdit);
    modifier->setParent(model.get());
    RewriterView *rewriterView = new RewriterView(QmlDesigner::RewriterView::Validate, model.get());
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(modifier);
    model->attachView(rewriterView);
    rewriterView->restoreAuxiliaryData();
    ModelNode rootNode = rewriterView->rootModelNode();
    if (!rootNode.isValid()) {
        ExportNotification::addError(tr("Cannot preprocess file: %1").arg(path.toString()));
        return;
    }

    if (assignUuids(rootNode)) {
        // Some UUIDs were assigned. Rewrite the file.
        rewriterView->writeAuxiliaryData();
        const QByteArray data = textEdit.toPlainText().toUtf8();
        Utils::FileSaver saver(path.toString(), QIODevice::Text);
        saver.write(data);
        if (!saver.finalize()) {
            ExportNotification::addError(tr("Cannot update %1.\n%2")
                                         .arg(path.toString()).arg(saver.errorString()));
            return;
        }
    }

    // Cache component UUID
    const QString uuid = rootNode.auxiliaryData(Constants::UuidAuxTag).toString();
    m_componentUuidCache[path.toString()] = uuid;
}

bool AssetExporter::assignUuids(const ModelNode &root)
{
    // Assign an UUID to the node without one.
    // Return true if an assignment takes place.
    bool changed = false;
    for (const ModelNode &node : root.allSubModelNodesAndThisNode()) {
        const QString uuid = node.auxiliaryData(Constants::UuidAuxTag).toString();
        if (uuid.isEmpty()) {
            // Assign an unique identifier to the node.
            QByteArray uuid = generateUuid(node);
            node.setAuxiliaryData(Constants::UuidAuxTag, QString::fromLatin1(uuid));
            changed = true;
        }
    }
    return changed;
}

QByteArray AssetExporter::generateUuid(const ModelNode &node)
{
    QByteArray uuid;
    do {
        uuid = generateHash(node.id());
    } while (m_usedHashes.contains(uuid));
    m_usedHashes.insert(uuid);
    return uuid;
}

QString AssetExporter::componentUuid(const ModelNode &instance) const
{
    // Returns the UUID of the component's root node
    // Empty string is returned if the node is not an instance of a component within
    // the project.
    NodeMetaInfo metaInfo = instance.metaInfo();
    if (!metaInfo.isValid())
        return {};
    const QString path = metaInfo.componentFileName();
    if (m_componentUuidCache.contains(path))
        return m_componentUuidCache[path];
    return {};
}

void AssetExporter::triggerLoadNextFile()
{
    QTimer::singleShot(0, this, &AssetExporter::loadNextFile);
}

void AssetExporter::loadNextFile()
{
    if (m_cancelled || m_exportFiles.isEmpty()) {
        notifyProgress(0.8);
        m_currentState.change(ParsingState::ParsingFinished);
        writeMetadata();
        return;
    }

    // Load the next pending file.
    const Utils::FilePath file = m_exportFiles.takeFirst();
    ExportNotification::addInfo(tr("Exporting file %1.").arg(file.toUserOutput()));
    qCDebug(loggerInfo) << "Loading next file" << file;
    m_view->loadQmlFile(file);
}

void AssetExporter::writeMetadata() const
{
    if (m_cancelled) {
        notifyProgress(1.0);
        ExportNotification::addInfo(tr("Export cancelled."));
        m_currentState.change(ParsingState::ExportingDone);
        return;
    }


    auto writeFile = [](const Utils::FilePath &path, const QJsonArray &artboards) {
        if (!makeParentPath(path)) {
            ExportNotification::addError(tr("Writing metadata failed. Cannot create file %1").
                                         arg(path.toString()));
            return;
        }

        ExportNotification::addInfo(tr("Writing metadata to file %1.").arg(path.toUserOutput()));

        QJsonObject jsonRoot; // TODO: Write plugin info to root
        jsonRoot.insert("artboards", artboards);
        QJsonDocument doc(jsonRoot);
        if (doc.isNull() || doc.isEmpty()) {
            ExportNotification::addError(tr("Empty JSON document."));
            return;
        }

        Utils::FileSaver saver(path.toString(), QIODevice::Text);
        saver.write(doc.toJson(QJsonDocument::Indented));
        if (!saver.finalize()) {
            ExportNotification::addError(tr("Writing metadata failed. %1").
                                         arg(saver.errorString()));
        }
    };

    m_currentState.change(ParsingState::WritingJson);

    auto const startupProject = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(startupProject, return);
    const QString projectName = startupProject->displayName();

    if (m_perComponentExport) {
        for (auto &component : m_components) {
            const Utils::FilePath path = componentExportDir(component.get());
            writeFile(path.pathAppended(component->name() + ".metadata"), {component->json()});
        }
    } else {
        QJsonArray artboards;
        std::transform(m_components.cbegin(), m_components.cend(), back_inserter(artboards),
                       [](const unique_ptr<Component> &c) {return c->json(); });
        writeFile(m_exportPath.pathAppended(projectName + ".metadata"), artboards);
    }
    notifyProgress(1.0);
    ExportNotification::addInfo(tr("Export finished."));
    if (m_assetDumper)
        m_assetDumper->quitDumper();
    m_currentState.change(ParsingState::ExportingDone);
}

AssetExporter::State::State(AssetExporter &exporter) :
    m_assetExporter(exporter)
{

}

void AssetExporter::State::change(const ParsingState &state)
{
    qCDebug(loggerInfo()) << "Assetimporter State change: Old: " << m_state << "New: " << state;
    if (m_state != state) {
        m_state = state;
        m_assetExporter.stateChanged(m_state);
    }
}

QDebug operator<<(QDebug os, const AssetExporter::ParsingState &s)
{
    os << static_cast<std::underlying_type<QmlDesigner::AssetExporter::ParsingState>::type>(s);
    return os;
}

AssetDumper::AssetDumper():
    m_quitDumper(false)
{
    m_dumpFuture = Utils::runAsync(&AssetDumper::doDumping, this);
}

AssetDumper::~AssetDumper()
{
    abortDumper();
}

void AssetDumper::dumpAsset(const QPixmap &p, const Utils::FilePath &path)
{
    addAsset(p, path);
}

void AssetDumper::quitDumper()
{
    m_quitDumper = true;
    m_queueCondition.wakeAll();
    if (!m_dumpFuture.isFinished())
        m_dumpFuture.waitForFinished();
}

void AssetDumper::abortDumper()
{
    if (!m_dumpFuture.isFinished()) {
        m_dumpFuture.cancel();
        m_queueCondition.wakeAll();
        m_dumpFuture.waitForFinished();
    }
}

void AssetDumper::addAsset(const QPixmap &p, const Utils::FilePath &path)
{
    QMutexLocker locker(&m_queueMutex);
    qDebug() << "Save Asset:" << path;
    m_assets.push({p, path});
}

void AssetDumper::doDumping(QFutureInterface<void> &fi)
{
    auto haveAsset = [this] (std::pair<QPixmap, Utils::FilePath> *asset) {
        QMutexLocker locker(&m_queueMutex);
        if (m_assets.empty())
            return false;
        *asset = m_assets.front();
        m_assets.pop();
        return true;
    };

    forever {
        std::pair<QPixmap, Utils::FilePath> asset;
        if (haveAsset(&asset)) {
            if (fi.isCanceled())
                break;
            savePixmap(asset.first, asset.second);
        } else {
            if (m_quitDumper)
                break;
            QMutexLocker locker(&m_queueMutex);
            m_queueCondition.wait(&m_queueMutex);
        }

        if (fi.isCanceled())
            break;
    }
    fi.reportFinished();
}

void AssetDumper::savePixmap(const QPixmap &p, Utils::FilePath &path) const
{
    if (p.isNull()) {
        qCDebug(loggerWarn) << "Dumping null pixmap" << path;
        return;
    }

    if (!makeParentPath(path)) {
        ExportNotification::addError(AssetExporter::tr("Error creating asset directory. %1")
                                     .arg(path.fileName()));
        return;
    }

    if (!p.save(path.toString())) {
        ExportNotification::addError(AssetExporter::tr("Error saving asset. %1")
                                     .arg(path.fileName()));
    }
}

}
