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

#pragma once

#include "fileutils.h"
#include "id.h"
#include "infolabel.h"
#include "macroexpander.h"
#include "optional.h"
#include "pathchooser.h"

#include <functional>
#include <memory>

namespace Utils {

class BaseAspects;
class LayoutBuilder;

namespace Internal {
class AspectContainerPrivate;
class BoolAspectPrivate;
class IntegerAspectPrivate;
class MultiSelectionAspectPrivate;
class SelectionAspectPrivate;
class StringAspectPrivate;
class StringListAspectPrivate;
class TextDisplayPrivate;
} // Internal

class QTCREATOR_UTILS_EXPORT BaseAspect : public QObject
{
    Q_OBJECT

public:
    BaseAspect();
    ~BaseAspect() override;

    void setId(Utils::Id id) { m_id = id; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setSettingsKey(const QString &settingsKey);
    void setSettingsKey(const QString &group, const QString &key);

    Utils::Id id() const { return m_id; }
    QString displayName() const { return m_displayName; }
    QString settingsKey() const;

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    using ConfigWidgetCreator = std::function<QWidget *()>;
    void setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator);
    QWidget *createConfigWidget() const;

    virtual void fromMap(const QVariantMap &);
    virtual void toMap(QVariantMap &) const;
    virtual void acquaintSiblings(const BaseAspects &);

    virtual void addToLayout(LayoutBuilder &builder);

signals:
    void changed();

protected:
    virtual void setVisibleDynamic(bool visible) { Q_UNUSED(visible) } // TODO: Better name? Merge with setVisible() somehow?
    void saveToMap(QVariantMap &data, const QVariant &value,
                   const QVariant &defaultValue, const QString &keyExtension = {}) const;

    Utils::Id m_id;
    QString m_displayName;
    QString m_settingsKey; // Name of data in settings.
    bool m_visible = true;
    ConfigWidgetCreator m_configWidgetCreator;
};

class QTCREATOR_UTILS_EXPORT BaseAspects
{
    BaseAspects(const BaseAspects &) = delete;
    BaseAspects &operator=(const BaseAspects &) = delete;

public:
    using const_iterator = QList<BaseAspect *>::const_iterator;
    using value_type = QList<BaseAspect *>::value_type;

    BaseAspects();
    ~BaseAspects();

    template <class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        auto aspect = new Aspect(args...);
        m_aspects.append(aspect);
        return aspect;
    }

    BaseAspect *aspect(Utils::Id id) const;

    template <typename T> T *aspect() const
    {
        for (BaseAspect *aspect : m_aspects)
            if (T *result = qobject_cast<T *>(aspect))
                return result;
        return nullptr;
    }

    void fromMap(const QVariantMap &map) const;
    void toMap(QVariantMap &map) const;

    const_iterator begin() const { return m_aspects.begin(); }
    const_iterator end() const { return m_aspects.end(); }

    void append(BaseAspect *const &aspect) { m_aspects.append(aspect); }

private:
    QList<BaseAspect *> m_aspects;
};

class QTCREATOR_UTILS_EXPORT BoolAspect : public BaseAspect
{
    Q_OBJECT

public:
    explicit BoolAspect(const QString &settingsKey = QString());
    ~BoolAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    bool value() const;
    void setValue(bool val);

    bool defaultValue() const;
    void setDefaultValue(bool defaultValue);

    enum class LabelPlacement { AtCheckBox, AtCheckBoxWithoutDummyLabel, InExtraLabel };
    void setLabel(const QString &labelText,
                  LabelPlacement labelPlacement = LabelPlacement::InExtraLabel);
    void setToolTip(const QString &tooltip);
    void setEnabled(bool enabled);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::BoolAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT SelectionAspect : public BaseAspect
{
    Q_OBJECT

public:
    SelectionAspect();
    ~SelectionAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    int value() const;
    void setValue(int val);

    QString stringValue() const;

    int defaultValue() const;
    void setDefaultValue(int defaultValue);

    enum class DisplayStyle { RadioButtons, ComboBox };
    void setDisplayStyle(DisplayStyle style);

    void setToolTip(const QString &tooltip);

    void addOption(const QString &displayName, const QString &toolTip = {});

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

protected:
    void setVisibleDynamic(bool visible) override;

private:
    std::unique_ptr<Internal::SelectionAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT MultiSelectionAspect : public BaseAspect
{
    Q_OBJECT

public:
    MultiSelectionAspect();
    ~MultiSelectionAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    enum class DisplayStyle { ListView };
    void setDisplayStyle(DisplayStyle style);

    QStringList value() const;
    void setValue(const QStringList &val);

    QStringList allValues() const;
    void setAllValues(const QStringList &val);

    void setLabelText(const QString &labelText);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::MultiSelectionAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT StringAspect : public BaseAspect
{
    Q_OBJECT

public:
    StringAspect();
    ~StringAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    // Hook between UI and StringAspect:
    using ValueAcceptor = std::function<Utils::optional<QString>(const QString &, const QString &)>;
    void setValueAcceptor(ValueAcceptor &&acceptor);
    QString value() const;
    void setValue(const QString &val);

    QString labelText() const;
    void setLabelText(const QString &labelText);
    void setLabelPixmap(const QPixmap &labelPixmap);
    void setShowToolTipOnLabel(bool show);
    void setEnabled(bool enabled);

    void setDisplayFilter(const std::function<QString (const QString &)> &displayFilter);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const QString &historyCompleterKey);
    void setExpectedKind(const Utils::PathChooser::Kind expectedKind);
    void setFileDialogOnly(bool requireFileDialog);
    void setEnvironment(const Utils::Environment &env);
    void setBaseFileName(const Utils::FilePath &baseFileName);
    void setToolTip(const QString &tooltip);
    void setReadOnly(bool readOnly);
    void setUndoRedoEnabled(bool readOnly);
    void setMacroExpanderProvider(const Utils::MacroExpanderProvider &expanderProvider);
    void setValidationFunction(const Utils::FancyLineEdit::ValidationFunction &validator);
    void setOpenTerminalHandler(const std::function<void()> &openTerminal);

    void validateInput();

    enum class UncheckedSemantics { Disabled, ReadOnly };
    enum class CheckBoxPlacement { Top, Right };
    void setUncheckedSemantics(UncheckedSemantics semantics);
    bool isChecked() const;
    void setChecked(bool checked);
    void makeCheckable(CheckBoxPlacement checkBoxPlacement, const QString &optionalLabel,
                       const QString &optionalBaseKey);

    enum DisplayStyle {
        LabelDisplay,
        LineEditDisplay,
        TextEditDisplay,
        PathChooserDisplay
    };
    void setDisplayStyle(DisplayStyle style);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    Utils::FilePath filePath() const;
    void setFilePath(const Utils::FilePath &value);

signals:
    void checkedChanged();

private:
    void update();

    std::unique_ptr<Internal::StringAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT IntegerAspect : public BaseAspect
{
    Q_OBJECT

public:
    IntegerAspect();
    ~IntegerAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    qint64 value() const;
    void setValue(qint64 val);

    void setRange(qint64 min, qint64 max);
    void setLabel(const QString &label);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setDisplayIntegerBase(int base);
    void setDisplayScaleFactor(qint64 factor);
    void setEnabled(bool enabled);
    void setDefaultValue(qint64 defaultValue);
    void setToolTip(const QString &tooltip);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::IntegerAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT TriState
{
    enum Value { EnabledValue, DisabledValue, DefaultValue };
    explicit TriState(Value v) : m_value(v) {}

public:
    TriState() = default;

    QVariant toVariant() const { return int(m_value); }
    static TriState fromVariant(const QVariant &variant);

    static const TriState Enabled;
    static const TriState Disabled;
    static const TriState Default;

    friend bool operator==(TriState a, TriState b) { return a.m_value == b.m_value; }
    friend bool operator!=(TriState a, TriState b) { return a.m_value != b.m_value; }

private:
    Value m_value = DefaultValue;
};

class QTCREATOR_UTILS_EXPORT TriStateAspect : public SelectionAspect
{
    Q_OBJECT
public:
    TriStateAspect(
            const QString onString = tr("Enable"),
            const QString &offString = tr("Disable"),
            const QString &defaultString = tr("Leave at Default"));

    TriState setting() const;
    void setSetting(TriState setting);
};

class QTCREATOR_UTILS_EXPORT StringListAspect : public BaseAspect
{
    Q_OBJECT

public:
    StringListAspect();
    ~StringListAspect() override;

    void addToLayout(LayoutBuilder &builder) override;

    QStringList value() const;
    void setValue(const QStringList &val);

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    std::unique_ptr<Internal::StringListAspectPrivate> d;
};

class QTCREATOR_UTILS_EXPORT TextDisplay : public BaseAspect
{
    Q_OBJECT

public:
    TextDisplay(const QString &message = {},
                Utils::InfoLabel::InfoType type = Utils::InfoLabel::None);
    ~TextDisplay() override;

    void addToLayout(LayoutBuilder &builder) override;

    void setVisible(bool visible);
    void setToolTip(const QString &tooltip);
    void setIconType(Utils::InfoLabel::InfoType t);

private:
    std::unique_ptr<Internal::TextDisplayPrivate> d;
};

class QTCREATOR_UTILS_EXPORT AspectContainer : public BaseAspect
{
    Q_OBJECT

public:
    AspectContainer();
    ~AspectContainer() override;

    template <class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        auto aspect = new Aspect(args...);
        addAspectHelper(aspect);
        return aspect;
    }

    void addToLayout(LayoutBuilder &builder) override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    void addAspectHelper(BaseAspect *aspect);

    std::unique_ptr<Internal::AspectContainerPrivate> d;
};

} // namespace Utils
