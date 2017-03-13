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

#include "watchhandler.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerprotocol.h"
#include "debuggertooltipmanager.h"
#include "simplifytype.h"
#include "imageviewer.h"
#include "watchutils.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/checkablemessagebox.h>
#include <utils/theme/theme.h>

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QProcess>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>

#include <algorithm>
#include <cstring>
#include <ctype.h>

using namespace Utils;

namespace Debugger {
namespace Internal {

// Creates debug output for accesses to the model.
enum { debugModel = 0 };

#define MODEL_DEBUG(s) do { if (debugModel) qDebug() << s; } while (0)

static QHash<QString, int> theWatcherNames;
static int theWatcherCount = 0;
static QHash<QString, int> theTypeFormats;
static QHash<QString, int> theIndividualFormats;
static int theUnprintableBase = -1;

const char INameProperty[] = "INameProperty";
const char KeyProperty[] = "KeyProperty";

struct TypeInfo
{
    TypeInfo(uint s = 0) : size(s) {}
    uint size;
};

static const WatchModel *watchModel(const WatchItem *item)
{
    return reinterpret_cast<const WatchModel *>(item->model());
}

template <class T>
void readNumericVectorHelper(std::vector<double> *v, const QByteArray &ba)
{
    const T *p = (const T *) ba.data();
    const int n = ba.size() / sizeof(T);
    v->resize(n);
    // Losing precision in case of 64 bit ints is ok here, as the result
    // is only used to plot data.
    for (int i = 0; i != n; ++i)
        (*v)[i] = static_cast<double>(p[i]);
}

static void readNumericVector(std::vector<double> *v, const QByteArray &rawData, DebuggerEncoding encoding)
{
    switch (encoding.type) {
        case DebuggerEncoding::HexEncodedSignedInteger:
            switch (encoding.size) {
                case 1:
                    readNumericVectorHelper<signed char>(v, rawData);
                    return;
                case 2:
                    readNumericVectorHelper<short>(v, rawData);
                    return;
                case 4:
                    readNumericVectorHelper<int>(v, rawData);
                    return;
                case 8:
                    readNumericVectorHelper<qint64>(v, rawData);
                    return;
                }
        case DebuggerEncoding::HexEncodedUnsignedInteger:
            switch (encoding.size) {
                case 1:
                    readNumericVectorHelper<char>(v, rawData);
                    return;
                case 2:
                    readNumericVectorHelper<unsigned short>(v, rawData);
                    return;
                case 4:
                    readNumericVectorHelper<unsigned int>(v, rawData);
                    return;
                case 8:
                    readNumericVectorHelper<quint64>(v, rawData);
                    return;
            }
        case DebuggerEncoding::HexEncodedFloat:
            switch (encoding.size) {
                case 4:
                    readNumericVectorHelper<float>(v, rawData);
                    return;
                case 8:
                    readNumericVectorHelper<double>(v, rawData);
                    return;
        }
        default:
            break;
    }
    qDebug() << "ENCODING ERROR: " << encoding.toString();
}

static QString stripForFormat(const QString &ba)
{
    QString res;
    res.reserve(ba.size());
    int inArray = 0;
    for (int i = 0; i != ba.size(); ++i) {
        const QChar c = ba.at(i);
        if (c == '<')
            break;
        if (c == '[')
            ++inArray;
        if (c == ']')
            --inArray;
        if (c == ' ')
            continue;
        if (c == '&') // Treat references like the referenced type.
            continue;
        if (inArray && c >= '0' && c <= '9')
            continue;
        res.append(c);
    }
    return res;
}

static void saveWatchers()
{
    setSessionValue("Watchers", WatchHandler::watchedExpressions());
}

static void loadFormats()
{
    QVariant value = sessionValue("DefaultFormats");
    QMapIterator<QString, QVariant> it(value.toMap());
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            theTypeFormats.insert(it.key(), it.value().toInt());
    }

    value = sessionValue("IndividualFormats");
    it = QMapIterator<QString, QVariant>(value.toMap());
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            theIndividualFormats.insert(it.key(), it.value().toInt());
    }
}

static void saveFormats()
{
    QMap<QString, QVariant> formats;
    QHashIterator<QString, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != AutomaticFormat) {
            const QString key = it.key().trimmed();
            if (!key.isEmpty())
                formats.insert(key, format);
        }
    }
    setSessionValue("DefaultFormats", formats);

    formats.clear();
    it = QHashIterator<QString, int>(theIndividualFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        const QString key = it.key().trimmed();
        if (!key.isEmpty())
            formats.insert(key, format);
    }
    setSessionValue("IndividualFormats", formats);
}

///////////////////////////////////////////////////////////////////////
//
// SeparatedView
//
///////////////////////////////////////////////////////////////////////

class SeparatedView : public QTabWidget
{
public:
    SeparatedView() : QTabWidget(Internal::mainWindow())
    {
        setTabsClosable(true);
        connect(this, &QTabWidget::tabCloseRequested, this, &SeparatedView::closeTab);
        setWindowFlags(windowFlags() | Qt::Window);
        setWindowTitle(WatchHandler::tr("Debugger - Qt Creator"));

        QVariant geometry = sessionValue("DebuggerSeparateWidgetGeometry");
        if (geometry.isValid()) {
            QRect rc = geometry.toRect();
            if (rc.width() < 200)
                rc.setWidth(200);
            if (rc.height() < 200)
                rc.setHeight(200);
            setGeometry(rc);
        }
    }

    void saveGeometry()
    {
        setSessionValue("DebuggerSeparateWidgetGeometry", geometry());
    }

    ~SeparatedView()
    {
        saveGeometry();
    }

    void removeObject(const QString &key)
    {
        saveGeometry();
        if (QWidget *w = findWidget(key)) {
            removeTab(indexOf(w));
            sanitize();
        }
    }

    void closeTab(int index)
    {
        saveGeometry();
        if (QObject *o = widget(index)) {
            QString iname = o->property(INameProperty).toString();
            theIndividualFormats.remove(iname);
            saveFormats();
        }
        removeTab(index);
        sanitize();
    }

    void sanitize()
    {
        if (count() == 0)
            hide();
    }

    QWidget *findWidget(const QString &needle)
    {
        for (int i = count(); --i >= 0; ) {
            QWidget *w = widget(i);
            QString key = w->property(KeyProperty).toString();
            if (key == needle)
                return w;
        }
        return 0;
    }

    template <class T> T *prepareObject(const WatchItem *item)
    {
        const QString key = item->key();
        T *t = 0;
        if (QWidget *w = findWidget(key)) {
            t = qobject_cast<T *>(w);
            if (!t)
                removeTab(indexOf(w));
        }
        if (!t) {
            t = new T;
            t->setProperty(KeyProperty, key);
            addTab(t, item->name);
        }
        setProperty(INameProperty, item->iname);

        setCurrentWidget(t);
        show();
        raise();
        return t;
    }
};


///////////////////////////////////////////////////////////////////////
//
// WatchModel
//
///////////////////////////////////////////////////////////////////////

class WatchModel : public WatchModelBase
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::WatchModel)
public:
    WatchModel(WatchHandler *handler, DebuggerEngine *engine);

    static QString nameForFormat(int format);

    QVariant data(const QModelIndex &idx, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &idx) const override;
    bool canFetchMore(const QModelIndex &idx) const override;
    void fetchMore(const QModelIndex &idx) override;

    QString displayForAutoTest(const QByteArray &iname) const;
    void reinitialize(bool includeInspectData = false);

    WatchItem *findItem(const QString &iname) const;

    void reexpandItems();

    void showEditValue(const WatchItem *item);
    void setTypeFormat(const QString &type, int format);
    void setIndividualFormat(const QString &iname, int format);

    QString removeNamespaces(QString str) const;

public:
    WatchHandler *m_handler; // Not owned.
    DebuggerEngine *m_engine; // Not owned.

    bool m_contentsValid;
    bool m_resetLocationScheduled;

    WatchItem *m_localsRoot; // Not owned.
    WatchItem *m_inspectorRoot; // Not owned.
    WatchItem *m_watchRoot; // Not owned.
    WatchItem *m_returnRoot; // Not owned.
    WatchItem *m_tooltipRoot; // Not owned.

    SeparatedView *m_separatedView; // Not owned.

    QSet<QString> m_expandedINames;
    QTimer m_requestUpdateTimer;

    QHash<QString, TypeInfo> m_reportedTypeInfo;
    QHash<QString, DisplayFormats> m_reportedTypeFormats; // Type name -> Dumper Formats
    QHash<QString, QString> m_valueCache;
};

WatchModel::WatchModel(WatchHandler *handler, DebuggerEngine *engine)
    : m_handler(handler), m_engine(engine), m_separatedView(new SeparatedView)
{
    setObjectName("WatchModel");

    m_contentsValid = false;
    m_contentsValid = true; // FIXME
    m_resetLocationScheduled = false;

    setHeader({ tr("Name"), tr("Value"), tr("Type") });
    m_localsRoot = new WatchItem;
    m_localsRoot->iname = "local";
    m_localsRoot->name = tr("Locals");
    m_inspectorRoot = new WatchItem;
    m_inspectorRoot->iname = "inspect";
    m_inspectorRoot->name = tr("Inspector");
    m_watchRoot = new WatchItem;
    m_watchRoot->iname = "watch";
    m_watchRoot->name = tr("Expressions");
    m_returnRoot = new WatchItem;
    m_returnRoot->iname = "return";
    m_returnRoot->name = tr("Return Value");
    m_tooltipRoot = new WatchItem;
    m_tooltipRoot->iname = "tooltip";
    m_tooltipRoot->name = tr("Tooltip");
    auto root = new WatchItem;
    root->appendChild(m_localsRoot);
    root->appendChild(m_inspectorRoot);
    root->appendChild(m_watchRoot);
    root->appendChild(m_returnRoot);
    root->appendChild(m_tooltipRoot);
    setRootItem(root);

    m_requestUpdateTimer.setSingleShot(true);
    connect(&m_requestUpdateTimer, &QTimer::timeout,
        this, &WatchModel::updateStarted);

    connect(action(SortStructMembers), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateLocals);
    connect(action(ShowStdNamespace), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateAll);
    connect(action(ShowQtNamespace), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateAll);
    connect(action(ShowQObjectNames), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateAll);
}

void WatchModel::reinitialize(bool includeInspectData)
{
    m_localsRoot->removeChildren();
    m_watchRoot->removeChildren();
    m_returnRoot->removeChildren();
    m_tooltipRoot->removeChildren();
    if (includeInspectData)
        m_inspectorRoot->removeChildren();
}

WatchItem *WatchModel::findItem(const QString &iname) const
{
    return findNonRooItem([iname](WatchItem *item) { return item->iname == iname; });
}

static QString parentName(const QString &iname)
{
    const int pos = iname.lastIndexOf('.');
    return pos == -1 ? QString() : iname.left(pos);
}

static QString niceTypeHelper(const QString &typeIn)
{
    typedef QMap<QString, QString> Cache;
    static Cache cache;
    const Cache::const_iterator it = cache.constFind(typeIn);
    if (it != cache.constEnd())
        return it.value();
    const QString simplified = simplifyType(typeIn);
    cache.insert(typeIn, simplified); // For simplicity, also cache unmodified types
    return simplified;
}

QString WatchModel::removeNamespaces(QString str) const
{
    if (!boolSetting(ShowStdNamespace))
        str.remove("std::");
    if (!boolSetting(ShowQtNamespace)) {
        const QString qtNamespace = m_engine->qtNamespace();
        if (!qtNamespace.isEmpty())
            str.remove(qtNamespace);
    }
    return str;
}

static int formatToIntegerBase(int format)
{
    switch (format) {
        case HexadecimalIntegerFormat:
            return 16;
        case BinaryIntegerFormat:
            return 2;
        case OctalIntegerFormat:
            return 8;
    }
    return 10;
}

template <class IntType> QString reformatInteger(IntType value, int format)
{
    switch (format) {
        case HexadecimalIntegerFormat:
            return "(hex) " + QString::number(value, 16);
        case BinaryIntegerFormat:
            return "(bin) " + QString::number(value, 2);
        case OctalIntegerFormat:
            return "(oct) " + QString::number(value, 8);
    }
    return QString::number(value, 10); // not reached
}

static QString reformatInteger(quint64 value, int format, int size, bool isSigned)
{
    // Follow convention and don't show negative non-decimal numbers.
    if (format != AutomaticFormat && format != DecimalIntegerFormat)
        isSigned = false;

    switch (size) {
        case 1:
            value = value & 0xff;
            return isSigned
                ? reformatInteger<qint8>(value, format)
                : reformatInteger<quint8>(value, format);
        case 2:
            value = value & 0xffff;
            return isSigned
                ? reformatInteger<qint16>(value, format)
                : reformatInteger<quint16>(value, format);
        case 4:
            value = value & 0xffffffff;
            return isSigned
                ? reformatInteger<qint32>(value, format)
                : reformatInteger<quint32>(value, format);
        default:
        case 8: return isSigned
                ? reformatInteger<qint64>(value, format)
                : reformatInteger<quint64>(value, format);
    }
}

// Format printable (char-type) characters
static QString reformatCharacter(int code, int size, bool isSigned)
{
    const QChar c = QChar(uint(code));
    QString out;
    if (c.isPrint())
        out = QString("'") + c + "' ";
    else if (code == 0)
        out = "'\\0'";
    else if (code == '\r')
        out = "'\\r'";
    else if (code == '\n')
        out = "'\\n'";
    else if (code == '\t')
        out = "'\\t'";
    else
        out = "    ";

    out += '\t';

    if (isSigned) {
        out += QString::number(code);
        if (code < 0)
            out += QString::fromLatin1("/%1    ").arg((1 << (8*size)) + code).left(2 + 2 * size);
        else
            out += QString(2 + 2 * size, QLatin1Char(' '));
    } else {
        out += QString::number(unsigned(code));
    }

    out += '\t';

    out += QString::fromLatin1("0x%1").arg(uint(code & ((1ULL << (8*size)) - 1)),
                                           2 * size, 16, QLatin1Char('0'));
    return out;
}

static QString quoteUnprintable(const QString &str)
{
    if (WatchHandler::unprintableBase() == 0)
        return str;

    QString encoded;
    if (WatchHandler::unprintableBase() == -1) {
        foreach (const QChar c, str) {
            int u = c.unicode();
            if (c.isPrint())
                encoded += c;
            else if (u == '\r')
                encoded += "\\r";
            else if (u == '\t')
                encoded += "\\t";
            else if (u == '\n')
                encoded += "\\n";
            else
                encoded += QString::fromLatin1("\\%1")
                    .arg(c.unicode(), 3, 8, QLatin1Char('0'));
        }
        return encoded;
    }

    foreach (const QChar c, str) {
        if (c.isPrint()) {
            encoded += c;
        } else if (WatchHandler::unprintableBase() == 8) {
            encoded += QString::fromLatin1("\\%1")
                .arg(c.unicode(), 3, 8, QLatin1Char('0'));
        } else {
            encoded += QString::fromLatin1("\\u%1")
                .arg(c.unicode(), 4, 16, QLatin1Char('0'));
        }
    }
    return encoded;
}

static int itemFormat(const WatchItem *item)
{
    const int individualFormat = theIndividualFormats.value(item->iname, AutomaticFormat);
    if (individualFormat != AutomaticFormat)
        return individualFormat;
    return theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);
}

static QString formattedValue(const WatchItem *item)
{
    if (item->type == "bool") {
        if (item->value == "0")
            return QLatin1String("false");
        if (item->value == "1")
            return QLatin1String("true");
        return item->value;
    }

    const int format = itemFormat(item);

    // Append quoted, printable character also for decimal.
    // FIXME: This is unreliable.
    if (item->type.endsWith("char")) {
        bool ok;
        const int code = item->value.toInt(&ok);
        bool isUnsigned = item->type == "unsigned char" || item->type == "uchar";
        if (ok)
            return reformatCharacter(code, 1, !isUnsigned);
    } else if (item->type.endsWith("wchar_t")) {
        bool ok;
        const int code = item->value.toInt(&ok);
        if (ok)
            return reformatCharacter(code, 4, false);
    } else if (item->type.endsWith("QChar")) {
        bool ok;
        const int code = item->value.toInt(&ok);
        if (ok)
            return reformatCharacter(code, 2, false);
    }

    if (format == HexadecimalIntegerFormat
            || format == DecimalIntegerFormat
            || format == OctalIntegerFormat
            || format == BinaryIntegerFormat) {
        bool isSigned = item->value.startsWith('-');
        quint64 raw = isSigned ? quint64(item->value.toLongLong()) : item->value.toULongLong();
        return reformatInteger(raw, format, item->size, isSigned);
    }

    if (format == ScientificFloatFormat) {
        double dd = item->value.toDouble();
        return QString::number(dd, 'e');
    }

    if (format == CompactFloatFormat) {
        double dd = item->value.toDouble();
        return QString::number(dd, 'g');
    }

    if (item->type == "va_list")
        return item->value;

    if (!isPointerType(item->type) && !item->isVTablePointer()) {
        bool ok = false;
        qulonglong integer = item->value.toULongLong(&ok, 0);
        if (ok) {
            const int format = itemFormat(item);
            return reformatInteger(integer, format, item->size, false);
        }
    }

    if (item->elided) {
        QString v = item->value;
        v.chop(1);
        QString len = item->elided > 0 ? QString::number(item->elided) : "unknown length";
        return quoteUnprintable(v) + "\"... (" + len  + ')';
    }

    return quoteUnprintable(item->value);
}

// Get a pointer address from pointer values reported by the debugger.
// Fix CDB formatting of pointers "0x00000000`000003fd class foo *", or
// "0x00000000`000003fd "Hallo"", or check gdb formatting of characters.
static inline quint64 pointerValue(QString data)
{
    const int blankPos = data.indexOf(' ');
    if (blankPos != -1)
        data.truncate(blankPos);
    data.remove('`');
    return data.toULongLong(0, 0);
}

// Return the type used for editing
int WatchItem::editType() const
{
    if (type == "bool")
        return QVariant::Bool;
    if (isIntType(type))
        return type.contains('u') ? QVariant::ULongLong : QVariant::LongLong;
    if (isFloatType(type))
        return QVariant::Double;
    // Check for pointers using hex values (0xAD00 "Hallo")
    if (isPointerType(type) && value.startsWith("0x"))
        return QVariant::ULongLong;
   return QVariant::String;
}

// Convert to editable (see above)
QVariant WatchItem::editValue() const
{
    switch (editType()) {
    case QVariant::Bool:
        return value != "0" && value != "false";
    case QVariant::ULongLong:
        if (isPointerType(type)) // Fix pointer values (0xAD00 "Hallo" -> 0xAD00)
            return QVariant(pointerValue(value));
        return QVariant(value.toULongLong());
    case QVariant::LongLong:
        return QVariant(value.toLongLong());
    case QVariant::Double:
        return QVariant(value.toDouble());
    default:
        break;
    }
    // Some string value: '0x434 "Hallo"':
    // Remove quotes and replace newlines, which will cause line edit troubles.
    QString stringValue = value;
    if (stringValue.endsWith('"')) {
        const int leadingDoubleQuote = stringValue.indexOf('"');
        if (leadingDoubleQuote != stringValue.size() - 1) {
            stringValue.truncate(stringValue.size() - 1);
            stringValue.remove(0, leadingDoubleQuote + 1);
            stringValue.replace("\n", "\\n");
        }
    }
    return QVariant(quoteUnprintable(stringValue));
}

// Truncate value for item view, maintaining quotes.
static QString truncateValue(QString v)
{
    enum { maxLength = 512 };
    if (v.size() < maxLength)
        return v;
    const bool isQuoted = v.endsWith('"'); // check for 'char* "Hallo"'
    v.truncate(maxLength);
    v += QLatin1String(isQuoted ? "...\"" : "...");
    return v;
}

static QString displayName(const WatchItem *item)
{
    QString result;

    const WatchItem *p = item->parentItem();
    if (!p)
        return result;
    if (item->arrayIndex >= 0) {
        result = QString::fromLatin1("[%1]").arg(item->arrayIndex);
        return result;
    }
    if (item->iname.startsWith("return") && item->name.startsWith('$'))
        result = WatchModel::tr("returned value");
    else if (item->name == "*")
        result = '*' + p->name;
    else
        result = watchModel(item)->removeNamespaces(item->name);

    // Simplify names that refer to base classes.
    if (result.startsWith('[')) {
        result = simplifyType(result);
        if (result.size() > 30)
            result = result.left(27) + "...]";
    }

    return result;
}

static QString displayValue(const WatchItem *item)
{
    QString result = watchModel(item)->removeNamespaces(truncateValue(formattedValue(item)));
    if (result.isEmpty() && item->address)
        result += QString::fromLatin1("@0x" + QByteArray::number(item->address, 16));
//    if (origaddr)
//        result += QString::fromLatin1(" (0x" + QByteArray::number(origaddr, 16) + ')');
    return result;
}

static QString displayType(const WatchItem *item)
{
    QString result = niceTypeHelper(item->type);
    if (item->bitsize)
        result += QString::fromLatin1(":%1").arg(item->bitsize);
    result.remove('\'');
    result = watchModel(item)->removeNamespaces(result);
    return result;
}

static QColor valueColor(const WatchItem *item, int column)
{
    Theme::Color color = Theme::Debugger_WatchItem_ValueNormal;
    if (const WatchModel *model = watchModel(item)) {
        if (!model->m_contentsValid && !item->isInspect()) {
            color = Theme::Debugger_WatchItem_ValueInvalid;
        } else if (column == 1) {
            if (!item->valueEnabled)
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (!model->m_contentsValid && !item->isInspect())
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (column == 1 && item->value.isEmpty()) // This might still show 0x...
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (column == 1 && item->value != model->m_valueCache.value(item->iname))
                color = Theme::Debugger_WatchItem_ValueChanged;
        }
    }
    return creatorTheme()->color(color);
}

static DisplayFormats typeFormatList(const WatchItem *item)
{
    DisplayFormats formats;

    // Types supported by dumpers:
    // Hack: Compensate for namespaces.
    QString t = stripForFormat(item->type);
    int pos = t.indexOf("::Q");
    if (pos >= 0 && t.count(':') == 2)
        t.remove(0, pos + 2);
    pos = t.indexOf('<');
    if (pos >= 0)
        t.truncate(pos);
    t.replace(':', '_');
    formats << watchModel(item)->m_reportedTypeFormats.value(t);

    if (t.contains(']'))
        formats.append(ArrayPlotFormat);

    // Fixed artificial string and pointer types.
    if (item->origaddr || isPointerType(item->type)) {
        formats.append(RawFormat);
        formats.append(Latin1StringFormat);
        formats.append(SeparateLatin1StringFormat);
        formats.append(Utf8StringFormat);
        formats.append(SeparateUtf8StringFormat);
        formats.append(Local8BitStringFormat);
        formats.append(Utf16StringFormat);
        formats.append(Ucs4StringFormat);
        formats.append(Array10Format);
        formats.append(Array100Format);
        formats.append(Array1000Format);
        formats.append(Array10000Format);
    } else if (item->type.contains("char[") || item->type.contains("char [")) {
        formats.append(RawFormat);
        formats.append(Latin1StringFormat);
        formats.append(SeparateLatin1StringFormat);
        formats.append(Utf8StringFormat);
        formats.append(SeparateUtf8StringFormat);
        formats.append(Local8BitStringFormat);
        formats.append(Utf16StringFormat);
        formats.append(Ucs4StringFormat);
    }

    // Fixed artificial floating point types.
    bool ok = false;
    item->value.toDouble(&ok);
    if (ok) {
        formats.append(CompactFloatFormat);
        formats.append(ScientificFloatFormat);
    }

    // Fixed artificial integral types.
    QString v = item->value;
    if (v.startsWith('-'))
        v = v.mid(1);
    v.toULongLong(&ok, 10);
    if (!ok)
        v.toULongLong(&ok, 16);
    if (!ok)
        v.toULongLong(&ok, 8);
    if (ok) {
        formats.append(DecimalIntegerFormat);
        formats.append(HexadecimalIntegerFormat);
        formats.append(BinaryIntegerFormat);
        formats.append(OctalIntegerFormat);
    }

    return formats;
}

QVariant WatchModel::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ExtraIndicesForColumnWidth) {
        QModelIndexList l;
        foreach (TreeItem *item, m_watchRoot->children())
            l.append(indexForItem(item));
        foreach (TreeItem *item, m_returnRoot->children())
            l.append(indexForItem(item));
        return QVariant::fromValue(l);
    }
    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return QVariant();

    const int column = idx.column();
    switch (role) {
        case LocalsEditTypeRole:
            return item->editType();

        case LocalsNameRole:
            return item->name;

        case LocalsIntegerBaseRole:
            if (isPointerType(item->type)) // Pointers using 0x-convention
                return QVariant(16);
            return formatToIntegerBase(itemFormat(item));

        case Qt::EditRole: {
            switch (column) {
                case 0:
                    return item->expression();
                case 1:
                    return item->editValue();
                case 2:
                    return item->type;
            }
        }

        case Qt::DisplayRole: {
            switch (column) {
                case 0:
                    return displayName(item);
                case 1:
                    return displayValue(item);
                case 2:
                    return displayType(item);
            }
        }

        case Qt::ToolTipRole:
            return boolSetting(UseToolTipsInLocalsView)
                ? item->toToolTip() : QVariant();

        case Qt::ForegroundRole:
            return valueColor(item, column);

        case LocalsExpressionRole:
            return item->expression();

        case LocalsRawExpressionRole:
            return item->exp;

        case LocalsINameRole:
            return item->iname;

        case LocalsExpandedRole:
            return m_expandedINames.contains(item->iname);

        case LocalsTypeFormatListRole:
            return QVariant::fromValue(typeFormatList(item));

        case LocalsTypeRole:
            return removeNamespaces(displayType(item));

        case LocalsRawTypeRole:
            return item->type;

        case LocalsTypeFormatRole:
            return theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);

        case LocalsIndividualFormatRole:
            return theIndividualFormats.value(item->iname, AutomaticFormat);

        case LocalsRawValueRole:
            return item->value;

        case LocalsObjectAddressRole:
            return item->address;

        case LocalsPointerAddressRole:
            return item->origaddr;

        case LocalsIsWatchpointAtObjectAddressRole: {
            BreakpointParameters bp(WatchpointAtAddress);
            bp.address = item->address;
            return m_engine->breakHandler()->findWatchpoint(bp) != 0;
        }

        case LocalsSizeRole:
            return item->size;

        case LocalsIsWatchpointAtPointerAddressRole:
            if (isPointerType(item->type)) {
                BreakpointParameters bp(WatchpointAtAddress);
                bp.address = pointerValue(item->value);
                return m_engine->breakHandler()->findWatchpoint(bp) != 0;
            }
            return false;

        default:
            break;
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (!idx.isValid())
        return false; // Triggered by ModelTester.

    WatchItem *item = itemForIndex(idx);
    QTC_ASSERT(item, return false);

    switch (role) {
        case Qt::EditRole:
            switch (idx.column()) {
            case 0: {
                m_handler->updateWatchExpression(item, value.toString().trimmed());
                break;
            }
            case 1: // Change value
                m_engine->assignValueInDebugger(item, item->expression(), value);
                break;
            case 2: // TODO: Implement change type.
                m_engine->assignValueInDebugger(item, item->expression(), value);
                break;
            }
            break;

        case LocalsExpandedRole:
            if (value.toBool()) {
                // Should already have been triggered by fetchMore()
                //QTC_CHECK(m_expandedINames.contains(item->iname));
                m_expandedINames.insert(item->iname);
            } else {
                m_expandedINames.remove(item->iname);
            }
            if (item->iname.contains('.'))
                emit columnAdjustmentRequested();
            break;

        case LocalsTypeFormatRole:
            setTypeFormat(item->type, value.toInt());
            m_engine->updateLocals();
            break;

        case LocalsIndividualFormatRole: {
            setIndividualFormat(item->iname, value.toInt());
            m_engine->updateLocals();
            break;
        }
    }

    //emit dataChanged(idx, idx);
    return true;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 0;

    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return Qt::ItemIsEnabled|Qt::ItemIsSelectable;

    const int column = idx.column();

    QTC_ASSERT(m_engine, return Qt::ItemFlags());
    const DebuggerState state = m_engine->state();

    // Enabled, editable, selectable, checkable, and can be used both as the
    // source of a drag and drop operation and as a drop target.

    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    const Qt::ItemFlags editable = notEditable | Qt::ItemIsEditable;
    bool isRunning = true;
    switch (state) {
    case InferiorStopOk:
    case InferiorUnrunnable:
    case DebuggerNotReady:
    case DebuggerFinished:
        isRunning = false;
        break;
    default:
        break;
    }

    if (item->isWatcher()) {
        if (state == InferiorUnrunnable)
            return (column == 0 && item->iname.count('.') == 1) ? editable : notEditable;

        if (isRunning && !m_engine->hasCapability(AddWatcherWhileRunningCapability))
            return notEditable;
        if (column == 0 && item->iname.count('.') == 1)
            return editable; // Watcher names are editable.
        if (column == 1 && item->arrayIndex >= 0)
            return editable;

        if (!item->name.isEmpty()) {
            // FIXME: Forcing types is not implemented yet.
            //if (idx.column() == 2)
            //    return editable; // Watcher types can be set by force.
            if (column == 1 && item->valueEditable && !item->elided)
                return editable; // Watcher values are sometimes editable.
        }
    } else if (item->isLocal()) {
        if (state == InferiorUnrunnable)
            return notEditable;
        if (isRunning && !m_engine->hasCapability(AddWatcherWhileRunningCapability))
            return notEditable;
        if (column == 1 && item->valueEditable && !item->elided)
            return editable; // Locals values are sometimes editable.
        if (column == 1 && item->arrayIndex >= 0)
            return editable;
    } else if (item->isInspect()) {
        if (column == 1 && item->valueEditable)
            return editable; // Inspector values are sometimes editable.
    }
    return notEditable;
}

bool WatchModel::canFetchMore(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;

    // See "hasChildren" below.
    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return false;
    if (!item->wantsChildren)
        return false;
    if (!m_contentsValid && !item->isInspect())
        return false;
    return true;
}

void WatchModel::fetchMore(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;

    WatchItem *item = nonRootItemForIndex(idx);
    if (item) {
        m_expandedINames.insert(item->iname);
        if (item->children().isEmpty()) {
            item->setChildrenNeeded();
            m_engine->expandItem(item->iname);
        }
    }
}

bool WatchModel::hasChildren(const QModelIndex &idx) const
{
    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return true;
    if (item->childCount() > 0)
        return true;

    // "Can fetch more", see above.
    if (!item->wantsChildren)
        return false;
    if (!m_contentsValid && !item->isInspect())
        return false;
    return true;
}

static inline QString msgArrayFormat(int n)
{
    return WatchModel::tr("Array of %n items", 0, n);
}

QString WatchModel::nameForFormat(int format)
{
    switch (format) {
        case AutomaticFormat: return tr("Automatic");

        case RawFormat: return tr("Raw Data");
        case SimpleFormat: return tr("Normal");
        case EnhancedFormat: return tr("Enhanced");
        case SeparateFormat: return tr("Separate Window");

        case Latin1StringFormat: return tr("Latin1 String");
        case SeparateLatin1StringFormat: return tr("Latin1 String in Separate Window");
        case Utf8StringFormat: return tr("UTF-8 String");
        case SeparateUtf8StringFormat: return tr("UTF-8 String in Separate Window");
        case Local8BitStringFormat: return tr("Local 8-Bit String");
        case Utf16StringFormat: return tr("UTF-16 String");
        case Ucs4StringFormat: return tr("UCS-4 String");

        case Array10Format: return msgArrayFormat(10);
        case Array100Format: return msgArrayFormat(100);
        case Array1000Format: return msgArrayFormat(1000);
        case Array10000Format: return msgArrayFormat(10000);
        case ArrayPlotFormat: return tr("Plot in Separate Window");

        case CompactMapFormat: return tr("Display Keys and Values Side by Side");
        case DirectQListStorageFormat: return tr("Force Display as Direct Storage Form");
        case IndirectQListStorageFormat: return tr("Force Display as Indirect Storage Form");

        case BoolTextFormat: return tr("Display Boolean Values as True or False");
        case BoolIntegerFormat: return tr("Display Boolean Values as 1 or 0");

        case DecimalIntegerFormat: return tr("Decimal Integer");
        case HexadecimalIntegerFormat: return tr("Hexadecimal Integer");
        case BinaryIntegerFormat: return tr("Binary Integer");
        case OctalIntegerFormat: return tr("Octal Integer");

        case CompactFloatFormat: return tr("Compact Float");
        case ScientificFloatFormat: return tr("Scientific Float");
    }

    QTC_CHECK(false);
    return QString();
}

///////////////////////////////////////////////////////////////////////
//
// WatchHandler
//
///////////////////////////////////////////////////////////////////////

WatchHandler::WatchHandler(DebuggerEngine *engine)
{
    m_model = new WatchModel(this, engine);
}

WatchHandler::~WatchHandler()
{
    // Do it manually to prevent calling back in model destructors
    // after m_cache is destroyed.
    delete m_model;
    m_model = 0;
}

void WatchHandler::cleanup()
{
    m_model->m_expandedINames.clear();
    theWatcherNames.remove(QString());
    saveWatchers();
    m_model->reinitialize();
    emit m_model->updateFinished();
    if (Internal::mainWindow())
        m_model->m_separatedView->hide();
}

static bool sortByName(const WatchItem *a, const WatchItem *b)
{
    return a->name < b->name;
}

void WatchHandler::insertItems(const GdbMi &data)
{
    QSet<WatchItem *> itemsToSort;

    const bool sortStructMembers = boolSetting(SortStructMembers);
    foreach (const GdbMi &child, data.children()) {
        auto item = new WatchItem;
        item->parse(child, sortStructMembers);
        const TypeInfo ti = m_model->m_reportedTypeInfo.value(item->type);
        if (ti.size && !item->size)
            item->size = ti.size;

        const bool added = insertItem(item);
        if (added && item->level() == 2)
            itemsToSort.insert(static_cast<WatchItem *>(item->parent()));
    }

    foreach (WatchItem *toplevel, itemsToSort)
        toplevel->sortChildren(&sortByName);
}

bool WatchHandler::insertItem(WatchItem *item)
{
    QTC_ASSERT(!item->iname.isEmpty(), return false);

    WatchItem *parent = m_model->findItem(parentName(item->iname));
    QTC_ASSERT(parent, return false);

    bool found = false;
    const QVector<TreeItem *> siblings = parent->children();
    for (int row = 0, n = siblings.size(); row < n; ++row) {
        if (static_cast<WatchItem *>(siblings.at(row))->iname == item->iname) {
            m_model->destroyItem(parent->children().at(row));
            parent->insertChild(row, item);
            found = true;
            break;
        }
    }
    if (!found)
        parent->appendChild(item);

    item->update();

    m_model->showEditValue(item);
    item->forAllChildren([this](WatchItem *sub) { m_model->showEditValue(sub); });

    return !found;
}

void WatchModel::reexpandItems()
{
    foreach (const QString &iname, m_expandedINames) {
        if (WatchItem *item = findItem(iname)) {
            emit itemIsExpanded(indexForItem(item));
            emit inameIsExpanded(iname);
        } else {
            // Can happen. We might have stepped into another frame
            // not containing that iname, but we still like to
            // remember the expanded state of iname in case we step
            // out of the frame again.
        }
    }
}

void WatchHandler::removeAllData(bool includeInspectData)
{
    m_model->reinitialize(includeInspectData);
}

/*!
  If a displayed item differs from the cached entry it is considered
  "new", and correspondingly marked in red. Calling \c resetValueCache()
  stores the currently displayed items in the cache, effectively
  marking the value as known, and consequently painted black.
 */
void WatchHandler::resetValueCache()
{
    m_model->m_valueCache.clear();
    m_model->forAllItems([this](WatchItem *item) {
        m_model->m_valueCache[item->iname] = item->value;
    });
}

void WatchHandler::resetWatchers()
{
    loadSessionData();
}

void WatchHandler::notifyUpdateStarted(const QStringList &inames)
{
    auto marker = [](WatchItem *item) { item->outdated = true; };

    if (inames.isEmpty()) {
        m_model->forFirstLevelItems([marker](WatchItem *item) {
            item->forAllChildren(marker);
        });
    } else {
        for (auto iname : inames) {
            if (WatchItem *item = m_model->findItem(iname))
                item->forAllChildren(marker);
        }
    }

    m_model->m_requestUpdateTimer.start(80);
    m_model->m_contentsValid = false;
    updateWatchersWindow();
}

void WatchHandler::notifyUpdateFinished()
{
    QList<WatchItem *> toRemove;
    m_model->forSelectedItems([this, &toRemove](WatchItem *item) {
        if (item->outdated) {
            toRemove.append(item);
            return false;
        }
        return true;
    });

    foreach (auto item, toRemove)
        m_model->destroyItem(item);

    m_model->forAllItems([this](WatchItem *item) {
        if (item->wantsChildren && isExpandedIName(item->iname)) {
            m_model->m_engine->showMessage(QString("ADJUSTING CHILD EXPECTATION FOR " + item->iname));
            item->wantsChildren = false;
        }
    });

    m_model->m_contentsValid = true;
    updateWatchersWindow();
    m_model->reexpandItems();
    m_model->m_requestUpdateTimer.stop();
    emit m_model->updateFinished();
}

void WatchHandler::reexpandItems()
{
    m_model->reexpandItems();
}

void WatchHandler::removeItemByIName(const QString &iname)
{
    WatchItem *item = m_model->findItem(iname);
    if (!item)
        return;
    if (item->isWatcher()) {
        theWatcherNames.remove(item->exp);
        saveWatchers();
    }
    m_model->destroyItem(item);
    updateWatchersWindow();
}

QString WatchHandler::watcherName(const QString &exp)
{
    return "watch." + QString::number(theWatcherNames[exp]);
}

void WatchHandler::watchExpression(const QString &exp, const QString &name)
{
    // Do not insert the same entry more then once.
    if (exp.isEmpty() || theWatcherNames.contains(exp))
        return;

    theWatcherNames[exp] = theWatcherCount++;

    auto item = new WatchItem;
    item->exp = exp;
    item->name = name.isEmpty() ? exp : name;
    item->iname = watcherName(exp);
    insertItem(item);
    saveWatchers();

    if (m_model->m_engine->state() == DebuggerNotReady) {
        item->setAllUnneeded();
        item->setValue(QString(QLatin1Char(' ')));
        item->update();
    } else {
        m_model->m_engine->updateWatchData(item->iname);
    }
    updateWatchersWindow();
}

void WatchHandler::updateWatchExpression(WatchItem *item, const QString &newExp)
{
    if (newExp.isEmpty())
        return;

    if (item->exp != newExp) {
        theWatcherNames.insert(newExp, theWatcherNames.value(item->exp));
        theWatcherNames.remove(item->exp);
        item->exp = newExp;
        item->name = newExp;
    }

    saveWatchers();
    if (m_model->m_engine->state() == DebuggerNotReady) {
        item->setAllUnneeded();
        item->setValue(QString(QLatin1Char(' ')));
        item->update();
    } else {
        m_model->m_engine->updateItem(item->iname);
    }
    updateWatchersWindow();
}

// Watch something obtained from the editor.
// Prefer to watch an existing local variable by its expression
// (address) if it can be found. Default to watchExpression().
void WatchHandler::watchVariable(const QString &exp)
{
    if (const WatchItem *localVariable = findCppLocalVariable(exp))
        watchExpression(localVariable->exp, exp);
    else
        watchExpression(exp);
}

static void swapEndian(char *d, int nchar)
{
    QTC_ASSERT(nchar % 4 == 0, return);
    for (int i = 0; i < nchar; i += 4) {
        char c = d[i];
        d[i] = d[i + 3];
        d[i + 3] = c;
        c = d[i + 1];
        d[i + 1] = d[i + 2];
        d[i + 2] = c;
    }
}

void WatchModel::showEditValue(const WatchItem *item)
{
    const QString &format = item->editformat;
    if (format.isEmpty()) {
        // Nothing
        m_separatedView->removeObject(item->key());
    } else if (format == DisplayImageData || format == DisplayImageFile) {
        // QImage
        int width = 0, height = 0, nbytes = 0, imformat = 0;
        QByteArray ba;
        uchar *bits = 0;
        if (format == DisplayImageData) {
            ba = QByteArray::fromHex(item->editvalue.toUtf8());
            QTC_ASSERT(ba.size() > 16, return);
            const int *header = (int *)(ba.data());
            if (!ba.at(0) && !ba.at(1)) // Check on 'width' for Python dumpers returning 4-byte swapped-data.
                swapEndian(ba.data(), 16);
            bits = 16 + (uchar *)(ba.data());
            width = header[0];
            height = header[1];
            nbytes = header[2];
            imformat = header[3];
        } else if (format == DisplayImageFile) {
            QTextStream ts(item->editvalue.toUtf8());
            QString fileName;
            ts >> width >> height >> nbytes >> imformat >> fileName;
            QFile f(fileName);
            f.open(QIODevice::ReadOnly);
            ba = f.readAll();
            bits = (uchar*)ba.data();
            nbytes = width * height;
        }
        QTC_ASSERT(0 < width && width < 10000, return);
        QTC_ASSERT(0 < height && height < 10000, return);
        QTC_ASSERT(0 < nbytes && nbytes < 10000 * 10000, return);
        QTC_ASSERT(0 < imformat && imformat < 32, return);
        QImage im(width, height, QImage::Format(imformat));
        std::memcpy(im.bits(), bits, nbytes);
        ImageViewer *v = m_separatedView->prepareObject<ImageViewer>(item);
        v->setInfo(item->address ?
            tr("%1 Object at %2").arg(item->type, item->hexAddress()) :
            tr("%1 Object at Unknown Address").arg(item->type) + "    " +
            ImageViewer::tr("Size: %1x%2, %3 byte, format: %4, depth: %5")
                .arg(width).arg(height).arg(nbytes).arg(im.format()).arg(im.depth())
        );
        v->setImage(im);
    } else if (format == DisplayLatin1String
            || format == DisplayUtf8String
            || format == DisplayUtf16String
            || format == DisplayUcs4String) {
         // String data.
        QByteArray ba = QByteArray::fromHex(item->editvalue.toUtf8());
        QString str;
        if (format == DisplayLatin1String)
            str = QString::fromLatin1(ba.constData(), ba.size());
        else if (format == DisplayUtf8String)
            str = QString::fromUtf8(ba.constData(), ba.size());
        else if (format == DisplayUtf16String)
            str = QString::fromUtf16((ushort *)ba.constData(), ba.size() / 2);
        else if (format == DisplayUtf16String)
            str = QString::fromUcs4((uint *)ba.constData(), ba.size() / 4);
        m_separatedView->prepareObject<QTextEdit>(item)->setPlainText(str);
    } else if (format == DisplayPlotData) {
        // Plots
        std::vector<double> data;
        readNumericVector(&data, QByteArray::fromHex(item->editvalue.toUtf8()), item->editencoding);
        m_separatedView->prepareObject<PlotViewer>(item)->setData(data);
    } else {
        QTC_ASSERT(false, qDebug() << "Display format: " << format);
    }
}

void WatchHandler::clearWatches()
{
    if (theWatcherNames.isEmpty())
        return;

    const QDialogButtonBox::StandardButton ret = CheckableMessageBox::doNotAskAgainQuestion(
                Core::ICore::mainWindow(), tr("Remove All Expression Evaluators"),
                tr("Are you sure you want to remove all expression evaluators?"),
                Core::ICore::settings(), "RemoveAllWatchers");
    if (ret != QDialogButtonBox::Yes)
        return;

    m_model->m_watchRoot->removeChildren();
    theWatcherNames.clear();
    theWatcherCount = 0;
    updateWatchersWindow();
    saveWatchers();
}

void WatchHandler::updateWatchersWindow()
{
    emit m_model->columnAdjustmentRequested();

    // Force show/hide of watchers and return view.
    int showWatch = !theWatcherNames.isEmpty();
    int showReturn = m_model->m_returnRoot->childCount() != 0;
    Internal::updateWatchersWindow(showWatch, showReturn);
}

QStringList WatchHandler::watchedExpressions()
{
    // Filter out invalid watchers.
    QStringList watcherNames;
    QHashIterator<QString, int> it(theWatcherNames);
    while (it.hasNext()) {
        it.next();
        const QString &watcherName = it.key();
        if (!watcherName.isEmpty())
            watcherNames.push_back(watcherName);
    }
    return watcherNames;
}

void WatchHandler::saveSessionData()
{
    saveWatchers();
    saveFormats();
}

void WatchHandler::loadSessionData()
{
    loadFormats();
    theWatcherNames.clear();
    theWatcherCount = 0;
    QVariant value = sessionValue("Watchers");
    m_model->m_watchRoot->removeChildren();
    foreach (const QString &exp, value.toStringList())
        watchExpression(exp.trimmed());
}

WatchModelBase *WatchHandler::model() const
{
    return m_model;
}

const WatchItem *WatchHandler::watchItem(const QModelIndex &idx) const
{
    return m_model->itemForIndex(idx);
}

void WatchHandler::fetchMore(const QString &iname) const
{
    if (WatchItem *item = m_model->findItem(iname)) {
        m_model->m_expandedINames.insert(iname);
        if (item->children().isEmpty()) {
            item->setChildrenNeeded();
            m_model->m_engine->expandItem(iname);
        }
    }
}

WatchItem *WatchHandler::findItem(const QString &iname) const
{
    return m_model->findItem(iname);
}

const WatchItem *WatchHandler::findCppLocalVariable(const QString &name) const
{
    // Can this be found as a local variable?
    const QString localsPrefix("local.");
    QString iname = localsPrefix + name;
    if (const WatchItem *item = findItem(iname))
        return item;
//    // Nope, try a 'local.this.m_foo'.
//    iname.insert(localsPrefix.size(), "this.");
//    if (const WatchData *wd = findData(iname))
//        return wd;
    return 0;
}

void WatchModel::setTypeFormat(const QString &type0, int format)
{
    const QString type = stripForFormat(type0);
    if (format == AutomaticFormat)
        theTypeFormats.remove(type);
    else
        theTypeFormats[type] = format;
    saveFormats();
    m_engine->updateAll();
}

void WatchModel::setIndividualFormat(const QString &iname, int format)
{
    if (format == AutomaticFormat)
        theIndividualFormats.remove(iname);
    else
        theIndividualFormats[iname] = format;
    saveFormats();
}

int WatchHandler::format(const QString &iname) const
{
    int result = AutomaticFormat;
    if (const WatchItem *item = m_model->findItem(iname)) {
        result = theIndividualFormats.value(item->iname, AutomaticFormat);
        if (result == AutomaticFormat)
            result = theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);
    }
    return result;
}

QString WatchHandler::nameForFormat(int format)
{
    return WatchModel::nameForFormat(format);
}

static QString formatStringFromFormatCode(int code)
{
    switch (code) {
    // Taken from debuggerprotocol.h, DisplayFormat.
    case Latin1StringFormat:
        return QLatin1String("latin");
    case SeparateLatin1StringFormat:
        return QLatin1String("latin:separate");
    case Utf8StringFormat:
        return QLatin1String("utf8");
    case SeparateUtf8StringFormat:
        return QLatin1String("utf8:separate");
    case Utf16StringFormat:
        return QLatin1String("utf16");
    }
    return QString();
}

QString WatchHandler::typeFormatRequests() const
{
    QString ba;
    if (!theTypeFormats.isEmpty()) {
        QHashIterator<QString, int> it(theTypeFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format != AutomaticFormat) {
                ba.append(toHex(it.key()));
                ba.append('=');
                ba.append(formatStringFromFormatCode(format));
                ba.append(',');
            }
        }
        ba.chop(1);
    }
    return ba;
}

QString WatchHandler::individualFormatRequests() const
{
    QString res;
    if (!theIndividualFormats.isEmpty()) {
        QHashIterator<QString, int> it(theIndividualFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format != AutomaticFormat) {
                res.append(it.key());
                res.append('=');
                res.append(formatStringFromFormatCode(it.value()));
                res.append(',');
            }
        }
        res.chop(1);
    }
    return res;
}

void WatchHandler::appendFormatRequests(DebuggerCommand *cmd)
{
    QJsonArray expanded;
    QSetIterator<QString> jt(m_model->m_expandedINames);
    while (jt.hasNext())
        expanded.append(jt.next());

    cmd->arg("expanded", expanded);

    QJsonObject typeformats;
    QHashIterator<QString, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != AutomaticFormat)
            typeformats.insert(it.key(), format);
    }
    cmd->arg("typeformats", typeformats);

    QJsonObject formats;
    QHashIterator<QString, int> it2(theIndividualFormats);
    while (it2.hasNext()) {
        it2.next();
        const int format = it2.value();
        if (format != AutomaticFormat)
            formats.insert(it2.key(), format);
    }
    cmd->arg("formats", formats);
}

static inline QJsonObject watcher(const QString &iname, const QString &exp)
{
    QJsonObject watcher;
    watcher.insert("iname", iname);
    watcher.insert("exp", toHex(exp));
    return watcher;
}

void WatchHandler::appendWatchersAndTooltipRequests(DebuggerCommand *cmd)
{
    QJsonArray watchers;
    DebuggerToolTipContexts toolTips = DebuggerToolTipManager::pendingTooltips(m_model->m_engine);
    foreach (const DebuggerToolTipContext &p, toolTips)
        watchers.append(watcher(p.iname, p.expression));

    QHashIterator<QString, int> it(WatchHandler::watcherNames());
    while (it.hasNext()) {
        it.next();
        watchers.append(watcher("watch." + QString::number(it.value()), it.key()));
    }
    cmd->arg("watchers", watchers);
}

void WatchHandler::addDumpers(const GdbMi &dumpers)
{
    foreach (const GdbMi &dumper, dumpers.children()) {
        DisplayFormats formats;
        formats.append(RawFormat);
        QString reportedFormats = dumper["formats"].data();
        foreach (const QString &format, reportedFormats.split(',')) {
            if (int f = format.toInt())
                formats.append(DisplayFormat(f));
        }
        addTypeFormats(dumper["type"].data(), formats);
    }
}

void WatchHandler::addTypeFormats(const QString &type, const DisplayFormats &formats)
{
    m_model->m_reportedTypeFormats.insert(stripForFormat(type), formats);
}

QString WatchHandler::editorContents(const QModelIndexList &list)
{
    QString contents;
    QTextStream ts(&contents);
    m_model->forAllItems([&ts, this, list](WatchItem *item) {
        if (list.isEmpty() || list.contains(m_model->indexForItem(item))) {
            const QChar tab = QLatin1Char('\t');
            const QChar nl = '\n';
            ts << QString(item->level(), tab) << item->name << tab << displayValue(item) << tab
               << item->type << nl;
        }
    });
    return contents;
}

void WatchHandler::scheduleResetLocation()
{
    m_model->m_contentsValid = false;
    m_model->m_resetLocationScheduled = true;
}

void WatchHandler::resetLocation()
{
    m_model->m_resetLocationScheduled = false;
}

void WatchHandler::setCurrentItem(const QString &iname)
{
    if (WatchItem *item = m_model->findItem(iname)) {
        QModelIndex idx = m_model->indexForItem(item);
        emit m_model->currentIndexRequested(idx);
    }
}

QHash<QString, int> WatchHandler::watcherNames()
{
    return theWatcherNames;
}

void WatchHandler::setUnprintableBase(int base)
{
    theUnprintableBase = base;
    m_model->layoutChanged();
}

int WatchHandler::unprintableBase()
{
    return theUnprintableBase;
}

bool WatchHandler::isExpandedIName(const QString &iname) const
{
    return m_model->m_expandedINames.contains(iname);
}

QSet<QString> WatchHandler::expandedINames() const
{
    return m_model->m_expandedINames;
}

void WatchHandler::recordTypeInfo(const GdbMi &typeInfo)
{
    if (typeInfo.type() == GdbMi::List) {
        foreach (const GdbMi &s, typeInfo.children()) {
            QString typeName = fromHex(s["name"].data());
            TypeInfo ti(s["size"].data().toUInt());
            m_model->m_reportedTypeInfo.insert(typeName, ti);
        }
    }
}

} // namespace Internal
} // namespace Debugger
