/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QTEXTCODEC_H
#define QTEXTCODEC_H

#include <QtCore/qlist.h>
#include <QtCore/qstringconverter.h>

#include <QtCore5Compat/qcore5global.h>

QT_REQUIRE_CONFIG(textcodec);

QT_BEGIN_NAMESPACE

class QTextDecoder;
class QTextEncoder;

class Q_CORE5COMPAT_EXPORT QTextCodec : public QStringConverterBase
{
    Q_DISABLE_COPY(QTextCodec)
public:
    using ConversionFlags = QStringConverterBase::Flags;
    using ConverterState = QStringConverterBase::State;

    static constexpr Flag ConvertInvalidToNull = Flag::ConvertInvalidToNull;
    static constexpr Flag DefaultConversion = Flag::WriteBom;
    static constexpr Flag IgnoreHeader = Flag::ConvertInitialBom;

    static QTextCodec* codecForName(const QByteArray &name);
    static QTextCodec* codecForName(const char *name) { return codecForName(QByteArray(name)); }
    static QTextCodec* codecForMib(int mib);

    static QList<QByteArray> availableCodecs();
    static QList<int> availableMibs();

    static QTextCodec* codecForLocale();
    static void setCodecForLocale(QTextCodec *c);

    static QTextCodec *codecForTr() { return codecForMib(106); /* Utf8 */ }

    static QTextCodec *codecForHtml(const QByteArray &ba);
    static QTextCodec *codecForHtml(const QByteArray &ba, QTextCodec *defaultCodec);

    static QTextCodec *codecForUtfText(const QByteArray &ba);
    static QTextCodec *codecForUtfText(const QByteArray &ba, QTextCodec *defaultCodec);

    bool canEncode(QChar) const;
#if QT_STRINGVIEW_LEVEL < 2
    bool canEncode(const QString&) const;
#endif
    bool canEncode(QStringView) const;

    QString toUnicode(const QByteArray&) const;
    QString toUnicode(const char* chars) const;
#if QT_STRINGVIEW_LEVEL < 2
    QByteArray fromUnicode(const QString& uc) const;
#endif
    QByteArray fromUnicode(QStringView uc) const;

    QString toUnicode(const char *in, int length, ConverterState *state = nullptr) const
    {
        if (in == nullptr || length <= 0)
            return {};
        return convertToUnicode(in, length, state);
    }
    QByteArray fromUnicode(const QChar *in, int length, ConverterState *state = nullptr) const
    {
        if (in == nullptr || length <= 0)
            return {};
        return convertFromUnicode(in, length, state);
    }

    QTextDecoder* makeDecoder(ConversionFlags flags = DefaultConversion) const;
    QTextEncoder* makeEncoder(ConversionFlags flags = DefaultConversion) const;

    virtual QByteArray name() const = 0;
    virtual QList<QByteArray> aliases() const;
    virtual int mibEnum() const = 0;

protected:
    virtual QString convertToUnicode(const char *in, int length, ConverterState *state) const = 0;
    virtual QByteArray convertFromUnicode(const QChar *in, int length, ConverterState *state) const = 0;

    QTextCodec();
    virtual ~QTextCodec();

private:
    friend struct QTextCodecData;
};

class Q_CORE5COMPAT_EXPORT QTextEncoder
{
    Q_DISABLE_COPY(QTextEncoder)
public:
    explicit QTextEncoder(const QTextCodec *codec) : c(codec), state() {}
    explicit QTextEncoder(const QTextCodec *codec, QTextCodec::ConversionFlags flags);
    ~QTextEncoder();
#if QT_STRINGVIEW_LEVEL < 2
    QByteArray fromUnicode(const QString& str);
#endif
    QByteArray fromUnicode(QStringView str);
    QByteArray fromUnicode(const QChar *uc, int len);
    bool hasFailure() const;
private:
    const QTextCodec *c = nullptr;
    QTextCodec::ConverterState state;
};

class Q_CORE5COMPAT_EXPORT QTextDecoder
{
    Q_DISABLE_COPY(QTextDecoder)
public:
    explicit QTextDecoder(const QTextCodec *codec) : c(codec), state() {}
    explicit QTextDecoder(const QTextCodec *codec, QTextCodec::ConversionFlags flags);
    ~QTextDecoder();
    QString toUnicode(const char* chars, int len);
    QString toUnicode(const QByteArray &ba);
    void toUnicode(QString *target, const char *chars, int len);
    bool hasFailure() const;
    bool needsMoreData() const;
private:
    const QTextCodec *c = nullptr;
    QTextCodec::ConverterState state;
};

namespace Qt
{
    Q_CORE5COMPAT_EXPORT QTextCodec *codecForHtml(const QByteArray &ba);
}

QT_END_NAMESPACE

#endif // QTEXTCODEC_H
