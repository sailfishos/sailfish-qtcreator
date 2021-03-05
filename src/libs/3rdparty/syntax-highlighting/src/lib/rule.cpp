/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "context_p.h"
#include "definition_p.h"
#include "ksyntaxhighlighting_logging.h"
#include "rule_p.h"
#include "xml_p.h"
#include "worddelimiters_p.h"

#include <QString>
#include <QXmlStreamReader>

using namespace KSyntaxHighlighting;

// QChar::isDigit() match any digit in unicode (romain numeral, etc)
static bool isDigit(QChar c)
{
    return (c <= QLatin1Char('9') && QLatin1Char('0') <= c);
}

static bool isOctalChar(QChar c)
{
    return (c <= QLatin1Char('7') && QLatin1Char('0') <= c);
}

static bool isHexChar(QChar c)
{
    return isDigit(c)
        || (c <= QLatin1Char('f') && QLatin1Char('a') <= c)
        || (c <= QLatin1Char('F') && QLatin1Char('A') <= c);
}

static int matchEscapedChar(const QString &text, int offset)
{
    if (text.at(offset) != QLatin1Char('\\') || text.size() < offset + 2)
        return offset;

    const auto c = text.at(offset + 1);
    switch (c.unicode()) {
    // control chars
    case 'a': case 'b': case 'e': case 'f':
    case 'n': case 'r': case 't': case 'v':
    case '"': case '\'': case '?': case '\\':
        return offset + 2;

    // hex encoded character
    case 'x':
        if (offset + 2 < text.size() && isHexChar(text.at(offset + 2))) {
            if (offset + 3 < text.size() && isHexChar(text.at(offset + 3)))
                return offset + 4;
            return offset + 3;
        }
        return offset;

    // octal encoding, simple \0 is OK, too, unlike simple \x above
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
        if (offset + 2 < text.size() && isOctalChar(text.at(offset + 2))) {
            if (offset + 3 < text.size() && isOctalChar(text.at(offset + 3)))
                return offset + 4;
            return offset + 3;
        }
        return offset + 2;
    }

    return offset;
}

static QString replaceCaptures(const QString &pattern, const QStringList &captures, bool quote)
{
    auto result = pattern;
    for (int i = captures.size() - 1; i >= 1; --i) {
        result.replace(QLatin1Char('%') + QString::number(i), quote ? QRegularExpression::escape(captures.at(i)) : captures.at(i));
    }
    return result;
}

Definition Rule::definition() const
{
    return m_def.definition();
}

void Rule::setDefinition(const Definition &def)
{
    m_def = def;
}

bool Rule::load(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    m_attribute = reader.attributes().value(QLatin1String("attribute")).toString();
    if (reader.name() != QLatin1String("IncludeRules")) // IncludeRules uses this with a different semantic
        m_context.parse(reader.attributes().value(QLatin1String("context")));
    m_firstNonSpace = Xml::attrToBool(reader.attributes().value(QLatin1String("firstNonSpace")));
    m_lookAhead = Xml::attrToBool(reader.attributes().value(QLatin1String("lookAhead")));
    bool colOk = false;
    m_column = reader.attributes().value(QLatin1String("column")).toInt(&colOk);
    if (!colOk)
        m_column = -1;

    auto regionName = reader.attributes().value(QLatin1String("beginRegion"));
    if (!regionName.isEmpty())
        m_beginRegion = FoldingRegion(FoldingRegion::Begin, DefinitionData::get(m_def.definition())->foldingRegionId(regionName.toString()));
    regionName = reader.attributes().value(QLatin1String("endRegion"));
    if (!regionName.isEmpty())
        m_endRegion = FoldingRegion(FoldingRegion::End, DefinitionData::get(m_def.definition())->foldingRegionId(regionName.toString()));

    auto result = doLoad(reader);

    if (m_lookAhead && m_context.isStay())
        result = false;

    // be done with this rule, skip all subelements, e.g. no longer supported sub-rules
    reader.skipCurrentElement();
    return result;
}

void Rule::resolveContext()
{
    auto const& def = m_def.definition();

    m_context.resolve(def);

    // cache for DefinitionData::wordDelimiters, is accessed VERY often
    m_wordDelimiters = &DefinitionData::get(def)->wordDelimiters;
}

void Rule::resolveAttributeFormat(Context *lookupContext)
{
    /**
     * try to get our format from the definition we stem from
     */
    if (!m_attribute.isEmpty()) {
        m_attributeFormat = DefinitionData::get(definition())->formatByName(m_attribute);
        if (!m_attributeFormat.isValid()) {
            qCWarning(Log) << "Rule: Unknown format" << m_attribute << "in context" << lookupContext->name() << "of definition" << definition().name();
        }
    }
}

bool Rule::doLoad(QXmlStreamReader &reader)
{
    Q_UNUSED(reader);
    return true;
}

Rule::Ptr Rule::create(const QStringView &name)
{
    if (name == QLatin1String("AnyChar"))
        return std::make_shared<AnyChar>();
    if (name == QLatin1String("DetectChar"))
        return std::make_shared<DetectChar>();
    if (name == QLatin1String("Detect2Chars"))
        return std::make_shared<Detect2Char>();
    if (name == QLatin1String("DetectIdentifier"))
        return std::make_shared<DetectIdentifier>();
    if (name == QLatin1String("DetectSpaces"))
        return std::make_shared<DetectSpaces>();
    if (name == QLatin1String("Float"))
        return std::make_shared<Float>();
    if (name == QLatin1String("Int"))
        return std::make_shared<Int>();
    if (name == QLatin1String("HlCChar"))
        return std::make_shared<HlCChar>();
    if (name == QLatin1String("HlCHex"))
        return std::make_shared<HlCHex>();
    if (name == QLatin1String("HlCOct"))
        return std::make_shared<HlCOct>();
    if (name == QLatin1String("HlCStringChar"))
        return std::make_shared<HlCStringChar>();
    if (name == QLatin1String("IncludeRules"))
        return std::make_shared<IncludeRules>();
    if (name == QLatin1String("keyword"))
        return std::make_shared<KeywordListRule>();
    if (name == QLatin1String("LineContinue"))
        return std::make_shared<LineContinue>();
    if (name == QLatin1String("RangeDetect"))
        return std::make_shared<RangeDetect>();
    if (name == QLatin1String("RegExpr"))
        return std::make_shared<RegExpr>();
    if (name == QLatin1String("StringDetect"))
        return std::make_shared<StringDetect>();
    if (name == QLatin1String("WordDetect"))
        return std::make_shared<WordDetect>();

    qCWarning(Log) << "Unknown rule type:" << name;
    return Ptr(nullptr);
}

bool Rule::isWordDelimiter(QChar c) const
{
    return m_wordDelimiters->contains(c);
}

bool AnyChar::doLoad(QXmlStreamReader &reader)
{
    m_chars = reader.attributes().value(QLatin1String("String")).toString();
    if (m_chars.size() == 1)
        qCDebug(Log) << "AnyChar rule with just one char: use DetectChar instead.";
    return !m_chars.isEmpty();
}

MatchResult AnyChar::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (m_chars.contains(text.at(offset)))
        return offset + 1;
    return offset;
}

bool DetectChar::doLoad(QXmlStreamReader &reader)
{
    const auto s = reader.attributes().value(QLatin1String("char"));
    if (s.isEmpty())
        return false;
    m_char = s.at(0);
    m_dynamic = Xml::attrToBool(reader.attributes().value(QLatin1String("dynamic")));
    if (m_dynamic) {
        m_captureIndex = m_char.digitValue();
    }
    return true;
}

MatchResult DetectChar::doMatch(const QString &text, int offset, const QStringList &captures) const
{
    if (m_dynamic) {
        if (m_captureIndex == 0 || captures.size() <= m_captureIndex || captures.at(m_captureIndex).isEmpty())
            return offset;
        if (text.at(offset) == captures.at(m_captureIndex).at(0))
            return offset + 1;
        return offset;
    }

    if (text.at(offset) == m_char)
        return offset + 1;
    return offset;
}

bool Detect2Char::doLoad(QXmlStreamReader &reader)
{
    const auto s1 = reader.attributes().value(QLatin1String("char"));
    const auto s2 = reader.attributes().value(QLatin1String("char1"));
    if (s1.isEmpty() || s2.isEmpty())
        return false;
    m_char1 = s1.at(0);
    m_char2 = s2.at(0);
    return true;
}

MatchResult Detect2Char::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (text.size() - offset < 2)
        return offset;
    if (text.at(offset) == m_char1 && text.at(offset + 1) == m_char2)
        return offset + 2;
    return offset;
}

MatchResult DetectIdentifier::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (!text.at(offset).isLetter() && text.at(offset) != QLatin1Char('_'))
        return offset;

    for (int i = offset + 1; i < text.size(); ++i) {
        const auto c = text.at(i);
        if (!c.isLetterOrNumber() && c != QLatin1Char('_'))
            return i;
    }

    return text.size();
}

MatchResult DetectSpaces::doMatch(const QString &text, int offset, const QStringList &) const
{
    while (offset < text.size() && text.at(offset).isSpace())
        ++offset;
    return offset;
}

MatchResult Float::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (offset > 0 && !isWordDelimiter(text.at(offset - 1)))
        return offset;

    auto newOffset = offset;
    while (newOffset < text.size() && isDigit(text.at(newOffset)))
        ++newOffset;

    if (newOffset >= text.size() || text.at(newOffset) != QLatin1Char('.'))
        return offset;
    ++newOffset;

    while (newOffset < text.size() && isDigit(text.at(newOffset)))
        ++newOffset;

    if (newOffset == offset + 1) // we only found a decimal point
        return offset;

    auto expOffset = newOffset;
    if (expOffset >= text.size() || (text.at(expOffset) != QLatin1Char('e') && text.at(expOffset) != QLatin1Char('E')))
        return newOffset;
    ++expOffset;

    if (expOffset < text.size() && (text.at(expOffset) == QLatin1Char('+') || text.at(expOffset) == QLatin1Char('-')))
        ++expOffset;
    bool foundExpDigit = false;
    while (expOffset < text.size() && isDigit(text.at(expOffset))) {
        ++expOffset;
        foundExpDigit = true;
    }

    if (!foundExpDigit)
        return newOffset;
    return expOffset;
}

MatchResult HlCChar::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (text.size() < offset + 3)
        return offset;

    if (text.at(offset) != QLatin1Char('\'') || text.at(offset + 1) == QLatin1Char('\''))
        return offset;

    auto newOffset = matchEscapedChar(text, offset + 1);
    if (newOffset == offset + 1) {
        if (text.at(newOffset) == QLatin1Char('\\'))
            return offset;
        else
            ++newOffset;
    }
    if (newOffset >= text.size())
        return offset;

    if (text.at(newOffset) == QLatin1Char('\''))
        return newOffset + 1;

    return offset;
}

MatchResult HlCHex::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (offset > 0 && !isWordDelimiter(text.at(offset - 1)))
        return offset;

    if (text.size() < offset + 3)
        return offset;

    if (text.at(offset) != QLatin1Char('0') || (text.at(offset + 1) != QLatin1Char('x') && text.at(offset + 1) != QLatin1Char('X')))
        return offset;

    if (!isHexChar(text.at(offset + 2)))
        return offset;

    offset += 3;
    while (offset < text.size() && isHexChar(text.at(offset)))
        ++offset;

    // TODO Kate matches U/L suffix, QtC does not?

    return offset;
}

MatchResult HlCOct::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (offset > 0 && !isWordDelimiter(text.at(offset - 1)))
        return offset;

    if (text.size() < offset + 2)
        return offset;

    if (text.at(offset) != QLatin1Char('0'))
        return offset;

    if (!isOctalChar(text.at(offset + 1)))
        return offset;

    offset += 2;
    while (offset < text.size() && isOctalChar(text.at(offset)))
        ++offset;

    return offset;
}

MatchResult HlCStringChar::doMatch(const QString &text, int offset, const QStringList &) const
{
    return matchEscapedChar(text, offset);
}

QString IncludeRules::contextName() const
{
    return m_contextName;
}

QString IncludeRules::definitionName() const
{
    return m_defName;
}

bool IncludeRules::includeAttribute() const
{
    return m_includeAttribute;
}

bool IncludeRules::doLoad(QXmlStreamReader &reader)
{
    const auto s = reader.attributes().value(QLatin1String("context"));
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const auto split = s.split(QLatin1String("##"), QString::KeepEmptyParts);
#else
    const auto split = s.split(QString::fromLatin1("##"), Qt::KeepEmptyParts);
#endif
    if (split.isEmpty())
        return false;
    m_contextName = split.at(0).toString();
    if (split.size() > 1)
        m_defName = split.at(1).toString();
    m_includeAttribute = Xml::attrToBool(reader.attributes().value(QLatin1String("includeAttrib")));

    return !m_contextName.isEmpty() || !m_defName.isEmpty();
}

MatchResult IncludeRules::doMatch(const QString &text, int offset, const QStringList &) const
{
    Q_UNUSED(text);
    qCWarning(Log) << "Unresolved include rule for" << m_contextName << "##" << m_defName;
    return offset;
}

MatchResult Int::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (offset > 0 && !isWordDelimiter(text.at(offset - 1)))
        return offset;

    while (offset < text.size() && isDigit(text.at(offset)))
        ++offset;
    return offset;
}

bool KeywordListRule::doLoad(QXmlStreamReader &reader)
{
    /**
     * get our keyword list, if not found => bail out
     */
    auto defData = DefinitionData::get(definition());
    m_keywordList = defData->keywordList(reader.attributes().value(QLatin1String("String")).toString());
    if (!m_keywordList) {
        return false;
    }

    /**
     * we might overwrite the case sensitivity
     * then we need to init the list for lookup of that sensitivity setting
     */
    if (reader.attributes().hasAttribute(QLatin1String("insensitive"))) {
        m_hasCaseSensitivityOverride = true;
        m_caseSensitivityOverride = Xml::attrToBool(reader.attributes().value(QLatin1String("insensitive"))) ? Qt::CaseInsensitive : Qt::CaseSensitive;
        m_keywordList->initLookupForCaseSensitivity(m_caseSensitivityOverride);
    } else {
        m_hasCaseSensitivityOverride = false;
    }

    return !m_keywordList->isEmpty();
}

MatchResult KeywordListRule::doMatch(const QString &text, int offset, const QStringList &) const
{
    auto newOffset = offset;
    while (text.size() > newOffset && !isWordDelimiter(text.at(newOffset)))
        ++newOffset;
    if (newOffset == offset)
        return offset;

    if (m_hasCaseSensitivityOverride) {
        if (m_keywordList->contains(QStringView(text).mid(offset, newOffset - offset),
                                    m_caseSensitivityOverride))
            return newOffset;
    } else {
        if (m_keywordList->contains(QStringView(text).mid(offset, newOffset - offset)))
            return newOffset;
    }

    // we don't match, but we can skip until newOffset as we can't start a keyword in-between
    return MatchResult(offset, newOffset);
}

bool LineContinue::doLoad(QXmlStreamReader &reader)
{
    const auto s = reader.attributes().value(QLatin1String("char"));
    if (s.isEmpty())
        m_char = QLatin1Char('\\');
    else
        m_char = s.at(0);
    return true;
}

MatchResult LineContinue::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (offset == text.size() - 1 && text.at(offset) == m_char)
        return offset + 1;
    return offset;
}

bool RangeDetect::doLoad(QXmlStreamReader &reader)
{
    const auto s1 = reader.attributes().value(QLatin1String("char"));
    const auto s2 = reader.attributes().value(QLatin1String("char1"));
    if (s1.isEmpty() || s2.isEmpty())
        return false;
    m_begin = s1.at(0);
    m_end = s2.at(0);
    return true;
}

MatchResult RangeDetect::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (text.size() - offset < 2)
        return offset;
    if (text.at(offset) != m_begin)
        return offset;

    auto newOffset = offset + 1;
    while (newOffset < text.size()) {
        if (text.at(newOffset) == m_end)
            return newOffset + 1;
        ++newOffset;
    }
    return offset;
}

bool RegExpr::doLoad(QXmlStreamReader &reader)
{
    m_regexp.setPattern(reader.attributes().value(QLatin1String("String")).toString());

    const auto isMinimal = Xml::attrToBool(reader.attributes().value(QLatin1String("minimal")));
    const auto isCaseInsensitive = Xml::attrToBool(reader.attributes().value(QLatin1String("insensitive")));
    m_regexp.setPatternOptions((isMinimal ? QRegularExpression::InvertedGreedinessOption : QRegularExpression::NoPatternOption) | (isCaseInsensitive ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption));

    // optimize the pattern for the non-dynamic case, we use them OFTEN
    m_dynamic = Xml::attrToBool(reader.attributes().value(QLatin1String("dynamic")));
    if (!m_dynamic) {
        m_regexp.optimize();
    }

    // always using m_regexp.isValid() would be better, but parses the regexp and thus is way too expensive for release builds

    if (Log().isDebugEnabled()) {
        if (!m_regexp.isValid())
            qCDebug(Log) << "Invalid regexp:" << m_regexp.pattern();
    }
    return !m_regexp.pattern().isEmpty();
}

MatchResult RegExpr::doMatch(const QString &text, int offset, const QStringList &captures) const
{
    /**
     * for dynamic case: create new pattern with right instantiation
     */
    const auto &regexp = m_dynamic ? QRegularExpression(replaceCaptures(m_regexp.pattern(), captures, true), m_regexp.patternOptions()) : m_regexp;

    /**
     * match the pattern
     */
    const auto result = regexp.match(text, offset, QRegularExpression::NormalMatch, QRegularExpression::DontCheckSubjectStringMatchOption);
    if (result.capturedStart() == offset) {
        /**
         * we only need to compute the captured texts if we have real capture groups
         * highlightings should only address %1..%.., see e.g. replaceCaptures
         * DetectChar ignores %0, too
         */
        if (result.lastCapturedIndex() > 0) {
            return MatchResult(offset + result.capturedLength(), result.capturedTexts());
        }

        /**
         * else: ignore the implicit 0 group we always capture, no need to allocate stuff for that
         */
        return MatchResult(offset + result.capturedLength());
    }

    /**
     * no match
     * we can always compute the skip offset as the highlighter will invalidate the cache for changed captures for dynamic rules!
     */
    return MatchResult(offset, result.capturedStart());
}

bool StringDetect::doLoad(QXmlStreamReader &reader)
{
    m_string = reader.attributes().value(QLatin1String("String")).toString();
    m_caseSensitivity = Xml::attrToBool(reader.attributes().value(QLatin1String("insensitive"))) ? Qt::CaseInsensitive : Qt::CaseSensitive;
    m_dynamic = Xml::attrToBool(reader.attributes().value(QLatin1String("dynamic")));
    return !m_string.isEmpty();
}

MatchResult StringDetect::doMatch(const QString &text, int offset, const QStringList &captures) const
{
    /**
     * for dynamic case: create new pattern with right instantiation
     */
    const auto &pattern = m_dynamic ? replaceCaptures(m_string, captures, false) : m_string;

    if (offset + pattern.size() <= text.size()
        && QStringView(text).mid(offset, pattern.size()).compare(pattern, m_caseSensitivity) == 0)
        return offset + pattern.size();
    return offset;
}

bool WordDetect::doLoad(QXmlStreamReader &reader)
{
    m_word = reader.attributes().value(QLatin1String("String")).toString();
    m_caseSensitivity = Xml::attrToBool(reader.attributes().value(QLatin1String("insensitive"))) ? Qt::CaseInsensitive : Qt::CaseSensitive;
    return !m_word.isEmpty();
}

MatchResult WordDetect::doMatch(const QString &text, int offset, const QStringList &) const
{
    if (text.size() - offset < m_word.size())
        return offset;

    /**
     * detect delimiter characters on the inner and outer boundaries of the string
     * NOTE: m_word isn't empty
     */
    if (offset > 0 && !isWordDelimiter(text.at(offset - 1)) && !isWordDelimiter(text.at(offset)))
        return offset;

    if (QStringView(text).mid(offset, m_word.size()).compare(m_word, m_caseSensitivity) != 0)
        return offset;

    if (text.size() == offset + m_word.size() || isWordDelimiter(text.at(offset + m_word.size())) || isWordDelimiter(text.at(offset + m_word.size() - 1)))
        return offset + m_word.size();

    return offset;
}
