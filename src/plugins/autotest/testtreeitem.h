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

#pragma once

#include <utils/treemodel.h>

#include <QList>
#include <QString>
#include <QMetaType>

namespace {
    enum ItemRole {
        LinkRole = Qt::UserRole + 2, // can be removed if AnnotationRole comes back
        ItalicRole, // used only inside the delegate
        TypeRole,
        EnabledRole
    };
}

namespace Autotest {
namespace Internal {

class TestParseResult;
class TestConfiguration;

class TestTreeItem : public Utils::TreeItem
{
public:
    enum Type
    {
        Root,
        TestCase,
        TestFunctionOrSet,
        TestDataTag,
        TestDataFunction,
        TestSpecialFunction
    };

    enum SortMode {
        Alphabetically,
        Naturally
    };

    TestTreeItem(const QString &name = QString(), const QString &filePath = QString(),
                 Type type = Root);

    virtual QVariant data(int column, int role) const override;
    virtual bool setData(int column, const QVariant &data, int role) override;
    virtual Qt::ItemFlags flags(int column) const override;
    bool modifyTestCaseContent(const QString &name, unsigned line, unsigned column);
    bool modifyTestFunctionContent(const TestParseResult *result);
    bool modifyDataTagContent(const QString &name, const QString &fileName, unsigned line,
                              unsigned column);
    bool modifyLineAndColumn(unsigned line, unsigned column);

    const QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }
    const QString filePath() const { return m_filePath; }
    void setFilePath(const QString &filePath) { m_filePath = filePath; }
    void setLine(unsigned line) { m_line = line;}
    unsigned line() const { return m_line; }
    void setColumn(unsigned column) { m_column = column; }
    unsigned column() const { return m_column; }
    QString proFile() const { return m_proFile; }
    void setProFile(const QString &proFile) { m_proFile = proFile; }
    virtual void setChecked(const Qt::CheckState checked);
    virtual Qt::CheckState checked() const;
    Type type() const { return m_type; }
    void markForRemoval(bool mark);
    void markForRemovalRecursively(bool mark);
    virtual void markForRemovalRecursively(const QString &filePath);
    bool markedForRemoval() const { return m_status == MarkedForRemoval; }
    bool newlyAdded() const { return m_status == NewlyAdded; }
    TestTreeItem *parentItem() const;
    TestTreeItem *childItem(int row) const;

    TestTreeItem *findChildByName(const QString &name);
    TestTreeItem *findChildByFile(const QString &filePath);
    TestTreeItem *findChildByNameAndFile(const QString &name, const QString &filePath);

    virtual bool canProvideTestConfiguration() const { return false; }
    virtual bool canProvideDebugConfiguration() const { return false; }
    virtual TestConfiguration *testConfiguration() const { return 0; }
    virtual TestConfiguration *debugConfiguration() const { return 0; }
    virtual QList<TestConfiguration *> getAllTestConfigurations() const;
    virtual QList<TestConfiguration *> getSelectedTestConfigurations() const;
    virtual bool lessThan(const TestTreeItem *other, SortMode mode) const;
    virtual TestTreeItem *find(const TestParseResult *result) = 0;
    virtual bool modify(const TestParseResult *result) = 0;

protected:
    bool modifyFilePath(const QString &filePath);
    typedef std::function<bool(const TestTreeItem *)> CompareFunction;
    TestTreeItem *findChildBy(CompareFunction compare) const;

private:
    void revalidateCheckState();
    bool modifyName(const QString &name);

    enum Status
    {
        NewlyAdded,
        MarkedForRemoval,
        Cleared
    };

    QString m_name;
    QString m_filePath;
    Qt::CheckState m_checked;
    Type m_type;
    unsigned m_line;
    unsigned m_column;
    QString m_proFile;
    Status m_status;
};

class TestCodeLocationAndType
{
public:
    QString m_name;     // tag name for m_type == TEST_DATATAG, file name for other values
    unsigned m_line;
    unsigned m_column;
    TestTreeItem::Type m_type;
};

typedef QVector<TestCodeLocationAndType> TestCodeLocationList;

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestTreeItem *)
Q_DECLARE_METATYPE(Autotest::Internal::TestCodeLocationAndType)
