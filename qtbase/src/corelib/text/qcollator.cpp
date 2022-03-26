/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "qcollator_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include "qdebug.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCollator
    \inmodule QtCore
    \brief The QCollator class compares strings according to a localized collation algorithm.

    \since 5.2

    \reentrant
    \ingroup i18n
    \ingroup string-processing
    \ingroup shared

    QCollator is initialized with a QLocale. It can then be used to compare and
    sort strings in using the ordering appropriate to the locale.

    A QCollator object can be used together with template-based sorting
    algorithms, such as std::sort(), to sort a list with QString entries.

    \snippet code/src_corelib_text_qcollator.cpp 0

    In addition to the locale, several optional flags can be set that influence
    the result of the collation.

    \note On Linux, Qt is normally compiled to use ICU. When it isn't, all
    options are ignored and the only supported locales are the system default
    (that \c{setlocale(LC_COLLATE, nullptr)} would report) and the "C" locale.
*/

/*!
  \since 5.13

  Constructs a QCollator using the default locale's collation locale.

  The system locale, when used as default locale, may have a collation locale
  other than itself (e.g. on Unix, if LC_COLLATE is set differently to LANG in
  the environment). All other locales are their own collation locales.

  \sa setLocale(), QLocale::collation(), QLocale::setDefault()
*/
QCollator::QCollator()
    : d(new QCollatorPrivate(QLocale().collation()))
{
    d->init();
}

/*!
    Constructs a QCollator using the given \a locale.

    \sa setLocale()
*/
QCollator::QCollator(const QLocale &locale)
    : d(new QCollatorPrivate(locale))
{
}

/*!
    Creates a copy of \a other.
*/
QCollator::QCollator(const QCollator &other)
    : d(other.d)
{
    if (d) {
        // Ensure clean, lest both copies try to init() at the same time:
        if (d->dirty)
            d->init();
        d->ref.ref();
    }
}

/*!
    Destroys this collator.
*/
QCollator::~QCollator()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Assigns \a other to this collator.
*/
QCollator &QCollator::operator=(const QCollator &other)
{
    if (this != &other) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d) {
            // Ensure clean, lest both copies try to init() at the same time:
            if (d->dirty)
                d->init();
            d->ref.ref();
        }
    }
    return *this;
}

/*!
    \fn QCollator::QCollator(QCollator &&other)

    Move constructor. Moves from \a other into this collator.

    Note that a moved-from QCollator can only be destroyed or assigned to.
    The effect of calling other functions than the destructor or one of the
    assignment operators is undefined.
*/

/*!
    \fn QCollator & QCollator::operator=(QCollator && other)

    Move-assigns from \a other to this collator.

    Note that a moved-from QCollator can only be destroyed or assigned to.
    The effect of calling other functions than the destructor or one of the
    assignment operators is undefined.
*/

/*!
    \fn void QCollator::swap(QCollator &other)

    Swaps this collator with \a other. This function is very fast and
    never fails.
*/

/*!
    \internal
*/
void QCollator::detach()
{
    if (d->ref.loadRelaxed() != 1) {
        QCollatorPrivate *x = new QCollatorPrivate(d->locale);
        if (!d->ref.deref())
            delete d;
        d = x;
    }
    // All callers need this, because about to modify the object:
    d->dirty = true;
}

/*!
    Sets the locale of the collator to \a locale.

    \sa locale()
*/
void QCollator::setLocale(const QLocale &locale)
{
    if (locale == d->locale)
        return;

    detach();
    d->locale = locale;
}

/*!
    Returns the locale of the collator.

    Unless supplied to the constructor or by calling setLocale(), the system's
    default collation locale is used.

    \sa setLocale(), QLocale::collation()
*/
QLocale QCollator::locale() const
{
    return d->locale;
}

/*!
    Sets the case-sensitivity of the collator to \a cs.

    \sa caseSensitivity()
*/
void QCollator::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    if (d->caseSensitivity == cs)
        return;

    detach();
    d->caseSensitivity = cs;
}

/*!
    Returns case sensitivity of the collator.

    This defaults to case-sensitive until set.

    \note In the C locale, when case-sensitive, all lower-case letters sort
    after all upper-case letters, where most locales sort each lower-case letter
    either immediately before or immediately after its upper-case partner.  Thus
    "Zap" sorts before "ape" in the C locale but after in most others.

    \sa setCaseSensitivity()
*/
Qt::CaseSensitivity QCollator::caseSensitivity() const
{
    return d->caseSensitivity;
}

/*!
    Enables numeric sorting mode when \a on is \c true.

    \sa numericMode()
*/
void QCollator::setNumericMode(bool on)
{
    if (d->numericMode == on)
        return;

    detach();
    d->numericMode = on;
}

/*!
    Returns \c true if numeric sorting is enabled, \c false otherwise.

    When \c true, numerals are recognized as numbers and sorted in arithmetic
    order; for example, 100 sortes after 99. When \c false, numbers are sorted
    in lexical order, so that 100 sorts before 99 (because 1 is before 9). By
    default, this option is disabled.

    \sa setNumericMode()
*/
bool QCollator::numericMode() const
{
    return d->numericMode;
}

/*!
    Ignores punctuation and symbols if \a on is \c true, attends to them if \c false.

    \sa ignorePunctuation()
*/
void QCollator::setIgnorePunctuation(bool on)
{
    if (d->ignorePunctuation == on)
        return;

    detach();
    d->ignorePunctuation = on;
}

/*!
    Returns whether punctuation and symbols are ignored when collating.

    When \c true, strings are compared as if all punctuation and symbols were
    removed from each string.

    \sa setIgnorePunctuation()
*/
bool QCollator::ignorePunctuation() const
{
    return d->ignorePunctuation;
}

/*!
    \since 5.13
    \fn bool QCollator::operator()(QStringView s1, QStringView s2) const

    A QCollator can be used as the comparison function of a sorting algorithm.
    It returns \c true if \a s1 sorts before \a s2, otherwise \c false.

    \sa compare()
*/

/*!
    \since 5.13
    \fn int QCollator::compare(QStringView s1, QStringView s2) const

    Compares \a s1 with \a s2.

    Returns an integer less than, equal to, or greater than zero depending on
    whether \a s1 sorts before, with or after \a s2.
*/
#if QT_STRINGVIEW_LEVEL < 2
/*!
    \fn bool QCollator::operator()(const QString &s1, const QString &s2) const
    \overload
    \since 5.2
*/

/*!
    \fn int QCollator::compare(const QString &s1, const QString &s2) const
    \overload
    \since 5.2
*/

/*!
    \fn int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
    \overload
    \since 5.2

    Compares \a s1 with \a s2. \a len1 and \a len2 specify the lengths of the
    QChar arrays pointed to by \a s1 and \a s2.

    Returns an integer less than, equal to, or greater than zero depending on
    whether \a s1 sorts before, with or after \a s2.
*/
#endif // QT_STRINGVIEW_LEVEL < 2

/*!
    \fn QCollatorSortKey QCollator::sortKey(const QString &string) const

    Returns a sortKey for \a string.

    Creating the sort key is usually somewhat slower, than using the compare()
    methods directly. But if the string is compared repeatedly (e.g. when
    sorting a whole list of strings), it's usually faster to create the sort
    keys for each string and then sort using the keys.

    \note Not supported with the C (a.k.a. POSIX) locale on Darwin.
*/

/*!
    \class QCollatorSortKey
    \inmodule QtCore
    \brief The QCollatorSortKey class can be used to speed up string collation.

    \since 5.2

    The QCollatorSortKey class is always created by QCollator::sortKey() and is
    used for fast strings collation, for example when collating many strings.

    \reentrant
    \ingroup i18n
    \ingroup string-processing
    \ingroup shared

    \sa QCollator, QCollator::sortKey(), compare()
*/

/*!
    \internal
*/
QCollatorSortKey::QCollatorSortKey(QCollatorSortKeyPrivate *d)
    : d(d)
{
}

/*!
    Constructs a copy of the \a other collator key.
*/
QCollatorSortKey::QCollatorSortKey(const QCollatorSortKey &other)
    : d(other.d)
{
}

/*!
    Destroys the collator key.
*/
QCollatorSortKey::~QCollatorSortKey()
{
}

/*!
    Assigns \a other to this collator key.
*/
QCollatorSortKey& QCollatorSortKey::operator=(const QCollatorSortKey &other)
{
    if (this != &other) {
        d = other.d;
    }
    return *this;
}

/*!
    \fn QCollatorSortKey &QCollatorSortKey::operator=(QCollatorSortKey && other)

    Move-assigns \a other to this collator key.
*/

/*!
    \fn bool QCollatorSortKey::operator<(const QCollatorSortKey &lhs, const QCollatorSortKey &rhs)

    Both keys must have been created by the same QCollator's sortKey(). Returns
    \c true if \a lhs should be sorted before \a rhs, according to the QCollator
    that created them; otherwise returns \c false.

    \sa QCollatorSortKey::compare()
*/

/*!
    \fn void QCollatorSortKey::swap(QCollatorSortKey & other)

    Swaps this collator key with \a other.
*/

/*!
    \fn int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const

    Compares this key to \a otherKey, which must have been created by the same
    QCollator's sortKey() as this key. The comparison is performed in accordance
    with that QCollator's sort order.

    Returns a negative value if this key sorts before \a otherKey, 0 if the two
    keys are equal or a positive value if this key sorts after \a otherKey.

    \sa operator<()
*/

QT_END_NAMESPACE
