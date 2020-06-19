/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "languageclientsettings.h"

#include "client.h"
#include "languageclientmanager.h"
#include "languageclient_global.h"
#include "languageclientinterface.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/algorithm.h>
#include <utils/utilsicons.h>
#include <utils/delegates.h>
#include <utils/fancylineedit.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/jsontreeitem.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QToolButton>
#include <QTreeView>

constexpr char nameKey[] = "name";
constexpr char idKey[] = "id";
constexpr char enabledKey[] = "enabled";
constexpr char startupBehaviorKey[] = "startupBehavior";
constexpr char mimeTypeKey[] = "mimeType";
constexpr char filePatternKey[] = "filePattern";
constexpr char executableKey[] = "executable";
constexpr char argumentsKey[] = "arguments";
constexpr char settingsGroupKey[] = "LanguageClient";
constexpr char clientsKey[] = "clients";
constexpr char mimeType[] = "application/language.client.setting";

namespace LanguageClient {

class LanguageClientSettingsModel : public QAbstractListModel
{
public:
    LanguageClientSettingsModel() = default;
    ~LanguageClientSettingsModel() override;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const final { return m_settings.count(); }
    QVariant data(const QModelIndex &index, int role) const final;
    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) final;
    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) final;
    bool setData(const QModelIndex &index, const QVariant &value, int role) final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
    QStringList mimeTypes() const override { return {mimeType}; }
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override;

    void reset(const QList<BaseSettings *> &settings);
    QList<BaseSettings *> settings() const { return m_settings; }
    void insertSettings(BaseSettings *settings);
    void enableSetting(const QString &id);
    QList<BaseSettings *> removed() const { return m_removed; }
    BaseSettings *settingForIndex(const QModelIndex &index) const;
    QModelIndex indexForSetting(BaseSettings *setting) const;

private:
    static constexpr int idRole = Qt::UserRole + 1;
    QList<BaseSettings *> m_settings; // owned
    QList<BaseSettings *> m_removed;
};

class LanguageClientSettingsPageWidget : public QWidget
{
public:
    LanguageClientSettingsPageWidget(LanguageClientSettingsModel &settings);
    void currentChanged(const QModelIndex &index);
    int currentRow() const;
    void resetCurrentSettings(int row);
    void applyCurrentSettings();

private:
    LanguageClientSettingsModel &m_settings;
    QTreeView *m_view = nullptr;
    struct CurrentSettings {
        BaseSettings *setting = nullptr;
        QWidget *widget = nullptr;
    } m_currentSettings;

    void addItem();
    void deleteItem();
};

class LanguageClientSettingsPage : public Core::IOptionsPage
{
    Q_DECLARE_TR_FUNCTIONS(LanguageClientSettingsPage)
public:
    LanguageClientSettingsPage();
    ~LanguageClientSettingsPage() override;

    void init();

    // IOptionsPage interface
    QWidget *widget() override;
    void apply() override;
    void finish() override;

    QList<BaseSettings *> settings() const;
    void addSettings(BaseSettings *settings);
    void enableSettings(const QString &id);

private:
    LanguageClientSettingsModel m_model;
    QPointer<LanguageClientSettingsPageWidget> m_widget;
};

LanguageClientSettingsPageWidget::LanguageClientSettingsPageWidget(LanguageClientSettingsModel &settings)
    : m_settings(settings)
    , m_view(new QTreeView())
{
    auto mainLayout = new QVBoxLayout();
    auto layout = new QHBoxLayout();
    m_view->setModel(&m_settings);
    m_view->setHeaderHidden(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_view->setDragEnabled(true);
    m_view->viewport()->setAcceptDrops(true);
    m_view->setDropIndicatorShown(true);
    m_view->setDragDropMode(QAbstractItemView::InternalMove);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &LanguageClientSettingsPageWidget::currentChanged);
    auto buttonLayout = new QVBoxLayout();
    auto addButton = new QPushButton(LanguageClientSettingsPage::tr("&Add"));
    connect(addButton, &QPushButton::pressed, this, &LanguageClientSettingsPageWidget::addItem);
    auto deleteButton = new QPushButton(LanguageClientSettingsPage::tr("&Delete"));
    connect(deleteButton, &QPushButton::pressed, this, &LanguageClientSettingsPageWidget::deleteItem);
    mainLayout->addLayout(layout);
    setLayout(mainLayout);
    layout->addWidget(m_view);
    layout->addLayout(buttonLayout);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch(10);
}

void LanguageClientSettingsPageWidget::currentChanged(const QModelIndex &index)
{
    if (m_currentSettings.widget) {
        applyCurrentSettings();
        layout()->removeWidget(m_currentSettings.widget);
        delete m_currentSettings.widget;
    }

    if (index.isValid()) {
        m_currentSettings.setting = m_settings.settingForIndex(index);
        m_currentSettings.widget = m_currentSettings.setting->createSettingsWidget(this);
        layout()->addWidget(m_currentSettings.widget);
    } else {
        m_currentSettings.setting = nullptr;
        m_currentSettings.widget = nullptr;
    }
}

int LanguageClientSettingsPageWidget::currentRow() const
{
    return m_settings.indexForSetting(m_currentSettings.setting).row();
}

void LanguageClientSettingsPageWidget::resetCurrentSettings(int row)
{
    if (m_currentSettings.widget) {
        layout()->removeWidget(m_currentSettings.widget);
        delete m_currentSettings.widget;
    }

    m_currentSettings.setting = nullptr;
    m_currentSettings.widget = nullptr;
    m_view->setCurrentIndex(m_settings.index(row));
}

void LanguageClientSettingsPageWidget::applyCurrentSettings()
{
    if (!m_currentSettings.setting)
        return;

    m_currentSettings.setting->applyFromSettingsWidget(m_currentSettings.widget);
    auto index = m_settings.indexForSetting(m_currentSettings.setting);
    emit m_settings.dataChanged(index, index);
}

void LanguageClientSettingsPageWidget::addItem()
{
    const int row = m_settings.rowCount();
    m_settings.insertRows(row);
    m_view->setCurrentIndex(m_settings.index(row));
}

void LanguageClientSettingsPageWidget::deleteItem()
{
    auto index = m_view->currentIndex();
    if (!index.isValid())
        return;

    m_settings.removeRows(index.row());
}

LanguageClientSettingsPage::LanguageClientSettingsPage()
{
    setId(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
    setDisplayName(tr("General"));
    setCategory(Constants::LANGUAGECLIENT_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("LanguageClient",
                                                   Constants::LANGUAGECLIENT_SETTINGS_TR));
    setCategoryIconPath(":/languageclient/images/settingscategory_languageclient.png");
}

LanguageClientSettingsPage::~LanguageClientSettingsPage()
{
    if (m_widget)
        delete m_widget;
}

void LanguageClientSettingsPage::init()
{
    m_model.reset(LanguageClientSettings::fromSettings(Core::ICore::settings()));
    apply();
    finish();
}

QWidget *LanguageClientSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new LanguageClientSettingsPageWidget(m_model);
    return m_widget;
}

void LanguageClientSettingsPage::apply()
{
    if (m_widget)
        m_widget->applyCurrentSettings();
    LanguageClientManager::applySettings();

    for (BaseSettings *setting : m_model.removed()) {
        for (Client *client : LanguageClientManager::clientForSetting(setting))
            LanguageClientManager::shutdownClient(client);
    }

    if (m_widget) {
        int row = m_widget->currentRow();
        m_model.reset(LanguageClientManager::currentSettings());
        m_widget->resetCurrentSettings(row);
    } else {
        m_model.reset(LanguageClientManager::currentSettings());
    }
}

void LanguageClientSettingsPage::finish()
{
    m_model.reset(LanguageClientManager::currentSettings());
}

QList<BaseSettings *> LanguageClientSettingsPage::settings() const
{
    return m_model.settings();
}

void LanguageClientSettingsPage::addSettings(BaseSettings *settings)
{
    m_model.insertSettings(settings);
}

void LanguageClientSettingsPage::enableSettings(const QString &id)
{
    m_model.enableSetting(id);
}

LanguageClientSettingsModel::~LanguageClientSettingsModel()
{
    qDeleteAll(m_settings);
}

QVariant LanguageClientSettingsModel::data(const QModelIndex &index, int role) const
{
    BaseSettings *setting = settingForIndex(index);
    if (!setting)
        return QVariant();
    if (role == Qt::DisplayRole)
        return Utils::globalMacroExpander()->expand(setting->m_name);
    else if (role == Qt::CheckStateRole)
        return setting->m_enabled ? Qt::Checked : Qt::Unchecked;
    else if (role == idRole)
        return setting->m_id;
    return QVariant();
}

bool LanguageClientSettingsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= int(m_settings.size()))
        return false;
    const int end = qMin(row + count - 1, int(m_settings.size()) - 1);
    beginRemoveRows(parent, row, end);
    for (auto i = end; i >= row; --i)
        m_removed << m_settings.takeAt(i);
    endRemoveRows();
    return true;
}

bool LanguageClientSettingsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > m_settings.size() || row < 0)
        return false;
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_settings.insert(row + i, new StdIOSettings());
    endInsertRows();
    return true;
}

bool LanguageClientSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    BaseSettings *setting = settingForIndex(index);
    if (!setting || role != Qt::CheckStateRole)
        return false;

    if (setting->m_enabled != value.toBool()) {
        setting->m_enabled = !setting->m_enabled;
        emit dataChanged(index, index, { Qt::CheckStateRole });
    }
    return true;
}

Qt::ItemFlags LanguageClientSettingsModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags dragndropFlags = index.isValid() ? Qt::ItemIsDragEnabled
                                                         : Qt::ItemIsDropEnabled;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | dragndropFlags;
}

QMimeData *LanguageClientSettingsModel::mimeData(const QModelIndexList &indexes) const
{
    QTC_ASSERT(indexes.count() == 1, return nullptr);

    QMimeData *mimeData = new QMimeData;
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes) {
        if (index.isValid())
            stream << data(index, idRole).toString();
    }

    mimeData->setData(mimeType, indexes.first().data(idRole).toString().toUtf8());
    return mimeData;
}

bool LanguageClientSettingsModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    const QString id = QString::fromUtf8(data->data(mimeType));
    auto setting = Utils::findOrDefault(m_settings, [id](const BaseSettings *setting) {
        return setting->m_id == id;
    });
    if (!setting)
        return false;

    if (row == -1)
        row = parent.isValid() ? parent.row() : rowCount(QModelIndex());

    beginInsertRows(parent, row, row);
    m_settings.insert(row, setting->copy());
    endInsertRows();

    return true;
}

void LanguageClientSettingsModel::reset(const QList<BaseSettings *> &settings)
{
    beginResetModel();
    qDeleteAll(m_settings);
    qDeleteAll(m_removed);
    m_removed.clear();
    m_settings = Utils::transform(settings, [](const BaseSettings *other) { return other->copy(); });
    endResetModel();
}

void LanguageClientSettingsModel::insertSettings(BaseSettings *settings)
{
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    m_settings.insert(row, settings);
    endInsertRows();
}

void LanguageClientSettingsModel::enableSetting(const QString &id)
{
    BaseSettings *setting = Utils::findOrDefault(m_settings, Utils::equal(&BaseSettings::m_id, id));
    if (!setting)
        return;
    setting->m_enabled = true;
    const QModelIndex &index = indexForSetting(setting);
    if (index.isValid())
        emit dataChanged(index, index, {Qt::CheckStateRole});
}

BaseSettings *LanguageClientSettingsModel::settingForIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_settings.size())
        return nullptr;
    return m_settings[index.row()];
}

QModelIndex LanguageClientSettingsModel::indexForSetting(BaseSettings *setting) const
{
    const int index = m_settings.indexOf(setting);
    return index < 0 ? QModelIndex() : createIndex(index, 0, setting);
}

void BaseSettings::applyFromSettingsWidget(QWidget *widget)
{
    if (auto settingsWidget = qobject_cast<BaseSettingsWidget *>(widget)) {
        m_name = settingsWidget->name();
        m_languageFilter = settingsWidget->filter();
        m_startBehavior = settingsWidget->startupBehavior();
    }
}

QWidget *BaseSettings::createSettingsWidget(QWidget *parent) const
{
    return new BaseSettingsWidget(this, parent);
}

bool BaseSettings::needsRestart() const
{
    const QVector<Client *> clients = LanguageClientManager::clientForSetting(this);
    if (clients.isEmpty())
        return m_enabled;
    if (!m_enabled)
        return true;
    return Utils::anyOf(clients, [this](const Client *client) {
        return client->needsRestart(this);
    });
}

bool BaseSettings::isValid() const
{
    return !m_name.isEmpty();
}

Client *BaseSettings::createClient()
{
    if (!isValid() || !m_enabled)
        return nullptr;
    BaseClientInterface *interface = createInterface();
    QTC_ASSERT(interface, return nullptr);
    auto *client = new Client(interface);
    client->setName(Utils::globalMacroExpander()->expand(m_name));
    client->setSupportedLanguage(m_languageFilter);
    return client;
}

QVariantMap BaseSettings::toMap() const
{
    QVariantMap map;
    map.insert(nameKey, m_name);
    map.insert(idKey, m_id);
    map.insert(enabledKey, m_enabled);
    map.insert(startupBehaviorKey, m_startBehavior);
    map.insert(mimeTypeKey, m_languageFilter.mimeTypes);
    map.insert(filePatternKey, m_languageFilter.filePattern);
    return map;
}

void BaseSettings::fromMap(const QVariantMap &map)
{
    m_name = map[nameKey].toString();
    m_id = map.value(idKey, QUuid::createUuid().toString()).toString();
    m_enabled = map[enabledKey].toBool();
    m_startBehavior = BaseSettings::StartBehavior(
        map.value(startupBehaviorKey, BaseSettings::RequiresFile).toInt());
    m_languageFilter.mimeTypes = map[mimeTypeKey].toStringList();
    m_languageFilter.filePattern = map[filePatternKey].toStringList();
    m_languageFilter.filePattern.removeAll({}); // remove empty entries
}

static LanguageClientSettingsPage &settingsPage()
{
    static LanguageClientSettingsPage settingsPage;
    return settingsPage;
}

void LanguageClientSettings::init()
{
    settingsPage().init();
}

QList<BaseSettings *> LanguageClientSettings::fromSettings(QSettings *settingsIn)
{
    settingsIn->beginGroup(settingsGroupKey);
    auto variants = settingsIn->value(clientsKey).toList();
    auto settings = Utils::transform(variants, [](const QVariant& var){
        BaseSettings *settings = new StdIOSettings();
        settings->fromMap(var.toMap());
        return settings;
    });
    settingsIn->endGroup();
    return settings;
}

QList<BaseSettings *> LanguageClientSettings::currentPageSettings()
{
    return settingsPage().settings();
}

void LanguageClientSettings::addSettings(BaseSettings *settings)
{
    settingsPage().addSettings(settings);
}

void LanguageClientSettings::enableSettings(const QString &id)
{
    settingsPage().enableSettings(id);
}

void LanguageClientSettings::toSettings(QSettings *settings,
                                        const QList<BaseSettings *> &languageClientSettings)
{
    settings->beginGroup(settingsGroupKey);
    settings->setValue(clientsKey, Utils::transform(languageClientSettings,
                                                    [](const BaseSettings *setting){
        return QVariant(setting->toMap());
    }));
    settings->endGroup();
}

void StdIOSettings::applyFromSettingsWidget(QWidget *widget)
{
    if (auto settingsWidget = qobject_cast<StdIOSettingsWidget *>(widget)) {
        BaseSettings::applyFromSettingsWidget(settingsWidget);
        m_executable = settingsWidget->executable();
        m_arguments = settingsWidget->arguments();
    }
}

QWidget *StdIOSettings::createSettingsWidget(QWidget *parent) const
{
    return new StdIOSettingsWidget(this, parent);
}

bool StdIOSettings::needsRestart() const
{
    if (BaseSettings::needsRestart())
        return true;
    return Utils::anyOf(LanguageClientManager::clientForSetting(this),
                        [this](QPointer<Client> client) {
                            if (auto stdIOInterface = qobject_cast<const StdIOClientInterface *>(
                                    client->clientInterface()))
                                return stdIOInterface->needsRestart(this);
                            return false;
                        });
}

bool StdIOSettings::isValid() const
{
    return BaseSettings::isValid() && !m_executable.isEmpty();
}

QVariantMap StdIOSettings::toMap() const
{
    QVariantMap map = BaseSettings::toMap();
    map.insert(executableKey, m_executable);
    map.insert(argumentsKey, m_arguments);
    return map;
}

void StdIOSettings::fromMap(const QVariantMap &map)
{
    BaseSettings::fromMap(map);
    m_executable = map[executableKey].toString();
    m_arguments = map[argumentsKey].toString();
}

QString StdIOSettings::arguments() const
{
    return Utils::globalMacroExpander()->expand(m_arguments);
}

Utils::CommandLine StdIOSettings::command() const
{
    return Utils::CommandLine(Utils::FilePath::fromUserInput(m_executable),
                              arguments(),
                              Utils::CommandLine::Raw);
}

BaseClientInterface *StdIOSettings::createInterface() const
{
    return new StdIOClientInterface(m_executable, arguments());
}

class JsonTreeItemDelegate : public QStyledItemDelegate
{
public:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        QString result = value.toString();
        if (result.size() == 1) {
            switch (result.at(0).toLatin1()) {
            case '\n':
                return QString("\\n");
            case '\t':
                return QString("\\t");
            case '\r':
                return QString("\\r");
            }
        }
        return result;
    }
};

static QWidget *createCapabilitiesView(const QJsonValue &capabilities)
{
    auto root = new Utils::JsonTreeItem("Capabilities", capabilities);
    if (root->canFetchMore())
        root->fetchMore();

    auto capabilitiesModel = new Utils::TreeModel<Utils::JsonTreeItem>(root);
    capabilitiesModel->setHeader({BaseSettingsWidget::tr("Name"),
                                  BaseSettingsWidget::tr("Value"),
                                  BaseSettingsWidget::tr("Type")});
    auto capabilitiesView = new QTreeView();
    capabilitiesView->setModel(capabilitiesModel);
    capabilitiesView->setAlternatingRowColors(true);
    capabilitiesView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    capabilitiesView->setItemDelegate(new JsonTreeItemDelegate);
    return capabilitiesView;
}

static QString startupBehaviorString(BaseSettings::StartBehavior behavior)
{
    switch (behavior) {
    case BaseSettings::AlwaysOn:
        return QCoreApplication::translate("LanguageClient::BaseSettings", "Always On");
    case BaseSettings::RequiresFile:
        return QCoreApplication::translate("LanguageClient::BaseSettings", "Requires an Open File");
    case BaseSettings::RequiresProject:
        return QCoreApplication::translate("LanguageClient::BaseSettings",
                                           "Start Server per Project");
    default:
        break;
    }
    return {};
}

BaseSettingsWidget::BaseSettingsWidget(const BaseSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_name(new QLineEdit(settings->m_name, this))
    , m_mimeTypes(new QLabel(settings->m_languageFilter.mimeTypes.join(filterSeparator), this))
    , m_filePattern(new QLineEdit(settings->m_languageFilter.filePattern.join(filterSeparator), this))
    , m_startupBehavior(new QComboBox)
{
    int row = 0;
    auto *mainLayout = new QGridLayout;
    mainLayout->addWidget(new QLabel(tr("Name:")), row, 0);
    mainLayout->addWidget(m_name, row, 1);
    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_name);
    mainLayout->addWidget(new QLabel(tr("Language:")), ++row, 0);
    auto mimeLayout = new QHBoxLayout;
    mimeLayout->addWidget(m_mimeTypes);
    mimeLayout->addStretch();
    auto addMimeTypeButton = new QPushButton(tr("Set MIME Types..."), this);
    mimeLayout->addWidget(addMimeTypeButton);
    mainLayout->addLayout(mimeLayout, row, 1);
    m_filePattern->setPlaceholderText(tr("File pattern"));
    mainLayout->addWidget(m_filePattern, ++row, 1);
    mainLayout->addWidget(new QLabel(tr("Startup behavior:")), ++row, 0);
    for (int behavior = 0; behavior < BaseSettings::LastSentinel ; ++behavior)
        m_startupBehavior->addItem(startupBehaviorString(BaseSettings::StartBehavior(behavior)));
    m_startupBehavior->setCurrentIndex(settings->m_startBehavior);
    mainLayout->addWidget(m_startupBehavior, row, 1);


    connect(addMimeTypeButton, &QPushButton::pressed,
            this, &BaseSettingsWidget::showAddMimeTypeDialog);

    auto createInfoLabel = []() {
        return new QLabel(tr("Available after server was initialized"));
    };

    mainLayout->addWidget(new QLabel(tr("Capabilities:")), ++row, 0, Qt::AlignTop);
    QVector<Client *> clients = LanguageClientManager::clientForSetting(settings);
    if (clients.isEmpty()) {
        mainLayout->addWidget(createInfoLabel());
    } else { // TODO move the capabilities view into a new widget outside of the settings
        Client *client = clients.first();
        if (client->state() == Client::Initialized)
            mainLayout->addWidget(createCapabilitiesView(QJsonValue(client->capabilities())));
        else
            mainLayout->addWidget(createInfoLabel(), row, 1);
        connect(client, &Client::finished, mainLayout, [mainLayout, row, createInfoLabel]() {
            delete mainLayout->itemAtPosition(row, 1)->widget();
            mainLayout->addWidget(createInfoLabel(), row, 1);
        });
        connect(client, &Client::initialized, mainLayout,
                [mainLayout, row](
                    const LanguageServerProtocol::ServerCapabilities &capabilities) {
                    delete mainLayout->itemAtPosition(row, 1)->widget();
                    mainLayout->addWidget(createCapabilitiesView(QJsonValue(capabilities)), row, 1);
                });
    }
    setLayout(mainLayout);
}

QString BaseSettingsWidget::name() const
{
    return m_name->text();
}

LanguageFilter BaseSettingsWidget::filter() const
{
    return {m_mimeTypes->text().split(filterSeparator, QString::SkipEmptyParts),
                m_filePattern->text().split(filterSeparator, QString::SkipEmptyParts)};
}

BaseSettings::StartBehavior BaseSettingsWidget::startupBehavior() const
{
    return BaseSettings::StartBehavior(m_startupBehavior->currentIndex());
}

class MimeTypeModel : public QStringListModel
{
public:
    using QStringListModel::QStringListModel;
    QVariant data(const QModelIndex &index, int role) const final
    {
        if (index.isValid() && role == Qt::CheckStateRole)
            return m_selectedMimeTypes.contains(index.data().toString()) ? Qt::Checked : Qt::Unchecked;
        return QStringListModel::data(index, role);
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) final
    {
        if (index.isValid() && role == Qt::CheckStateRole) {
            QString mimeType = index.data().toString();
            if (value.toInt() == Qt::Checked) {
                if (!m_selectedMimeTypes.contains(mimeType))
                    m_selectedMimeTypes.append(index.data().toString());
            } else {
                m_selectedMimeTypes.removeAll(index.data().toString());
            }
            return true;
        }
        return QStringListModel::setData(index, value, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const final
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return (QStringListModel::flags(index)
                & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled))
                | Qt::ItemIsUserCheckable;
    }
    QStringList m_selectedMimeTypes;
};

class MimeTypeDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(MimeTypeDialog)
public:
    explicit MimeTypeDialog(const QStringList &selectedMimeTypes, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr("Select MIME Types"));
        auto mainLayout = new QVBoxLayout;
        auto filter = new Utils::FancyLineEdit(this);
        filter->setFiltering(true);
        mainLayout->addWidget(filter);
        auto listView = new QListView(this);
        mainLayout->addWidget(listView);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(buttons);
        setLayout(mainLayout);

        filter->setPlaceholderText(tr("Filter"));
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        auto proxy = new QSortFilterProxyModel(this);
        m_mimeTypeModel = new MimeTypeModel(Utils::transform(Utils::allMimeTypes(),
                                                             &Utils::MimeType::name), this);
        m_mimeTypeModel->m_selectedMimeTypes = selectedMimeTypes;
        proxy->setSourceModel(m_mimeTypeModel);
        proxy->sort(0);
        connect(filter, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterWildcard);
        listView->setModel(proxy);

        setModal(true);
    }

    MimeTypeDialog(const MimeTypeDialog &other) = delete;
    MimeTypeDialog(MimeTypeDialog &&other) = delete;

    MimeTypeDialog operator=(const MimeTypeDialog &other) = delete;
    MimeTypeDialog operator=(MimeTypeDialog &&other) = delete;


    QStringList mimeTypes() const
    {
        return m_mimeTypeModel->m_selectedMimeTypes;
    }
private:
    MimeTypeModel *m_mimeTypeModel = nullptr;
};

void BaseSettingsWidget::showAddMimeTypeDialog()
{
    MimeTypeDialog dialog(m_mimeTypes->text().split(filterSeparator, QString::SkipEmptyParts),
                          Core::ICore::dialogParent());
    if (dialog.exec() == QDialog::Rejected)
        return;
    m_mimeTypes->setText(dialog.mimeTypes().join(filterSeparator));
}

StdIOSettingsWidget::StdIOSettingsWidget(const StdIOSettings *settings, QWidget *parent)
    : BaseSettingsWidget(settings, parent)
    , m_executable(new Utils::PathChooser(this))
    , m_arguments(new QLineEdit(settings->m_arguments, this))
{
    auto mainLayout = qobject_cast<QGridLayout *>(layout());
    QTC_ASSERT(mainLayout, return);
    const int baseRows = mainLayout->rowCount();
    mainLayout->addWidget(new QLabel(tr("Executable:")), baseRows, 0);
    mainLayout->addWidget(m_executable, baseRows, 1);
    mainLayout->addWidget(new QLabel(tr("Arguments:")), baseRows + 1, 0);
    m_executable->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_executable->setPath(QDir::toNativeSeparators(settings->m_executable));
    mainLayout->addWidget(m_arguments, baseRows + 1, 1);

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_arguments);
}

QString StdIOSettingsWidget::executable() const
{
    return m_executable->path();
}

QString StdIOSettingsWidget::arguments() const
{
    return m_arguments->text();
}

bool LanguageFilter::isSupported(const Utils::FilePath &filePath, const QString &mimeType) const
{
    if (mimeTypes.contains(mimeType))
        return true;
    if (filePattern.isEmpty() && filePath.isEmpty())
        return mimeTypes.isEmpty();
    auto regexps = Utils::transform(filePattern, [](const QString &pattern){
        return QRegExp(pattern, Utils::HostOsInfo::fileNameCaseSensitivity(), QRegExp::Wildcard);
    });
    return Utils::anyOf(regexps, [filePath](const QRegExp &reg){
        return reg.exactMatch(filePath.toString()) || reg.exactMatch(filePath.fileName());
    });
}

bool LanguageFilter::isSupported(const Core::IDocument *document) const
{
    return isSupported(document->filePath(), document->mimeType());
}

} // namespace LanguageClient
