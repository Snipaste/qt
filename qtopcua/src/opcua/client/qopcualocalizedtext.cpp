/****************************************************************************
**
** Copyright (C) 2015 basysKom GmbH, opensource@basyskom.com
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtOpcUa module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopcualocalizedtext.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpcUaLocalizedText
    \inmodule QtOpcUa
    \brief The OPC UA LocalizedText type.

    This is the Qt OPC UA representation for the OPC UA LocalizedText type defined in OPC-UA part 3, 8.5.
    A LocalizedText value contains a text string with associated locale information in a second string (e. g. "en" or "en-US").
    The format of the locale information string is <language>[-<country/region>]. Language is usually given as ISO 639 two letter code,
    country/region as ISO 3166 two letter code. Custom codes are also allowed (see OPC-UA part 3, 8.4).
    It can be used to provide multiple text strings in different languages for a value using an array of LocalizedText elements.
*/

/*!
    \qmltype LocalizedText
    \inqmlmodule QtOpcUa
    \brief Contains a text with associated locale.
    \since QtOpcUa 5.12

    The two members of this type contain the actual text and the locale of the text.
*/

/*!
    \property QOpcUaLocalizedText::text

    Textual content.
*/

/*!
    \qmlproperty string LocalizedText::text

    Textual content.
*/

/*!
    \property QOpcUaLocalizedText::locale

    Locale of the contained text.
    This has to be in a modified ISO standard notation, for example \c en-US.
    See OPC UA specification part 3, 8.4 for details.
*/

/*!
    \qmlproperty string LocalizedText::locale

    Locale of the contained text.
    This has to be in a modified ISO standard notation, for example \c en-US.
    See OPC UA specification part 3, 8.4 for details.
*/

class QOpcUaLocalizedTextData : public QSharedData
{
public:
    QString locale;
    QString text;
};

QOpcUaLocalizedText::QOpcUaLocalizedText()
    : data(new QOpcUaLocalizedTextData)
{
}

/*!
    Constructs a localized text from \a rhs.
*/
QOpcUaLocalizedText::QOpcUaLocalizedText(const QOpcUaLocalizedText &rhs)
    : data(rhs.data)
{
}

/*!
    Constructs a localized text with the locale \a locale and the text \a text.
*/
QOpcUaLocalizedText::QOpcUaLocalizedText(const QString &locale, const QString &text)
    : data(new QOpcUaLocalizedTextData)
{
    data->locale = locale;
    data->text = text;
}

/*!
    Sets the values from \a rhs in this localized text.
*/
QOpcUaLocalizedText &QOpcUaLocalizedText::operator=(const QOpcUaLocalizedText &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

/*!
    Returns \c true if this localized text has the same value as \a rhs.
*/
bool QOpcUaLocalizedText::operator==(const QOpcUaLocalizedText &rhs) const
{
    return data->locale == rhs.locale() &&
            data->text == rhs.text();
}

/*!
    Converts this localized text to \l QVariant.
*/
QOpcUaLocalizedText::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

QOpcUaLocalizedText::~QOpcUaLocalizedText()
{
}

/*!
    Returns the text.
*/
QString QOpcUaLocalizedText::text() const
{
    return data->text;
}

/*!
    Sets the text to \a text.
*/
void QOpcUaLocalizedText::setText(const QString &text)
{
    data->text = text;
}

/*!
    Returns the locale.
*/
QString QOpcUaLocalizedText::locale() const
{
    return data->locale;
}

/*!
    Sets the locale to \a locale.
*/
void QOpcUaLocalizedText::setLocale(const QString &locale)
{
    data->locale = locale;
}

QT_END_NAMESPACE
