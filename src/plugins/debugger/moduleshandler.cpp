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

#include "moduleshandler.h"

#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QSortFilterProxyModel>

using namespace Utils;

//////////////////////////////////////////////////////////////////
//
// ModulesModel
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class ModuleItem : public TreeItem
{
public:
    QVariant data(int column, int role) const;

public:
    Module module;
    bool updated;
};

QVariant ModuleItem::data(int column, int role) const
{
    switch (column) {
    case 0:
        if (role == Qt::DisplayRole)
            return module.moduleName;
        // FIXME: add icons
        //if (role == Qt::DecorationRole)
        //    return module.symbolsRead ? icon2 : icon;
        break;
    case 1:
        if (role == Qt::DisplayRole)
            return module.modulePath;
        if (role == Qt::ToolTipRole) {
            QString msg;
            if (!module.elfData.buildId.isEmpty())
                msg += QString::fromLatin1("Build Id: " + module.elfData.buildId);
            if (!module.elfData.debugLink.isEmpty())
                msg += QString::fromLatin1("Debug Link: " + module.elfData.debugLink);
            return msg;
        }
        break;
    case 2:
        if (role == Qt::DisplayRole)
            switch (module.symbolsRead) {
            case Module::UnknownReadState: return ModulesHandler::tr("Unknown");
            case Module::ReadFailed: return ModulesHandler::tr("No");
            case Module::ReadOk: return ModulesHandler::tr("Yes");
            }
        break;
    case 3:
        if (role == Qt::DisplayRole)
            switch (module.elfData.symbolsType) {
            case UnknownSymbols:
                return ModulesHandler::tr("Unknown");
            case NoSymbols:
                return ModulesHandler::tr("None");
            case PlainSymbols:
                return ModulesHandler::tr("Plain");
            case FastSymbols:
                return ModulesHandler::tr("Fast");
            case LinkedSymbols:
                return ModulesHandler::tr("debuglnk");
            case BuildIdSymbols:
                return ModulesHandler::tr("buildid");
            }
        else if (role == Qt::ToolTipRole)
            switch (module.elfData.symbolsType) {
            case UnknownSymbols:
                return ModulesHandler::tr(
                            "It is unknown whether this module contains debug "
                            "information.\nUse \"Examine Symbols\" from the "
                            "context menu to initiate a check.");
            case NoSymbols:
                return ModulesHandler::tr(
                            "This module neither contains nor references debug "
                            "information.\nStepping into the module or setting "
                            "breakpoints by file and line will not work.");
            case PlainSymbols:
                return ModulesHandler::tr(
                            "This module contains debug information.\nStepping "
                            "into the module or setting breakpoints by file and "
                            "line is expected to work.");
            case FastSymbols:
                return ModulesHandler::tr(
                            "This module contains debug information.\nStepping "
                            "into the module or setting breakpoints by file and "
                            "line is expected to work.");
            case LinkedSymbols:
            case BuildIdSymbols:
                return ModulesHandler::tr(
                            "This module does not contain debug information "
                            "itself, but contains a reference to external "
                            "debug information.");
            }
        break;
    case 4:
        if (role == Qt::DisplayRole)
            if (module.startAddress)
                return QString(QLatin1String("0x")
                               + QString::number(module.startAddress, 16));
        break;
    case 5:
        if (role == Qt::DisplayRole) {
            if (module.endAddress)
                return QString(QLatin1String("0x")
                               + QString::number(module.endAddress, 16));
            //: End address of loaded module
            return ModulesHandler::tr("<unknown>", "address");
        }
        break;
    }
    return QVariant();
}

//////////////////////////////////////////////////////////////////
//
// ModulesHandler
//
//////////////////////////////////////////////////////////////////

static ModuleItem *moduleFromPath(TreeItem *root, const QString &modulePath)
{
    // Recent modules are more likely to be unloaded first.
    for (int i = root->childCount(); --i >= 0; ) {
        auto item = static_cast<ModuleItem *>(root->childAt(i));
        if (item->module.modulePath == modulePath)
            return item;
    }
    return 0;
}

ModulesHandler::ModulesHandler(DebuggerEngine *engine)
{
    m_engine = engine;

    QString pad = QLatin1String("        ");
    m_model = new TreeModel(this);
    m_model->setObjectName(QLatin1String("ModulesModel"));
    m_model->setHeader(QStringList()
        << ModulesHandler::tr("Module Name") + pad
        << ModulesHandler::tr("Module Path") + pad
        << ModulesHandler::tr("Symbols Read") + pad
        << ModulesHandler::tr("Symbols Type") + pad
        << ModulesHandler::tr("Start Address") + pad
        << ModulesHandler::tr("End Address") + pad);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setObjectName(QLatin1String("ModulesProxyModel"));
    m_proxyModel->setSourceModel(m_model);
}

QAbstractItemModel *ModulesHandler::model() const
{
    return m_proxyModel;
}

void ModulesHandler::removeAll()
{
    m_model->clear();
}

Modules ModulesHandler::modules() const
{
    Modules mods;
    TreeItem *root = m_model->rootItem();
    for (int i = root->childCount(); --i >= 0; )
        mods.append(static_cast<ModuleItem *>(root->childAt(i))->module);
    return mods;
}

void ModulesHandler::removeModule(const QString &modulePath)
{
    if (ModuleItem *item = moduleFromPath(m_model->rootItem(), modulePath))
        m_model->destroyItem(item);
}

void ModulesHandler::updateModule(const Module &module)
{
    const QString path = module.modulePath;
    if (path.isEmpty())
        return;

    ModuleItem *item = moduleFromPath(m_model->rootItem(), path);
    if (item) {
        item->module = module;
    } else {
        item = new ModuleItem;
        item->module = module;
        m_model->rootItem()->appendChild(item);
    }

    try { // MinGW occasionallly throws std::bad_alloc.
        ElfReader reader(path);
        item->module.elfData = reader.readHeaders();
        item->update();
    } catch(...) {
        qWarning("%s: An exception occurred while reading module '%s'",
                 Q_FUNC_INFO, qPrintable(module.modulePath));
    }
    item->updated = true;
}

void ModulesHandler::beginUpdateAll()
{
    TreeItem *root = m_model->rootItem();
    for (int i = root->childCount(); --i >= 0; )
        static_cast<ModuleItem *>(root->childAt(i))->updated = false;
}

void ModulesHandler::endUpdateAll()
{
    TreeItem *root = m_model->rootItem();
    for (int i = root->childCount(); --i >= 0; ) {
        auto item = static_cast<ModuleItem *>(root->childAt(i));
        if (!item->updated)
            m_model->destroyItem(item);
    }
}

} // namespace Internal
} // namespace Debugger
