/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "../../../../../src/corelib/tools/qalgorithms.h"
#include <QTest>

QT_WARNING_DISABLE_DEPRECATED

#include <iostream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <qalgorithms.h>
#include <QList>
#include <QRandomGenerator>
#include <QString>
#include <QStringList>

#define Q_TEST_PERFORMANCE 0

using namespace std;

class tst_QAlgorithms : public QObject
{
    Q_OBJECT
private slots:
    void swap();
    void swap2();
    void convenienceAPI();

    void popCount08_data() { popCount_data_impl(sizeof(quint8 )); }
    void popCount16_data() { popCount_data_impl(sizeof(quint16)); }
    void popCount32_data() { popCount_data_impl(sizeof(quint32)); }
    void popCount64_data() { popCount_data_impl(sizeof(quint64)); }
    void popCount08()      { popCount_impl<quint8 >(); }
    void popCount16()      { popCount_impl<quint16>(); }
    void popCount32()      { popCount_impl<quint32>(); }
    void popCount64()      { popCount_impl<quint64>(); }

    void countTrailing08_data() { countTrailing_data_impl(sizeof(quint8 )); }
    void countTrailing16_data() { countTrailing_data_impl(sizeof(quint16)); }
    void countTrailing32_data() { countTrailing_data_impl(sizeof(quint32)); }
    void countTrailing64_data() { countTrailing_data_impl(sizeof(quint64)); }
    void countTrailing08()      { countTrailing_impl<quint8 >(); }
    void countTrailing16()      { countTrailing_impl<quint16>(); }
    void countTrailing32()      { countTrailing_impl<quint32>(); }
    void countTrailing64()      { countTrailing_impl<quint64>(); }

    void countLeading08_data() { countLeading_data_impl(sizeof(quint8 )); }
    void countLeading16_data() { countLeading_data_impl(sizeof(quint16)); }
    void countLeading32_data() { countLeading_data_impl(sizeof(quint32)); }
    void countLeading64_data() { countLeading_data_impl(sizeof(quint64)); }
    void countLeading08()      { countLeading_impl<quint8 >(); }
    void countLeading16()      { countLeading_impl<quint16>(); }
    void countLeading32()      { countLeading_impl<quint32>(); }
    void countLeading64()      { countLeading_impl<quint64>(); }

private:
    void popCount_data_impl(size_t sizeof_T_Int);
    template <typename T_Int>
    void popCount_impl();

    void countTrailing_data_impl(size_t sizeof_T_Int);
    template <typename T_Int>
    void countTrailing_impl();

    void countLeading_data_impl(size_t sizeof_T_Int);
    template <typename T_Int>
    void countLeading_impl();
};

void tst_QAlgorithms::swap()
{
    {
        int a = 1, b = 2;
        qSwap(a, b);
        QVERIFY(a == 2);
        QVERIFY(b == 1);

        qSwap(a, a);
        QVERIFY(a == 2);
        QVERIFY(b == 1);

        qSwap(b, b);
        QVERIFY(a == 2);
        QVERIFY(b == 1);

        qSwap(a, b);
        QVERIFY(a == 1);
        QVERIFY(b == 2);

        qSwap(b, a);
        QVERIFY(a == 2);
        QVERIFY(b == 1);
    }

    {
        double a = 1.0, b = 2.0;
        qSwap(a, b);
        QVERIFY(a == 2.0);
        QVERIFY(b == 1.0);

        qSwap(a, a);
        QVERIFY(a == 2.0);
        QVERIFY(b == 1.0);

        qSwap(b, b);
        QVERIFY(a == 2.0);
        QVERIFY(b == 1.0);

        qSwap(a, b);
        QVERIFY(a == 1.0);
        QVERIFY(b == 2.0);

        qSwap(b, a);
        QVERIFY(a == 2.0);
        QVERIFY(b == 1.0);
    }

    {
        QString a = "1", b = "2";
        qSwap(a, b);
        QCOMPARE(a, QLatin1String("2"));
        QCOMPARE(b, QLatin1String("1"));

        qSwap(a, a);
        QCOMPARE(a, QLatin1String("2"));
        QCOMPARE(b, QLatin1String("1"));

        qSwap(b, b);
        QCOMPARE(a, QLatin1String("2"));
        QCOMPARE(b, QLatin1String("1"));

        qSwap(a, b);
        QCOMPARE(a, QLatin1String("1"));
        QCOMPARE(b, QLatin1String("2"));

        qSwap(b, a);
        QCOMPARE(a, QLatin1String("2"));
        QCOMPARE(b, QLatin1String("1"));
    }

    {
        void *a = nullptr, *b = nullptr;
        qSwap(a, b);
    }

    {
        const void *a = nullptr, *b = nullptr;
        qSwap(a, b);
    }

    {
        QString *a = nullptr, *b = nullptr;
        qSwap(a, b);
    }

    {
        const QString *a = nullptr, *b = nullptr;
        qSwap(a, b);
    }

    {
        QString **a = nullptr, **b = nullptr;
        qSwap(a, b);
    }

    {
        const QString **a = nullptr, **b = nullptr;
        qSwap(a, b);
    }

    {
        QString * const *a = nullptr, * const *b = nullptr;
        qSwap(a, b);
    }

    {
        const QString * const *a = nullptr, * const *b = nullptr;
        qSwap(a, b);
    }
}

namespace SwapTest {
    struct ST { int i; int j; };
    void swap(ST &a, ST &b) {
        a.i = b.j;
        b.i = a.j;
    }
}

void tst_QAlgorithms::swap2()
{
    {
#ifndef QT_NO_SQL
        //check the namespace lookup works correctly
        SwapTest::ST a = { 45, 65 };
        SwapTest::ST b = { 48, 68 };
        qSwap(a, b);
        QCOMPARE(a.i, 68);
        QCOMPARE(b.i, 65);
#endif
    }
}

void tst_QAlgorithms::convenienceAPI()
{
    // Compile-test for QAlgorithm convenience functions.

    QList<int *> pointerList;
    qDeleteAll(pointerList);
    qDeleteAll(pointerList.begin(), pointerList.end());
}

// alternative implementation of qPopulationCount for comparison:
static constexpr const uint bitsSetInNibble[] = {
    0, 1, 1, 2, 1, 2, 2, 3,
    1, 2, 2, 3, 2, 3, 3, 4,
};
static_assert(sizeof bitsSetInNibble / sizeof *bitsSetInNibble == 16);

static constexpr uint bitsSetInByte(quint8 byte)
{
    return bitsSetInNibble[byte & 0xF] + bitsSetInNibble[byte >> 4];
}
static constexpr uint bitsSetInShort(quint16 word)
{
    return bitsSetInByte(word & 0xFF) + bitsSetInByte(word >> 8);
}
static constexpr uint bitsSetInInt(quint32 word)
{
    return bitsSetInShort(word & 0xFFFF) + bitsSetInShort(word >> 16);
}
static constexpr uint bitsSetInInt64(quint64 word)
{
    return bitsSetInInt(word & 0xFFFFFFFF) + bitsSetInInt(word >> 32);
}


void tst_QAlgorithms::popCount_data_impl(size_t sizeof_T_Int)
{
    using namespace QTest;
    addColumn<quint64>("input");
    addColumn<uint>("expected");

    for (uint i = 0; i < UCHAR_MAX; ++i) {
        const uchar byte = static_cast<uchar>(i);
        const uint bits = bitsSetInByte(byte);
        const quint64 value = static_cast<quint64>(byte);
        const quint64 input = value << ((i % sizeof_T_Int) * 8U);
        QTest::addRow("0x%016llx", input) << input << bits;
    }

    // and some random ones:
    if (sizeof_T_Int >= 8)
        for (size_t i = 0; i < 1000; ++i) {
            const quint64 input = QRandomGenerator::global()->generate64();
            QTest::addRow("0x%016llx", input) << input << bitsSetInInt64(input);
        }
        else if (sizeof_T_Int >= 2)
            for (size_t i = 0; i < 1000 ; ++i) {
                const quint32 input = QRandomGenerator::global()->generate();
                if (sizeof_T_Int >= 4)
                    QTest::addRow("0x%08x", input) << quint64(input) << bitsSetInInt(input);
                else
                    QTest::addRow("0x%04x", quint16(input & 0xFFFF)) << quint64(input & 0xFFFF) << bitsSetInShort(input & 0xFFFF);
            }
}

template <typename T_Int>
void tst_QAlgorithms::popCount_impl()
{
    QFETCH(quint64, input);
    QFETCH(uint, expected);

    const T_Int value = static_cast<T_Int>(input);

    QCOMPARE(qPopulationCount(value), expected);
}

void tst_QAlgorithms::countTrailing_data_impl(size_t sizeof_T_Int)
{
    using namespace QTest;
    addColumn<quint64>("input");
    addColumn<uint>("expected");

    int nibs = sizeof_T_Int*2;

    newRow(("0x"+QByteArray::number(0,16).rightJustified(nibs,'0')).constData()) << Q_UINT64_C(0) << uint(sizeof_T_Int*8);
    for (uint i = 0; i < sizeof_T_Int*8; ++i) {
        const quint64 input = Q_UINT64_C(1) << i;
        newRow(("0x"+QByteArray::number(input,16).rightJustified(nibs,'0')).constData()) << input << i;
    }

    quint64 type_mask;
    if (sizeof_T_Int>=8)
        type_mask = ~Q_UINT64_C(0);
    else
        type_mask = (Q_UINT64_C(1) << (sizeof_T_Int*8))-1;

    // and some random ones:
    for (uint i = 0; i < sizeof_T_Int*8; ++i) {
        for (uint j = 0; j < sizeof_T_Int*3; ++j) {  // 3 is arbitrary
            const quint64 r = QRandomGenerator::global()->generate64();
            const quint64 b = Q_UINT64_C(1) << i;
            const quint64 mask = ((~(b-1)) ^ b) & type_mask;
            const quint64 input = (r&mask) | b;
            newRow(("0x"+QByteArray::number(input,16).rightJustified(nibs,'0')).constData()) << input << i;
        }
    }
}

template <typename T_Int>
void tst_QAlgorithms::countTrailing_impl()
{
    QFETCH(quint64, input);
    QFETCH(uint, expected);

    const T_Int value = static_cast<T_Int>(input);

    QCOMPARE(qCountTrailingZeroBits(value), expected);
}

void tst_QAlgorithms::countLeading_data_impl(size_t sizeof_T_Int)
{
    using namespace QTest;
    addColumn<quint64>("input");
    addColumn<uint>("expected");

    int nibs = sizeof_T_Int*2;

    newRow(("0x"+QByteArray::number(0,16).rightJustified(nibs,'0')).constData()) << Q_UINT64_C(0) << uint(sizeof_T_Int*8);
    for (uint i = 0; i < sizeof_T_Int*8; ++i) {
        const quint64 input = Q_UINT64_C(1) << i;
        newRow(("0x"+QByteArray::number(input,16).rightJustified(nibs,'0')).constData()) << input << uint(sizeof_T_Int*8-i-1);
    }

    // and some random ones:
    for (uint i = 0; i < sizeof_T_Int*8; ++i) {
        for (uint j = 0; j < sizeof_T_Int*3; ++j) {  // 3 is arbitrary
            const quint64 r = QRandomGenerator::global()->generate64();
            const quint64 b = Q_UINT64_C(1) << i;
            const quint64 mask = b-1;
            const quint64 input = (r&mask) | b;
            newRow(("0x"+QByteArray::number(input,16).rightJustified(nibs,'0')).constData()) << input << uint(sizeof_T_Int*8-i-1);
        }
    }
}

template <typename T_Int>
void tst_QAlgorithms::countLeading_impl()
{
    QFETCH(quint64, input);
    QFETCH(uint, expected);

    const T_Int value = static_cast<T_Int>(input);

    QCOMPARE(qCountLeadingZeroBits(value), expected);
}

QTEST_APPLESS_MAIN(tst_QAlgorithms)
#include "tst_qalgorithms.moc"

