/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2021 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qglobal.h"

#if (defined(QT_STATIC) || defined(QT_BOOTSTRAPPED)) && defined(Q_CC_GNU) && Q_CC_GNU >= 1000
QT_WARNING_DISABLE_GCC("-Wfree-nonheap-object") // false positive tracking
#endif

#if defined(Q_OS_MACOS)
#   include "private/qcore_mac_p.h"
#   include <CoreFoundation/CoreFoundation.h>
#endif

#include "qplatformdefs.h"

#include "qdatastream.h"
#include "qdebug.h"
#include "qhashfunctions.h"
#include "qstring.h"
#include "qlocale.h"
#include "qlocale_p.h"
#include "qlocale_tools_p.h"
#if QT_CONFIG(datetimeparser)
#include "private/qdatetimeparser_p.h"
#endif
#include "qnamespace.h"
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"
#include "qstringbuilder.h"
#include "private/qnumeric_p.h"
#include <cmath>
#ifndef QT_NO_SYSTEMLOCALE
#   include "qmutex.h"
#endif
#ifdef Q_OS_WIN
#   include <qt_windows.h>
#   include <time.h>
#endif

#include "private/qcalendarbackend_p.h"
#include "private/qgregoriancalendar_p.h"
#include "qcalendar.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE
static QSystemLocale *_systemLocale = nullptr;
class QSystemLocaleSingleton: public QSystemLocale
{
public:
    QSystemLocaleSingleton() : QSystemLocale(true) {}
};

Q_GLOBAL_STATIC(QSystemLocaleSingleton, QSystemLocale_globalSystemLocale)
static QLocaleData systemLocaleData;
#endif

/******************************************************************************
** Helpers for accessing Qt locale database
*/

QT_BEGIN_INCLUDE_NAMESPACE
#include "qlocale_data_p.h"
QT_END_INCLUDE_NAMESPACE

QLocale::Language QLocalePrivate::codeToLanguage(QStringView code) noexcept
{
    const auto len = code.size();
    if (len != 2 && len != 3)
        return QLocale::AnyLanguage;
    char16_t uc1 = code[0].toLower().unicode();
    char16_t uc2 = code[1].toLower().unicode();
    char16_t uc3 = len > 2 ? code[2].toLower().unicode() : 0;

    const unsigned char *c = language_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
            return QLocale::Language((c - language_code_list)/3);
    }

    if (uc3 == 0) {
        // legacy codes
        if (uc1 == 'n' && uc2 == 'o') // no -> nb
            return QLocale::NorwegianBokmal;
        if (uc1 == 't' && uc2 == 'l') // tl -> fil
            return QLocale::Filipino;
        if (uc1 == 's' && uc2 == 'h') // sh -> sr[_Latn]
            return QLocale::Serbian;
        if (uc1 == 'm' && uc2 == 'o') // mo -> ro
            return QLocale::Romanian;
        // Android uses the following deprecated codes
        if (uc1 == 'i' && uc2 == 'w') // iw -> he
            return QLocale::Hebrew;
        if (uc1 == 'i' && uc2 == 'n') // in -> id
            return QLocale::Indonesian;
        if (uc1 == 'j' && uc2 == 'i') // ji -> yi
            return QLocale::Yiddish;
    }
    return QLocale::AnyLanguage;
}

QLocale::Script QLocalePrivate::codeToScript(QStringView code) noexcept
{
    const auto len = code.size();
    if (len != 4)
        return QLocale::AnyScript;

    // script is titlecased in our data
    unsigned char c0 = code[0].toUpper().toLatin1();
    unsigned char c1 = code[1].toLower().toLatin1();
    unsigned char c2 = code[2].toLower().toLatin1();
    unsigned char c3 = code[3].toLower().toLatin1();

    const unsigned char *c = script_code_list;
    for (int i = 0; i < QLocale::LastScript; ++i, c += 4) {
        if (c0 == c[0] && c1 == c[1] && c2 == c[2] && c3 == c[3])
            return QLocale::Script(i);
    }
    return QLocale::AnyScript;
}

QLocale::Territory QLocalePrivate::codeToTerritory(QStringView code) noexcept
{
    const auto len = code.size();
    if (len != 2 && len != 3)
        return QLocale::AnyTerritory;

    char16_t uc1 = code[0].toUpper().unicode();
    char16_t uc2 = code[1].toUpper().unicode();
    char16_t uc3 = len > 2 ? code[2].toUpper().unicode() : 0;

    const unsigned char *c = territory_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
            return QLocale::Territory((c - territory_code_list)/3);
    }

    return QLocale::AnyTerritory;
}

QLatin1String QLocalePrivate::languageToCode(QLocale::Language language)
{
    if (language == QLocale::AnyLanguage || language > QLocale::LastLanguage)
        return QLatin1String();
    if (language == QLocale::C)
        return QLatin1String("C");

    const unsigned char *c = language_code_list + 3 * language;
    return QLatin1String(reinterpret_cast<const char*>(c), c[2] == 0 ? 2 : 3);
}

QLatin1String QLocalePrivate::scriptToCode(QLocale::Script script)
{
    if (script == QLocale::AnyScript || script > QLocale::LastScript)
        return QLatin1String();
    const unsigned char *c = script_code_list + 4 * script;
    return QLatin1String(reinterpret_cast<const char *>(c), 4);
}

QLatin1String QLocalePrivate::territoryToCode(QLocale::Territory territory)
{
    if (territory == QLocale::AnyTerritory || territory > QLocale::LastTerritory)
        return QLatin1String();

    const unsigned char *c = territory_code_list + 3 * territory;
    return QLatin1String(reinterpret_cast<const char*>(c), c[2] == 0 ? 2 : 3);
}

namespace {
struct LikelyPair
{
    QLocaleId key; // Search key.
    QLocaleId value = QLocaleId { 0, 0, 0 };
};

bool operator<(const LikelyPair &lhs, const LikelyPair &rhs)
{
    // Must match the comparison LocaleDataWriter.likelySubtags() uses when
    // sorting, see qtbase/util/locale_database.qlocalexml2cpp.py
    const auto compare = [](int lhs, int rhs) {
        // 0 sorts after all other values; lhs and rhs are passed ushort values.
        const int huge = 0x10000;
        return (lhs ? lhs : huge) - (rhs ? rhs : huge);
    };
    const auto &left = lhs.key;
    const auto &right = rhs.key;
    // Comparison order: language, region, script:
    if (int cmp = compare(left.language_id, right.language_id))
        return cmp < 0;
    if (int cmp = compare(left.territory_id, right.territory_id))
        return cmp < 0;
    return compare(left.script_id, right.script_id) < 0;
}
} // anonymous namespace

/*!
    Fill in blank fields of a locale ID.

    An ID in which some fields are zero stands for any locale that agrees with
    it in its non-zero fields.  CLDR's likely-subtag data is meant to help us
    chose which candidate to prefer.  (Note, however, that CLDR does have some
    cases where it maps an ID to a "best match" for which CLDR does not provide
    data, even though there are locales for which CLDR does provide data that do
    match the given ID.  It's telling us, unhelpfully but truthfully, what
    locale would (most likely) be meant by (someone using) the combination
    requested, even when that locale isn't yet supported.)  It may also map an
    obsolete or generic tag to a modern or more specific replacement, possibly
    filling in some of the other fields in the process (presently only for
    countries).  Note that some fields of the result may remain blank, but there
    is no more specific recommendation available.

    For the formal specification, see
    https://www.unicode.org/reports/tr35/#Likely_Subtags

    \note We also search und_script_region and und_region; they're not mentioned
    in the spec, but the examples clearly presume them and CLDR does provide
    such likely matches.
*/
QLocaleId QLocaleId::withLikelySubtagsAdded() const
{
    /* Each pattern that appears in a comments below, language_script_region and
       similar, indicates which of this's fields (even if blank) are being
       attended to in a given search; for fields left out of the pattern, the
       search uses 0 regardless of whether this has specified the field.

       If a key matches what we're searching for (possibly with a wildcard in
       the key matching a non-wildcard in our search), the tags from this that
       are specified in the key are replaced by the match (even if different);
       but the other tags of this replace what's in the match (even when the
       match does specify a value).
    */
    static_assert(std::size(likely_subtags) % 2 == 0);
    auto *pairs = reinterpret_cast<const LikelyPair *>(likely_subtags);
    auto *const afterPairs = pairs + std::size(likely_subtags) / 2;
    LikelyPair sought { *this };
    // Our array is sorted in the order that puts all candidate matches in the
    // order we would want them; ones we should prefer appear before the others.
    if (language_id) {
        // language_script_region, language_region, language_script, language:
        pairs = std::lower_bound(pairs, afterPairs, sought);
        // Single language's block isn't long enough to warrant more binary
        // chopping within it - just traverse it all:
        for (; pairs < afterPairs && pairs->key.language_id == language_id; ++pairs) {
            const QLocaleId &key = pairs->key;
            if (key.territory_id && key.territory_id != territory_id)
                continue;
            if (key.script_id && key.script_id != script_id)
                continue;
            QLocaleId value = pairs->value;
            if (territory_id && !key.territory_id)
                value.territory_id = territory_id;
            if (script_id && !key.script_id)
                value.script_id = script_id;
            return value;
        }
    }
    // und_script_region or und_region (in that order):
    if (territory_id) {
        sought.key = QLocaleId { 0, script_id, territory_id };
        pairs = std::lower_bound(pairs, afterPairs, sought);
        // Again, individual und_?_region block isn't long enough to make binary
        // chop a win:
        for (; pairs < afterPairs && pairs->key.territory_id == territory_id; ++pairs) {
            const QLocaleId &key = pairs->key;
            Q_ASSERT(!key.language_id);
            if (key.script_id && key.script_id != script_id)
                continue;
            QLocaleId value = pairs->value;
            if (language_id)
                value.language_id = language_id;
            if (script_id && !key.script_id)
                value.script_id = script_id;
            return value;
        }
    }
    // und_script:
    if (script_id) {
        sought.key = QLocaleId { 0, script_id, 0 };
        pairs = std::lower_bound(pairs, afterPairs, sought);
        if (pairs < afterPairs && pairs->key.script_id == script_id) {
            Q_ASSERT(!pairs->key.language_id && !pairs->key.territory_id);
            QLocaleId value = pairs->value;
            if (language_id)
                value.language_id = language_id;
            if (territory_id)
                value.territory_id = territory_id;
            return value;
        }
    }
    if (matchesAll()) { // Skipped all of the above.
        // CLDR has no match-all at v37, but might get one some day ...
        pairs = std::lower_bound(pairs, afterPairs, sought);
        if (pairs < afterPairs) {
            // All other keys are < match-all.
            Q_ASSERT(pairs + 1 == afterPairs);
            Q_ASSERT(pairs->key.matchesAll());
            return pairs->value;
        }
    }
    return *this;
}

QLocaleId QLocaleId::withLikelySubtagsRemoved() const
{
    QLocaleId max = withLikelySubtagsAdded();
    // language
    {
        QLocaleId id { language_id, 0, 0 };
        if (id.withLikelySubtagsAdded() == max)
            return id;
    }
    // language_region
    if (territory_id) {
        QLocaleId id { language_id, 0, territory_id };
        if (id.withLikelySubtagsAdded() == max)
            return id;
    }
    // language_script
    if (script_id) {
        QLocaleId id { language_id, script_id, 0 };
        if (id.withLikelySubtagsAdded() == max)
            return id;
    }
    return max;
}

QByteArray QLocaleId::name(char separator) const
{
    if (language_id == QLocale::AnyLanguage)
        return QByteArray();
    if (language_id == QLocale::C)
        return QByteArrayLiteral("C");

    const unsigned char *lang = language_code_list + 3 * language_id;
    const unsigned char *script =
            (script_id != QLocale::AnyScript ? script_code_list + 4 * script_id : nullptr);
    const unsigned char *country =
            (territory_id != QLocale::AnyTerritory
             ? territory_code_list + 3 * territory_id : nullptr);
    char len = (lang[2] != 0 ? 3 : 2) + (script ? 4 + 1 : 0)
        + (country ? (country[2] != 0 ? 3 : 2) + 1 : 0);
    QByteArray name(len, Qt::Uninitialized);
    char *uc = name.data();
    *uc++ = lang[0];
    *uc++ = lang[1];
    if (lang[2] != 0)
        *uc++ = lang[2];
    if (script) {
        *uc++ = separator;
        *uc++ = script[0];
        *uc++ = script[1];
        *uc++ = script[2];
        *uc++ = script[3];
    }
    if (country) {
        *uc++ = separator;
        *uc++ = country[0];
        *uc++ = country[1];
        if (country[2] != 0)
            *uc++ = country[2];
    }
    return name;
}

QByteArray QLocalePrivate::bcp47Name(char separator) const
{
    if (m_data->m_language_id == QLocale::AnyLanguage)
        return QByteArray();
    if (m_data->m_language_id == QLocale::C)
        return QByteArrayLiteral("en");

    return m_data->id().withLikelySubtagsRemoved().name(separator);
}

static int findLocaleIndexById(const QLocaleId &localeId)
{
    quint16 idx = locale_index[localeId.language_id];
    // If there are no locales for specified language (so we we've got the
    // default language, which has no associated script or country), give up:
    if (localeId.language_id && idx == 0)
        return idx;

    Q_ASSERT(localeId.acceptLanguage(locale_data[idx].m_language_id));

    do {
        if (localeId.acceptScriptTerritory(locale_data[idx].id()))
            return idx;
        ++idx;
    } while (localeId.acceptLanguage(locale_data[idx].m_language_id));

    return -1;
}

int QLocaleData::findLocaleIndex(QLocaleId lid)
{
    QLocaleId localeId = lid;
    QLocaleId likelyId = localeId.withLikelySubtagsAdded();
    const ushort fallback = likelyId.language_id;

    // Try a straight match with the likely data:
    int index = findLocaleIndexById(likelyId);
    if (index >= 0)
        return index;
    QList<QLocaleId> tried;
    tried.push_back(likelyId);

#define CheckCandidate(id) do { \
        if (!tried.contains(id)) { \
            index = findLocaleIndexById(id); \
            if (index >= 0) \
                return index; \
            tried.push_back(id); \
        } \
    } while (false) // end CheckCandidate

    // No match; try again with raw data:
    CheckCandidate(localeId);

    // No match; try again with likely country for language_script
    if (lid.territory_id && (lid.language_id || lid.script_id)) {
        localeId.territory_id = 0;
        likelyId = localeId.withLikelySubtagsAdded();
        CheckCandidate(likelyId);

        // No match; try again with any country
        CheckCandidate(localeId);
    }

    // No match; try again with likely script for language_region
    if (lid.script_id && (lid.language_id || lid.territory_id)) {
        localeId = QLocaleId { lid.language_id, 0, lid.territory_id };
        likelyId = localeId.withLikelySubtagsAdded();
        CheckCandidate(likelyId);

        // No match; try again with any script
        CheckCandidate(localeId);
    }
#undef CheckCandidate

    // No match; return base index for initial likely language:
    return locale_index[fallback];
}

static QStringView findTag(QStringView name)
{
    const QString separators = QStringLiteral("_-.@");
    int i = 0;
    while (i < name.size() && !separators.contains(name[i]))
        i++;
    return name.first(i);
}

static bool validTag(QStringView tag)
{
    // Is tag is a non-empty sequence of ASCII letters and/or digits ?
    for (QChar uc : tag) {
        const char16_t ch = uc.unicode();
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
            return false;
    }
    return tag.size() > 0;
}

static bool isScript(QStringView tag)
{
    // Every script name is 4 characters, a capital followed by three lower-case;
    // so a search for tag in allScripts *can* only match if it's aligned.
    static const QString allScripts =
        QString::fromLatin1(reinterpret_cast<const char *>(script_code_list),
                            sizeof(script_code_list) - 1);
    return tag.length() == 4 && allScripts.indexOf(tag) % 4 == 0;
}

bool qt_splitLocaleName(QStringView name, QStringView *lang, QStringView *script, QStringView *land)
{
    // Assume each of lang, script and land is nullptr or points to an empty QStringView.
    enum ParserState { NoState, LangState, ScriptState, CountryState };
    ParserState state = LangState;
    while (name.size() && state != NoState) {
        const QStringView tag = findTag(name);
        if (!validTag(tag))
            break;
        name = name.sliced(tag.size());
        const bool sep = name.size() > 0;
        if (sep) // tag wasn't all that remained; there was a separator
            name = name.sliced(1);

        switch (state) {
        case LangState:
            if (tag.size() != 2 && tag.size() != 3)
                return false;
            if (lang)
                *lang = tag;
            state = sep ? ScriptState : NoState;
            break;
        case ScriptState:
            if (isScript(tag)) {
                if (script)
                    *script = tag;
                state = sep ? CountryState : NoState;
                break;
            }
            // It wasn't a script, assume it's a country.
            Q_FALLTHROUGH();
        case CountryState:
            if (land)
                *land = tag;
            state = NoState;
            break;
        case NoState: // Precluded by loop condition !
            Q_UNREACHABLE();
            break;
        }
    }
    return state != LangState;
}

QLocaleId QLocaleId::fromName(const QString &name)
{
    QStringView lang;
    QStringView script;
    QStringView land;
    if (!qt_splitLocaleName(name, &lang, &script, &land))
        return { QLocale::C, 0, 0 };

    QLocale::Language langId = QLocalePrivate::codeToLanguage(lang);
    if (langId == QLocale::AnyLanguage)
        return { QLocale::C, 0, 0 };
    return { langId, QLocalePrivate::codeToScript(script), QLocalePrivate::codeToTerritory(land) };
}

QString qt_readEscapedFormatString(QStringView format, int *idx)
{
    int &i = *idx;

    Q_ASSERT(format.at(i) == QLatin1Char('\''));
    ++i;
    if (i == format.size())
        return QString();
    if (format.at(i).unicode() == '\'') { // "''" outside of a quoted string
        ++i;
        return QLatin1String("'");
    }

    QString result;

    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            if (format.mid(i + 1).startsWith(QLatin1Char('\''))) {
                // "''" inside a quoted string
                result.append(QLatin1Char('\''));
                i += 2;
            } else {
                break;
            }
        } else {
            result.append(format.at(i++));
        }
    }
    if (i < format.size())
        ++i;

    return result;
}

/*!
    \internal

    Counts the number of identical leading characters in \a s.

    If \a s is empty, returns 0.

    Otherwise, returns the number of consecutive \c{s.front()}
    characters at the start of \a s.

    \code
    qt_repeatCount(u"a");   // == 1
    qt_repeatCount(u"ab");  // == 1
    qt_repeatCount(u"aab"); // == 2
    \endcode
*/
int qt_repeatCount(QStringView s)
{
    if (s.isEmpty())
        return 0;
    const QChar c = s.front();
    qsizetype j = 1;
    while (j < s.size() && s.at(j) == c)
        ++j;
    return int(j);
}

static const QLocaleData *default_data = nullptr;

static const QLocaleData *const c_data = locale_data;
static QLocalePrivate *c_private()
{
    static QLocalePrivate c_locale(c_data, 0, QLocale::OmitGroupSeparator, 1);
    return &c_locale;
}

#ifndef QT_NO_SYSTEMLOCALE
/******************************************************************************
** Default system locale behavior
*/

/*!
  Constructs a QSystemLocale object.

  The constructor will automatically install this object as the system locale,
  if there's not one active.  It also resets the flag that'll prompt
  QLocale::system() to re-initialize its data, so that instantiating a
  QSystemLocale transiently (doesn't install the transient as system locale if
  there was one already and) triggers an update to the system locale's data.
*/
QSystemLocale::QSystemLocale()
{
    if (!_systemLocale)
        _systemLocale = this;

    systemLocaleData.m_language_id = 0;
}

/*!
    \internal
*/
QSystemLocale::QSystemLocale(bool)
{ }

/*!
  Deletes the object.
*/
QSystemLocale::~QSystemLocale()
{
    if (_systemLocale == this) {
        _systemLocale = nullptr;

        systemLocaleData.m_language_id = 0;
    }
}

static const QSystemLocale *systemLocale()
{
    if (_systemLocale)
        return _systemLocale;
    return QSystemLocale_globalSystemLocale();
}

static void updateSystemPrivate()
{
    // This function is NOT thread-safe!
    // It *should not* be called by anything but systemData()
    // It *is* called before {system,default}LocalePrivate exist.
    const QSystemLocale *sys_locale = systemLocale();

    // tell the object that the system locale has changed.
    sys_locale->query(QSystemLocale::LocaleChanged);

    // Populate system locale with fallback as basis
    systemLocaleData = locale_data[sys_locale->fallbackLocaleIndex()];

    QVariant res = sys_locale->query(QSystemLocale::LanguageId);
    if (!res.isNull()) {
        systemLocaleData.m_language_id = res.toInt();
        systemLocaleData.m_script_id = QLocale::AnyScript; // default for compatibility
    }
    res = sys_locale->query(QSystemLocale::TerritoryId);
    if (!res.isNull()) {
        systemLocaleData.m_territory_id = res.toInt();
        systemLocaleData.m_script_id = QLocale::AnyScript; // default for compatibility
    }
    res = sys_locale->query(QSystemLocale::ScriptId);
    if (!res.isNull())
        systemLocaleData.m_script_id = res.toInt();

    // Should we replace Any values based on likely sub-tags ?
}
#endif // !QT_NO_SYSTEMLOCALE

static const QLocaleData *systemData()
{
#ifndef QT_NO_SYSTEMLOCALE
    /*
      Copy over the information from the fallback locale and modify.

      This modifies (cross-thread) global state, so take care to only call it in
      one thread.
    */
    {
        static QBasicMutex systemDataMutex;
        systemDataMutex.lock();
        if (systemLocaleData.m_language_id == 0)
            updateSystemPrivate();
        systemDataMutex.unlock();
    }

    return &systemLocaleData;
#else
    return locale_data;
#endif
}

static const QLocaleData *defaultData()
{
    if (!default_data)
        default_data = systemData();
    return default_data;
}

static uint defaultIndex()
{
    const QLocaleData *const data = defaultData();
#ifndef QT_NO_SYSTEMLOCALE
    if (data == &systemLocaleData) {
        // Work out a suitable index matching the system data, for use when
        // accessing calendar data, when not fetched from system.
        return QLocaleData::findLocaleIndex(data->id());
    }
#endif

    Q_ASSERT(data >= locale_data);
    Q_ASSERT(data < locale_data + std::size(locale_data));
    return data - locale_data;
}

const QLocaleData *QLocaleData::c()
{
    Q_ASSERT(locale_index[QLocale::C] == 0);
    return c_data;
}

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &ds, const QLocale &l)
{
    ds << l.name();
    return ds;
}

QDataStream &operator>>(QDataStream &ds, QLocale &l)
{
    QString s;
    ds >> s;
    l = QLocale(s);
    return ds;
}
#endif // QT_NO_DATASTREAM

static const int locale_data_size = sizeof(locale_data)/sizeof(QLocaleData) - 1;

QBasicAtomicInt QLocalePrivate::s_generation = Q_BASIC_ATOMIC_INITIALIZER(0);
Q_GLOBAL_STATIC_WITH_ARGS(QSharedDataPointer<QLocalePrivate>, defaultLocalePrivate,
                          (new QLocalePrivate(defaultData(), defaultIndex())))

static QLocalePrivate *localePrivateByName(const QString &name)
{
    if (name == QLatin1String("C"))
        return c_private();
    const int index = QLocaleData::findLocaleIndex(QLocaleId::fromName(name));
    Q_ASSERT(index >= 0 && size_t(index) < std::size(locale_data) - 1);
    return new QLocalePrivate(locale_data + index, index,
                              locale_data[index].m_language_id == QLocale::C
                              ? QLocale::OmitGroupSeparator : QLocale::DefaultNumberOptions);
}

static QLocalePrivate *findLocalePrivate(QLocale::Language language, QLocale::Script script,
                                         QLocale::Territory territory)
{
    if (language == QLocale::C)
        return c_private();

    int index = QLocaleData::findLocaleIndex(QLocaleId { language, script, territory });
    Q_ASSERT(index >= 0 && size_t(index) < std::size(locale_data) - 1);
    const QLocaleData *data = locale_data + index;

    QLocale::NumberOptions numberOptions = QLocale::DefaultNumberOptions;

    // If not found, should use default locale:
    if (data->m_language_id == QLocale::C) {
        if (defaultLocalePrivate.exists())
            numberOptions = defaultLocalePrivate->data()->m_numberOptions;
        data = defaultData();
        index = defaultIndex();
    }
    return new QLocalePrivate(data, index, numberOptions);
}

QString QLocaleData::decimalPoint() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (this == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::DecimalPoint).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return decimalSeparator().getData(single_character_data);
}

QString QLocaleData::groupSeparator() const
{
    // Empty => don't do grouping
#ifndef QT_NO_SYSTEMLOCALE
    if (this == &systemLocaleData) {
        QVariant res = systemLocale()->query(QSystemLocale::GroupSeparator);
        if (!res.isNull())
            return res.toString();
    }
#endif
    return groupDelim().getData(single_character_data);
}

QString QLocaleData::percentSign() const
{
    return percent().getData(single_character_data);
}

QString QLocaleData::listSeparator() const
{
    return listDelimit().getData(single_character_data);
}

QString QLocaleData::zeroDigit() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (this == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::ZeroDigit).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return zero().getData(single_character_data);
}

char32_t QLocaleData::zeroUcs() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (this == &systemLocaleData) {
        const auto text = systemLocale()->query(QSystemLocale::ZeroDigit).toString();
        if (!text.isEmpty()) {
            if (text.size() == 1 && !text.at(0).isSurrogate())
                return text.at(0).unicode();
            if (text.size() == 2 && text.at(0).isHighSurrogate())
                return QChar::surrogateToUcs4(text.at(0), text.at(1));
        }
    }
#endif
    return zero().ucsFirst(single_character_data);
}

QString QLocaleData::negativeSign() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (this == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::NegativeSign).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return minus().getData(single_character_data);
}

QString QLocaleData::positiveSign() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (this == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::PositiveSign).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return plus().getData(single_character_data);
}

QString QLocaleData::exponentSeparator() const
{
    return exponential().getData(single_character_data);
}

/*!
 \internal
*/
QLocale::QLocale(QLocalePrivate &dd)
    : d(&dd)
{}


/*!
    Constructs a QLocale object with the specified \a name,
    which has the format
    "language[_script][_territory][.codeset][@modifier]" or "C", where:

    \list
    \li language is a lowercase, two-letter, ISO 639 language code (some
        three-letter codes are also recognized),
    \li script is a capitalized, four-letter, ISO 15924 script code,
    \li territory is an uppercase, two-letter, ISO 3166 territory code
        (some numeric codes are also recognized), and
    \li codeset and modifier are ignored.
    \endlist

    The separator can be either underscore \c{'_'} (U+005F, "low line") or a
    dash \c{'-'} (U+002D, "hyphen-minus"). If QLocale has no data for the
    specified combination of language, script, and territory, then it uses the
    most suitable match it can find instead. If the string violates the locale
    format, or no suitable data can be found for the specified keys, the "C"
    locale is used instead.

    This constructor is much slower than QLocale(Language, Script, Territory) or
    QLocale(Language, Territory).

    \sa bcp47Name(), {Matching combinations of language, script and territory}
*/

QLocale::QLocale(const QString &name)
    : d(localePrivateByName(name))
{
}

/*!
    Constructs a QLocale object initialized with the default locale.

    If no default locale was set using setDefault(), this locale will be the
    same as the one returned by system().

    \sa setDefault()
*/

QLocale::QLocale()
    : d(*defaultLocalePrivate)
{
    // Make sure system data is up to date:
    systemData();
}

/*!
    Constructs a QLocale object for the specified \a language and \a territory.

    If there is more than one script in use for this combination, a likely
    script will be selected. If QLocale has no data for the specified \a
    language, the default locale is used. If QLocale has no data for the
    specified combination of \a language and \a territory, an alternative
    territory may be used instead.

    \sa setDefault(), {Matching combinations of language, script and territory}
*/

QLocale::QLocale(Language language, Territory territory)
    : d(findLocalePrivate(language, QLocale::AnyScript, territory))
{
}

/*!
    \since 4.8

    Constructs a QLocale object for the specified \a language, \a script and \a
    territory.

    If QLocale does not have data for the given combination, it will find data
    for as good a match as it can. It falls back on the default locale if

    \list
    \li \a language is \c AnyLanguage and no language can be inferred from \a
        script and \a territory
    \li QLocale has no data for the language, either given as \a language or
        inferred as above.
    \endlist

    \sa setDefault(), {Matching combinations of language, script and territory}
*/

QLocale::QLocale(Language language, Script script, Territory territory)
    : d(findLocalePrivate(language, script, territory))
{
}

/*!
    Constructs a QLocale object as a copy of \a other.
*/

QLocale::QLocale(const QLocale &other)
{
    d = other.d;
}

/*!
    Destructor
*/

QLocale::~QLocale()
{
}

/*!
    Assigns \a other to this QLocale object and returns a reference
    to this QLocale object.
*/

QLocale &QLocale::operator=(const QLocale &other)
{
    d = other.d;
    return *this;
}

/*!
    \internal
    Equality comparison.
*/

bool QLocale::equals(const QLocale &other) const
{
    return d->m_data == other.d->m_data && d->m_numberOptions == other.d->m_numberOptions;
}

/*!
    \fn void QLocale::swap(QLocale &other)
    \since 5.6

    Swaps locale \a other with this locale. This operation is very fast and
    never fails.
*/

/*!
    \since 5.6
    \relates QLocale

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
*/
size_t qHash(const QLocale &key, size_t seed) noexcept
{
    return qHashMulti(seed, key.d->m_data, key.d->m_numberOptions);
}

/*!
    \since 4.2

    Sets the \a options related to number conversions for this
    QLocale instance.

    \sa numberOptions(), FloatingPointPrecisionOption
*/
void QLocale::setNumberOptions(NumberOptions options)
{
    d->m_numberOptions = options;
}

/*!
    \since 4.2

    Returns the options related to number conversions for this
    QLocale instance.

    By default, no options are set for the standard locales, except
    for the "C" locale, which has OmitGroupSeparator set by default.

    \sa setNumberOptions(), toString(), groupSeparator(), FloatingPointPrecisionOption
*/
QLocale::NumberOptions QLocale::numberOptions() const
{
    return static_cast<NumberOptions>(d->m_numberOptions);
}

/*!
  \fn QString QLocale::quoteString(const QString &str, QuotationStyle style) const

    \since 4.8

    Returns \a str quoted according to the current locale using the given
    quotation \a style.
*/

/*!
    \since 6.0

    \overload
*/
QString QLocale::quoteString(QStringView str, QuotationStyle style) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res;
        if (style == QLocale::AlternateQuotation)
            res = systemLocale()->query(QSystemLocale::StringToAlternateQuotation,
                                        QVariant::fromValue(str));
        if (res.isNull() || style == QLocale::StandardQuotation)
            res = systemLocale()->query(QSystemLocale::StringToStandardQuotation,
                                        QVariant::fromValue(str));
        if (!res.isNull())
            return res.toString();
    }
#endif

    QLocaleData::DataRange start, end;
    if (style == QLocale::StandardQuotation) {
        start = d->m_data->quoteStart();
        end = d->m_data->quoteEnd();
    } else {
        start = d->m_data->quoteStartAlternate();
        end = d->m_data->quoteEndAlternate();
    }

    return start.viewData(single_character_data) % str % end.viewData(single_character_data);
}

/*!
    \since 4.8

    Returns a string that represents a join of a given \a list of strings with
    a separator defined by the locale.
*/
QString QLocale::createSeparatedList(const QStringList &list) const
{
    // May be empty if list is empty or sole entry is empty.
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res =
            systemLocale()->query(QSystemLocale::ListToSeparatedString, QVariant::fromValue(list));

        if (!res.isNull())
            return res.toString();
    }
#endif

    const int size = list.size();
    if (size < 1)
        return QString();

    if (size == 1)
        return list.at(0);

    if (size == 2)
        return d->m_data->pairListPattern().getData(
            list_pattern_part_data).arg(list.at(0), list.at(1));

    QStringView formatStart = d->m_data->startListPattern().viewData(list_pattern_part_data);
    QStringView formatMid = d->m_data->midListPattern().viewData(list_pattern_part_data);
    QStringView formatEnd = d->m_data->endListPattern().viewData(list_pattern_part_data);
    QString result = formatStart.arg(list.at(0), list.at(1));
    for (int i = 2; i < size - 1; ++i)
        result = formatMid.arg(result, list.at(i));
    result = formatEnd.arg(result, list.at(size - 1));
    return result;
}

/*!
    \nonreentrant

    Sets the global default locale to \a locale. These
    values are used when a QLocale object is constructed with
    no arguments. If this function is not called, the system's
    locale is used.

    \warning In a multithreaded application, the default locale
    should be set at application startup, before any non-GUI threads
    are created.

    \sa system(), c()
*/

void QLocale::setDefault(const QLocale &locale)
{
    default_data = locale.d->m_data;

    if (defaultLocalePrivate.isDestroyed())
        return; // avoid crash on exit
    if (!defaultLocalePrivate.exists()) {
        // Force it to exist; see QTBUG-83016
        QLocale ignoreme;
        Q_ASSERT(defaultLocalePrivate.exists());
    }

    // update the cached private
    *defaultLocalePrivate = locale.d;
    QLocalePrivate::s_generation.fetchAndAddRelaxed(1);
}

/*!
    Returns the language of this locale.

    \sa script(), territory(), languageToString(), bcp47Name()
*/
QLocale::Language QLocale::language() const
{
    return Language(d->languageId());
}

/*!
    \since 4.8

    Returns the script of this locale.

    \sa language(), territory(), languageToString(), scriptToString(), bcp47Name()
*/
QLocale::Script QLocale::script() const
{
    return Script(d->m_data->m_script_id);
}

/*!
    \since 6.2

    Returns the territory of this locale.

    \sa language(), script(), territoryToString(), bcp47Name()
*/
QLocale::Territory QLocale::territory() const
{
    return Territory(d->territoryId());
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use \l territory() instead.

    Returns the territory of this locale.

    \sa language(), script(), territoryToString(), bcp47Name()
*/
QLocale::Country QLocale::country() const
{
    return territory();
}
#endif

/*!
    Returns the language and country of this locale as a
    string of the form "language_country", where
    language is a lowercase, two-letter ISO 639 language code,
    and country is an uppercase, two- or three-letter ISO 3166 country code.

    Note that even if QLocale object was constructed with an explicit script,
    name() will not contain it for compatibility reasons. Use \l bcp47Name() instead
    if you need a full locale name.

    \sa QLocale(), language(), script(), territory(), bcp47Name()
*/

QString QLocale::name() const
{
    Language l = language();
    if (l == C)
        return d->languageCode();

    Territory c = territory();
    if (c == AnyTerritory)
        return d->languageCode();

    return d->languageCode() + QLatin1Char('_') + d->territoryCode();
}

static qlonglong toIntegral_helper(const QLocaleData *d, QStringView str, bool *ok,
                                   QLocale::NumberOptions mode, qlonglong)
{
    return d->stringToLongLong(str, 10, ok, mode);
}

static qulonglong toIntegral_helper(const QLocaleData *d, QStringView str, bool *ok,
                                    QLocale::NumberOptions mode, qulonglong)
{
    return d->stringToUnsLongLong(str, 10, ok, mode);
}

template <typename T> static inline
T toIntegral_helper(const QLocalePrivate *d, QStringView str, bool *ok)
{
    using Int64 =
        typename std::conditional<std::is_unsigned<T>::value, qulonglong, qlonglong>::type;

    // we select the right overload by the last, unused parameter
    Int64 val = toIntegral_helper(d->m_data, str, ok, d->m_numberOptions, Int64());
    if (T(val) != val) {
        if (ok != nullptr)
            *ok = false;
        val = 0;
    }
    return T(val);
}


/*!
    \since 4.8

    Returns the dash-separated language, script and country (and possibly other
    BCP47 fields) of this locale as a string.

    Unlike the uiLanguages() the returned value of the bcp47Name() represents
    the locale name of the QLocale data but not the language the user-interface
    should be in.

    This function tries to conform the locale name to BCP47.

    \sa language(), territory(), script(), uiLanguages()
*/
QString QLocale::bcp47Name() const
{
    return QString::fromLatin1(d->bcp47Name());
}

/*!
    Returns the two- or three-letter language code for \a language, as defined
    in the ISO 639 standards.

    \note For \c{QLocale::C} the function returns \c{"C"}.
    For \c QLocale::AnyLanguage an empty string is returned.

    \since 6.1
    \sa codeToLanguage(), language(), name(), bcp47Name(), territoryToCode(), scriptToCode()
*/
QString QLocale::languageToCode(Language language)
{
    return QLocalePrivate::languageToCode(language);
}

/*!
    Returns the QLocale::Language enum corresponding to the two- or three-letter
    \a languageCode, as defined in the ISO 639 standards.

    If the code is invalid or not known QLocale::AnyLanguage is returned.

    \since 6.1
    \sa languageToCode(), codeToTerritory(), codeToScript()
*/
QLocale::Language QLocale::codeToLanguage(QStringView languageCode) noexcept
{
    return QLocalePrivate::codeToLanguage(languageCode);
}

/*!
    \since 6.2

    Returns the two-letter territory code for \a territory, as defined
    in the ISO 3166 standard.

    \note For \c{QLocale::AnyTerritory} an empty string is returned.

    \sa codeToTerritory(), territory(), name(), bcp47Name(), languageToCode(), scriptToCode()
*/
QString QLocale::territoryToCode(QLocale::Territory territory)
{
    return QLocalePrivate::territoryToCode(territory);
}

/*!
    \since 6.2

    Returns the QLocale::Territory enum corresponding to the two-letter or
    three-digit \a territoryCode, as defined in the ISO 3166 standard.

    If the code is invalid or not known QLocale::AnyTerritory is returned.

    \sa territoryToCode(), codeToLanguage(), codeToScript()
*/
QLocale::Territory QLocale::codeToTerritory(QStringView territoryCode) noexcept
{
    return QLocalePrivate::codeToTerritory(territoryCode);
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use \l territoryToCode() instead.

    Returns the two-letter territory code for \a country, as defined
    in the ISO 3166 standard.

    \note For \c{QLocale::AnyTerritory} or \c{QLocale::AnyCountry} an empty string is returned.

    \sa codeToTerritory(), territory(), name(), bcp47Name(), languageToCode(), scriptToCode()
*/
QString QLocale::countryToCode(Country country)
{
    return territoryToCode(country);
}

/*!
    Returns the QLocale::Territory enum corresponding to the two-letter or
    three-digit \a countryCode, as defined in the ISO 3166 standard.

    If the code is invalid or not known QLocale::AnyTerritory is returned.

    \deprecated [6.6] Use codeToTerritory(QStringView) instead.
    \since 6.1
    \sa territoryToCode(), codeToLanguage(), codeToScript()
*/
QLocale::Country QLocale::codeToCountry(QStringView countryCode) noexcept
{
    return QLocalePrivate::codeToTerritory(countryCode);
}
#endif

/*!
    Returns the four-letter script code for \a script, as defined in the
    ISO 15924 standard.

    \note For \c{QLocale::AnyScript} an empty string is returned.

    \since 6.1
    \sa script(), name(), bcp47Name(), languageToCode(), territoryToCode()
*/
QString QLocale::scriptToCode(Script script)
{
    return QLocalePrivate::scriptToCode(script);
}

/*!
    Returns the QLocale::Script enum corresponding to the four-letter script
    \a scriptCode, as defined in the ISO 15924 standard.

    If the code is invalid or not known QLocale::AnyScript is returned.

    \since 6.1
    \sa scriptToCode(), codeToLanguage(), codeToTerritory()
*/
QLocale::Script QLocale::codeToScript(QStringView scriptCode) noexcept
{
    return QLocalePrivate::codeToScript(scriptCode);
}

/*!
    Returns a QString containing the name of \a language.

    \sa territoryToString(), scriptToString(), bcp47Name()
*/

QString QLocale::languageToString(Language language)
{
    if (language > QLocale::LastLanguage)
        return QLatin1String("Unknown");
    return QLatin1String(language_name_list + language_name_index[language]);
}

/*!
    \since 6.2

    Returns a QString containing the name of \a territory.

    \sa languageToString(), scriptToString(), territory(), bcp47Name()
*/
QString QLocale::territoryToString(QLocale::Territory territory)
{
    if (territory > QLocale::LastTerritory)
        return QLatin1String("Unknown");
    return QLatin1String(territory_name_list + territory_name_index[territory]);
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use \l territoryToString() instead.

    Returns a QString containing the name of \a country.

    \sa languageToString(), scriptToString(), territory(), bcp47Name()
*/
QString QLocale::countryToString(Country country)
{
    return territoryToString(country);
}
#endif

/*!
    \since 4.8

    Returns a QString containing the name of \a script.

    \sa languageToString(), territoryToString(), script(), bcp47Name()
*/
QString QLocale::scriptToString(QLocale::Script script)
{
    if (script > QLocale::LastScript)
        return QLatin1String("Unknown");
    return QLatin1String(script_name_list + script_name_index[script]);
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    \fn short QLocale::toShort(const QString &s, bool *ok) const

    Returns the short int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUShort(), toString()
*/

/*!
    \fn ushort QLocale::toUShort(const QString &s, bool *ok) const

    Returns the unsigned short int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toShort(), toString()
*/

/*!
    \fn int QLocale::toInt(const QString &s, bool *ok) const
    Returns the int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUInt(), toString()
*/

/*!
    \fn uint QLocale::toUInt(const QString &s, bool *ok) const
    Returns the unsigned int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toString()
*/

/*!
    \since 5.13
    \fn long QLocale::toLong(const QString &s, bool *ok) const

    Returns the long int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULong(), toDouble(), toString()
*/

/*!
    \since 5.13
    \fn ulong QLocale::toULong(const QString &s, bool *ok) const

    Returns the unsigned long int represented by the localized
    string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLong(), toInt(), toDouble(), toString()
*/

/*!
    \fn qlonglong QLocale::toLongLong(const QString &s, bool *ok) const
    Returns the long long int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULongLong(), toDouble(), toString()
*/

/*!
    \fn qulonglong QLocale::toULongLong(const QString &s, bool *ok) const

    Returns the unsigned long long int represented by the localized
    string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLongLong(), toInt(), toDouble(), toString()
*/

/*!
    \fn float QLocale::toFloat(const QString &s, bool *ok) const

    Returns the float represented by the localized string \a s.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for any other reason (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function does not fall back to the 'C' locale if the string
    cannot be interpreted in this locale.

    This function ignores leading and trailing whitespace.

    \sa toDouble(), toInt(), toString()
*/

/*!
    \fn double QLocale::toDouble(const QString &s, bool *ok) const
    Returns the double represented by the localized string \a s.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for any other reason (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function does not fall back to the 'C' locale if the string
    cannot be interpreted in this locale.

    \snippet code/src_corelib_text_qlocale.cpp 3

    Notice that the last conversion returns 1234.0, because '.' is the
    thousands group separator in the German locale.

    This function ignores leading and trailing whitespace.

    \sa toFloat(), toInt(), toString()
*/
#endif // QT_STRINGVIEW_LEVEL < 2

/*!
    Returns the short int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUShort(), toString()

    \since 5.10
*/

short QLocale::toShort(QStringView s, bool *ok) const
{
    return toIntegral_helper<short>(d, s, ok);
}

/*!
    Returns the unsigned short int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toShort(), toString()

    \since 5.10
*/

ushort QLocale::toUShort(QStringView s, bool *ok) const
{
    return toIntegral_helper<ushort>(d, s, ok);
}

/*!
    Returns the int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUInt(), toString()

    \since 5.10
*/

int QLocale::toInt(QStringView s, bool *ok) const
{
    return toIntegral_helper<int>(d, s, ok);
}

/*!
    Returns the unsigned int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toString()

    \since 5.10
*/

uint QLocale::toUInt(QStringView s, bool *ok) const
{
    return toIntegral_helper<uint>(d, s, ok);
}

/*!
    \since 5.13
    Returns the long int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULong(), toDouble(), toString()
*/

long QLocale::toLong(QStringView s, bool *ok) const
{
    return toIntegral_helper<long>(d, s, ok);
}

/*!
    \since 5.13
    Returns the unsigned long int represented by the localized
    string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLong(), toInt(), toDouble(), toString()
*/

ulong QLocale::toULong(QStringView s, bool *ok) const
{
    return toIntegral_helper<ulong>(d, s, ok);
}

/*!
    Returns the long long int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULongLong(), toDouble(), toString()

    \since 5.10
*/


qlonglong QLocale::toLongLong(QStringView s, bool *ok) const
{
    return toIntegral_helper<qlonglong>(d, s, ok);
}

/*!
    Returns the unsigned long long int represented by the localized
    string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLongLong(), toInt(), toDouble(), toString()

    \since 5.10
*/

qulonglong QLocale::toULongLong(QStringView s, bool *ok) const
{
    return toIntegral_helper<qulonglong>(d, s, ok);
}

/*!
    Returns the float represented by the localized string \a s.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for any other reason (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toDouble(), toInt(), toString()

    \since 5.10
*/

float QLocale::toFloat(QStringView s, bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(s, ok), ok);
}

/*!
    Returns the double represented by the localized string \a s.

    Returns an infinity if the conversion overflows or 0.0 if the
    conversion fails for any other reason (e.g. underflow).

    If \a ok is not \nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    Unlike QString::toDouble(), this function does not fall back to
    the "C" locale if the string cannot be interpreted in this
    locale.

    \snippet code/src_corelib_text_qlocale.cpp 3-qstringview

    Notice that the last conversion returns 1234.0, because '.' is the
    thousands group separator in the German locale.

    This function ignores leading and trailing whitespace.

    \sa toFloat(), toInt(), toString()

    \since 5.10
*/

double QLocale::toDouble(QStringView s, bool *ok) const
{
    return d->m_data->stringToDouble(s, ok, d->m_numberOptions);
}

/*!
    Returns a localized string representation of \a i.

    \sa toLongLong(), numberOptions(), zeroDigit(), positiveSign()
*/

QString QLocale::toString(qlonglong i) const
{
    int flags = (d->m_numberOptions & OmitGroupSeparator
                 ? 0 : QLocaleData::GroupDigits);

    return d->m_data->longLongToString(i, -1, 10, -1, flags);
}

/*!
    \overload

    \sa toULongLong(), numberOptions(), zeroDigit(), positiveSign()
*/

QString QLocale::toString(qulonglong i) const
{
    int flags = (d->m_numberOptions & OmitGroupSeparator
                 ? 0 : QLocaleData::GroupDigits);

    return d->m_data->unsLongLongToString(i, -1, 10, -1, flags);
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    Returns a localized string representation of the given \a date in the
    specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QDate::toString()
*/

QString QLocale::toString(QDate date, const QString &format) const
{
    return toString(date, qToStringViewIgnoringNull(format));
}

/*!
    Returns a localized string representation of the given \a time according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QTime::toString()
*/

QString QLocale::toString(QTime time, const QString &format) const
{
    return toString(time, qToStringViewIgnoringNull(format));
}

/*!
    \since 4.4
    \fn QString QLocale::toString(const QDateTime &dateTime, const QString &format) const

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QDateTime::toString(), QDate::toString(), QTime::toString()
*/
#endif

/*!
    \since 5.14

    Returns a localized string representation of the given \a date in the
    specified \a format, optionally for a specified calendar \a cal.
    If \a format is an empty string, an empty string is returned.

    \sa QDate::toString()
*/
QString QLocale::toString(QDate date, QStringView format, QCalendar cal) const
{
    return cal.dateTimeToString(format, QDateTime(), date, QTime(), *this);
}

/*!
    \since 5.10
    \overload
*/
QString QLocale::toString(QDate date, QStringView format) const
{
    return QCalendar().dateTimeToString(format, QDateTime(), date, QTime(), *this);
}

/*!
    \since 5.14

    Returns a localized string representation of the given \a date according
    to the specified \a format (see dateFormat()), optionally for a specified
    calendar \a cal.

    \note Some locales may use formats that limit the range of years they can
    represent.
*/
QString QLocale::toString(QDate date, FormatType format, QCalendar cal) const
{
    if (!date.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (cal.isGregorian() && d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateToStringLong
                                             : QSystemLocale::DateToStringShort,
                                             date);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = dateFormat(format);
    return toString(date, format_str, cal);
}

/*!
    \since 4.5
    \overload
*/
QString QLocale::toString(QDate date, FormatType format) const
{
    if (!date.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateToStringLong
                                             : QSystemLocale::DateToStringShort,
                                             date);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = dateFormat(format);
    return toString(date, format_str);
}

static bool timeFormatContainsAP(QStringView format)
{
    int i = 0;
    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            qt_readEscapedFormatString(format, &i);
            continue;
        }

        if (format.at(i).toLower().unicode() == 'a')
            return true;

        ++i;
    }
    return false;
}

/*!
    \since 4.5

    Returns a localized string representation of the given \a time according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QTime::toString()
*/
QString QLocale::toString(QTime time, QStringView format) const
{
    return QCalendar().dateTimeToString(format, QDateTime(), QDate(), time, *this);
}

/*!
    \since 5.14

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format, optionally for a specified calendar \a cal.
    If \a format is an empty string, an empty string is returned.

    \sa QDateTime::toString(), QDate::toString(), QTime::toString()
*/
QString QLocale::toString(const QDateTime &dateTime, QStringView format, QCalendar cal) const
{
    return cal.dateTimeToString(format, dateTime, QDate(), QTime(), *this);
}

/*!
    \since 5.10
    \overload
*/
QString QLocale::toString(const QDateTime &dateTime, QStringView format) const
{
    return QCalendar().dateTimeToString(format, dateTime, QDate(), QTime(), *this);
}

/*!
    \since 5.14

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format (see dateTimeFormat()), optionally for a
    specified calendar \a cal.

    \note Some locales may use formats that limit the range of years they can
    represent.
*/
QString QLocale::toString(const QDateTime &dateTime, FormatType format, QCalendar cal) const
{
    if (!dateTime.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (cal.isGregorian() && d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeToStringLong
                                             : QSystemLocale::DateTimeToStringShort,
                                             dateTime);
        if (!res.isNull())
            return res.toString();
    }
#endif

    const QString format_str = dateTimeFormat(format);
    return toString(dateTime, format_str, cal);
}

/*!
    \since 4.4
    \overload
*/
QString QLocale::toString(const QDateTime &dateTime, FormatType format) const
{
    if (!dateTime.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeToStringLong
                                             : QSystemLocale::DateTimeToStringShort,
                                             dateTime);
        if (!res.isNull())
            return res.toString();
    }
#endif

    const QString format_str = dateTimeFormat(format);
    return toString(dateTime, format_str);
}


/*!
    Returns a localized string representation of the given \a time in the
    specified \a format (see timeFormat()).
*/

QString QLocale::toString(QTime time, FormatType format) const
{
    if (!time.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::TimeToStringLong
                                             : QSystemLocale::TimeToStringShort,
                                             time);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = timeFormat(format);
    return toString(time, format_str);
}

/*!
    \since 4.1

    Returns the date format used for the current locale.

    If \a format is LongFormat, the format will be elaborate, otherwise it will be short.
    For example, LongFormat for the \c{en_US} locale is \c{dddd, MMMM d, yyyy},
    ShortFormat is \c{M/d/yy}.

    \sa QDate::toString(), QDate::fromString()
*/

QString QLocale::dateFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateFormatLong
                                             : QSystemLocale::DateFormatShort,
                                             QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif

    return (format == LongFormat
            ? d->m_data->longDateFormat()
            : d->m_data->shortDateFormat()
           ).getData(date_format_data);
}

/*!
    \since 4.1

    Returns the time format used for the current locale.

    If \a format is LongFormat, the format will be elaborate, otherwise it will be short.
    For example, LongFormat for the \c{en_US} locale is \c{h:mm:ss AP t},
    ShortFormat is \c{h:mm AP}.

    \sa QTime::toString(), QTime::fromString()
*/

QString QLocale::timeFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::TimeFormatLong
                                             : QSystemLocale::TimeFormatShort,
                                             QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif

    return (format == LongFormat
            ? d->m_data->longTimeFormat()
            : d->m_data->shortTimeFormat()
           ).getData(time_format_data);
}

/*!
    \since 4.4

    Returns the date time format used for the current locale.

    If \a format is LongFormat, the format will be elaborate, otherwise it will be short.
    For example, LongFormat for the \c{en_US} locale is \c{dddd, MMMM d, yyyy h:mm:ss AP t},
    ShortFormat is \c{M/d/yy h:mm AP}.

    \sa QDateTime::toString(), QDateTime::fromString()
*/

QString QLocale::dateTimeFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeFormatLong
                                             : QSystemLocale::DateTimeFormatShort,
                                             QVariant());
        if (!res.isNull()) {
            return res.toString();
        }
    }
#endif
    return dateFormat(format) + QLatin1Char(' ') + timeFormat(format);
}

#if QT_CONFIG(datestring)
/*!
    \since 4.4

    Parses the time string given in \a string and returns the
    time. The format of the time string is chosen according to the
    \a format parameter (see timeFormat()).

    If the time could not be parsed, returns an invalid time.

    \sa timeFormat(), toDate(), toDateTime(), QTime::fromString()
*/
QTime QLocale::toTime(const QString &string, FormatType format) const
{
    return toTime(string, timeFormat(format));
}

/*!
    \since 4.4

    Parses the date string given in \a string and returns the
    date. The format of the date string is chosen according to the
    \a format parameter (see dateFormat()).

    If the date could not be parsed, returns an invalid date.

    \sa dateFormat(), toTime(), toDateTime(), QDate::fromString()
*/
QDate QLocale::toDate(const QString &string, FormatType format) const
{
    return toDate(string, dateFormat(format));
}

/*!
    \since 5.14
    \overload
*/
QDate QLocale::toDate(const QString &string, FormatType format, QCalendar cal) const
{
    return toDate(string, dateFormat(format), cal);
}

/*!
    \since 4.4

    Parses the date/time string given in \a string and returns the
    time. The format of the date/time string is chosen according to the
    \a format parameter (see dateTimeFormat()).

    If the string could not be parsed, returns an invalid QDateTime.

    \sa dateTimeFormat(), toTime(), toDate(), QDateTime::fromString()
*/
QDateTime QLocale::toDateTime(const QString &string, FormatType format) const
{
    return toDateTime(string, dateTimeFormat(format));
}

/*!
    \since 5.14
    \overload
*/
QDateTime QLocale::toDateTime(const QString &string, FormatType format, QCalendar cal) const
{
    return toDateTime(string, dateTimeFormat(format), cal);
}

/*!
    \since 4.4

    Parses the time string given in \a string and returns the
    time. See QTime::fromString() for information on what is a valid
    format string.

    If the time could not be parsed, returns an invalid time.

    \sa timeFormat(), toDate(), toDateTime(), QTime::fromString()
*/
QTime QLocale::toTime(const QString &string, const QString &format) const
{
    QTime time;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QMetaType::QTime, QDateTimeParser::FromString, QCalendar());
    dt.setDefaultLocale(*this);
    if (dt.parseFormat(format))
        dt.fromString(string, nullptr, &time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return time;
}

/*!
    \since 4.4

    Parses the date string given in \a string and returns the
    date. See QDate::fromString() for information on the expressions
    that can be used with this function.

    This function searches month names and the names of the days of
    the week in the current locale.

    If the date could not be parsed, returns an invalid date.

    \sa dateFormat(), toTime(), toDateTime(), QDate::fromString()
*/
QDate QLocale::toDate(const QString &string, const QString &format) const
{
    return toDate(string, format, QCalendar());
}

/*!
    \since 5.14
    \overload
*/
QDate QLocale::toDate(const QString &string, const QString &format, QCalendar cal) const
{
    QDate date;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QMetaType::QDate, QDateTimeParser::FromString, cal);
    dt.setDefaultLocale(*this);
    if (dt.parseFormat(format))
        dt.fromString(string, &date, nullptr);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
    Q_UNUSED(cal);
#endif
    return date;
}

/*!
    \since 4.4

    Parses the date/time string given in \a string and returns the
    time.  See QDateTime::fromString() for information on the expressions
    that can be used with this function.

    \note The month and day names used must be given in the user's local
    language.

    If the string could not be parsed, returns an invalid QDateTime.  If the
    string can be parsed and represents an invalid date-time (e.g. in a gap
    skipped by a time-zone transition), an invalid QDateTime is returned, whose
    toMSecsSinceEpoch() represents a near-by date-time that is valid. Passing
    that to fromMSecsSinceEpoch() will produce a valid date-time that isn't
    faithfully represented by the string parsed.

    \sa dateTimeFormat(), toTime(), toDate(), QDateTime::fromString()
*/
QDateTime QLocale::toDateTime(const QString &string, const QString &format) const
{
    return toDateTime(string, format, QCalendar());
}

/*!
    \since 5.14
    \overload
*/
QDateTime QLocale::toDateTime(const QString &string, const QString &format, QCalendar cal) const
{
#if QT_CONFIG(datetimeparser)
    QDateTime datetime;

    QDateTimeParser dt(QMetaType::QDateTime, QDateTimeParser::FromString, cal);
    dt.setDefaultLocale(*this);
    if (dt.parseFormat(format) && (dt.fromString(string, &datetime) || !datetime.isValid()))
        return datetime;
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
    Q_UNUSED(cal);
#endif
    return QDateTime();
}
#endif // datestring

/*!
    \since 4.1

    Returns the decimal point character of this locale.

    \sa groupSeparator(), toString()
*/
QString QLocale::decimalPoint() const
{
    return d->m_data->decimalPoint();
}

/*!
    \since 4.1

    Returns the group separator character of this locale.

    \sa decimalPoint(), toString()
*/
QString QLocale::groupSeparator() const
{
    return d->m_data->groupSeparator();
}

/*!
    \since 4.1

    Returns the percent character of this locale.

    \sa toString()
*/
QString QLocale::percent() const
{
    return d->m_data->percentSign();
}

/*!
    \since 4.1

    Returns the zero digit character of this locale.

    \sa toString()
*/
QString QLocale::zeroDigit() const
{
    return d->m_data->zeroDigit();
}

/*!
    \since 4.1

    Returns the negative sign character of this locale.

    \sa positiveSign(), toString()
*/
QString QLocale::negativeSign() const
{
    return d->m_data->negativeSign();
}

/*!
    \since 4.5

    Returns the positive sign character of this locale.

    \sa negativeSign(), toString()
*/
QString QLocale::positiveSign() const
{
    return d->m_data->positiveSign();
}

/*!
    \since 4.1

    Returns the exponential character of this locale, used to separate exponent
    from mantissa in some floating-point numeric representations.

    \sa toString(double, char, int)
*/
QString QLocale::exponential() const
{
    return d->m_data->exponentSeparator();
}

static bool qIsUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static char qToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    else
        return c;
}

/*!
    \overload
    Returns a string representing the floating-point number \a f.

    The form of the representation is controlled by the \a format and \a
    precision parameters.

    The \a format defaults to \c{'g'}. It can be any of the following:

    \table
    \header \li Format \li Meaning
    \row \li \c 'e' \li format as [-]9.9e[+|-]999
    \row \li \c 'E' \li format as [-]9.9E[+|-]999
    \row \li \c 'f' \li format as [-]9.9
    \row \li \c 'g' \li use \c 'e' or \c 'f' format, whichever is more concise
    \row \li \c 'G' \li use \c 'E' or \c 'f' format, whichever is more concise
    \endtable

    For the \c 'e', \c 'E', and \c 'f' formats, the \a precision represents the
    number of digits \e after the decimal point. For the \c 'g' and \c 'G'
    formats, the \a precision represents the maximum number of significant
    digits (trailing zeroes are omitted). The special \a precision value
    QLocale::FloatingPointShortest selects the shortest representation that,
    when read as a number, gets back the original floating-point value. Aside
    from that, any negative \a precision is ignored in favor of the default, 6.

    For the \c 'e', \c 'f' and \c 'g' formats, positive infinity is represented
    as "inf", negative infinity as "-inf" and floating-point NaN (not-a-number)
    values are represented as "nan". For the \c 'E' and \c 'G' formats, "INF"
    and "NAN" are used instead. This does not vary with locale.

    \sa toDouble(), numberOptions(), exponential(), decimalPoint(), zeroDigit(),
        positiveSign(), percent(), toCurrencyString(), formattedDataSize(),
        QLocale::FloatingPointPrecisionOption
*/

QString QLocale::toString(double f, char format, int precision) const
{
    QLocaleData::DoubleForm form = QLocaleData::DFDecimal;
    uint flags = qIsUpper(format) ? QLocaleData::CapitalEorX : 0;

    switch (qToLower(format)) {
        case 'f':
            form = QLocaleData::DFDecimal;
            break;
        case 'e':
            form = QLocaleData::DFExponent;
            break;
        case 'g':
            form = QLocaleData::DFSignificantDigits;
            break;
        default:
            break;
    }

    if (!(d->m_numberOptions & OmitGroupSeparator))
        flags |= QLocaleData::GroupDigits;
    if (!(d->m_numberOptions & OmitLeadingZeroInExponent))
        flags |= QLocaleData::ZeroPadExponent;
    if (d->m_numberOptions & IncludeTrailingZeroesAfterDot)
        flags |= QLocaleData::AddTrailingZeroes;
    return d->m_data->doubleToString(f, precision, form, -1, flags);
}

/*!
    \fn QLocale QLocale::c()

    Returns a QLocale object initialized to the "C" locale.

    This locale is based on en_US but with various quirks of its own, such as
    simplified number formatting and its own date formatting. It implements the
    POSIX standards that describe the behavior of standard library functions of
    the "C" programming language.

    Among other things, this means its collation order is based on the ASCII
    values of letters, so that (for case-sensitive sorting) all upper-case
    letters sort before any lower-case one (rather than each letter's upper- and
    lower-case forms sorting adjacent to one another, before the next letter's
    two forms).

    \sa system()
*/

/*!
    Returns a QLocale object initialized to the system locale.

    The system locale may use system-specific sources for locale data, where
    available, otherwise falling back on QLocale's built-in database entry for
    the language, script and territory the system reports.

    For example, on Windows and Mac, this locale will use the decimal/grouping
    characters and date/time formats specified in the system configuration
    panel.

    \sa c()
*/

QLocale QLocale::system()
{
    QT_PREPEND_NAMESPACE(systemData)(); // Ensure system data is up to date.
    static QLocalePrivate locale(systemData(), defaultIndex(), DefaultNumberOptions, 1);

    return QLocale(locale);
}

/*!
    Returns a list of valid locale objects that match the given \a language, \a
    script and \a territory.

    Getting a list of all locales:
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                                         QLocale::AnyTerritory);

    Getting a list of locales suitable for Russia:
    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript,
                                                      QLocale::Russia);
*/
QList<QLocale> QLocale::matchingLocales(QLocale::Language language, QLocale::Script script,
                                        QLocale::Territory territory)
{
    const QLocaleId filter { language, script, territory };
    if (!filter.isValid())
        return QList<QLocale>();

    if (language == QLocale::C)
        return QList<QLocale>() << QLocale(QLocale::C);

    QList<QLocale> result;
    if (filter.matchesAll())
        result.reserve(locale_data_size);

    quint16 index = locale_index[language];
    // There may be no matches, for some languages (e.g. Abkhazian at CLDR v39).
    while (filter.acceptLanguage(locale_data[index].m_language_id)) {
        const QLocaleId id = locale_data[index].id();
        if (filter.acceptScriptTerritory(id)) {
            result.append(QLocale(*(id.language_id == C ? c_private()
                                    : new QLocalePrivate(locale_data + index, index))));
        }
        ++index;
    }

    return result;
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use \l matchingLocales() instead and consult the \l territory() of each.
    \since 4.3

    Returns the list of countries that have entries for \a language in Qt's locale
    database. If the result is an empty list, then \a language is not represented in
    Qt's locale database.

    \sa matchingLocales()
*/
QList<QLocale::Country> QLocale::countriesForLanguage(Language language)
{
    const auto locales = matchingLocales(language, AnyScript, AnyCountry);
    QList<QLocale::Country> result;
    result.reserve(locales.size());
    for (const auto &locale : locales)
        result.append(locale.territory());
    return result;
}
#endif

/*!
    \since 4.2

    Returns the localized name of \a month, in the format specified
    by \a type.

    For example, if the locale is \c en_US and \a month is 1,
    \l LongFormat will return \c January. \l ShortFormat \c Jan,
    and \l NarrowFormat \c J.

    \sa dayName(), standaloneMonthName()
*/
QString QLocale::monthName(int month, FormatType type) const
{
    return QCalendar().monthName(*this, month, QCalendar::Unspecified, type);
}

/*!
    \since 4.5

    Returns the localized name of \a month that is used as a
    standalone text, in the format specified by \a type.

    If the locale information doesn't specify the standalone month
    name then return value is the same as in monthName().

    \sa monthName(), standaloneDayName()
*/
QString QLocale::standaloneMonthName(int month, FormatType type) const
{
    return QCalendar().standaloneMonthName(*this, month, QCalendar::Unspecified, type);
}

/*!
    \since 4.2

    Returns the localized name of the \a day (where 1 represents
    Monday, 2 represents Tuesday and so on), in the format specified
    by \a type.

    For example, if the locale is \c en_US and \a day is 1,
    \l LongFormat will return \c Monday, \l ShortFormat \c Mon,
    and \l NarrowFormat \c M.

    \sa monthName(), standaloneDayName()
*/
QString QLocale::dayName(int day, FormatType type) const
{
    return QCalendar().weekDayName(*this, day, type);
}

/*!
    \since 4.5

    Returns the localized name of the \a day (where 1 represents
    Monday, 2 represents Tuesday and so on) that is used as a
    standalone text, in the format specified by \a type.

    If the locale information does not specify the standalone day
    name then return value is the same as in dayName().

    \sa dayName(), standaloneMonthName()
*/
QString QLocale::standaloneDayName(int day, FormatType type) const
{
    return QCalendar().standaloneWeekDayName(*this, day, type);
}

// Calendar look-up of month and day names:

/*!
  \internal
 */

static QString rawMonthName(const QCalendarLocale &localeData,
                            const char16_t *monthsData, int month,
                            QLocale::FormatType type)
{
    QLocaleData::DataRange range;
    switch (type) {
    case QLocale::LongFormat:
        range = localeData.longMonth();
        break;
    case QLocale::ShortFormat:
        range = localeData.shortMonth();
        break;
    case QLocale::NarrowFormat:
        range = localeData.narrowMonth();
        break;
    default:
        return QString();
    }
    return range.getListEntry(monthsData, month - 1);
}

/*!
  \internal
 */

static QString rawStandaloneMonthName(const QCalendarLocale &localeData,
                                      const char16_t *monthsData, int month,
                                      QLocale::FormatType type)
{
    QLocaleData::DataRange range;
    switch (type) {
    case QLocale::LongFormat:
        range = localeData.longMonthStandalone();
        break;
    case QLocale::ShortFormat:
        range = localeData.shortMonthStandalone();
        break;
    case QLocale::NarrowFormat:
        range = localeData.narrowMonthStandalone();
        break;
    default:
        return QString();
    }
    QString name = range.getListEntry(monthsData, month - 1);
    return name.isEmpty() ? rawMonthName(localeData, monthsData, month, type) : name;
}

/*!
  \internal
 */

static QString rawWeekDayName(const QLocaleData *data, const int day,
                              QLocale::FormatType type)
{
    QLocaleData::DataRange range;
    switch (type) {
    case QLocale::LongFormat:
        range = data->longDayNames();
        break;
    case QLocale::ShortFormat:
        range = data->shortDayNames();
        break;
    case QLocale::NarrowFormat:
        range = data->narrowDayNames();
        break;
    default:
        return QString();
    }
    return range.getListEntry(days_data, day == 7 ? 0 : day);
}

/*!
  \internal
 */

static QString rawStandaloneWeekDayName(const QLocaleData *data, const int day,
                                        QLocale::FormatType type)
{
    QLocaleData::DataRange range;
    switch (type) {
    case QLocale::LongFormat:
        range =data->longDayNamesStandalone();
        break;
    case QLocale::ShortFormat:
        range = data->shortDayNamesStandalone();
        break;
    case QLocale::NarrowFormat:
        range = data->narrowDayNamesStandalone();
        break;
    default:
        return QString();
    }
    QString name = range.getListEntry(days_data, day == 7 ? 0 : day);
    if (name.isEmpty())
        return rawWeekDayName(data, day, type);
    return name;
}

// Refugees from qcalendar.cpp that need functions above:

QString QCalendarBackend::monthName(const QLocale &locale, int month, int,
                                    QLocale::FormatType format) const
{
    Q_ASSERT(month >= 1 && month <= maximumMonthsInYear());
    return rawMonthName(localeMonthIndexData()[locale.d->m_index],
                        localeMonthData(), month, format);
}

QString QGregorianCalendar::monthName(const QLocale &locale, int month, int year,
                                      QLocale::FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (locale.d->m_data == &systemLocaleData) {
        Q_ASSERT(month >= 1 && month <= 12);
        QSystemLocale::QueryType queryType = QSystemLocale::MonthNameLong;
        switch (format) {
        case QLocale::LongFormat:
            queryType = QSystemLocale::MonthNameLong;
            break;
        case QLocale::ShortFormat:
            queryType = QSystemLocale::MonthNameShort;
            break;
        case QLocale::NarrowFormat:
            queryType = QSystemLocale::MonthNameNarrow;
            break;
        }
        QVariant res = systemLocale()->query(queryType, month);
        if (!res.isNull())
            return res.toString();
    }
#endif

    return QCalendarBackend::monthName(locale, month, year, format);
}

QString QCalendarBackend::standaloneMonthName(const QLocale &locale, int month, int,
                                              QLocale::FormatType format) const
{
    Q_ASSERT(month >= 1 && month <= maximumMonthsInYear());
    return rawStandaloneMonthName(localeMonthIndexData()[locale.d->m_index],
                                  localeMonthData(), month, format);
}

QString QGregorianCalendar::standaloneMonthName(const QLocale &locale, int month, int year,
                                                QLocale::FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (locale.d->m_data == &systemLocaleData) {
        Q_ASSERT(month >= 1 && month <= 12);
        QSystemLocale::QueryType queryType = QSystemLocale::StandaloneMonthNameLong;
        switch (format) {
        case QLocale::LongFormat:
            queryType = QSystemLocale::StandaloneMonthNameLong;
            break;
        case QLocale::ShortFormat:
            queryType = QSystemLocale::StandaloneMonthNameShort;
            break;
        case QLocale::NarrowFormat:
            queryType = QSystemLocale::StandaloneMonthNameNarrow;
            break;
        }
        QVariant res = systemLocale()->query(queryType, month);
        if (!res.isNull())
            return res.toString();
    }
#endif

    return QCalendarBackend::standaloneMonthName(locale, month, year, format);
}

// Most calendars share the common week-day naming, modulo locale.
// Calendars that don't must override these methods.
QString QCalendarBackend::weekDayName(const QLocale &locale, int day,
                                      QLocale::FormatType format) const
{
    if (day < 1 || day > 7)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (locale.d->m_data == &systemLocaleData) {
        QSystemLocale::QueryType queryType = QSystemLocale::DayNameLong;
        switch (format) {
        case QLocale::LongFormat:
            queryType = QSystemLocale::DayNameLong;
            break;
        case QLocale::ShortFormat:
            queryType = QSystemLocale::DayNameShort;
            break;
        case QLocale::NarrowFormat:
            queryType = QSystemLocale::DayNameNarrow;
            break;
        }
        QVariant res = systemLocale()->query(queryType, day);
        if (!res.isNull())
            return res.toString();
    }
#endif

    return rawWeekDayName(locale.d->m_data, day, format);
}

QString QCalendarBackend::standaloneWeekDayName(const QLocale &locale, int day,
                                                QLocale::FormatType format) const
{
    if (day < 1 || day > 7)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (locale.d->m_data == &systemLocaleData) {
        QSystemLocale::QueryType queryType = QSystemLocale::StandaloneDayNameLong;
        switch (format) {
        case QLocale::LongFormat:
            queryType = QSystemLocale::StandaloneDayNameLong;
            break;
        case QLocale::ShortFormat:
            queryType = QSystemLocale::StandaloneDayNameShort;
            break;
        case QLocale::NarrowFormat:
            queryType = QSystemLocale::StandaloneDayNameNarrow;
            break;
        }
        QVariant res = systemLocale()->query(queryType, day);
        if (!res.isNull())
            return res.toString();
    }
#endif

    return rawStandaloneWeekDayName(locale.d->m_data, day, format);
}

// End of this block of qcalendar.cpp refugees.  (One more follows.)

/*!
    \since 4.8

    Returns the first day of the week according to the current locale.
*/
Qt::DayOfWeek QLocale::firstDayOfWeek() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        const auto res = systemLocale()->query(QSystemLocale::FirstDayOfWeek);
        if (!res.isNull())
            return static_cast<Qt::DayOfWeek>(res.toUInt());
    }
#endif
    return static_cast<Qt::DayOfWeek>(d->m_data->m_first_day_of_week);
}

QLocale::MeasurementSystem QLocalePrivate::measurementSystem() const
{
    for (int i = 0; i < ImperialMeasurementSystemsCount; ++i) {
        if (ImperialMeasurementSystems[i].languageId == m_data->m_language_id
            && ImperialMeasurementSystems[i].territoryId == m_data->m_territory_id) {
            return ImperialMeasurementSystems[i].system;
        }
    }
    return QLocale::MetricSystem;
}

/*!
    \since 4.8

    Returns a list of days that are considered weekdays according to the current locale.
*/
QList<Qt::DayOfWeek> QLocale::weekdays() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        auto res
            = qvariant_cast<QList<Qt::DayOfWeek> >(systemLocale()->query(QSystemLocale::Weekdays));
        if (!res.isEmpty())
            return res;
    }
#endif
    QList<Qt::DayOfWeek> weekdays;
    quint16 weekendStart = d->m_data->m_weekend_start;
    quint16 weekendEnd = d->m_data->m_weekend_end;
    for (int day = Qt::Monday; day <= Qt::Sunday; day++) {
        if ((weekendEnd >= weekendStart && (day < weekendStart || day > weekendEnd)) ||
            (weekendEnd < weekendStart && (day > weekendEnd && day < weekendStart)))
                weekdays << static_cast<Qt::DayOfWeek>(day);
    }
    return weekdays;
}

/*!
    \since 4.4

    Returns the measurement system for the locale.
*/
QLocale::MeasurementSystem QLocale::measurementSystem() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        const auto res = systemLocale()->query(QSystemLocale::MeasurementSystem);
        if (!res.isNull())
            return MeasurementSystem(res.toInt());
    }
#endif

    return d->measurementSystem();
}

/*!
  \since 4.7

  Returns the text direction of the language.
*/
Qt::LayoutDirection QLocale::textDirection() const
{
    switch (script()) {
    case QLocale::AdlamScript:
    case QLocale::ArabicScript:
    case QLocale::AvestanScript:
    case QLocale::CypriotScript:
    case QLocale::HatranScript:
    case QLocale::HebrewScript:
    case QLocale::ImperialAramaicScript:
    case QLocale::InscriptionalPahlaviScript:
    case QLocale::InscriptionalParthianScript:
    case QLocale::KharoshthiScript:
    case QLocale::LydianScript:
    case QLocale::MandaeanScript:
    case QLocale::ManichaeanScript:
    case QLocale::MendeKikakuiScript:
    case QLocale::MeroiticCursiveScript:
    case QLocale::MeroiticScript:
    case QLocale::NabataeanScript:
    case QLocale::NkoScript:
    case QLocale::OldHungarianScript:
    case QLocale::OldNorthArabianScript:
    case QLocale::OldSouthArabianScript:
    case QLocale::OrkhonScript:
    case QLocale::PalmyreneScript:
    case QLocale::PhoenicianScript:
    case QLocale::PsalterPahlaviScript:
    case QLocale::SamaritanScript:
    case QLocale::SyriacScript:
    case QLocale::ThaanaScript:
        return Qt::RightToLeft;
    default:
        break;
    }
    return Qt::LeftToRight;
}

/*!
  \since 4.8

  Returns an uppercase copy of \a str.

  If Qt Core is using the ICU libraries, they will be used to perform
  the transformation according to the rules of the current locale.
  Otherwise the conversion may be done in a platform-dependent manner,
  with QString::toUpper() as a generic fallback.

  \sa QString::toUpper()
*/
QString QLocale::toUpper(const QString &str) const
{
#if QT_CONFIG(icu)
    bool ok = true;
    QString result = QIcu::toUpper(d->bcp47Name('_'), str, &ok);
    if (ok)
        return result;
    // else fall through and use Qt's toUpper
#endif
    return str.toUpper();
}

/*!
  \since 4.8

  Returns a lowercase copy of \a str.

  If Qt Core is using the ICU libraries, they will be used to perform
  the transformation according to the rules of the current locale.
  Otherwise the conversion may be done in a platform-dependent manner,
  with QString::toLower() as a generic fallback.

  \sa QString::toLower()
*/
QString QLocale::toLower(const QString &str) const
{
#if QT_CONFIG(icu)
    bool ok = true;
    const QString result = QIcu::toLower(d->bcp47Name('_'), str, &ok);
    if (ok)
        return result;
    // else fall through and use Qt's toUpper
#endif
    return str.toLower();
}


/*!
    \since 4.5

    Returns the localized name of the "AM" suffix for times specified using
    the conventions of the 12-hour clock.

    \sa pmText()
*/
QString QLocale::amText() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::AMText).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return d->m_data->anteMeridiem().getData(am_data);
}

/*!
    \since 4.5

    Returns the localized name of the "PM" suffix for times specified using
    the conventions of the 12-hour clock.

    \sa amText()
*/
QString QLocale::pmText() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::PMText).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return d->m_data->postMeridiem().getData(pm_data);
}

// Another intrusion from QCalendar, using some of the tools above:

QString QCalendarBackend::dateTimeToString(QStringView format, const QDateTime &datetime,
                                           QDate dateOnly, QTime timeOnly,
                                           const QLocale &locale) const
{
    QDate date;
    QTime time;
    bool formatDate = false;
    bool formatTime = false;
    if (datetime.isValid()) {
        date = datetime.date();
        time = datetime.time();
        formatDate = true;
        formatTime = true;
    } else if (dateOnly.isValid()) {
        date = dateOnly;
        formatDate = true;
    } else if (timeOnly.isValid()) {
        time = timeOnly;
        formatTime = true;
    } else {
        return QString();
    }

    QString result;
    int year = 0, month = 0, day = 0;
    if (formatDate) {
        const auto parts = julianDayToDate(date.toJulianDay());
        if (!parts.isValid())
            return QString();
        year = parts.year;
        month = parts.month;
        day = parts.day;
    }

    int i = 0;
    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            result.append(qt_readEscapedFormatString(format, &i));
            continue;
        }

        const QChar c = format.at(i);
        int repeat = qt_repeatCount(format.mid(i));
        bool used = false;
        if (formatDate) {
            switch (c.unicode()) {
            case 'y':
                used = true;
                if (repeat >= 4)
                    repeat = 4;
                else if (repeat >= 2)
                    repeat = 2;

                switch (repeat) {
                case 4: {
                    const int len = (year < 0) ? 5 : 4;
                    result.append(locale.d->m_data->longLongToString(year, -1, 10, len,
                                                                     QLocaleData::ZeroPadded));
                    break;
                }
                case 2:
                    result.append(locale.d->m_data->longLongToString(year % 100, -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                default:
                    repeat = 1;
                    result.append(c);
                    break;
                }
                break;

            case 'M':
                used = true;
                repeat = qMin(repeat, 4);
                switch (repeat) {
                case 1:
                    result.append(locale.d->m_data->longLongToString(month));
                    break;
                case 2:
                    result.append(locale.d->m_data->longLongToString(month, -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                case 3:
                    result.append(monthName(locale, month, year, QLocale::ShortFormat));
                    break;
                case 4:
                    result.append(monthName(locale, month, year, QLocale::LongFormat));
                    break;
                }
                break;

            case 'd':
                used = true;
                repeat = qMin(repeat, 4);
                switch (repeat) {
                case 1:
                    result.append(locale.d->m_data->longLongToString(day));
                    break;
                case 2:
                    result.append(locale.d->m_data->longLongToString(day, -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                case 3:
                    result.append(locale.dayName(
                                      dayOfWeek(date.toJulianDay()), QLocale::ShortFormat));
                    break;
                case 4:
                    result.append(locale.dayName(
                                      dayOfWeek(date.toJulianDay()), QLocale::LongFormat));
                    break;
                }
                break;

            default:
                break;
            }
        }
        if (!used && formatTime) {
            switch (c.unicode()) {
            case 'h': {
                used = true;
                repeat = qMin(repeat, 2);
                int hour = time.hour();
                if (timeFormatContainsAP(format)) {
                    if (hour > 12)
                        hour -= 12;
                    else if (hour == 0)
                        hour = 12;
                }

                switch (repeat) {
                case 1:
                    result.append(locale.d->m_data->longLongToString(hour));
                    break;
                case 2:
                    result.append(locale.d->m_data->longLongToString(hour, -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                }
                break;
            }
            case 'H':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(locale.d->m_data->longLongToString(time.hour()));
                    break;
                case 2:
                    result.append(locale.d->m_data->longLongToString(time.hour(), -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                }
                break;

            case 'm':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(locale.d->m_data->longLongToString(time.minute()));
                    break;
                case 2:
                    result.append(locale.d->m_data->longLongToString(time.minute(), -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                }
                break;

            case 's':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(locale.d->m_data->longLongToString(time.second()));
                    break;
                case 2:
                    result.append(locale.d->m_data->longLongToString(time.second(), -1, 10, 2,
                                                                     QLocaleData::ZeroPadded));
                    break;
                }
                break;

            case 'a':
                used = true;
                repeat = format.mid(i + 1).startsWith(QLatin1Char('p')) ? 2 : 1;
                result.append(time.hour() < 12 ? locale.amText().toLower()
                                               : locale.pmText().toLower());
                break;

            case 'A':
                used = true;
                repeat = format.mid(i + 1).startsWith(QLatin1Char('P')) ? 2 : 1;
                result.append(time.hour() < 12 ? locale.amText().toUpper()
                                                : locale.pmText().toUpper());
                break;

            case 'z':
                used = true;
                repeat = (repeat >= 3) ? 3 : 1;

                // note: the millisecond component is treated like the decimal part of the seconds
                // so ms == 2 is always printed as "002", but ms == 200 can be either "2" or "200"
                result.append(locale.d->m_data->longLongToString(time.msec(), -1, 10, 3,
                                                                 QLocaleData::ZeroPadded));
                if (repeat == 1) {
                    if (result.endsWith(locale.zeroDigit()))
                        result.chop(1);
                    if (result.endsWith(locale.zeroDigit()))
                        result.chop(1);
                }
                break;

            case 't':
                used = true;
                repeat = 1;
                // If we have a QDateTime use the time spec otherwise use the current system tzname
                result.append(formatDate ? datetime.timeZoneAbbreviation()
                                         : QDateTime::currentDateTime().timeZoneAbbreviation());
                break;

            default:
                break;
            }
        }
        if (!used)
            result.append(QString(repeat, c));
        i += repeat;
    }

    return result;
}

// End of QCalendar intrustions

QString QLocaleData::doubleToString(double d, int precision, DoubleForm form,
                                    int width, unsigned flags) const
{
    // Although the special handling of F.P.Shortest below is limited to
    // DFSignificantDigits, the double-conversion library does treat it
    // specially for the other forms, shedding trailing zeros for DFDecimal and
    // using the shortest mantissa that faithfully represents the value for
    // DFExponent.
    if (precision != QLocale::FloatingPointShortest && precision < 0)
        precision = 6;
    if (width < 0)
        width = 0;

    int decpt;
    int bufSize = 1;
    if (precision == QLocale::FloatingPointShortest)
        bufSize += std::numeric_limits<double>::max_digits10;
    else if (form == DFDecimal && qIsFinite(d))
        bufSize += wholePartSpace(qAbs(d)) + precision;
    else // Add extra digit due to different interpretations of precision.
        bufSize += qMax(2, precision) + 1; // Must also be big enough for "nan" or "inf"

    QVarLengthArray<char> buf(bufSize);
    int length;
    bool negative = false;
    qt_doubleToAscii(d, form, precision, buf.data(), bufSize, negative, length, decpt);

    const QString prefix = signPrefix(negative && !isZero(d), flags);
    QString numStr;

    if (qstrncmp(buf.data(), "inf", 3) == 0 || qstrncmp(buf.data(), "nan", 3) == 0) {
        numStr = QString::fromLatin1(buf.data(), length);
    } else { // Handle finite values
        const QString zero = zeroDigit();
        QString digits = QString::fromLatin1(buf.data(), length);

        if (zero == u"0") {
            // No need to convert digits.
        } else if (zero.size() == 2 && zero.at(0).isHighSurrogate()) {
            const char32_t zeroUcs4 = QChar::surrogateToUcs4(zero.at(0), zero.at(1));
            QString converted;
            converted.reserve(2 * digits.size());
            for (int i = 0; i < digits.length(); ++i) {
                const char32_t digit = unicodeForDigit(digits.at(i).unicode() - '0', zeroUcs4);
                Q_ASSERT(QChar::requiresSurrogates(digit));
                converted.append(QChar::highSurrogate(digit));
                converted.append(QChar::lowSurrogate(digit));
            }
            digits = converted;
        } else {
            Q_ASSERT(zero.size() == 1);
            Q_ASSERT(!zero.at(0).isSurrogate());
            char16_t z = zero.at(0).unicode();
            char16_t *const value = reinterpret_cast<char16_t *>(digits.data());
            for (int i = 0; i < digits.length(); ++i)
                value[i] = unicodeForDigit(value[i] - '0', z);
        }

        const bool mustMarkDecimal = flags & ForcePoint;
        const bool groupDigits = flags & GroupDigits;
        const int minExponentDigits = flags & ZeroPadExponent ? 2 : 1;
        switch (form) {
            case DFExponent:
                numStr = exponentForm(std::move(digits), decpt, precision, PMDecimalDigits,
                                      mustMarkDecimal, minExponentDigits);
                break;
            case DFDecimal:
                numStr = decimalForm(std::move(digits), decpt, precision, PMDecimalDigits,
                                     mustMarkDecimal, groupDigits);
                break;
            case DFSignificantDigits: {
                PrecisionMode mode = (flags & AddTrailingZeroes) ?
                            PMSignificantDigits : PMChopTrailingZeros;

                /* POSIX specifies sprintf() to follow fprintf(), whose 'g/G'
                   format says; with P = 6 if precision unspecified else 1 if
                   precision is 0 else precision; when 'e/E' would have exponent
                   X, use:
                     * 'f/F' if P > X >= -4, with precision P-1-X
                     * 'e/E' otherwise, with precision P-1
                   Helpfully, we already have mapped precision < 0 to 6 - except
                   for F.P.Shortest mode, which is its own story - and those of
                   our callers with unspecified precision either used 6 or -1
                   for it.
                */
                bool useDecimal;
                if (precision == QLocale::FloatingPointShortest) {
                    // Find out which representation is shorter.
                    // Set bias to everything added to exponent form but not
                    // decimal, minus the converse.

                    // Exponent adds separator, sign and digits:
                    int bias = 2 + minExponentDigits;
                    // Decimal form may get grouping separators inserted:
                    if (groupDigits && decpt >= m_grouping_top + m_grouping_least)
                        bias -= (decpt - m_grouping_top - m_grouping_least) / m_grouping_higher + 1;
                    // X = decpt - 1 needs two digits if decpt > 10:
                    if (decpt > 10 && minExponentDigits == 1)
                        ++bias;
                    // Assume digitCount < 95, so we can ignore the 3-digit
                    // exponent case (we'll set useDecimal false anyway).

                    const int digitCount = digits.length() / zero.size();
                    if (!mustMarkDecimal) {
                        // Decimal separator is skipped if at end; adjust if
                        // that happens for only one form:
                        if (digitCount <= decpt && digitCount > 1)
                            ++bias; // decimal but not exponent
                        else if (digitCount == 1 && decpt <= 0)
                            --bias; // exponent but not decimal
                    }
                    // When 0 < decpt <= digitCount, the forms have equal digit
                    // counts, plus things bias has taken into account;
                    // otherwise decimal form's digit count is right-padded with
                    // zeros to decpt, when decpt is positive, otherwise it's
                    // left-padded with 1 - decpt zeros.
                    useDecimal = (decpt <= 0 ? 1 - decpt <= bias
                                  : decpt <= digitCount ? 0 <= bias
                                  : decpt <= digitCount + bias);
                } else {
                    // X == decpt - 1, POSIX's P; -4 <= X < P iff -4 < decpt <= P
                    Q_ASSERT(precision >= 0);
                    useDecimal = decpt > -4 && decpt <= (precision ? precision : 1);
                }

                numStr = useDecimal
                    ? decimalForm(std::move(digits), decpt, precision, mode,
                                  mustMarkDecimal, groupDigits)
                    : exponentForm(std::move(digits), decpt, precision, mode,
                                   mustMarkDecimal, minExponentDigits);
                break;
            }
        }

        // Pad with zeros. LeftAdjusted overrides ZeroPadded.
        if (flags & ZeroPadded && !(flags & LeftAdjusted)) {
            for (int i = numStr.length() / zero.length() + prefix.size(); i < width; ++i)
                numStr.prepend(zero);
        }
    }

    return prefix + (flags & CapitalEorX ? std::move(numStr).toUpper() : numStr);
}

QString QLocaleData::decimalForm(QString &&digits, int decpt, int precision,
                                 PrecisionMode pm, bool mustMarkDecimal,
                                 bool groupDigits) const
{
    const QString zero = zeroDigit();
    const auto digitWidth = zero.size();
    Q_ASSERT(digitWidth == 1 || digitWidth == 2);
    Q_ASSERT(digits.size() % digitWidth == 0);

    // Separator needs to go at index decpt: so add zeros before or after the
    // given digits, if they don't reach that position already:
    if (decpt < 0) {
        for (; decpt < 0; ++decpt)
            digits.prepend(zero);
    } else {
        for (int i = digits.length() / digitWidth; i < decpt; ++i)
            digits.append(zero);
    }

    switch (pm) {
    case PMDecimalDigits:
        for (int i = digits.length() / digitWidth - decpt; i < precision; ++i)
            digits.append(zero);
        break;
    case  PMSignificantDigits:
        for (int i = digits.length() / digitWidth; i < precision; ++i)
            digits.append(zero);
        break;
    case PMChopTrailingZeros:
        Q_ASSERT(digits.length() / digitWidth <= qMax(decpt, 1) || !digits.endsWith(zero));
        break;
    }

    if (mustMarkDecimal || decpt < digits.length() / digitWidth)
        digits.insert(decpt * digitWidth, decimalPoint());

    if (groupDigits) {
        const QString group = groupSeparator();
        int i = decpt - m_grouping_least;
        if (i >= m_grouping_top) {
            digits.insert(i * digitWidth, group);
            while ((i -= m_grouping_higher) >= m_grouping_top)
                digits.insert(i * digitWidth, group);
        }
    }

    if (decpt == 0)
        digits.prepend(zero);

    return std::move(digits);
}

QString QLocaleData::exponentForm(QString &&digits, int decpt, int precision,
                                  PrecisionMode pm, bool mustMarkDecimal,
                                  int minExponentDigits) const
{
    const QString zero = zeroDigit();
    const auto digitWidth = zero.size();
    Q_ASSERT(digitWidth == 1 || digitWidth == 2);
    Q_ASSERT(digits.size() % digitWidth == 0);

    switch (pm) {
    case PMDecimalDigits:
        for (int i = digits.length() / digitWidth; i < precision + 1; ++i)
            digits.append(zero);
        break;
    case PMSignificantDigits:
        for (int i = digits.length() / digitWidth; i < precision; ++i)
            digits.append(zero);
        break;
    case PMChopTrailingZeros:
        Q_ASSERT(digits.length() / digitWidth <= 1 || !digits.endsWith(zero));
        break;
    }

    if (mustMarkDecimal || digits.length() > digitWidth)
        digits.insert(digitWidth, decimalPoint());

    digits.append(exponentSeparator());
    digits.append(longLongToString(decpt - 1, minExponentDigits, 10, -1, AlwaysShowSign));

    return std::move(digits);
}

QString QLocaleData::signPrefix(bool negative, unsigned flags) const
{
    if (negative)
        return negativeSign();
    if (flags & AlwaysShowSign)
        return positiveSign();
    if (flags & BlankBeforePositive)
        return QStringView(u" ").toString();
    return {};
}

QString QLocaleData::longLongToString(qlonglong l, int precision,
                                      int base, int width, unsigned flags) const
{
    bool negative = l < 0;

QT_WARNING_PUSH
    /* "unary minus operator applied to unsigned type, result still unsigned" */
QT_WARNING_DISABLE_MSVC(4146)
    /*
      Negating std::numeric_limits<qlonglong>::min() hits undefined behavior, so
      taking an absolute value has to cast to unsigned to change sign.
     */
    QString numStr = qulltoa(negative ? -qulonglong(l) : qulonglong(l), base, zeroDigit());
QT_WARNING_POP

    return applyIntegerFormatting(std::move(numStr), negative, precision, base, width, flags);
}

QString QLocaleData::unsLongLongToString(qulonglong l, int precision,
                                         int base, int width, unsigned flags) const
{
    const QString zero = zeroDigit();
    QString resultZero = base == 10 ? zero : QStringLiteral("0");
    return applyIntegerFormatting(l ? qulltoa(l, base, zero) : resultZero,
                                  false, precision, base, width, flags);
}

QString QLocaleData::applyIntegerFormatting(QString &&numStr, bool negative, int precision,
                                            int base, int width, unsigned flags) const
{
    const QString zero = base == 10 ? zeroDigit() : QStringLiteral("0");
    const auto digitWidth = zero.size();
    const auto digitCount = numStr.length() / digitWidth;

    const auto basePrefix = [&] () -> QStringView {
        if (flags & ShowBase) {
            const bool upper = flags & UppercaseBase;
            if (base == 16)
                return upper ? u"0X" : u"0x";
            if (base == 2)
                return upper ? u"0B" : u"0b";
            if (base == 8 && !numStr.startsWith(zero))
                return zero;
        }
        return {};
    }();

    const QString prefix = signPrefix(negative, flags) + basePrefix;
    // Count how much of width we've used up.  Each digit counts as one
    int usedWidth = digitCount + prefix.size();

    if (base == 10 && flags & GroupDigits) {
        const QString group = groupSeparator();
        int i = digitCount - m_grouping_least;
        if (i >= m_grouping_top) {
            numStr.insert(i * digitWidth, group);
            ++usedWidth;
            while ((i -= m_grouping_higher) >= m_grouping_top) {
                numStr.insert(i * digitWidth, group);
                ++usedWidth;
            }
        }
        // TODO: should we group any zero-padding we add later ?
    }

    const bool noPrecision = precision == -1;
    if (noPrecision)
        precision = 1;

    for (int i = numStr.length(); i < precision; ++i) {
        numStr.prepend(zero);
        usedWidth++;
    }

    // LeftAdjusted overrides ZeroPadded; and sprintf() only pads when
    // precision is not specified in the format string.
    if (noPrecision && flags & ZeroPadded && !(flags & LeftAdjusted)) {
        for (int i = usedWidth; i < width; ++i)
            numStr.prepend(zero);
    }

    QString result(flags & CapitalEorX ? std::move(numStr).toUpper() : std::move(numStr));
    if (prefix.size())
        result.prepend(prefix);
    return result;
}

/*
    Converts a number in locale to its representation in the C locale.
    Only has to guarantee that a string that is a correct representation of
    a number will be converted. If junk is passed in, junk will be passed
    out and the error will be detected during the actual conversion to a
    number. We can't detect junk here, since we don't even know the base
    of the number.
*/
bool QLocaleData::numberToCLocale(QStringView s, QLocale::NumberOptions number_options,
                                  CharBuff *result) const
{
    s = s.trimmed();
    if (s.size() < 1)
        return false;

    const QChar *uc = s.data();
    auto length = s.size();
    decltype(length) idx = 0;

    int digitsInGroup = 0;
    int group_cnt = 0; // counts number of group chars
    int decpt_idx = -1;
    int last_separator_idx = -1;
    int start_of_digits_idx = -1;
    int exponent_idx = -1;

    while (idx < length) {
        const QStringView in = QStringView(uc + idx, uc[idx].isHighSurrogate() ? 2 : 1);

        char out = numericToCLocale(in);
        if (out == 0) {
            const QChar simple = in.size() == 1 ? in.front() : QChar::Null;
            if (in == listSeparator())
                out = ';';
            else if (in == percentSign())
                out = '%';
            // for handling base-x numbers
            else if (simple.toLatin1() >= 'A' && simple.toLatin1() <= 'Z')
                out = simple.toLower().toLatin1();
            else if (simple.toLatin1() >= 'a' && simple.toLatin1() <= 'z')
                out = simple.toLatin1();
            else
                break;
        } else if (out == '.') {
            // Fail if more than one decimal point or point after e
            if (decpt_idx != -1 || exponent_idx != -1)
                return false;
            decpt_idx = idx;
        } else if (out == 'e') {
            exponent_idx = idx;
        }

        if (number_options & QLocale::RejectLeadingZeroInExponent) {
            if (exponent_idx != -1 && out == '0' && idx < length - 1) {
                // After the exponent there can only be '+', '-' or digits.
                // If we find a '0' directly after some non-digit, then that is a leading zero.
                if (result->last() < '0' || result->last() > '9')
                    return false;
            }
        }

        if (number_options & QLocale::RejectTrailingZeroesAfterDot) {
            // If we've seen a decimal point and the last character after the exponent is 0, then
            // that is a trailing zero.
            if (decpt_idx >= 0 && idx == exponent_idx && result->last() == '0')
                    return false;
        }

        if (!(number_options & QLocale::RejectGroupSeparator)) {
            if (start_of_digits_idx == -1 && out >= '0' && out <= '9') {
                start_of_digits_idx = idx;
                digitsInGroup++;
            } else if (out == ',') {
                // Don't allow group chars after the decimal point or exponent
                if (decpt_idx != -1 || exponent_idx != -1)
                    return false;

                if (last_separator_idx == -1) {
                    // Check distance from the beginning of the digits:
                    if (start_of_digits_idx == -1 || m_grouping_top > digitsInGroup
                        || digitsInGroup >= m_grouping_higher + m_grouping_top) {
                        return false;
                    }
                } else {
                    // Check distance from the last separator:
                    if (digitsInGroup != m_grouping_higher)
                        return false;
                }

                last_separator_idx = idx;
                ++group_cnt;
                digitsInGroup = 0;

                // don't add the group separator
                idx += in.size();
                continue;
            } else if (out == '.' || idx == exponent_idx) {
                // Were there enough digits since the last separator?
                if (last_separator_idx != -1 && digitsInGroup != m_grouping_least)
                    return false;
                // If we saw no separator, should we fail if
                // digitsInGroup > m_grouping_top + m_grouping_least ?

                // stop processing separators
                last_separator_idx = -1;
            } else if (out >= '0' && out <= '9') {
                digitsInGroup++;
            }
        }

        result->append(out);
        idx += in.size();
    }

    if (!(number_options & QLocale::RejectGroupSeparator)) {
        // group separator post-processing
        // did we end in a separator?
        if (last_separator_idx + 1 == idx)
            return false;
        // Were there enough digits since the last separator?
        if (last_separator_idx != -1 && digitsInGroup != m_grouping_least)
            return false;
        // If we saw no separator, and no decimal point, should we fail if
        // digitsInGroup > m_grouping_top + m_grouping_least ?
    }

    if (number_options & QLocale::RejectTrailingZeroesAfterDot) {
        // In decimal form, the last character can be a trailing zero if we've seen a decpt.
        if (decpt_idx != -1 && exponent_idx == -1 && result->last() == '0')
            return false;
    }

    result->append('\0');
    return idx == length;
}

bool QLocaleData::validateChars(QStringView str, NumberMode numMode, QByteArray *buff,
                                int decDigits, QLocale::NumberOptions number_options) const
{
    buff->clear();
    buff->reserve(str.length());

    const bool scientific = numMode == DoubleScientificMode;
    bool lastWasE = false;
    bool lastWasDigit = false;
    int eCnt = 0;
    int decPointCnt = 0;
    bool dec = false;
    int decDigitCnt = 0;

    for (qsizetype i = 0; i < str.size();) {
        const QStringView in = str.mid(i, str.at(i).isHighSurrogate() ? 2 : 1);
        char c = numericToCLocale(in);

        if (c >= '0' && c <= '9') {
            if (numMode != IntegerMode) {
                // If a double has too many digits after decpt, it shall be Invalid.
                if (dec && decDigits != -1 && decDigits < ++decDigitCnt)
                    return false;
            }

            // The only non-digit character after the 'e' can be '+' or '-'.
            // If a zero is directly after that, then the exponent is zero-padded.
            if ((number_options & QLocale::RejectLeadingZeroInExponent)
                && c == '0' && eCnt > 0 && !lastWasDigit) {
                return false;
            }

            lastWasDigit = true;
        } else {
            switch (c) {
                case '.':
                    if (numMode == IntegerMode) {
                        // If an integer has a decimal point, it shall be Invalid.
                        return false;
                    } else {
                        // If a double has more than one decimal point, it shall be Invalid.
                        if (++decPointCnt > 1)
                            return false;
#if 0
                        // If a double with no decimal digits has a decimal point, it shall be
                        // Invalid.
                        if (decDigits == 0)
                            return false;
#endif                  // On second thoughts, it shall be Valid.

                        dec = true;
                    }
                    break;

                case '+':
                case '-':
                    if (scientific) {
                        // If a scientific has a sign that's not at the beginning or after
                        // an 'e', it shall be Invalid.
                        if (i != 0 && !lastWasE)
                            return false;
                    } else {
                        // If a non-scientific has a sign that's not at the beginning,
                        // it shall be Invalid.
                        if (i != 0)
                            return false;
                    }
                    break;

                case ',':
                    //it can only be placed after a digit which is before the decimal point
                    if ((number_options & QLocale::RejectGroupSeparator) || !lastWasDigit ||
                            decPointCnt > 0)
                        return false;
                    break;

                case 'e':
                    if (scientific) {
                        // If a scientific has more than one 'e', it shall be Invalid.
                        if (++eCnt > 1)
                            return false;
                        dec = false;
                    } else {
                        // If a non-scientific has an 'e', it shall be Invalid.
                        return false;
                    }
                    break;

                default:
                    // If it's not a valid digit, it shall be Invalid.
                    return false;
            }
            lastWasDigit = false;
        }

        lastWasE = c == 'e';
        if (c != ',')
            buff->append(c);

        i += in.size();
    }

    return true;
}

double QLocaleData::stringToDouble(QStringView str, bool *ok,
                                   QLocale::NumberOptions number_options) const
{
    CharBuff buff;
    if (!numberToCLocale(str, number_options, &buff)) {
        if (ok != nullptr)
            *ok = false;
        return 0.0;
    }
    int processed = 0;
    bool nonNullOk = false;
    double d = qt_asciiToDouble(buff.constData(), buff.length() - 1, nonNullOk, processed);
    if (ok != nullptr)
        *ok = nonNullOk;
    return d;
}

qlonglong QLocaleData::stringToLongLong(QStringView str, int base, bool *ok,
                                        QLocale::NumberOptions number_options) const
{
    CharBuff buff;
    if (!numberToCLocale(str, number_options, &buff)) {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    return bytearrayToLongLong(buff.constData(), base, ok);
}

qulonglong QLocaleData::stringToUnsLongLong(QStringView str, int base, bool *ok,
                                            QLocale::NumberOptions number_options) const
{
    CharBuff buff;
    if (!numberToCLocale(str, number_options, &buff)) {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    return bytearrayToUnsLongLong(buff.constData(), base, ok);
}

qlonglong QLocaleData::bytearrayToLongLong(const char *num, int base, bool *ok)
{
    bool _ok;
    const char *endptr;

    if (*num == '\0') {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    qlonglong l = qstrtoll(num, &endptr, base, &_ok);

    if (!_ok) {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    if (*endptr != '\0') {
        while (ascii_isspace(*endptr))
            ++endptr;
    }

    if (*endptr != '\0') {
        // we stopped at a non-digit character after converting some digits
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    if (ok != nullptr)
        *ok = true;
    return l;
}

qulonglong QLocaleData::bytearrayToUnsLongLong(const char *num, int base, bool *ok)
{
    bool _ok;
    const char *endptr;
    qulonglong l = qstrtoull(num, &endptr, base, &_ok);

    if (!_ok) {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    if (*endptr != '\0') {
        while (ascii_isspace(*endptr))
            ++endptr;
    }

    if (*endptr != '\0') {
        if (ok != nullptr)
            *ok = false;
        return 0;
    }

    if (ok != nullptr)
        *ok = true;
    return l;
}

/*!
    \since 4.8

    \enum QLocale::CurrencySymbolFormat

    Specifies the format of the currency symbol.

    \value CurrencyIsoCode a ISO-4217 code of the currency.
    \value CurrencySymbol a currency symbol.
    \value CurrencyDisplayName a user readable name of the currency.
*/

/*!
    \since 4.8
    Returns a currency symbol according to the \a format.
*/
QString QLocale::currencySymbol(QLocale::CurrencySymbolFormat format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::CurrencySymbol, format).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    switch (format) {
    case CurrencySymbol:
        return d->m_data->currencySymbol().getData(currency_symbol_data);
    case CurrencyDisplayName:
        return d->m_data->currencyDisplayName().getData(currency_display_name_data);
    case CurrencyIsoCode: {
        const char *code = d->m_data->m_currency_iso_code;
        if (auto len = qstrnlen(code, 3))
            return QString::fromLatin1(code, int(len));
        break;
    }
    }
    return QString();
}

/*!
    \since 4.8

    Returns a localized string representation of \a value as a currency.
    If the \a symbol is provided it is used instead of the default currency symbol.

    \sa currencySymbol()
*/
QString QLocale::toCurrencyString(qlonglong value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        auto res = systemLocale()->query(QSystemLocale::CurrencyToString,
                                         QVariant::fromValue(arg)).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    QLocaleData::DataRange range = d->m_data->currencyFormatNegative();
    if (!range.size || value >= 0)
        range = d->m_data->currencyFormat();
    else
        value = -value;
    QString str = toString(value);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    return range.getData(currency_format_data).arg(str, sym);
}

/*!
    \since 4.8
    \overload
*/
QString QLocale::toCurrencyString(qulonglong value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        auto res = systemLocale()->query(QSystemLocale::CurrencyToString,
                                         QVariant::fromValue(arg)).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    QString str = toString(value);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    return d->m_data->currencyFormat().getData(currency_format_data).arg(str, sym);
}

/*!
    \since 5.7
    \overload toCurrencyString()

    Returns a localized string representation of \a value as a currency.
    If the \a symbol is provided it is used instead of the default currency symbol.
    If the \a precision is provided it is used to set the precision of the currency value.

    \sa currencySymbol()
 */
QString QLocale::toCurrencyString(double value, const QString &symbol, int precision) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        auto res = systemLocale()->query(QSystemLocale::CurrencyToString,
                                         QVariant::fromValue(arg)).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    QLocaleData::DataRange range = d->m_data->currencyFormatNegative();
    if (!range.size || value >= 0)
        range = d->m_data->currencyFormat();
    else
        value = -value;
    QString str = toString(value, 'f', precision == -1 ? d->m_data->m_currency_digits : precision);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    return range.getData(currency_format_data).arg(str, sym);
}

/*!
  \fn QString QLocale::toCurrencyString(float i, const QString &symbol, int precision) const
  \overload toCurrencyString()
*/

/*!
    \since 5.10

    \enum QLocale::DataSizeFormat

    Specifies the format for representation of data quantities.

    \omitvalue DataSizeBase1000
    \omitvalue DataSizeSIQuantifiers
    \value DataSizeIecFormat            format using base 1024 and IEC prefixes: KiB, MiB, GiB, ...
    \value DataSizeTraditionalFormat    format using base 1024 and SI prefixes: kB, MB, GB, ...
    \value DataSizeSIFormat             format using base 1000 and SI prefixes: kB, MB, GB, ...

    \sa formattedDataSize()
*/

/*!
    \since 5.10

    Converts a size in bytes to a human-readable localized string, comprising a
    number and a quantified unit. The quantifier is chosen such that the number
    is at least one, and as small as possible. For example if \a bytes is
    16384, \a precision is 2, and \a format is \l DataSizeIecFormat (the
    default), this function returns "16.00 KiB"; for 1330409069609 bytes it
    returns "1.21 GiB"; and so on. If \a format is \l DataSizeIecFormat or
    \l DataSizeTraditionalFormat, the given number of bytes is divided by a
    power of 1024, with result less than 1024; for \l DataSizeSIFormat, it is
    divided by a power of 1000, with result less than 1000.
    \c DataSizeIecFormat uses the new IEC standard quantifiers Ki, Mi and so on,
    whereas \c DataSizeSIFormat uses the older SI quantifiers k, M, etc., and
    \c DataSizeTraditionalFormat abuses them.
*/
QString QLocale::formattedDataSize(qint64 bytes, int precision, DataSizeFormats format) const
{
    int power, base = 1000;
    if (!bytes) {
        power = 0;
    } else if (format & DataSizeBase1000) {
        power = int(std::log10(qAbs(bytes)) / 3);
    } else { // Compute log2(bytes) / 10:
        power = int((63 - qCountLeadingZeroBits(quint64(qAbs(bytes)))) / 10);
        base = 1024;
    }
    // Only go to doubles if we'll be using a quantifier:
    const QString number = power
        ? toString(bytes / std::pow(double(base), power), 'f', qMin(precision, 3 * power))
        : toString(bytes);

    // We don't support sizes in units larger than exbibytes because
    // the number of bytes would not fit into qint64.
    Q_ASSERT(power <= 6 && power >= 0);
    QString unit;
    if (power > 0) {
        QLocaleData::DataRange range = (format & DataSizeSIQuantifiers)
            ? d->m_data->byteAmountSI() : d->m_data->byteAmountIEC();
        unit = range.getListEntry(byte_unit_data, power - 1);
    } else {
        unit = d->m_data->byteCount().getData(byte_unit_data);
    }

    return number + QLatin1Char(' ') + unit;
}

/*!
    \since 4.8

    Returns an ordered list of locale names for translation purposes in
    preference order (like "en-Latn-US", "en-US", "en").

    The return value represents locale names that the user expects to see the
    UI translation in.

    Most like you do not need to use this function directly, but just pass the
    QLocale object to the QTranslator::load() function.

    The first item in the list is the most preferred one.

    \sa QTranslator, bcp47Name()
*/
QStringList QLocale::uiLanguages() const
{
    QStringList uiLanguages;
    QList<QLocale> locales;
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        uiLanguages = systemLocale()->query(QSystemLocale::UILanguages).toStringList();
        // ... but we need to include likely-adjusted forms of each of those, too:
        for (const auto &entry : std::as_const(uiLanguages))
            locales.append(QLocale(entry));
        if (locales.isEmpty())
            locales.append(systemLocale()->fallbackLocale());
    } else
#endif
    {
        locales.append(*this);
    }
    for (int i = locales.size(); i-- > 0; ) {
        const QLocale &locale = locales.at(i);
        const auto data = locale.d->m_data;
        QLocaleId id = data->id();

        int j;
        QByteArray prior;
        if (i < uiLanguages.size()) {
            // Adding likely-adjusted forms to system locale's list.
            // Name the locale is derived from:
            prior = uiLanguages.at(i).toLatin1();
            // Insert just after the entry we're supplementing:
            j = i + 1;
        } else if (id.language_id == C) {
            // Attempt no likely sub-tag amendments to C:
            uiLanguages.append(locale.name());
            continue;
        } else {
            // Plain locale, not system locale; just append.
            const QString name = locale.bcp47Name();
            uiLanguages.append(name);
            prior = name.toLatin1();
            j = uiLanguages.size();
        }

        const QLocaleId max = id.withLikelySubtagsAdded();
        const QLocaleId min = max.withLikelySubtagsRemoved();
        id.script_id = 0; // For re-use as script-less variant.

        // Include version with all likely sub-tags (last) if distinct from the rest:
        if (max != min && max != id && max.name() != prior)
            uiLanguages.insert(j, QString::fromLatin1(max.name()));

        // Include scriptless version if likely-equivalent and distinct:
        if (data->m_script_id && id != min && id.name() != prior
            && id.withLikelySubtagsAdded() == max) {
            uiLanguages.insert(j, QString::fromLatin1(id.name()));
        }

        // Include minimal version (first) unless it's what our locale is derived from:
        if (min.name() != prior)
            uiLanguages.insert(j, QString::fromLatin1(min.name()));
    }
    return uiLanguages;
}

/*!
  \since 5.13

  Returns the locale to use for collation.

  The result is usually this locale; however, the system locale (which is
  commonly the default locale) will return the system collation locale.
  The result is suitable for passing to QCollator's constructor.

  \sa QCollator
*/
QLocale QLocale::collation() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        const auto res = systemLocale()->query(QSystemLocale::Collation).toString();
        if (!res.isEmpty())
            return QLocale(res);
    }
#endif
    return *this;
}

/*!
    \since 4.8

    Returns a native name of the language for the locale. For example
    "Schwiizertüütsch" for Swiss-German locale.

    \sa nativeTerritoryName(), languageToString()
*/
QString QLocale::nativeLanguageName() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::NativeLanguageName).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return d->m_data->endonymLanguage().getData(endonyms_data);
}

/*!
    \since 6.2

    Returns a native name of the territory for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName(), territoryToString()
*/
QString QLocale::nativeTerritoryName() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == &systemLocaleData) {
        auto res = systemLocale()->query(QSystemLocale::NativeTerritoryName).toString();
        if (!res.isEmpty())
            return res;
    }
#endif
    return d->m_data->endonymTerritory().getData(endonyms_data);
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \deprecated [6.6] Use \l nativeTerritoryName() instead.
    \since 4.8

    Returns a native name of the territory for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName(), territoryToString()
*/
QString QLocale::nativeCountryName() const
{
    return nativeTerritoryName();
}
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QLocale &l)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace().noquote()
        << "QLocale(" << QLocale::languageToString(l.language())
        << ", " << QLocale::scriptToString(l.script())
        << ", " << QLocale::territoryToString(l.territory()) << ')';
    return dbg;
}
#endif
QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qlocale.cpp"
#endif
