/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "uvtargetdeviceselection.h"

#include <QComboBox>
#include <QDataWidgetMapper>
#include <QHBoxLayout>
#include <QLineEdit>

using namespace Utils;

namespace BareMetal {
namespace Internal {
namespace Uv {

// Software package data keys.
constexpr char packageDescrKeyC[] = "BareMetal.UvscServerProvider.PackageDescription";
constexpr char packageFileKeyC[] = "BareMetal.UvscServerProvider.PackageFile";
constexpr char packageNameKeyC[] = "BareMetal.UvscServerProvider.PackageName";
constexpr char packageUrlKeyC[] = "BareMetal.UvscServerProvider.PackageUrl";
constexpr char packageVendorNameKeyC[] = "BareMetal.UvscServerProvider.PackageVendorName";
constexpr char packageVendorIdKeyC[] = "BareMetal.UvscServerProvider.PackageVendorId";
constexpr char packageVersionKeyC[] = "BareMetal.UvscServerProvider.PackageVersion";
// Device data keys.
constexpr char deviceNameKeyC[] = "BareMetal.UvscServerProvider.DeviceName";
constexpr char deviceDescrKeyC[] = "BareMetal.UvscServerProvider.DeviceDescription";
constexpr char deviceFamilyKeyC[] = "BareMetal.UvscServerProvider.DeviceFamily";
constexpr char deviceSubFamilyKeyC[] = "BareMetal.UvscServerProvider.DeviceSubFamily";
constexpr char deviceVendorNameKeyC[] = "BareMetal.UvscServerProvider.DeviceVendorName";
constexpr char deviceVendorIdKeyC[] = "BareMetal.UvscServerProvider.DeviceVendorId";
constexpr char deviceSvdKeyC[] = "BareMetal.UvscServerProvider.DeviceSVD";
// Device CPU data keys.
constexpr char deviceClockKeyC[] = "BareMetal.UvscServerProvider.DeviceClock";
constexpr char deviceCoreKeyC[] = "BareMetal.UvscServerProvider.DeviceCore";
constexpr char deviceFpuKeyC[] = "BareMetal.UvscServerProvider.DeviceFPU";
constexpr char deviceMpuKeyC[] = "BareMetal.UvscServerProvider.DeviceMPU";
// Device MEMORY data keys.
constexpr char deviceMemoryKeyC[] = "BareMetal.UvscServerProvider.DeviceMemory";
constexpr char deviceMemoryIdKeyC[] = "BareMetal.UvscServerProvider.DeviceMemoryId";
constexpr char deviceMemoryStartKeyC[] = "BareMetal.UvscServerProvider.DeviceMemoryStart";
constexpr char deviceMemorySizeKeyC[] = "BareMetal.UvscServerProvider.DeviceMemorySize";
// Device ALGORITHM data keys.
constexpr char deviceAlgorithmKeyC[] = "BareMetal.UvscServerProvider.DeviceAlgorithm";
constexpr char deviceAlgorithmPathKeyC[] = "BareMetal.UvscServerProvider.DeviceAlgorithmPath";
constexpr char deviceAlgorithmStartKeyC[] = "BareMetal.UvscServerProvider.DeviceAlgorithmStart";
constexpr char deviceAlgorithmSizeKeyC[] = "BareMetal.UvscServerProvider.DeviceAlgorithmSize";
constexpr char deviceAlgorithmIndexKeyC[] = "BareMetal.UvscServerProvider.DeviceAlgorithmIndex";

// DeviceSelection

QVariantMap DeviceSelection::toMap() const
{
    QVariantMap map;
    // Software package.
    map.insert(packageDescrKeyC, package.desc);
    map.insert(packageFileKeyC, package.file);
    map.insert(packageNameKeyC, package.name);
    map.insert(packageUrlKeyC, package.url);
    map.insert(packageVendorNameKeyC, package.vendorName);
    map.insert(packageVendorIdKeyC, package.vendorId);
    map.insert(packageVersionKeyC, package.version);
    // Device.
    map.insert(deviceNameKeyC, name);
    map.insert(deviceDescrKeyC, desc);
    map.insert(deviceFamilyKeyC, family);
    map.insert(deviceSubFamilyKeyC, subfamily);
    map.insert(deviceVendorNameKeyC, vendorName);
    map.insert(deviceVendorIdKeyC, vendorId);
    map.insert(deviceSvdKeyC, svd);
    // Device CPU.
    map.insert(deviceClockKeyC, cpu.clock);
    map.insert(deviceCoreKeyC, cpu.core);
    map.insert(deviceFpuKeyC, cpu.fpu);
    map.insert(deviceMpuKeyC, cpu.mpu);
    // Device MEMORY.
    QVariantList memoryList;
    for (const DeviceSelection::Memory &memory : qAsConst(memories)) {
        QVariantMap m;
        m.insert(deviceMemoryIdKeyC, memory.id);
        m.insert(deviceMemoryStartKeyC, memory.start);
        m.insert(deviceMemorySizeKeyC, memory.size);
        memoryList.push_back(m);
    }
    map.insert(deviceMemoryKeyC, memoryList);
    // Device ALGORITHM.
    QVariantList algorithmList;
    for (const DeviceSelection::Algorithm &algorithm : qAsConst(algorithms)) {
        QVariantMap m;
        m.insert(deviceAlgorithmPathKeyC, algorithm.path);
        m.insert(deviceAlgorithmStartKeyC, algorithm.start);
        m.insert(deviceAlgorithmSizeKeyC, algorithm.size);
        algorithmList.push_back(m);
    }
    map.insert(deviceAlgorithmKeyC, algorithmList);
    map.insert(deviceAlgorithmIndexKeyC, algorithmIndex);
    return map;
}

void DeviceSelection::fromMap(const QVariantMap &map)
{
    // Software package.
    package.desc = map.value(packageDescrKeyC).toString();
    package.file = map.value(packageFileKeyC).toString();
    package.name = map.value(packageNameKeyC).toString();
    package.url = map.value(packageUrlKeyC).toString();
    package.vendorName = map.value(packageVendorNameKeyC).toString();
    package.vendorId = map.value(packageVendorIdKeyC).toString();
    package.version = map.value(packageVersionKeyC).toString();
    // Device.
    name = map.value(deviceNameKeyC).toString();
    desc = map.value(deviceDescrKeyC).toString();
    family = map.value(deviceFamilyKeyC).toString();
    subfamily = map.value(deviceSubFamilyKeyC).toString();
    vendorName = map.value(deviceVendorNameKeyC).toString();
    vendorId = map.value(deviceVendorIdKeyC).toString();
    svd = map.value(deviceSvdKeyC).toString();
    // Device CPU.
    cpu.clock = map.value(deviceClockKeyC).toString();
    cpu.core = map.value(deviceCoreKeyC).toString();
    cpu.fpu = map.value(deviceFpuKeyC).toString();
    cpu.mpu = map.value(deviceMpuKeyC).toString();
    // Device MEMORY.
    const QVariantList memoryList = map.value(deviceMemoryKeyC).toList();
    for (const auto &entry : memoryList) {
        const auto m = entry.toMap();
        DeviceSelection::Memory memory;
        memory.id = m.value(deviceMemoryIdKeyC).toString();
        memory.start = m.value(deviceMemoryStartKeyC).toString();
        memory.size = m.value(deviceMemorySizeKeyC).toString();
        memories.push_back(memory);
    }
    // Device ALGORITHM.
    algorithmIndex = map.value(deviceAlgorithmIndexKeyC).toInt();
    const QVariantList algorithmList = map.value(deviceAlgorithmKeyC).toList();
    for (const auto &entry : algorithmList) {
        const auto m = entry.toMap();
        DeviceSelection::Algorithm algorithm;
        algorithm.path = m.value(deviceAlgorithmPathKeyC).toString();
        algorithm.start = m.value(deviceAlgorithmStartKeyC).toString();
        algorithm.size = m.value(deviceAlgorithmSizeKeyC).toString();
        algorithms.push_back(algorithm);
    }
}

bool DeviceSelection::Package::operator==(const Package &other) const
{
    return desc == other.desc && file == other.file
            && name == other.name && url == other.url
            && vendorName == other.vendorName && vendorId == other.vendorId
            && version == other.version;
}

bool DeviceSelection::Cpu::operator==(const Cpu &other) const
{
    return core == other.core && clock == other.clock
            && fpu == other.fpu && mpu == other.mpu;
}

bool DeviceSelection::Memory::operator==(const Memory &other) const
{
    return id == other.id && start == other.start && size == other.size;
}

bool DeviceSelection::Algorithm::operator==(const Algorithm &other) const
{
    return path == other.path && start == other.start && size == other.size;
}

bool DeviceSelection::operator==(const DeviceSelection &other) const
{
    return package == other.package && name == other.name && desc == other.desc
            && family == other.family && subfamily == other.subfamily
            && vendorName == other.vendorName && vendorId == other.vendorId
            && svd == other.svd && cpu == other.cpu
            && memories == other.memories && algorithms == other.algorithms
            && algorithmIndex == other.algorithmIndex;
}

// DeviceSelectionMemoryItem

class DeviceSelectionMemoryItem final : public TreeItem
{
public:
    enum Column { IdColumn, StartColumn, SizeColumn};
    explicit DeviceSelectionMemoryItem(int index, DeviceSelection &selection)
        : m_index(index), m_selection(selection)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const auto &memory = m_selection.memories.at(m_index);
            switch (column) {
            case IdColumn: return memory.id;
            case StartColumn: return memory.start;
            case SizeColumn: return memory.size;
            }
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) final
    {
        if (role == Qt::EditRole) {
            auto &memory = m_selection.memories.at(m_index);
            switch (column) {
            case StartColumn:
                memory.start = data.toString();
                return true;
            case SizeColumn:
                memory.size = data.toString();
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const final
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (column == StartColumn || column == SizeColumn)
            flags |= Qt::ItemIsEditable;
        return flags;
    }

private:
    const int m_index;
    DeviceSelection &m_selection;
};

// DeviceSelectionMemoryModel

DeviceSelectionMemoryModel::DeviceSelectionMemoryModel(DeviceSelection &selection, QObject *parent)
    : TreeModel<TreeItem, DeviceSelectionMemoryItem>(parent), m_selection(selection)
{
    setHeader({tr("ID"), tr("Start"), tr("Size")});
    refresh();
}

void DeviceSelectionMemoryModel::refresh()
{
    clear();

    const auto begin = m_selection.memories.begin();
    for (auto it = begin; it < m_selection.memories.end(); ++it) {
        const auto index = std::distance(begin, it);
        const auto item = new DeviceSelectionMemoryItem(index, m_selection);
        rootItem()->appendChild(item);
    }
}

// DeviceSelectionMemoryView

DeviceSelectionMemoryView::DeviceSelectionMemoryView(DeviceSelection &selection, QWidget *parent)
    : TreeView(parent)
{
    setRootIsDecorated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    const auto model = new DeviceSelectionMemoryModel(selection, this);
    setModel(model);

    connect(model, &DeviceSelectionMemoryModel::dataChanged,
            this, &DeviceSelectionMemoryView::memoryChanged);
}

void DeviceSelectionMemoryView::refresh()
{
    qobject_cast<DeviceSelectionMemoryModel *>(model())->refresh();
}

// DeviceSelectionAlgorithmItem

class DeviceSelectionAlgorithmItem final : public TreeItem
{
public:
    enum Column { PathColumn, StartColumn, SizeColumn };
    explicit DeviceSelectionAlgorithmItem(int index, DeviceSelection &selection)
        : m_index(index), m_selection(selection)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const auto &algorithm = m_selection.algorithms.at(m_index);
            switch (column) {
            case PathColumn: return algorithm.path;
            case StartColumn: return algorithm.start;
            case SizeColumn: return algorithm.size;
            }
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) final
    {
        if (role == Qt::EditRole) {
            auto &algorithm = m_selection.algorithms.at(m_index);
            switch (column) {
            case StartColumn:
                algorithm.start = data.toString();
                return true;
            case SizeColumn:
                algorithm.size = data.toString();
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const final
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (column == StartColumn || column == SizeColumn)
            flags |= Qt::ItemIsEditable;
        return flags;
    }

private:
    const int m_index;
    DeviceSelection &m_selection;
};

// DeviceSelectionAlgorithmModel

DeviceSelectionAlgorithmModel::DeviceSelectionAlgorithmModel(DeviceSelection &selection,
                                                             QObject *parent)
    : TreeModel<TreeItem, DeviceSelectionAlgorithmItem>(parent), m_selection(selection)
{
    setHeader({tr("Name"), tr("Start"), tr("Size")});
    refresh();
}

void DeviceSelectionAlgorithmModel::refresh()
{
    clear();

    const auto begin = m_selection.algorithms.begin();
    for (auto it = begin; it < m_selection.algorithms.end(); ++it) {
        const auto index = std::distance(begin, it);
        const auto item = new DeviceSelectionAlgorithmItem(index, m_selection);
        rootItem()->appendChild(item);
    }
}

// DeviceSelectionAlgorithmView

DeviceSelectionAlgorithmView::DeviceSelectionAlgorithmView(DeviceSelection &selection,
                                                           QWidget *parent)
    : QWidget(parent)
{
    const auto model = new DeviceSelectionAlgorithmModel(selection, this);
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    m_comboBox = new QComboBox;
    m_comboBox->setToolTip(tr("Algorithm path."));
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBox->setModel(model);
    layout->addWidget(m_comboBox);
    const auto startEdit = new QLineEdit;
    startEdit->setToolTip(tr("Start address."));
    layout->addWidget(startEdit);
    const auto sizeEdit = new QLineEdit;
    sizeEdit->setToolTip(tr("Size."));
    layout->addWidget(sizeEdit);
    setLayout(layout);

    const auto mapper = new QDataWidgetMapper(this);
    mapper->setModel(model);
    mapper->addMapping(startEdit, DeviceSelectionAlgorithmItem::StartColumn);
    mapper->addMapping(sizeEdit, DeviceSelectionAlgorithmItem::SizeColumn);

    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [mapper, this](int index) {
        mapper->setCurrentIndex(index);
        emit algorithmChanged(index);
    });

    connect(model, &DeviceSelectionAlgorithmModel::dataChanged, this, [this]() {
        emit algorithmChanged(-1);
    });

    connect(startEdit, &QLineEdit::editingFinished, mapper, &QDataWidgetMapper::submit);
    connect(sizeEdit, &QLineEdit::editingFinished, mapper, &QDataWidgetMapper::submit);
}

void DeviceSelectionAlgorithmView::setAlgorithm(int index)
{
    m_comboBox->setCurrentIndex(index);
}

void DeviceSelectionAlgorithmView::refresh()
{
    const QSignalBlocker blocker(this);
    qobject_cast<DeviceSelectionAlgorithmModel *>(m_comboBox->model())->refresh();
}

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
