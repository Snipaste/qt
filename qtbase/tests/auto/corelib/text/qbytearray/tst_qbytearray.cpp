/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QTest>

#include <qbytearray.h>
#include <qfile.h>
#include <qhash.h>
#include <limits.h>
#include <private/qtools_p.h>

class tst_QByteArray : public QObject
{
    Q_OBJECT

public:
    tst_QByteArray();
private slots:
    void swap();
    void qChecksum_data();
    void qChecksum();
    void qCompress_data();
#ifndef QT_NO_COMPRESS
    void qCompress();
    void qUncompressCorruptedData_data();
    void qUncompressCorruptedData();
    void qCompressionZeroTermination();
#endif
    void constByteArray();
    void leftJustified();
    void rightJustified();
    void setNum();
    void iterators();
    void reverseIterators();
    void split_data();
    void split();
    void base64_data();
    void base64();
    void fromBase64_data();
    void fromBase64();
    void qvsnprintf();
    void qstrlen();
    void qstrnlen();
    void qstrcpy();
    void qstrncpy();
    void chop_data();
    void chop();
    void prepend();
    void prependExtended_data();
    void prependExtended();
    void append();
    void appendExtended_data();
    void appendExtended();
    void insert();
    void insertExtended_data();
    void insertExtended();
    void remove_data();
    void remove();
    void removeIf();
    void replace_data();
    void replace();
    void replaceWithSpecifiedLength();
    void toLong_data();
    void toLong();
    void toULong_data();
    void toULong();
    void toLongLong_data();
    void toLongLong();
    void toULongLong_data();
    void toULongLong();

    void number();
    void number_base_data();
    void number_base();
    void toShort();
    void toUShort();
    void toInt_data();
    void toInt();
    void toUInt_data();
    void toUInt();
    void toFloat();
    void toDouble_data();
    void toDouble();
    void blockSizeCalculations();

    void resizeAfterFromRawData();
    void appendAfterFromRawData();
    void toFromHex_data();
    void toFromHex();
    void toFromPercentEncoding();
    void fromPercentEncoding_data();
    void fromPercentEncoding();
    void toPercentEncoding_data();
    void toPercentEncoding();
    void toPercentEncoding2_data();
    void toPercentEncoding2();

    void qstrcmp_data();
    void qstrcmp();
    void compare_singular();
    void compareCharStar_data();
    void compareCharStar();

    void repeatedSignature() const;
    void repeated() const;
    void repeated_data() const;

    void byteRefDetaching() const;

    void reserve();
    void reserveExtended_data();
    void reserveExtended();
    void movablity_data();
    void movablity();
    void literals();
    void userDefinedLiterals();
    void toUpperLower_data();
    void toUpperLower();
    void isUpper();
    void isLower();

    void macTypes();

    void stdString();

    void emptyAndClear();
    void fill();
    void dataPointers();
    void truncate();
    void trimmed();
    void trimmed_data();
    void simplified();
    void simplified_data();
    void left();
    void right();
    void mid();
    void length();
    void length_data();
};

static const QByteArray::DataPointer staticStandard = {
    nullptr,
    const_cast<char *>("data"),
    4
};
static const QByteArray::DataPointer staticNotNullTerminated = {
    nullptr,
    const_cast<char *>("dataBAD"),
    4
};

template <class T> const T &verifyZeroTermination(const T &t) { return t; }

QByteArray verifyZeroTermination(const QByteArray &ba)
{
    // This test does some evil stuff, it's all supposed to work.

    QByteArray::DataPointer baDataPtr = const_cast<QByteArray &>(ba).data_ptr();

    // Skip if !isMutable() as those offer no guarantees
    if (!baDataPtr->isMutable())
        return ba;

    int baSize = ba.size();
    char baTerminator = ba.constData()[baSize];
    if ('\0' != baTerminator)
        return QString::fromUtf8(
            "*** Result ('%1') not null-terminated: 0x%2 ***").arg(QString::fromUtf8(ba))
                .arg(baTerminator, 2, 16, QChar('0')).toUtf8();

    // Skip mutating checks on shared strings
    if (baDataPtr->isShared())
        return ba;

    const char *baData = ba.constData();
    const QByteArray baCopy(baData, baSize); // Deep copy

    const_cast<char *>(baData)[baSize] = 'x';
    if ('x' != ba.constData()[baSize]) {
        return "*** Failed to replace null-terminator in "
                "result ('" + ba + "') ***";
    }
    if (ba != baCopy) {
        return  "*** Result ('" + ba + "') differs from its copy "
                "after null-terminator was replaced ***";
    }
    const_cast<char *>(baData)[baSize] = '\0'; // Restore sanity

    return ba;
}

// Overriding QTest's QCOMPARE, to check QByteArray for null termination
#undef QCOMPARE
#define QCOMPARE(actual, expected)                                      \
    do {                                                                \
        if (!QTest::qCompare(verifyZeroTermination(actual), expected,   \
                #actual, #expected, __FILE__, __LINE__))                \
            return;                                                     \
    } while (0)                                                         \
    /**/
#undef QTEST
#define QTEST(actual, testElement)                                      \
    do {                                                                \
        if (!QTest::qTest(verifyZeroTermination(actual), testElement,   \
                #actual, #testElement, __FILE__, __LINE__))             \
            return;                                                     \
    } while (0)                                                         \
    /**/

Q_DECLARE_METATYPE(QByteArray::Base64DecodingStatus);

tst_QByteArray::tst_QByteArray()
{
}

void tst_QByteArray::qChecksum_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<uint>("len");
    QTest::addColumn<Qt::ChecksumType>("standard");
    QTest::addColumn<uint>("checksum");

    // Examples from ISO 14443-3
    QTest::newRow("1") << QByteArray("\x00\x00", 2)         << 2U << Qt::ChecksumItuV41  << 0x1EA0U;
    QTest::newRow("2") << QByteArray("\x12\x34", 2)         << 2U << Qt::ChecksumItuV41  << 0xCF26U;
    QTest::newRow("3") << QByteArray("\x00\x00\x00", 3)     << 3U << Qt::ChecksumIso3309 << 0xC6CCU;
    QTest::newRow("4") << QByteArray("\x0F\xAA\xFF", 3)     << 3U << Qt::ChecksumIso3309 << 0xD1FCU;
    QTest::newRow("5") << QByteArray("\x0A\x12\x34\x56", 4) << 4U << Qt::ChecksumIso3309 << 0xF62CU;
}

void tst_QByteArray::qChecksum()
{
    QFETCH(QByteArray, data);
    QFETCH(uint, len);
    QFETCH(Qt::ChecksumType, standard);
    QFETCH(uint, checksum);

    QCOMPARE(data.length(), int(len));
    if (standard == Qt::ChecksumIso3309) {
        QCOMPARE(::qChecksum(QByteArrayView(data.constData(), len)), static_cast<quint16>(checksum));
    }
    QCOMPARE(::qChecksum(QByteArrayView(data.constData(), len), standard), static_cast<quint16>(checksum));
}

void tst_QByteArray::qCompress_data()
{
    QTest::addColumn<QByteArray>("ba");

    const int size1 = 1024*1024;
    QByteArray ba1( size1, 0 );

    QTest::newRow( "00" ) << QByteArray();

    int i;
    for ( i=0; i<size1; i++ )
        ba1[i] = (char)( i / 1024 );
    QTest::newRow( "01" ) << ba1;

    for ( i=0; i<size1; i++ )
        ba1[i] = (char)( i % 256 );
    QTest::newRow( "02" ) << ba1;

    ba1.fill( 'A' );
    QTest::newRow( "03" ) << ba1;

    QFile file( QFINDTESTDATA("rfc3252.txt") );
    QVERIFY( file.open(QIODevice::ReadOnly) );
    QTest::newRow( "04" ) << file.readAll();
}

#ifndef QT_NO_COMPRESS
void tst_QByteArray::qCompress()
{
    QFETCH( QByteArray, ba );
    QByteArray compressed = ::qCompress( ba );
    QTEST( ::qUncompress( compressed ), "ba" );
}

void tst_QByteArray::qUncompressCorruptedData_data()
{
    QTest::addColumn<QByteArray>("in");

    QTest::newRow("0x00000000") << QByteArray("\x00\x00\x00\x00", 4);
    QTest::newRow("0x000000ff") << QByteArray("\x00\x00\x00\xff", 4);
    QTest::newRow("0x3f000000") << QByteArray("\x3f\x00\x00\x00", 4);
    QTest::newRow("0x3fffffff") << QByteArray("\x3f\xff\xff\xff", 4);
    QTest::newRow("0x7fffff00") << QByteArray("\x7f\xff\xff\x00", 4);
    QTest::newRow("0x7fffffff") << QByteArray("\x7f\xff\xff\xff", 4);
    QTest::newRow("0x80000000") << QByteArray("\x80\x00\x00\x00", 4);
    QTest::newRow("0x800000ff") << QByteArray("\x80\x00\x00\xff", 4);
    QTest::newRow("0xcf000000") << QByteArray("\xcf\x00\x00\x00", 4);
    QTest::newRow("0xcfffffff") << QByteArray("\xcf\xff\xff\xff", 4);
    QTest::newRow("0xffffff00") << QByteArray("\xff\xff\xff\x00", 4);
    QTest::newRow("0xffffffff") << QByteArray("\xff\xff\xff\xff", 4);
}

// Corrupt data causes this test to lock up on HP-UX / PA-RISC with gcc,
// SOLARIS, and Windows.
// This test is expected to produce some warning messages in the test output.
void tst_QByteArray::qUncompressCorruptedData()
{
#if !(defined(Q_OS_HPUX) && !defined(__ia64) && defined(Q_CC_GNU)) && !defined(Q_OS_SOLARIS) && !defined(Q_OS_WIN)
    QFETCH(QByteArray, in);

    QByteArray res;
    res = ::qUncompress(in);
    QCOMPARE(res, QByteArray());

    res = ::qUncompress(in + "blah");
    QCOMPARE(res, QByteArray());
#else
    QSKIP("This test freezes on this platform");
#endif
}

void tst_QByteArray::qCompressionZeroTermination()
{
    QString s = "Hello, I'm a string.";
    QByteArray ba = ::qUncompress(::qCompress(s.toLocal8Bit()));
    QVERIFY((int) *(ba.data() + ba.size()) == 0);
}

#endif

void tst_QByteArray::constByteArray()
{
    const char *ptr = "abc";
    QByteArray cba = QByteArray::fromRawData(ptr, 3);
    QVERIFY(cba.constData() == ptr);
    cba.squeeze();
    QVERIFY(cba.constData() == ptr);
    cba.detach();
    QVERIFY(cba.size() == 3);
    QVERIFY(cba.capacity() == 3);
    QVERIFY(cba.constData() != ptr);
    QVERIFY(cba.constData()[0] == 'a');
    QVERIFY(cba.constData()[1] == 'b');
    QVERIFY(cba.constData()[2] == 'c');
    QVERIFY(cba.constData()[3] == '\0');
}

void tst_QByteArray::leftJustified()
{
    QByteArray a;

    QCOMPARE(a.leftJustified(3, '-'), QByteArray("---"));
    QCOMPARE(a.leftJustified(2, ' '), QByteArray("  "));
    QVERIFY(!a.isDetached());

    a = "ABC";
    QCOMPARE(a.leftJustified(5,'-'), QByteArray("ABC--"));
    QCOMPARE(a.leftJustified(4,'-'), QByteArray("ABC-"));
    QCOMPARE(a.leftJustified(4), QByteArray("ABC "));
    QCOMPARE(a.leftJustified(3), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(2), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(1), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(0), QByteArray("ABC"));

    QCOMPARE(a.leftJustified(4,' ',true), QByteArray("ABC "));
    QCOMPARE(a.leftJustified(3,' ',true), QByteArray("ABC"));
    QCOMPARE(a.leftJustified(2,' ',true), QByteArray("AB"));
    QCOMPARE(a.leftJustified(1,' ',true), QByteArray("A"));
    QCOMPARE(a.leftJustified(0,' ',true), QByteArray(""));
}

void tst_QByteArray::rightJustified()
{
    QByteArray a;

    QCOMPARE(a.rightJustified(3, '-'), QByteArray("---"));
    QCOMPARE(a.rightJustified(2, ' '), QByteArray("  "));
    QVERIFY(!a.isDetached());

    a="ABC";
    QCOMPARE(a.rightJustified(5,'-'),QByteArray("--ABC"));
    QCOMPARE(a.rightJustified(4,'-'),QByteArray("-ABC"));
    QCOMPARE(a.rightJustified(4),QByteArray(" ABC"));
    QCOMPARE(a.rightJustified(3),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(2),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(1),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(0),QByteArray("ABC"));

    QCOMPARE(a.rightJustified(4,'-',true),QByteArray("-ABC"));
    QCOMPARE(a.rightJustified(4,' ',true),QByteArray(" ABC"));
    QCOMPARE(a.rightJustified(3,' ',true),QByteArray("ABC"));
    QCOMPARE(a.rightJustified(2,' ',true),QByteArray("AB"));
    QCOMPARE(a.rightJustified(1,' ',true),QByteArray("A"));
    QCOMPARE(a.rightJustified(0,' ',true),QByteArray(""));
    QCOMPARE(a,QByteArray("ABC"));
}

void tst_QByteArray::setNum()
{
    QByteArray a;
    QCOMPARE(a.setNum(-1), QByteArray("-1"));
    QCOMPARE(a.setNum(0), QByteArray("0"));
    QCOMPARE(a.setNum(0, 2), QByteArray("0"));
    QCOMPARE(a.setNum(0, 36), QByteArray("0"));
    QCOMPARE(a.setNum(1), QByteArray("1"));
    QCOMPARE(a.setNum(35, 36), QByteArray("z"));
    QCOMPARE(a.setNum(37, 2), QByteArray("100101"));
    QCOMPARE(a.setNum(37, 36), QByteArray("11"));

    QCOMPARE(a.setNum(short(-1), 16), QByteArray("-1"));
    QCOMPARE(a.setNum(int(-1), 16), QByteArray("-1"));
    QCOMPARE(a.setNum(qlonglong(-1), 16), QByteArray("-1"));

    QCOMPARE(a.setNum(short(-1), 10), QByteArray("-1"));
    QCOMPARE(a.setNum(int(-1), 10), QByteArray("-1"));
    QCOMPARE(a.setNum(qlonglong(-1), 10), QByteArray("-1"));

    QCOMPARE(a.setNum(-123), QByteArray("-123"));
    QCOMPARE(a.setNum(0x123, 16), QByteArray("123"));
    QCOMPARE(a.setNum(short(123)), QByteArray("123"));

    QCOMPARE(a.setNum(1.23), QByteArray("1.23"));
    QCOMPARE(a.setNum(1.234567), QByteArray("1.23457"));

    // Note that there are no 'long' overloads, so not all of the
    // QString::setNum() tests can be re-used.
    QCOMPARE(a.setNum(Q_INT64_C(123)), QByteArray("123"));
    // 2^40 == 1099511627776
    QCOMPARE(a.setNum(Q_INT64_C(-1099511627776)), QByteArray("-1099511627776"));
    QCOMPARE(a.setNum(Q_UINT64_C(1099511627776)), QByteArray("1099511627776"));
    QCOMPARE(a.setNum(Q_INT64_C(9223372036854775807)), // LLONG_MAX
            QByteArray("9223372036854775807"));
    QCOMPARE(a.setNum(-Q_INT64_C(9223372036854775807) - Q_INT64_C(1)),
            QByteArray("-9223372036854775808"));
    QCOMPARE(a.setNum(Q_UINT64_C(18446744073709551615)), // ULLONG_MAX
            QByteArray("18446744073709551615"));
    QCOMPARE(a.setNum(0.000000000931322574615478515625), QByteArray("9.31323e-10"));
}

void tst_QByteArray::iterators()
{
    QByteArray emptyArr;
    QCOMPARE(emptyArr.constBegin(), emptyArr.constEnd());
    QCOMPARE(emptyArr.cbegin(), emptyArr.cend());
    QVERIFY(!emptyArr.isDetached());
    QCOMPARE(emptyArr.begin(), emptyArr.end());

    QByteArray a("0123456789");

    auto it = a.begin();
    auto constIt = a.cbegin();
    qsizetype idx = 0;

    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it++;
    constIt++;
    idx++;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it += 5;
    constIt += 5;
    idx += 5;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it -= 3;
    constIt -= 3;
    idx -= 3;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);

    it--;
    constIt--;
    idx--;
    QCOMPARE(*it, a[idx]);
    QCOMPARE(*constIt, a[idx]);
}

void tst_QByteArray::reverseIterators()
{
    QByteArray emptyArr;
    QCOMPARE(emptyArr.crbegin(), emptyArr.crend());
    QVERIFY(!emptyArr.isDetached());
    QCOMPARE(emptyArr.rbegin(), emptyArr.rend());

    QByteArray s = "1234";
    QByteArray sr = s;
    std::reverse(sr.begin(), sr.end());
    const QByteArray &csr = sr;
    QVERIFY(std::equal(s.begin(), s.end(), sr.rbegin()));
    QVERIFY(std::equal(s.begin(), s.end(), sr.crbegin()));
    QVERIFY(std::equal(s.begin(), s.end(), csr.rbegin()));
    QVERIFY(std::equal(sr.rbegin(), sr.rend(), s.begin()));
    QVERIFY(std::equal(sr.crbegin(), sr.crend(), s.begin()));
    QVERIFY(std::equal(csr.rbegin(), csr.rend(), s.begin()));
}

void tst_QByteArray::split_data()
{
    QTest::addColumn<QByteArray>("sample");
    QTest::addColumn<int>("size");

    QTest::newRow("1") << QByteArray("-rw-r--r--  1 0  0  519240 Jul  9  2002 bigfile") << 14;
    QTest::newRow("2") << QByteArray("abcde") << 1;
    QTest::newRow("one empty") << QByteArray("") << 1;
    QTest::newRow("two empty") << QByteArray(" ") << 2;
    QTest::newRow("three empty") << QByteArray("  ") << 3;
    QTest::newRow("null") << QByteArray() << 1;
}

void tst_QByteArray::split()
{
    QFETCH(QByteArray, sample);
    QFETCH(int, size);

    QList<QByteArray> list = sample.split(' ');
    QCOMPARE(list.count(), size);
}

void tst_QByteArray::swap()
{
    QByteArray b1 = "b1", b2 = "b2";
    b1.swap(b2);
    QCOMPARE(b1, QByteArray("b2"));
    QCOMPARE(b2, QByteArray("b1"));
}

void tst_QByteArray::base64_data()
{
    QTest::addColumn<QByteArray>("rawdata");
    QTest::addColumn<QByteArray>("base64");

    QTest::newRow("null") << QByteArray() << QByteArray();
    QTest::newRow("1") << QByteArray("") << QByteArray("");
    QTest::newRow("2") << QByteArray("1") << QByteArray("MQ==");
    QTest::newRow("3") << QByteArray("12") << QByteArray("MTI=");
    QTest::newRow("4") << QByteArray("123") << QByteArray("MTIz");
    QTest::newRow("5") << QByteArray("1234") << QByteArray("MTIzNA==");
    QTest::newRow("6") << QByteArray("\n") << QByteArray("Cg==");
    QTest::newRow("7") << QByteArray("a\n") << QByteArray("YQo=");
    QTest::newRow("8") << QByteArray("ab\n") << QByteArray("YWIK");
    QTest::newRow("9") << QByteArray("abc\n") << QByteArray("YWJjCg==");
    QTest::newRow("a") << QByteArray("abcd\n") << QByteArray("YWJjZAo=");
    QTest::newRow("b") << QByteArray("abcde\n") << QByteArray("YWJjZGUK");
    QTest::newRow("c") << QByteArray("abcdef\n") << QByteArray("YWJjZGVmCg==");
    QTest::newRow("d") << QByteArray("abcdefg\n") << QByteArray("YWJjZGVmZwo=");
    QTest::newRow("e") << QByteArray("abcdefgh\n") << QByteArray("YWJjZGVmZ2gK");

    QByteArray ba;
    ba.resize(256);
    for (int i = 0; i < 256; ++i)
        ba[i] = i;
    QTest::newRow("f") << ba << QByteArray("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==");

    QTest::newRow("g") << QByteArray("foo\0bar", 7) << QByteArray("Zm9vAGJhcg==");
    QTest::newRow("h") << QByteArray("f\xd1oo\x9ctar") << QByteArray("ZtFvb5x0YXI=");
    QTest::newRow("i") << QByteArray("\"\0\0\0\0\0\0\"", 8) << QByteArray("IgAAAAAAACI=");
}


void tst_QByteArray::base64()
{
    QFETCH(QByteArray, rawdata);
    QFETCH(QByteArray, base64);
    QByteArray::FromBase64Result result;

    result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    QByteArray arr = base64;
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    QByteArray arr64 = rawdata.toBase64();
    QCOMPARE(arr64, base64);

    arr64 = rawdata.toBase64(QByteArray::Base64Encoding);
    QCOMPARE(arr64, base64);

    QByteArray base64noequals = base64;
    base64noequals.replace('=', "");
    arr64 = rawdata.toBase64(QByteArray::Base64Encoding | QByteArray::OmitTrailingEquals);
    QCOMPARE(arr64, base64noequals);

    QByteArray base64url = base64;
    base64url.replace('/', '_').replace('+', '-');
    arr64 = rawdata.toBase64(QByteArray::Base64UrlEncoding);
    QCOMPARE(arr64, base64url);

    QByteArray base64urlnoequals = base64url;
    base64urlnoequals.replace('=', "");
    arr64 = rawdata.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QCOMPARE(arr64, base64urlnoequals);
}

//different from the previous test as the input are invalid
void tst_QByteArray::fromBase64_data()
{
    QTest::addColumn<QByteArray>("rawdata");
    QTest::addColumn<QByteArray>("base64");
    QTest::addColumn<QByteArray::Base64DecodingStatus>("status");

    QTest::newRow("1") << QByteArray("") << QByteArray("  ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("2") << QByteArray("1") << QByteArray("MQ=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("3") << QByteArray("12") << QByteArray("MTI       ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("4") << QByteArray("123") << QByteArray("M=TIz") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("5") << QByteArray("1234") << QByteArray("MTI zN A ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("6") << QByteArray("\n") << QByteArray("Cg@") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("7") << QByteArray("a\n") << QByteArray("======YQo=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("8") << QByteArray("ab\n") << QByteArray("Y\nWIK ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("9") << QByteArray("abc\n") << QByteArray("YWJjCg=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("a") << QByteArray("abcd\n") << QByteArray("YWJ\1j\x9cZAo=") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("b") << QByteArray("abcde\n") << QByteArray("YW JjZ\n G\tUK") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("c") << QByteArray("abcdef\n") << QByteArray("YWJjZGVmCg=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("d") << QByteArray("abcdefg\n") << QByteArray("YWJ\rjZGVmZwo") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("e") << QByteArray("abcdefgh\n") << QByteArray("YWJjZGVmZ2gK====") << QByteArray::Base64DecodingStatus::IllegalPadding;

    QByteArray ba;
    ba.resize(256);
    for (int i = 0; i < 256; ++i)
        ba[i] = i;
    QTest::newRow("f") << ba << QByteArray("AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Nj\n"
                                           "c4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1u\n"
                                           "b3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpa\n"
                                           "anqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd\n"
                                           "3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==                            ") << QByteArray::Base64DecodingStatus::IllegalCharacter;


    QTest::newRow("g") << QByteArray("foo\0bar", 7) << QByteArray("Zm9vAGJhcg=") << QByteArray::Base64DecodingStatus::IllegalInputLength;
    QTest::newRow("h") << QByteArray("f\xd1oo\x9ctar") << QByteArray("ZtFvb5x 0YXI") << QByteArray::Base64DecodingStatus::IllegalCharacter;
    QTest::newRow("i") << QByteArray("\"\0\0\0\0\0\0\"", 8) << QByteArray("IgAAAAAAACI ") << QByteArray::Base64DecodingStatus::IllegalCharacter;
}


void tst_QByteArray::fromBase64()
{
    QFETCH(QByteArray, rawdata);
    QFETCH(QByteArray, base64);
    QFETCH(QByteArray::Base64DecodingStatus, status);

    QByteArray::FromBase64Result result;

    result = QByteArray::fromBase64Encoding(base64);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64Encoding);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    QByteArray arr = base64;
    QVERIFY(!arr.isDetached());
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(!arr.isEmpty());
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    arr.detach();
    QVERIFY(arr.isDetached());
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(arr.isEmpty());
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    // try "base64url" encoding
    QByteArray base64url = base64;
    base64url.replace('/', '_').replace('+', '-');
    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    arr = base64url;
    arr.detach();
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(arr.isEmpty());
    QVERIFY(!result);
    QCOMPARE(result.decodingStatus, status);
    QVERIFY(result.decoded.isEmpty());

    if (base64 != base64url) {
        // check that the invalid decodings fail
        result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64UrlEncoding);
        QVERIFY(result);
        QVERIFY(result.decoded != rawdata);
        result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64Encoding);
        QVERIFY(result);
        QVERIFY(result.decoded != rawdata);

        result = QByteArray::fromBase64Encoding(base64, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());

        arr = base64;
        arr.detach();
        result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(arr.isEmpty());
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());

        result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());

        arr = base64url;
        arr.detach();
        result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        QVERIFY(arr.isEmpty());
        QVERIFY(!result);
        QVERIFY(result.decoded.isEmpty());
    }

    // also remove padding, if any, and test again. note that by doing
    // that we might be sanitizing the illegal input, so we can't assume now
    // that result will be invalid in all cases
    {
        auto rightmostNotEqualSign = std::find_if_not(base64url.rbegin(), base64url.rend(), [](char c) { return c == '='; });
        base64url.chop(std::distance(base64url.rbegin(), rightmostNotEqualSign)); // no QByteArray::erase...
    }

    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding);
    QVERIFY(result);
    QCOMPARE(result.decoded, rawdata);

    result = QByteArray::fromBase64Encoding(base64url, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    if (result) {
        QCOMPARE(result.decoded, rawdata);
    } else {
        QCOMPARE(result.decodingStatus, status);
        QVERIFY(result.decoded.isEmpty());
    }

    arr = base64url;
    arr.detach();
    result = QByteArray::fromBase64Encoding(std::move(arr), QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    QVERIFY(arr.isEmpty());
    if (result) {
        QCOMPARE(result.decoded, rawdata);
    } else {
        QCOMPARE(result.decodingStatus, status);
        QVERIFY(result.decoded.isEmpty());
    }
}

void tst_QByteArray::qvsnprintf()
{
    char buf[20];
    memset(buf, 42, sizeof(buf));

    QCOMPARE(::qsnprintf(buf, 10, "%s", "bubu"), 4);
    QCOMPARE(static_cast<const char *>(buf), "bubu");
#ifndef Q_CC_MSVC
    // MSVC implementation of vsnprintf overwrites bytes after null terminator so this would fail.
    QCOMPARE(buf[5], char(42));
#endif

    memset(buf, 42, sizeof(buf));
    QCOMPARE(::qsnprintf(buf, 5, "%s", "bubu"), 4);
    QCOMPARE(static_cast<const char *>(buf), "bubu");
    QCOMPARE(buf[5], char(42));

    memset(buf, 42, sizeof(buf));
#ifdef Q_OS_WIN
    // VS 2005 uses the Qt implementation of vsnprintf.
# if defined(_MSC_VER)
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), -1);
    QCOMPARE(static_cast<const char*>(buf), "bu");
# else
    // windows has to do everything different, of course.
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), -1);
    buf[19] = '\0';
    QCOMPARE(static_cast<const char *>(buf), "bub****************");
# endif
#else
    QCOMPARE(::qsnprintf(buf, 3, "%s", "bubu"), 4);
    QCOMPARE(static_cast<const char*>(buf), "bu");
#endif
    QCOMPARE(buf[4], char(42));

#ifndef Q_OS_WIN
    memset(buf, 42, sizeof(buf));
    QCOMPARE(::qsnprintf(buf, 10, ""), 0);
#endif
}


void tst_QByteArray::qstrlen()
{
    const char *src = "Something about ... \0 a string.";
    QCOMPARE(::qstrlen((char*)0), (uint)0);
    QCOMPARE(::qstrlen(src), (uint)20);
}

void tst_QByteArray::qstrnlen()
{
    const char *src = "Something about ... \0 a string.";
    QCOMPARE(::qstrnlen((char*)0, 1), (uint)0);
    QCOMPARE(::qstrnlen(src, 31), (uint)20);
    QCOMPARE(::qstrnlen(src, 19), (uint)19);
    QCOMPARE(::qstrnlen(src, 21), (uint)20);
    QCOMPARE(::qstrnlen(src, 20), (uint)20);
}

void tst_QByteArray::qstrcpy()
{
    const char *src = "Something about ... \0 a string.";
    const char *expected = "Something about ... ";
    char dst[128];

    QCOMPARE(::qstrcpy(0, 0), (char*)0);
    QCOMPARE(::qstrcpy(dst, 0), (char*)0);

    QCOMPARE(::qstrcpy(dst ,src), (char *)dst);
    QCOMPARE((char *)dst, const_cast<char *>(expected));
}

void tst_QByteArray::qstrncpy()
{
    QByteArray src(1024, 'a'), dst(1024, 'b');

    // dst == nullptr
    QCOMPARE(::qstrncpy(0, src.data(),  0), (char*)0);
    QCOMPARE(::qstrncpy(0, src.data(), 10), (char*)0);

    // src == nullptr
    QCOMPARE(::qstrncpy(dst.data(), 0,  0), (char*)0);
    QCOMPARE(::qstrncpy(dst.data(), 0, 10), (char*)0);

    // valid pointers, but len == 0
    QCOMPARE(::qstrncpy(dst.data(), src.data(), 0), dst.data());
    QCOMPARE(*dst.data(), 'b'); // must not have written to dst

    // normal copy
    QCOMPARE(::qstrncpy(dst.data(), src.data(), src.size()), dst.data());

    src = QByteArray( "Tumdelidum" );
    QCOMPARE(QByteArray(::qstrncpy(dst.data(), src.data(), src.size())),
            QByteArray("Tumdelidu"));

    // normal copy with length is longer than necessary
    src = QByteArray( "Tumdelidum\0foo" );
    dst.resize(128*1024);
    QCOMPARE(QByteArray(::qstrncpy(dst.data(), src.data(), dst.size())),
            QByteArray("Tumdelidum"));
}

void tst_QByteArray::chop_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("choplength");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("1") << QByteArray("short1") << 128 << QByteArray();
    QTest::newRow("2") << QByteArray("short2") << int(strlen("short2"))
                    << QByteArray();
    QTest::newRow("3") << QByteArray("abcdef\0foo", 10) << 2
                    << QByteArray("abcdef\0f", 8);
    QTest::newRow("4") << QByteArray("STARTTLS\r\n") << 2
                    << QByteArray("STARTTLS");
    QTest::newRow("5") << QByteArray("") << 1 << QByteArray();
    QTest::newRow("6") << QByteArray("foo") << 0 << QByteArray("foo");
    QTest::newRow("7") << QByteArray(0) << 28 << QByteArray();
    QTest::newRow("null 0") << QByteArray() << 0 << QByteArray();
    QTest::newRow("null 10") << QByteArray() << 10 << QByteArray();
}

void tst_QByteArray::chop()
{
    QFETCH(QByteArray, src);
    QFETCH(int, choplength);
    QFETCH(QByteArray, expected);

    src.chop(choplength);
    QCOMPARE(src, expected);
}

void tst_QByteArray::prepend()
{
    const char data[] = "data";

    QCOMPARE(QByteArray().prepend(QByteArray()), QByteArray());
    QCOMPARE(QByteArray().prepend('a'), QByteArray("a"));
    QCOMPARE(QByteArray().prepend(2, 'a'), QByteArray("aa"));
    QCOMPARE(QByteArray().prepend(QByteArray("data")), QByteArray("data"));
    QCOMPARE(QByteArray().prepend(data), QByteArray("data"));
    QCOMPARE(QByteArray().prepend(data, 2), QByteArray("da"));
    QCOMPARE(QByteArray().prepend(QByteArrayView(data)), QByteArray("data"));

    QByteArray ba("foo");
    QCOMPARE(ba.prepend((char*)0), QByteArray("foo"));
    QCOMPARE(ba.prepend(QByteArray()), QByteArray("foo"));
    QCOMPARE(ba.prepend("1"), QByteArray("1foo"));
    QCOMPARE(ba.prepend(QByteArray("2")), QByteArray("21foo"));
    QCOMPARE(ba.prepend('3'), QByteArray("321foo"));
    QCOMPARE(ba.prepend(-1, 'x'), QByteArray("321foo"));
    QCOMPARE(ba.prepend(3, 'x'), QByteArray("xxx321foo"));
    QCOMPARE(ba.prepend("\0 ", 2), QByteArray::fromRawData("\0 xxx321foo", 11));

    QByteArray tenChars;
    tenChars.reserve(10);
    QByteArray twoChars("ab");
    tenChars.prepend(twoChars);
    QCOMPARE(tenChars.capacity(), 10);
}

void tst_QByteArray::prependExtended_data()
{
    QTest::addColumn<QByteArray>("array");
    QTest::newRow("literal") << QByteArray(QByteArrayLiteral("data"));
    QTest::newRow("standard") << QByteArray(staticStandard);
    QTest::newRow("notNullTerminated") << QByteArray(staticNotNullTerminated);
    QTest::newRow("non static data") << QByteArray("data");
    QTest::newRow("from raw data") << QByteArray::fromRawData("data", 4);
    QTest::newRow("from raw data not terminated") << QByteArray::fromRawData("dataBAD", 4);
}

void tst_QByteArray::prependExtended()
{
    QFETCH(QByteArray, array);

    QCOMPARE(QByteArray().prepend(array), QByteArray("data"));
    QCOMPARE(QByteArray("").prepend(array), QByteArray("data"));

    QCOMPARE(array.prepend((char*)0), QByteArray("data"));
    QCOMPARE(array.prepend(QByteArray()), QByteArray("data"));
    QCOMPARE(array.prepend("1"), QByteArray("1data"));
    QCOMPARE(array.prepend(QByteArray("2")), QByteArray("21data"));
    QCOMPARE(array.prepend('3'), QByteArray("321data"));
    QCOMPARE(array.prepend(-1, 'x'), QByteArray("321data"));
    QCOMPARE(array.prepend(3, 'x'), QByteArray("xxx321data"));
    QCOMPARE(array.prepend("\0 ", 2), QByteArray::fromRawData("\0 xxx321data", 12));
    QCOMPARE(array.size(), 12);
}

void tst_QByteArray::append()
{
    const char data[] = "data";

    QCOMPARE(QByteArray().append(QByteArray()), QByteArray());
    QCOMPARE(QByteArray().append('a'), QByteArray("a"));
    QCOMPARE(QByteArray().append(2, 'a'), QByteArray("aa"));
    QCOMPARE(QByteArray().append(QByteArray("data")), QByteArray("data"));
    QCOMPARE(QByteArray().append(data), QByteArray("data"));
    QCOMPARE(QByteArray().append(data, 2), QByteArray("da"));
    QCOMPARE(QByteArray().append(QByteArrayView(data)), QByteArray("data"));

    QByteArray ba("foo");
    QCOMPARE(ba.append((char*)0), QByteArray("foo"));
    QCOMPARE(ba.append(QByteArray()), QByteArray("foo"));
    QCOMPARE(ba.append("1"), QByteArray("foo1"));
    QCOMPARE(ba.append(QByteArray("2")), QByteArray("foo12"));
    QCOMPARE(ba.append('3'), QByteArray("foo123"));
    QCOMPARE(ba.append(-1, 'x'), QByteArray("foo123"));
    QCOMPARE(ba.append(3, 'x'), QByteArray("foo123xxx"));
    QCOMPARE(ba.append("\0"), QByteArray("foo123xxx"));
    QCOMPARE(ba.append("\0", 1), QByteArray::fromRawData("foo123xxx\0", 10));
    QCOMPARE(ba.size(), 10);

    QByteArray tenChars;
    tenChars.reserve(10);
    QByteArray twoChars("ab");
    tenChars.append(twoChars);
    QCOMPARE(tenChars.capacity(), 10);

    {
        QByteArray prepended("abcd");
        prepended.prepend('a');
        const qsizetype freeAtEnd = prepended.data_ptr()->freeSpaceAtEnd();
        QVERIFY(prepended.size() + freeAtEnd < prepended.capacity());
        prepended += QByteArray(freeAtEnd, 'b');
        prepended.append('c');
        QCOMPARE(prepended, QByteArray("aabcd") + QByteArray(freeAtEnd, 'b') + QByteArray("c"));
    }

    {
        QByteArray prepended2("aaaaaaaaaa");
        while (prepended2.size())
            prepended2.remove(0, 1);
        QVERIFY(prepended2.data_ptr()->freeSpaceAtBegin() > 0);
        QByteArray array(prepended2.data_ptr()->freeSpaceAtEnd(), 'a');
        prepended2 += array;
        prepended2.append('b');
        QCOMPARE(prepended2, array + QByteArray("b"));
    }
}

void tst_QByteArray::appendExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::appendExtended()
{
    QFETCH(QByteArray, array);

    QCOMPARE(QByteArray().append(array), QByteArray("data"));
    QCOMPARE(QByteArray("").append(array), QByteArray("data"));

    QCOMPARE(array.append((char*)0), QByteArray("data"));
    QCOMPARE(array.append(QByteArray()), QByteArray("data"));
    QCOMPARE(array.append("1"), QByteArray("data1"));
    QCOMPARE(array.append(QByteArray("2")), QByteArray("data12"));
    QCOMPARE(array.append('3'), QByteArray("data123"));
    QCOMPARE(array.append(-1, 'x'), QByteArray("data123"));
    QCOMPARE(array.append(3, 'x'), QByteArray("data123xxx"));
    QCOMPARE(array.append("\0"), QByteArray("data123xxx"));
    QCOMPARE(array.append("\0", 1), QByteArray::fromRawData("data123xxx\0", 11));
    QCOMPARE(array.size(), 11);
}

void tst_QByteArray::insert()
{
    const char data[] = "data";

    QCOMPARE(QByteArray().insert(0, QByteArray()), QByteArray());
    QCOMPARE(QByteArray().insert(0, 'a'), QByteArray("a"));
    QCOMPARE(QByteArray().insert(0, 2, 'a'), QByteArray("aa"));
    QCOMPARE(QByteArray().insert(0, QByteArray("data")), QByteArray("data"));
    QCOMPARE(QByteArray().insert(0, data), QByteArray("data"));
    QCOMPARE(QByteArray().insert(0, data, 2), QByteArray("da"));
    QCOMPARE(QByteArray().insert(0, QByteArrayView(data)), QByteArray("data"));

    // insert into empty with offset
    QCOMPARE(QByteArray().insert(2, QByteArray()), QByteArray());
    QCOMPARE(QByteArray().insert(2, 'a'), QByteArray("  a"));
    QCOMPARE(QByteArray().insert(2, 2, 'a'), QByteArray("  aa"));
    QCOMPARE(QByteArray().insert(2, QByteArray("data")), QByteArray("  data"));
    QCOMPARE(QByteArray().insert(2, data), QByteArray("  data"));
    QCOMPARE(QByteArray().insert(2, data, 2), QByteArray("  da"));
    QCOMPARE(QByteArray().insert(2, QByteArrayView(data)), QByteArray("  data"));

    QByteArray ba("Meal");
    QCOMPARE(ba.insert(1, QByteArray("ontr")), QByteArray("Montreal"));
    QCOMPARE(ba.insert(ba.size(), "foo"), QByteArray("Montrealfoo"));

    ba = QByteArray("13");
    QCOMPARE(ba.insert(1, QByteArray("2")), QByteArray("123"));

    ba = "ac";
    QCOMPARE(ba.insert(1, 'b'), QByteArray("abc"));
    QCOMPARE(ba.size(), 3);

    ba = "ac";
    QCOMPARE(ba.insert(-1, 3, 'x'), QByteArray("ac"));
    QCOMPARE(ba.insert(1, 3, 'x'), QByteArray("axxxc"));
    QCOMPARE(ba.insert(6, 3, 'x'), QByteArray("axxxc xxx"));
    QCOMPARE(ba.size(), 9);

    ba = "ikl";
    QCOMPARE(ba.insert(1, "j"), QByteArray("ijkl"));
    QCOMPARE(ba.size(), 4);

    ba = "ab";
    QCOMPARE(ba.insert(1, "\0X\0", 3), QByteArray::fromRawData("a\0X\0b", 5));
    QCOMPARE(ba.size(), 5);

    ba = "Hello World";
    QCOMPARE(ba.insert(5, QByteArrayView(",")), QByteArray("Hello, World"));
    QCOMPARE(ba.size(), 12);

    ba = "one";
    QCOMPARE(ba.insert(1, ba), QByteArray("oonene"));
    QCOMPARE(ba.size(), 6);

    ba = "one";
    QCOMPARE(ba.insert(1, QByteArrayView(ba)), QByteArray("oonene"));
    QCOMPARE(ba.size(), 6);

    {
        ba = "one";
        ba.prepend('a');
        QByteArray b(ba.data_ptr()->freeSpaceAtEnd(), 'b');
        QCOMPARE(ba.insert(ba.size() + 1, QByteArrayView(b)), QByteArray("aone ") + b);
    }

    {
        ba = "onetwothree";
        while (ba.size() - 1)
            ba.remove(0, 1);
        QByteArray b(ba.data_ptr()->freeSpaceAtEnd() + 1, 'b');
        QCOMPARE(ba.insert(ba.size() + 1, QByteArrayView(b)), QByteArray("e ") + b);
    }

    {
        ba = "one";
        ba.prepend('a');
        const qsizetype freeAtEnd = ba.data_ptr()->freeSpaceAtEnd();
        QCOMPARE(ba.insert(ba.size() + 1, freeAtEnd + 1, 'b'),
                 QByteArray("aone ") + QByteArray(freeAtEnd + 1, 'b'));
    }

    {
        ba = "onetwothree";
        while (ba.size() - 1)
            ba.remove(0, 1);
        const qsizetype freeAtEnd = ba.data_ptr()->freeSpaceAtEnd();
        QCOMPARE(ba.insert(ba.size() + 1, freeAtEnd + 1, 'b'),
                 QByteArray("e ") + QByteArray(freeAtEnd + 1, 'b'));
    }
}

void tst_QByteArray::insertExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::insertExtended()
{
    QFETCH(QByteArray, array);
    QCOMPARE(array.insert(1, "i"), QByteArray("diata"));
    QCOMPARE(array.insert(1, 3, 'x'), QByteArray("dxxxiata"));
    QCOMPARE(array.size(), 8);
}

void tst_QByteArray::remove_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("length");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null 0 0") << QByteArray() << 0 << 0 << QByteArray();
    QTest::newRow("null 0 5") << QByteArray() << 0 << 5 << QByteArray();
    QTest::newRow("null 3 5") << QByteArray() << 3 << 5 << QByteArray();
    QTest::newRow("null -1 5") << QByteArray() << -1 << 5 << QByteArray();

    QTest::newRow("1") << QByteArray("Montreal") << 1 << 4
                    << QByteArray("Meal");
    QTest::newRow("2") << QByteArray() << 10 << 10 << QByteArray();
    QTest::newRow("3") << QByteArray("hi") << 0 << 10 << QByteArray();
    QTest::newRow("4") << QByteArray("Montreal") << 4 << 100
                    << QByteArray("Mont");

    // index out of range
    QTest::newRow("5") << QByteArray("Montreal") << 8 << 1
                    << QByteArray("Montreal");
    QTest::newRow("6") << QByteArray("Montreal") << 18 << 4
                    << QByteArray("Montreal");
}

void tst_QByteArray::remove()
{
    QFETCH(QByteArray, src);
    QFETCH(int, position);
    QFETCH(int, length);
    QFETCH(QByteArray, expected);
    QCOMPARE(src.remove(position, length), expected);
}

void tst_QByteArray::removeIf()
{
    auto removeA = [](const char c) { return c == 'a' || c == 'A'; };

    QByteArray a;
    QCOMPARE(a.removeIf(removeA), QByteArray());
    QVERIFY(!a.isDetached());

    a = QByteArray("aBcAbC");
    QCOMPARE(a.removeIf(removeA), QByteArray("BcbC"));
}

void tst_QByteArray::replace_data()
{
    // Try to cover both the index and specific char cases.
    // If "before" is empty, use "pos" as an index
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("len");
    QTest::addColumn<QByteArray>("before");
    QTest::addColumn<QByteArray>("after");
    QTest::addColumn<QByteArray>("expected");

    // Using pos

    QTest::newRow("1") << QByteArray("Say yes!") << 4 << 3 << QByteArray() << QByteArray("no")
                       << QByteArray("Say no!");
    QTest::newRow("2") << QByteArray("rock and roll") << 5 << 3 << QByteArray() << QByteArray("&")
                       << QByteArray("rock & roll");
    QTest::newRow("3") << QByteArray("foo") << 3 << 0 << QByteArray() << QByteArray("bar")
                       << QByteArray("foobar");
    QTest::newRow("4") << QByteArray() << 0 << 0 << QByteArray() << QByteArray() << QByteArray();
    // index out of range
    QTest::newRow("5") << QByteArray() << 3 << 0 << QByteArray() << QByteArray("hi")
                       << QByteArray("   hi");
    // Optimized path
    QTest::newRow("6") << QByteArray("abcdef") << 3 << 12 << QByteArray()
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("7") << QByteArray("abcdef") << 3 << 4 << QByteArray()
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("8") << QByteArray("abcdef") << 3 << 3 << QByteArray()
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijkl");
    QTest::newRow("9") << QByteArray("abcdef") << 3 << 2 << QByteArray()
                       << QByteArray("abcdefghijkl") << QByteArray("abcabcdefghijklf");
    QTest::newRow("10") << QByteArray("abcdef") << 2 << 2 << QByteArray() << QByteArray("xx")
                        << QByteArray("abxxef");

    // Using before

    QTest::newRow("null") << QByteArray() << 0 << 0 << QByteArray("abc") << QByteArray()
                          << QByteArray();
    QTest::newRow("text to text") << QByteArray("abcdefghbcd") << 0 << 0 << QByteArray("bcd")
                                  << QByteArray("1234") << QByteArray("a1234efgh1234");
    QTest::newRow("char to text") << QByteArray("abcdefgch") << 0 << 0 << QByteArray("c")
                                  << QByteArray("1234") << QByteArray("ab1234defg1234h");
    QTest::newRow("char to char") << QByteArray("abcdefgch") << 0 << 0 << QByteArray("c")
                                  << QByteArray("1") << QByteArray("ab1defg1h");
}

void tst_QByteArray::replace()
{
    QFETCH(QByteArray, src);
    QFETCH(int, pos);
    QFETCH(int, len);
    QFETCH(QByteArray, before);
    QFETCH(QByteArray, after);
    QFETCH(QByteArray, expected);

    if (before.isEmpty()) {
        QByteArray copy = src;
        QCOMPARE(copy.replace(pos, len, after), expected);
        copy = src;
        QCOMPARE(copy.replace(pos, len, after.data(), after.size()), expected);
    } else {
        QByteArray copy = src;
        if (before.size() == 1) {
            if (after.size() == 1)
                QCOMPARE(copy.replace(before.front(), after.front()), expected);
            QCOMPARE(copy.replace(before.front(), after), expected);
        }
        copy = src;
        QCOMPARE(copy.replace(before, after), expected);
        copy = src;
        QCOMPARE(copy.replace(before.constData(), before.size(), after.constData(), after.size()), expected);
    }
}

void tst_QByteArray::replaceWithSpecifiedLength()
{
    const char after[] = "zxc\0vbnmqwert";
    qsizetype lenAfter = 6;
    QByteArray ba("abcdefghjk");
    ba.replace(qsizetype(0), 2, after, lenAfter);

    const char _expected[] = "zxc\0vbcdefghjk";
    QByteArray expected(_expected,sizeof(_expected)-1);
    QCOMPARE(ba,expected);
}

void tst_QByteArray::number()
{
    QCOMPARE(QString(QByteArray::number((quint64) 0)),
             QString(QByteArray("0")));
    QCOMPARE(QString(QByteArray::number(Q_UINT64_C(0xFFFFFFFFFFFFFFFF))),
             QString(QByteArray("18446744073709551615")));
    QCOMPARE(QString(QByteArray::number(Q_INT64_C(0xFFFFFFFFFFFFFFFF))),
             QString(QByteArray("-1")));
    QCOMPARE(QString(QByteArray::number(qint64(0))),
             QString(QByteArray("0")));
    QCOMPARE(QString(QByteArray::number(Q_INT64_C(0x7FFFFFFFFFFFFFFF))),
             QString(QByteArray("9223372036854775807")));
    QCOMPARE(QString(QByteArray::number(Q_INT64_C(0x8000000000000000))),
             QString(QByteArray("-9223372036854775808")));
}

void tst_QByteArray::number_base_data()
{
    QTest::addColumn<qlonglong>("n");
    QTest::addColumn<int>("base");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("base 10") << 12346LL << 10 << QByteArray("12346");
    QTest::newRow("base  2") << 12346LL <<  2 << QByteArray("11000000111010");
    QTest::newRow("base  8") << 12346LL <<  8 << QByteArray("30072");
    QTest::newRow("base 16") << 12346LL << 16 << QByteArray("303a");
    QTest::newRow("base 17") << 12346LL << 17 << QByteArray("28c4");
    QTest::newRow("base 36") << 2181789482LL << 36 << QByteArray("102zbje");

    QTest::newRow("largeint, base 10")
            << 123456789012LL << 10 << QByteArray("123456789012");
    QTest::newRow("largeint, base  2")
            << 123456789012LL <<  2 << QByteArray("1110010111110100110010001101000010100");
    QTest::newRow("largeint, base  8")
            << 123456789012LL <<  8 << QByteArray("1627646215024");
    QTest::newRow("largeint, base 16")
            << 123456789012LL << 16 << QByteArray("1cbe991a14");
    QTest::newRow("largeint, base 17")
            << 123456789012LL << 17 << QByteArray("10bec2b629");
}

void tst_QByteArray::number_base()
{
    QFETCH( qlonglong, n );
    QFETCH( int, base );
    QFETCH( QByteArray, expected );
    QCOMPARE(QByteArray::number(n, base), expected);
    QCOMPARE(QByteArray::number(-n, base), '-' + expected);

    // check qlonglong->QByteArray->qlonglong round trip
    for (int ibase = 2; ibase <= 36; ++ibase) {
        auto stringrep = QByteArray::number(n, ibase);
        QCOMPARE(QByteArray::number(-n, ibase), '-' + stringrep);
        bool ok(false);
        auto result = stringrep.toLongLong(&ok, ibase);
        QVERIFY(ok);
        QCOMPARE(n, result);
    }
    if (n <= std::numeric_limits<int>::max()) {
        QCOMPARE(QByteArray::number(int(n), base), expected);
        QCOMPARE(QByteArray::number(int(-n), base), '-' + expected);
    } else if (n <= std::numeric_limits<long>::max()) {
        QCOMPARE(QByteArray::number(long(n), base), expected);
        QCOMPARE(QByteArray::number(long(-n), base), '-' + expected);
    }
}

void tst_QByteArray::toShort()
{
    bool ok = true; // opposite to the next expected result

    QCOMPARE(QByteArray().toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("").toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("12345").toShort(&ok), 12345);
    QVERIFY(ok);

    QCOMPARE(QByteArray("-12345").toShort(&ok), -12345);
    QVERIFY(ok);

    QCOMPARE(QByteArray("32767").toShort(&ok), 32767);
    QVERIFY(ok);

    QCOMPARE(QByteArray("-32768").toShort(&ok), -32768);
    QVERIFY(ok);

    QCOMPARE(QByteArray("32768").toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("-32769").toShort(&ok), 0);
    QVERIFY(!ok);
}

void tst_QByteArray::toUShort()
{
    bool ok = true; // opposite to the next expected result

    QCOMPARE(QByteArray().toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("").toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("12345").toUShort(&ok), 12345);
    QVERIFY(ok);

    QCOMPARE(QByteArray("-12345").toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("32767").toUShort(&ok), 32767);
    QVERIFY(ok);

    QCOMPARE(QByteArray("32768").toUShort(&ok), 32768);
    QVERIFY(ok);

    QCOMPARE(QByteArray("65535").toUShort(&ok), 65535);
    QVERIFY(ok);

    QCOMPARE(QByteArray("65536").toUShort(&ok), 0);
    QVERIFY(!ok);
}

// defined later
extern const char globalChar;

void tst_QByteArray::toInt_data()
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<int>("base");
    QTest::addColumn<int>("expectednumber");
    QTest::addColumn<bool>("expectedok");

    QTest::newRow("null") << QByteArray() << 10 << 0 << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0 << false;

    QTest::newRow("base 10") << QByteArray("100") << 10 << int(100) << true;
    QTest::newRow("base 16-1") << QByteArray("100") << 16 << int(256) << true;
    QTest::newRow("base 16-2") << QByteArray("0400") << 16 << int(1024) << true;
    QTest::newRow("base 2") << QByteArray("1111") << 2 << int(15) << true;
    QTest::newRow("base 8") << QByteArray("100") << 8 << int(64) << true;
    QTest::newRow("base 0-1") << QByteArray("0x10") << 0 << int(16) << true;
    QTest::newRow("base 0-2") << QByteArray("10") << 0 << int(10) << true;
    QTest::newRow("base 0-3") << QByteArray("010") << 0 << int(8) << true;
    QTest::newRow("empty") << QByteArray() << 0 << int(0) << false;

    QTest::newRow("leading space") << QByteArray(" 100") << 10 << int(100) << true;
    QTest::newRow("trailing space") << QByteArray("100 ") << 10 << int(100) << true;
    QTest::newRow("leading junk") << QByteArray("x100") << 10 << int(0) << false;
    QTest::newRow("trailing junk") << QByteArray("100x") << 10 << int(0) << false;

    // using fromRawData
    QTest::newRow("raw1") << QByteArray::fromRawData("1", 1) << 10 << 1 << true;
    QTest::newRow("raw2") << QByteArray::fromRawData("1foo", 1) << 10 << 1 << true;
    QTest::newRow("raw3") << QByteArray::fromRawData("12", 1) << 10 << 1 << true;
    QTest::newRow("raw4") << QByteArray::fromRawData("123456789", 1) << 10 << 1 << true;
    QTest::newRow("raw5") << QByteArray::fromRawData("123456789", 2) << 10 << 12 << true;

    QTest::newRow("raw-static") << QByteArray::fromRawData(&globalChar, 1) << 10 << 1 << true;
}

void tst_QByteArray::toInt()
{
    QFETCH( QByteArray, string );
    QFETCH( int, base );
    QFETCH( int, expectednumber );
    QFETCH( bool, expectedok );

    bool ok;
    int number = string.toInt(&ok, base);

    QCOMPARE( ok, expectedok );
    QCOMPARE( number, expectednumber );
}

void tst_QByteArray::toUInt_data()
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<int>("base");
    QTest::addColumn<uint>("expectednumber");
    QTest::addColumn<bool>("expectedok");

    QTest::newRow("null") << QByteArray() << 10 << 0u << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0u << false;

    QTest::newRow("negative value") << QByteArray("-50") << 10 << 0u << false;
    QTest::newRow("more than MAX_INT") << QByteArray("3234567890") << 10 << 3234567890u << true;
    QTest::newRow("2^32 - 1") << QByteArray("4294967295") << 10 << 4294967295u << true;
    if (sizeof(int) == 4)
        QTest::newRow("2^32") << QByteArray("4294967296") << 10 << 0u << false;
}

void tst_QByteArray::toUInt()
{
    QFETCH(QByteArray, string);
    QFETCH(int, base);
    QFETCH(uint, expectednumber);
    QFETCH(bool, expectedok);

    bool ok;
    const uint number = string.toUInt(&ok, base);

    QCOMPARE(ok, expectedok);
    QCOMPARE(number, expectednumber);
}

void tst_QByteArray::toFloat()
{
    bool ok = true; // opposite to the next expected result

    QCOMPARE(QByteArray().toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    QCOMPARE(QByteArray("").toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    const QByteArray data("0.000000000931322574615478515625");
    const float expectedValue = 9.31322574615478515625e-10f;
    QCOMPARE(data.toFloat(&ok), expectedValue);
    QVERIFY(ok);
}

void tst_QByteArray::toDouble_data()
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<double>("expectedNumber");
    QTest::addColumn<bool>("expectedOk");

    QTest::newRow("null") << QByteArray() << 0.0 << false;
    QTest::newRow("empty") << QByteArray("") << 0.0 << false;

    QTest::newRow("decimal") << QByteArray("1.2345") << 1.2345 << true;
    QTest::newRow("exponent lowercase") << QByteArray("1.2345e+01") << 12.345 << true;
    QTest::newRow("exponent uppercase") << QByteArray("1.2345E+02") << 123.45 << true;
    QTest::newRow("leading spaces") << QByteArray(" \n\r\t1.2345") << 1.2345 << true;
    QTest::newRow("trailing spaces") << QByteArray("1.2345 \n\r\t") << 1.2345 << true;
    QTest::newRow("leading junk") << QByteArray("x1.2345") << 0.0 << false;
    QTest::newRow("trailing junk") << QByteArray("1.2345x") << 0.0 << false;
    QTest::newRow("high precision") << QByteArray("0.000000000931322574615478515625")
                                    << 0.000000000931322574615478515625 << true;

    QTest::newRow("raw, null plus junk") << QByteArray::fromRawData("1.2\0 junk", 9) << 0.0 << false;
    QTest::newRow("raw, null-terminator not included") << QByteArray::fromRawData("2.3", 3) << 2.3 << true;
}

void tst_QByteArray::toDouble()
{
    QFETCH(QByteArray, string);
    QFETCH(double, expectedNumber);
    QFETCH(bool, expectedOk);

    bool ok;
    const double number = string.toDouble(&ok);

    QCOMPARE(ok, expectedOk);
    QCOMPARE(number, expectedNumber);
}

void tst_QByteArray::toLong_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<long>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("null") << QByteArray() << 10 << 0L << false;
    QTest::newRow("empty") << QByteArray("") << 16 << 0L << false;
    QTest::newRow("in range dec") << QByteArray("1608507359") << 10 << 1608507359L << true;
    QTest::newRow("in range dec neg") << QByteArray("-1608507359") << 10 << -1608507359L << true;
    QTest::newRow("in range hex") << QByteArray("12ABCDEF") << 16 << 0x12ABCDEFL << true;
    QTest::newRow("in range hex neg") << QByteArray("-12ABCDEF") << 16 << -0x12ABCDEFL << true;
    QTest::newRow("Fibonacci's last int32") << QByteArray("1836311903") << 10 << 1836311903L
                                            << true;

    QTest::newRow("leading spaces") << QByteArray(" \r\n\tABC123") << 16 << 0xABC123L << true;
    QTest::newRow("trailing spaces") << QByteArray("1234567\t\r \n") << 10 << 1234567L << true;
    QTest::newRow("leading junk") << QByteArray("q12345") << 10 << 0L << false;
    QTest::newRow("trailing junk") << QByteArray("abc12345t") << 16 << 0L << false;

    QTest::newRow("dec with base 0") << QByteArray("123") << 0 << 123L << true;
    QTest::newRow("neg dec with base 0") << QByteArray("-123") << 0 << -123L << true;
    QTest::newRow("hex with base 0") << QByteArray("0x123") << 0 << 0x123L << true;
    QTest::newRow("neg hex with base 0") << QByteArray("-0x123") << 0 << -0x123L << true;
    QTest::newRow("oct with base 0") << QByteArray("0123") << 0 << 0123L << true;
    QTest::newRow("neg oct with base 0") << QByteArray("-0123") << 0 << -0123L << true;

    QTest::newRow("base 3") << QByteArray("12012") << 3 << 140L << true;
    QTest::newRow("neg base 3") << QByteArray("-201") << 3 << -19L << true;

    using Bounds = std::numeric_limits<long>;
    QTest::newRow("long max") << QByteArray::number(Bounds::max()) << 10 << Bounds::max() << true;
    QTest::newRow("long min") << QByteArray::number(Bounds::min()) << 10 << Bounds::min() << true;

    using B32 = std::numeric_limits<qint32>;
    QTest::newRow("int32 min bin") << (QByteArray("-1") + QByteArray(31, '0')) << 2
                                   << long(B32::min()) << true;
    QTest::newRow("int32 max bin") << QByteArray(31, '1') << 2 << long(B32::max()) << true;
    QTest::newRow("int32 min hex") << QByteArray("-80000000") << 16 << long(B32::min()) << true;
    QTest::newRow("int32 max hex") << QByteArray("7fffffff") << 16 << long(B32::max()) << true;
    QTest::newRow("int32 min dec") << QByteArray("-2147483648") << 10 << long(B32::min()) << true;
    QTest::newRow("int32 max dec") << QByteArray("2147483647") << 10 << long(B32::max()) << true;

    if constexpr (sizeof(long) < sizeof(qlonglong)) {
        const qlonglong longMaxPlusOne = static_cast<qlonglong>(Bounds::max()) + 1;
        const qlonglong longMinMinusOne = static_cast<qlonglong>(Bounds::min()) - 1;
        QTest::newRow("long max + 1") << QByteArray::number(longMaxPlusOne) << 10 << 0L << false;
        QTest::newRow("long min - 1") << QByteArray::number(longMinMinusOne) << 10 << 0L << false;
    }
}

void tst_QByteArray::toLong()
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(long, result);
    QFETCH(bool, ok);

    bool b;
    QCOMPARE(str.toLong(nullptr, base), result);
    QCOMPARE(str.toLong(&b, base), result);
    QCOMPARE(b, ok);
    if (base == 10) {
        // check that by default base is assumed to be 10
        QCOMPARE(str.toLong(&b), result);
        QCOMPARE(b, ok);
    }
}

void tst_QByteArray::toULong_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<ulong>("result");
    QTest::addColumn<bool>("ok");

    ulong LongMaxPlusOne = (ulong)LONG_MAX + 1;
    QTest::newRow("LONG_MAX+1") << QString::number(LongMaxPlusOne).toUtf8() << 10 << LongMaxPlusOne << true;
    QTest::newRow("null") << QByteArray() << 10 << 0UL << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0UL << false;
    QTest::newRow("ulong1") << QByteArray("3234567890") << 10 << 3234567890UL << true;
    QTest::newRow("ulong2") << QByteArray("fFFfFfFf") << 16 << 0xFFFFFFFFUL << true;

    QTest::newRow("leading spaces") << QByteArray(" \n\r\t100") << 10 << 100UL << true;
    QTest::newRow("trailing spaces") << QByteArray("100 \n\r\t") << 10 << 100UL << true;
    QTest::newRow("leading junk") << QByteArray("x100") << 10 << 0UL << false;
    QTest::newRow("trailing junk") << QByteArray("100x") << 10 << 0UL << false;
}

void tst_QByteArray::toULong()
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(ulong, result);
    QFETCH(bool, ok);

    bool b;
    QCOMPARE(str.toULong(0, base), result);
    QCOMPARE(str.toULong(&b, base), result);
    QCOMPARE(b, ok);
}

void tst_QByteArray::toLongLong_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<qlonglong>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("null") << QByteArray() << 10 << 0LL << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0LL << false;
    QTest::newRow("out of base bound") << QByteArray("c") << 10 << 0LL << false;

    QTest::newRow("in range dec") << QByteArray("7679359922672374856") << 10
                                  << 7679359922672374856LL << true;
    QTest::newRow("in range dec neg") << QByteArray("-7679359922672374856") << 10
                                      << -7679359922672374856LL << true;
    QTest::newRow("in range hex") << QByteArray("6A929129A5421448") << 16 << 0x6A929129A5421448LL
                                  << true;
    QTest::newRow("in range hex neg") << QByteArray("-6A929129A5421448") << 16
                                      << -0x6A929129A5421448LL << true;
    QTest::newRow("Fibonacci's last int64") << QByteArray("7540113804746346429") << 10
                                            << 7540113804746346429LL << true;

    QTest::newRow("leading spaces") << QByteArray(" \r\n\tABCFFFFFFF123") << 16
                                    << 0xABCFFFFFFF123LL << true;
    QTest::newRow("trailing spaces") << QByteArray("9876543210\t\r \n") << 10
                                     << 9876543210LL << true;
    QTest::newRow("leading junk") << QByteArray("q12345") << 10 << 0LL << false;
    QTest::newRow("trailing junk") << QByteArray("abc12345t") << 16 << 0LL << false;

    QTest::newRow("dec with base 0") << QByteArray("9876543210") << 0 << 9876543210LL << true;
    QTest::newRow("neg dec with base 0") << QByteArray("-9876543210") << 0 << -9876543210LL << true;
    QTest::newRow("hex with base 0") << QByteArray("0x9876543210") << 0 << 0x9876543210LL << true;
    QTest::newRow("neg hex with base 0") << QByteArray("-0x9876543210") << 0 << -0x9876543210LL
                                         << true;
    QTest::newRow("oct with base 0") << QByteArray("07654321234567") << 0 << 07654321234567LL
                                     << true;
    QTest::newRow("neg oct with base 0") << QByteArray("-07654321234567") << 0 << -07654321234567LL
                                         << true;

    QTest::newRow("base 3") << QByteArray("12012") << 3 << 140LL << true;
    QTest::newRow("neg base 3") << QByteArray("-201") << 3 << -19LL << true;

    QTest::newRow("max dec") << QByteArray("9223372036854775807") << 10 << 9223372036854775807LL
                             << true;
    QTest::newRow("mix hex") << QByteArray("-7FFFFFFFFFFFFFFF") << 16 << -0x7FFFFFFFFFFFFFFFLL
                             << true;

    QTest::newRow("max + 1 dec") << QByteArray("9223372036854775808") << 10 << 0LL << false;
    QTest::newRow("min - 1 hex") << QByteArray("-8000000000000001") << 16 << 0LL << false;
}

void tst_QByteArray::toLongLong()
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(qlonglong, result);
    QFETCH(bool, ok);

    bool b;
    QCOMPARE(str.toLongLong(nullptr, base), result);
    QCOMPARE(str.toLongLong(&b, base), result);
    QCOMPARE(b, ok);
    if (base == 10) {
        QCOMPARE(str.toLongLong(&b), result);
        QCOMPARE(b, ok);
    }
}

void tst_QByteArray::toULongLong_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<qulonglong>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("null") << QByteArray() << 10 << (qulonglong)0 << false;
    QTest::newRow("empty") << QByteArray("") << 10 << (qulonglong)0 << false;
    QTest::newRow("out of base bound") << QByteArray("c") << 10 << (qulonglong)0 << false;

    QTest::newRow("leading spaces") << QByteArray(" \n\r\t100") << 10 << qulonglong(100) << true;
    QTest::newRow("trailing spaces") << QByteArray("100 \n\r\t") << 10 << qulonglong(100) << true;
    QTest::newRow("leading junk") << QByteArray("x100") << 10 << qulonglong(0) << false;
    QTest::newRow("trailing junk") << QByteArray("100x") << 10 << qulonglong(0) << false;
}

void tst_QByteArray::toULongLong()
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(qulonglong, result);
    QFETCH(bool, ok);

    bool b;
    QCOMPARE(str.toULongLong(0, base), result);
    QCOMPARE(str.toULongLong(&b, base), result);
    QCOMPARE(b, ok);
}

static bool checkSize(qsizetype value, qsizetype min)
{
    return value >= min && value <= std::numeric_limits<qsizetype>::max();
}

// global functions defined in qbytearray.cpp
void tst_QByteArray::blockSizeCalculations()
{
    qsizetype MaxAllocSize = std::numeric_limits<qsizetype>::max();

    // Not very important, but please behave :-)
    QCOMPARE(qCalculateBlockSize(0, 1), qsizetype(0));
    QVERIFY(qCalculateGrowingBlockSize(0, 1).size <= MaxAllocSize);
    QVERIFY(qCalculateGrowingBlockSize(0, 1).elementCount <= MaxAllocSize);

    // boundary condition
    QCOMPARE(qCalculateBlockSize(MaxAllocSize, 1), qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/2, 2), qsizetype(MaxAllocSize) - 1);
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/2, 2, 1), qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize, 1).size, qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize, 1).elementCount, qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize/2, 2, 1).size, qsizetype(MaxAllocSize));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize/2, 2, 1).elementCount, qsizetype(MaxAllocSize)/2);

    // error conditions
    QCOMPARE(qCalculateBlockSize(quint64(MaxAllocSize) + 1, 1), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(qsizetype(-1), 1), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize, 1, 1), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/2 + 1, 2), qsizetype(-1));
    QCOMPARE(qCalculateGrowingBlockSize(quint64(MaxAllocSize) + 1, 1).size, qsizetype(-1));
    QCOMPARE(qCalculateGrowingBlockSize(MaxAllocSize/2 + 1, 2).size, qsizetype(-1));

    // overflow conditions
#if QT_POINTER_SIZE == 4
    // on 32-bit platforms, (1 << 16) * (1 << 16) = (1 << 32) which is zero
    QCOMPARE(qCalculateBlockSize(1 << 16, 1 << 16), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/4, 16), qsizetype(-1));
    // on 32-bit platforms, (1 << 30) * 3 + (1 << 30) would overflow to zero
    QCOMPARE(qCalculateBlockSize(1U << 30, 3, 1U << 30), qsizetype(-1));
#else
    // on 64-bit platforms, (1 << 32) * (1 << 32) = (1 << 64) which is zero
    QCOMPARE(qCalculateBlockSize(1LL << 32, 1LL << 32), qsizetype(-1));
    QCOMPARE(qCalculateBlockSize(MaxAllocSize/4, 16), qsizetype(-1));
    // on 64-bit platforms, (1 << 30) * 3 + (1 << 30) would overflow to zero
    QCOMPARE(qCalculateBlockSize(1ULL << 62, 3, 1ULL << 62), qsizetype(-1));
#endif
    // exact block sizes
    for (int i = 1; i < 1 << 31; i <<= 1) {
        QCOMPARE(qCalculateBlockSize(0, 1, i), qsizetype(i));
        QCOMPARE(qCalculateBlockSize(i, 1), qsizetype(i));
        QCOMPARE(qCalculateBlockSize(i + i/2, 1), qsizetype(i + i/2));
    }
    for (int i = 1; i < 1 << 30; i <<= 1) {
        QCOMPARE(qCalculateBlockSize(i, 2), 2 * qsizetype(i));
        QCOMPARE(qCalculateBlockSize(i, 2, 1), 2 * qsizetype(i) + 1);
        QCOMPARE(qCalculateBlockSize(i, 2, 16), 2 * qsizetype(i) + 16);
    }

    // growing sizes
    for (int i = 1; i < 1 << 31; i <<= 1) {
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1).size, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1).elementCount, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 16).size, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 16).elementCount, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 24).size, i));
        QVERIFY(checkSize(qCalculateGrowingBlockSize(i, 1, 16).elementCount, i));
    }

    // growth should be limited
    for (int elementSize = 1; elementSize < (1<<8); elementSize <<= 1) {
        qsizetype alloc = 1;
        forever {
            QVERIFY(checkSize(qCalculateGrowingBlockSize(alloc, elementSize).size, alloc * elementSize));
            qsizetype newAlloc = qCalculateGrowingBlockSize(alloc, elementSize).elementCount;
            QVERIFY(checkSize(newAlloc, alloc));
            if (newAlloc == alloc)
                break;  // no growth, we're at limit
            alloc = newAlloc;
        }
        QVERIFY(checkSize(alloc, qsizetype(MaxAllocSize) / elementSize));

        // the next allocation should be invalid
        if (alloc < MaxAllocSize) // lest alloc + 1 overflows (= UB)
            QCOMPARE(qCalculateGrowingBlockSize(alloc + 1, elementSize).size, qsizetype(-1));
    }
}

void tst_QByteArray::resizeAfterFromRawData()
{
    QByteArray buffer("hello world");

    QByteArray array = QByteArray::fromRawData(buffer.constData(), buffer.size());
    QVERIFY(array.constData() == buffer.constData());
    array.resize(5);
    QVERIFY(array.constData() != buffer.constData());
    // check null termination
    QVERIFY(array.constData()[5] == 0);
}

void tst_QByteArray::appendAfterFromRawData()
{
    QByteArray arr;
    {
        char data[] = "X";
        arr += QByteArray::fromRawData(data, sizeof(data));
        data[0] = 'Y';
    }
    QCOMPARE(arr.at(0), 'X');
}

void tst_QByteArray::toFromHex_data()
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<char>("sep");
    QTest::addColumn<QByteArray>("hex");
    QTest::addColumn<QByteArray>("hex_alt1");

    QTest::newRow("Qt is great! (default)")
        << QByteArray("Qt is great!")
        << '\0'
        << QByteArray("517420697320677265617421")
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21");

    QTest::newRow("Qt is great! (with space)")
        << QByteArray("Qt is great!")
        << ' '
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21")
        << QByteArray("51 74 20 69 73 20 67 72 65 61 74 21");

    QTest::newRow("Qt is great! (with minus)")
        << QByteArray("Qt is great!")
        << '-'
        << QByteArray("51-74-20-69-73-20-67-72-65-61-74-21")
        << QByteArray("51-74-20-69-73-20-67-72-65-61-74-21");

    QTest::newRow("Qt is so great!")
        << QByteArray("Qt is so great!")
        << '\0'
        << QByteArray("517420697320736f20677265617421")
        << QByteArray("51 74 20 69 73 20 73 6f 20 67 72 65 61 74 21");

    QTest::newRow("default-constructed")
        << QByteArray()
        << '\0'
        << QByteArray()
        << QByteArray();

    QTest::newRow("default-constructed (with space)")
        << QByteArray()
        << ' '
        << QByteArray()
        << QByteArray();

    QTest::newRow("empty")
        << QByteArray("")
        << '\0'
        << QByteArray("")
        << QByteArray("");

    QTest::newRow("null")
        << QByteArray()
        << '\0'
        << QByteArray()
        << QByteArray();

    QTest::newRow("empty (with space)")
        << QByteArray("")
        << ' '
        << QByteArray("")
        << QByteArray("");

    QTest::newRow("array-of-null")
        << QByteArray("\0", 1)
        << '\0'
        << QByteArray("00")
        << QByteArray("0");

    QTest::newRow("no-leading-zero")
        << QByteArray("\xf")
        << '\0'
        << QByteArray("0f")
        << QByteArray("f");

    QTest::newRow("single-byte")
        << QByteArray("\xaf")
        << '\0'
        << QByteArray("af")
        << QByteArray("xaf");

    QTest::newRow("no-leading-zero")
        << QByteArray("\xd\xde\xad\xc0\xde")
        << '\0'
        << QByteArray("0ddeadc0de")
        << QByteArray("ddeadc0de");

    QTest::newRow("garbage")
        << QByteArray("\xC\xde\xeC\xea\xee\xDe\xee\xee")
        << '\0'
        << QByteArray("0cdeeceaeedeeeee")
        << QByteArray("Code less. Create more. Deploy everywhere.");

    QTest::newRow("under-defined-1")
        << QByteArray("\x1\x23")
        << '\0'
        << QByteArray("0123")
        << QByteArray("x123");

    QTest::newRow("under-defined-2")
        << QByteArray("\x12\x34")
        << '\0'
        << QByteArray("1234")
        << QByteArray("x1234");
}

void tst_QByteArray::toFromHex()
{
    QFETCH(QByteArray, str);
    QFETCH(char,       sep);
    QFETCH(QByteArray, hex);
    QFETCH(QByteArray, hex_alt1);

    if (sep == 0) {
        const QByteArray th = str.toHex();
        QCOMPARE(th.size(), hex.size());
        QCOMPARE(th, hex);
    }

    {
        const QByteArray th = str.toHex(sep);
        QCOMPARE(th.size(), hex.size());
        QCOMPARE(th, hex);
    }

    {
        const QByteArray fh = QByteArray::fromHex(hex);
        QCOMPARE(fh.size(), str.size());
        QCOMPARE(fh, str);
    }

    QCOMPARE(QByteArray::fromHex(hex_alt1), str);
}

void tst_QByteArray::toFromPercentEncoding()
{
    QByteArray arr("Qt is great!");

    QCOMPARE(QByteArray().toPercentEncoding(), QByteArray());
    QCOMPARE(QByteArray("").toPercentEncoding(), QByteArray(""));

    QByteArray data = arr.toPercentEncoding();
    QCOMPARE(QString(data), QString("Qt%20is%20great%21"));
    QCOMPARE(QByteArray::fromPercentEncoding(data), arr);

    data = arr.toPercentEncoding("! ", "Qt");
    QCOMPARE(QString(data), QString("%51%74 is grea%74!"));
    QCOMPARE(QByteArray::fromPercentEncoding(data), arr);

    data = arr.toPercentEncoding(QByteArray(), "abcdefghijklmnopqrstuvwxyz", 'Q');
    QCOMPARE(QString(data), QString("Q51Q74Q20Q69Q73Q20Q67Q72Q65Q61Q74Q21"));
    QCOMPARE(QByteArray::fromPercentEncoding(data, 'Q'), arr);

    // verify that to/from percent encoding preserves nullity
    arr = "";
    QVERIFY(arr.isEmpty());
    QVERIFY(!arr.isNull());
    QVERIFY(arr.toPercentEncoding().isEmpty());
    QVERIFY(!arr.toPercentEncoding().isNull());
    QVERIFY(QByteArray::fromPercentEncoding("").isEmpty());
    QVERIFY(!QByteArray::fromPercentEncoding("").isNull());

    arr = QByteArray();
    QVERIFY(arr.isEmpty());
    QVERIFY(arr.isNull());
    QVERIFY(arr.toPercentEncoding().isEmpty());
    QVERIFY(arr.toPercentEncoding().isNull());
    QVERIFY(QByteArray::fromPercentEncoding(QByteArray()).isEmpty());
    QVERIFY(QByteArray::fromPercentEncoding(QByteArray()).isNull());
}

void tst_QByteArray::fromPercentEncoding_data()
{
    QTest::addColumn<QByteArray>("encodedString");
    QTest::addColumn<QByteArray>("decodedString");

    QTest::newRow("NormalString") << QByteArray("filename") << QByteArray("filename");
    QTest::newRow("NormalStringEncoded") << QByteArray("file%20name") << QByteArray("file name");
    QTest::newRow("JustEncoded") << QByteArray("%20") << QByteArray(" ");
    QTest::newRow("HTTPUrl") << QByteArray("http://qt-project.org") << QByteArray("http://qt-project.org");
    QTest::newRow("HTTPUrlEncoded") << QByteArray("http://qt-project%20org") << QByteArray("http://qt-project org");
    QTest::newRow("EmptyString") << QByteArray("") << QByteArray("");
    QTest::newRow("Task27166") << QByteArray("Fran%C3%A7aise") << QByteArray("Française");
}

void tst_QByteArray::fromPercentEncoding()
{
    QFETCH(QByteArray, encodedString);
    QFETCH(QByteArray, decodedString);

    QCOMPARE(QByteArray::fromPercentEncoding(encodedString), decodedString);
}

void tst_QByteArray::toPercentEncoding_data()
{
    QTest::addColumn<QByteArray>("decodedString");
    QTest::addColumn<QByteArray>("encodedString");

    QTest::newRow("NormalString") << QByteArray("filename") << QByteArray("filename");
    QTest::newRow("NormalStringEncoded") << QByteArray("file name") << QByteArray("file%20name");
    QTest::newRow("JustEncoded") << QByteArray(" ") << QByteArray("%20");
    QTest::newRow("HTTPUrl") << QByteArray("http://qt-project.org") << QByteArray("http%3A//qt-project.org");
    QTest::newRow("HTTPUrlEncoded") << QByteArray("http://qt-project org") << QByteArray("http%3A//qt-project%20org");
    QTest::newRow("EmptyString") << QByteArray("") << QByteArray("");
    QTest::newRow("Task27166") << QByteArray("Française") << QByteArray("Fran%C3%A7aise");
}

void tst_QByteArray::toPercentEncoding()
{
    QFETCH(QByteArray, decodedString);
    QFETCH(QByteArray, encodedString);

    QCOMPARE(decodedString.toPercentEncoding("/.").constData(), encodedString.constData());
}

void tst_QByteArray::toPercentEncoding2_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QByteArray>("excludeInEncoding");
    QTest::addColumn<QByteArray>("includeInEncoding");

    QTest::newRow("test_01") << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
                          << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_02") << QByteArray("{\t\n\r^\"abc}")
                          << QByteArray("%7B%09%0A%0D%5E%22abc%7D")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_03") << QByteArray("://?#[]@!$&'()*+,;=")
                          << QByteArray("%3A%2F%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2B%2C%3B%3D")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_04") << QByteArray("://?#[]@!$&'()*+,;=")
                          << QByteArray("%3A%2F%2F%3F%23%5B%5D%40!$&'()*+,;=")
                          << QByteArray("!$&'()*+,;=")
                          << QByteArray("");
    QTest::newRow("test_05") << QByteArray("abcd")
                          << QByteArray("a%62%63d")
                          << QByteArray("")
                          << QByteArray("bc");
}

void tst_QByteArray::toPercentEncoding2()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, encoded);
    QFETCH(QByteArray, excludeInEncoding);
    QFETCH(QByteArray, includeInEncoding);

    QByteArray encodedData = original.toPercentEncoding(excludeInEncoding, includeInEncoding);
    QCOMPARE(encodedData.constData(), encoded.constData());
    QCOMPARE(original, QByteArray::fromPercentEncoding(encodedData));
}

struct StringComparisonData
{
    const char *const left;
    const char *const right;
    const unsigned int clip;
    const int cmp, icmp, ncmp, nicmp;
    static int sign(int val) { return val < 0 ? -1 : val > 0 ? +1 : 0; }
};
Q_DECLARE_METATYPE(StringComparisonData);

void tst_QByteArray::qstrcmp_data()
{
    QTest::addColumn<StringComparisonData>("data");

    QTest::newRow("equal")
        << StringComparisonData{"abcEdb", "abcEdb", 3, 0, 0, 0, 0};
    QTest::newRow("upper")
        << StringComparisonData{"ABCedb", "ABCEDB", 3, 1, 0, 0, 0};
    QTest::newRow("lower")
        << StringComparisonData{"ABCEDB", "abcedb", 3, -1, 0, -1, 0};
    QTest::newRow("upper-late")
        << StringComparisonData{"abcEdb", "abcEDB", 3, 1, 0, 0, 0};
    QTest::newRow("lower-late")
        << StringComparisonData{"ABCEDB", "ABCedb", 3, -1, 0, 0, 0};
    QTest::newRow("longer")
        << StringComparisonData{"abcdef", "abcdefg", 6, -1, -1, 0, 0};
    QTest::newRow("long-up")
        << StringComparisonData{"abcdef", "abcdeFg", 6, 1, -1, 1, 0};
    QTest::newRow("long-down")
        << StringComparisonData{"abcdeF", "abcdefg", 6, -1, -1, -1, 0};
    QTest::newRow("shorter")
        << StringComparisonData{"abcdefg", "abcdef", 6, 1, 1, 0, 0};
    QTest::newRow("short-up")
        << StringComparisonData{"abcdefg", "abcdeF", 6, 1, 1, 1, 0};
    QTest::newRow("short-down")
        << StringComparisonData{"abcdeFg", "abcdef", 6, -1, 1, -1, 0};
    QTest::newRow("zero-length")
        << StringComparisonData{"abcdefg", "T", 0, 1, -1, 0, 0};
    QTest::newRow("null-null")
        << StringComparisonData{nullptr, nullptr, 6, 0, 0, 0, 0};
    QTest::newRow("null-empty")
        << StringComparisonData{nullptr, "", 0, -1, -1, -1, -1};
    QTest::newRow("empty-null")
        << StringComparisonData{"", nullptr, 0, 1, 1, 1, 1};
    QTest::newRow("empty-empty")
        << StringComparisonData{"", "", 0, 0, 0, 0, 0};
    QTest::newRow("null-some")
        << StringComparisonData{nullptr, "some", 0, -1, -1, -1, -1};
    QTest::newRow("some-null")
        << StringComparisonData{"some", nullptr, 0, 1, 1, 1, 1};
    QTest::newRow("empty-some")
        << StringComparisonData{"", "some", 0, -1, -1, 0, 0};
    QTest::newRow("some-empty")
        << StringComparisonData{"some", "", 0, 1, 1, 0, 0};
}

void tst_QByteArray::qstrcmp()
{
    QFETCH(StringComparisonData, data);
    QCOMPARE(data.sign(::qstrcmp(data.left, data.right)), data.cmp);
    QCOMPARE(data.sign(::qstricmp(data.left, data.right)), data.icmp);
    QCOMPARE(data.sign(::qstrncmp(data.left, data.right, data.clip)), data.ncmp);
    QCOMPARE(data.sign(::qstrnicmp(data.left, data.right, data.clip)), data.nicmp);
}

void tst_QByteArray::compare_singular()
{
    QCOMPARE(QByteArray().compare(nullptr, Qt::CaseInsensitive), 0);
    QCOMPARE(QByteArray().compare("", Qt::CaseInsensitive), 0);
    QVERIFY(QByteArray("a").compare(nullptr, Qt::CaseInsensitive) > 0);
    QVERIFY(QByteArray("a").compare("", Qt::CaseInsensitive) > 0);
    QVERIFY(QByteArray().compare("a", Qt::CaseInsensitive) < 0);
    QCOMPARE(QByteArray().compare(QByteArray(), Qt::CaseInsensitive), 0);
    QVERIFY(QByteArray().compare(QByteArray("a"), Qt::CaseInsensitive) < 0);
    QVERIFY(QByteArray("a").compare(QByteArray(), Qt::CaseInsensitive) > 0);
}

void tst_QByteArray::compareCharStar_data()
{
    QTest::addColumn<QByteArray>("str1");
    QTest::addColumn<QString>("string2");
    QTest::addColumn<int>("result");

    QTest::newRow("null-null") << QByteArray() << QString() << 0;
    QTest::newRow("null-empty") << QByteArray() << "" << 0;
    QTest::newRow("null-full") << QByteArray() << "abc" << -1;
    QTest::newRow("empty-null") << QByteArray("") << QString() << 0;
    QTest::newRow("empty-empty") << QByteArray("") << "" << 0;
    QTest::newRow("empty-full") << QByteArray("") << "abc" << -1;
    QTest::newRow("raw-null") << QByteArray::fromRawData("abc", 0) << QString() << 0;
    QTest::newRow("raw-empty") << QByteArray::fromRawData("abc", 0) << QString("") << 0;
    QTest::newRow("raw-full") << QByteArray::fromRawData("abc", 0) << "abc" << -1;

    QTest::newRow("full-null") << QByteArray("abc") << QString() << +1;
    QTest::newRow("full-empty") << QByteArray("abc") << "" << +1;

    QTest::newRow("equal1") << QByteArray("abc") << "abc" << 0;
    QTest::newRow("equal2") << QByteArray("abcd", 3) << "abc" << 0;
    QTest::newRow("equal3") << QByteArray::fromRawData("abcd", 3) << "abc" << 0;

    QTest::newRow("less1") << QByteArray("ab") << "abc" << -1;
    QTest::newRow("less2") << QByteArray("abb") << "abc" << -1;
    QTest::newRow("less3") << QByteArray::fromRawData("abc", 2) << "abc" << -1;
    QTest::newRow("less4") << QByteArray("", 1) << "abc" << -1;
    QTest::newRow("less5") << QByteArray::fromRawData("", 1) << "abc" << -1;
    QTest::newRow("less6") << QByteArray("a\0bc", 4) << "a.bc" << -1;

    QTest::newRow("greater1") << QByteArray("ac") << "abc" << +1;
    QTest::newRow("greater2") << QByteArray("abd") << "abc" << +1;
    QTest::newRow("greater3") << QByteArray("abcd") << "abc" << +1;
    QTest::newRow("greater4") << QByteArray::fromRawData("abcd", 4) << "abc" << +1;
}

void tst_QByteArray::compareCharStar()
{
    QFETCH(QByteArray, str1);
    QFETCH(QString, string2);
    QFETCH(int, result);

    const bool isEqual   = result == 0;
    const bool isLess    = result < 0;
    const bool isGreater = result > 0;
    QByteArray qba = string2.toUtf8();
    const char *str2 = qba.constData();
    if (string2.isNull())
        str2 = 0;

    // basic tests:
    QCOMPARE(str1 == str2, isEqual);
    QCOMPARE(str1 < str2, isLess);
    QCOMPARE(str1 > str2, isGreater);

    // composed tests:
    QCOMPARE(str1 <= str2, isLess || isEqual);
    QCOMPARE(str1 >= str2, isGreater || isEqual);
    QCOMPARE(str1 != str2, !isEqual);

    // inverted tests:
    QCOMPARE(str2 == str1, isEqual);
    QCOMPARE(str2 < str1, isGreater);
    QCOMPARE(str2 > str1, isLess);

    // composed, inverted tests:
    QCOMPARE(str2 <= str1, isGreater || isEqual);
    QCOMPARE(str2 >= str1, isLess || isEqual);
    QCOMPARE(str2 != str1, !isEqual);
}

void tst_QByteArray::repeatedSignature() const
{
    /* repated() should be a const member. */
    const QByteArray string;
    (void)string.repeated(3);
}

void tst_QByteArray::repeated() const
{
    QFETCH(QByteArray, string);
    QFETCH(QByteArray, expected);
    QFETCH(int, count);

    QCOMPARE(string.repeated(count), expected);
}

void tst_QByteArray::repeated_data() const
{
    QTest::addColumn<QByteArray>("string" );
    QTest::addColumn<QByteArray>("expected" );
    QTest::addColumn<int>("count" );

    /* Empty strings. */
    QTest::newRow("data1")
        << QByteArray()
        << QByteArray()
        << 0;

    QTest::newRow("data2")
        << QByteArray()
        << QByteArray()
        << -1004;

    QTest::newRow("data3")
        << QByteArray()
        << QByteArray()
        << 1;

    QTest::newRow("data4")
        << QByteArray()
        << QByteArray()
        << 5;

    /* On simple string. */
    QTest::newRow("data5")
        << QByteArray("abc")
        << QByteArray()
        << -1004;

    QTest::newRow("data6")
        << QByteArray("abc")
        << QByteArray()
        << -1;

    QTest::newRow("data7")
        << QByteArray("abc")
        << QByteArray()
        << 0;

    QTest::newRow("data8")
        << QByteArray("abc")
        << QByteArray("abc")
        << 1;

    QTest::newRow("data9")
        << QByteArray(("abc"))
        << QByteArray(("abcabc"))
        << 2;

    QTest::newRow("data10")
        << QByteArray(("abc"))
        << QByteArray(("abcabcabc"))
        << 3;

    QTest::newRow("data11")
        << QByteArray(("abc"))
        << QByteArray(("abcabcabcabc"))
        << 4;

    QTest::newRow("static not null terminated")
        << QByteArray(staticNotNullTerminated)
        << QByteArray("datadatadatadata")
        << 4;
    QTest::newRow("static standard")
        << QByteArray(staticStandard)
        << QByteArray("datadatadatadata")
        << 4;
}

void tst_QByteArray::byteRefDetaching() const
{
    {
        QByteArray str = "str";
        QByteArray copy = str;
        copy[0] = 'S';

        QCOMPARE(str, QByteArray("str"));
    }

    {
        char buf[] = { 's', 't', 'r' };
        QByteArray str = QByteArray::fromRawData(buf, 3);
        str[0] = 'S';

        QCOMPARE(buf[0], char('s'));
    }

    {
        static const char buf[] = { 's', 't', 'r' };
        QByteArray str = QByteArray::fromRawData(buf, 3);

        // this causes a crash in most systems if the detaching doesn't work
        str[0] = 'S';

        QCOMPARE(buf[0], char('s'));
    }
}

void tst_QByteArray::reserve()
{
    int capacity = 100;
    QByteArray qba;
    qba.reserve(capacity);
    QVERIFY(qba.capacity() == capacity);
    char *data = qba.data();

    for (int i = 0; i < capacity; i++) {
        qba.resize(i);
        QVERIFY(capacity == qba.capacity());
        QVERIFY(data == qba.data());
    }

    qba.resize(capacity);

    QByteArray copy = qba;
    qba.reserve(capacity / 2);
    QCOMPARE(qba.size(), capacity); // we didn't shrink the size!
    QCOMPARE(qba.capacity(), capacity);
    QCOMPARE(copy.capacity(), capacity);

    qba = copy;
    qba.reserve(capacity * 2);
    QCOMPARE(qba.size(), capacity);
    QCOMPARE(qba.capacity(), capacity * 2);
    QCOMPARE(copy.capacity(), capacity);
    QVERIFY(qba.constData() != data);

    QByteArray nil1, nil2;
    nil1.reserve(0);
    nil2.squeeze();
    nil1.squeeze();
    nil2.reserve(0);
    QCOMPARE(nil1.capacity(), 0);
    QCOMPARE(nil2.capacity(), 0);

    nil1.resize(5);
    QVERIFY(nil1.capacity() >= 5);
}

void tst_QByteArray::reserveExtended_data()
{
    prependExtended_data();
}

void tst_QByteArray::reserveExtended()
{
    QFETCH(QByteArray, array);
    array.reserve(1024);
    QVERIFY(array.capacity() == 1024);
    QCOMPARE(array, QByteArray("data"));
    array.squeeze();
    QCOMPARE(array, QByteArray("data"));
    QCOMPARE(array.capacity(), array.size());
}

void tst_QByteArray::movablity_data()
{
    QTest::addColumn<QByteArray>("array");

    QTest::newRow("0x00000000") << QByteArray("\x00\x00\x00\x00", 4);
    QTest::newRow("0x000000ff") << QByteArray("\x00\x00\x00\xff", 4);
    QTest::newRow("0xffffffff") << QByteArray("\xff\xff\xff\xff", 4);
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("null") << QByteArray();
    QTest::newRow("sss") << QByteArray(3, 's');

    prependExtended_data();
}

void tst_QByteArray::movablity()
{
    QFETCH(QByteArray, array);

    static_assert(QTypeInfo<QByteArray>::isRelocatable);

    const int size = array.size();
    const bool isEmpty = array.isEmpty();
    const bool isNull = array.isNull();
    const int capacity = array.capacity();

    QByteArray memSpace;

    // we need only memory space not the instance
    memSpace.~QByteArray();
    // move array -> memSpace
    memcpy((void *)&memSpace, (const void *)&array, sizeof(QByteArray));
    // reconstruct empty QByteArray
    new (&array) QByteArray;

    QCOMPARE(memSpace.size(), size);
    QCOMPARE(memSpace.isEmpty(), isEmpty);
    QCOMPARE(memSpace.isNull(), isNull);
    QCOMPARE(memSpace.capacity(), capacity);

    // try to not crash
    (void)memSpace.toLower();
    (void)memSpace.toUpper();
    memSpace.prepend('a');
    memSpace.append("b", 1);
    memSpace.squeeze();
    memSpace.reserve(array.size() + 16);

    QByteArray copy(memSpace);

    // reinitialize base values
    const int newSize = size + 2;
    const bool newIsEmpty = false;
    const bool newIsNull = false;
    const int newCapacity = memSpace.capacity();

    // move back memSpace -> array
    array.~QByteArray();
    memcpy((void *)&array, (const void *)&memSpace, sizeof(QByteArray));
    // reconstruct empty QByteArray
    new (&memSpace) QByteArray;

    QCOMPARE(array.size(), newSize);
    QCOMPARE(array.isEmpty(), newIsEmpty);
    QCOMPARE(array.isNull(), newIsNull);
    QCOMPARE(array.capacity(), newCapacity);
    QVERIFY(array.startsWith('a'));
    QVERIFY(array.endsWith('b'));

    QCOMPARE(copy.size(), newSize);
    QCOMPARE(copy.isEmpty(), newIsEmpty);
    QCOMPARE(copy.isNull(), newIsNull);
    QCOMPARE(copy.capacity(), newCapacity);
    QVERIFY(copy.startsWith('a'));
    QVERIFY(copy.endsWith('b'));

    // try to not crash
    array.squeeze();
    array.reserve(array.size() + 3);
    QVERIFY(true);
}

void tst_QByteArray::literals()
{
    QByteArray str(QByteArrayLiteral("abcd"));

    QVERIFY(str.length() == 4);
    QCOMPARE(str.capacity(), 0);
    QVERIFY(str == "abcd");
    QVERIFY(!str.data_ptr()->isMutable());

    const char *s = str.constData();
    QByteArray str2 = str;
    QVERIFY(str2.constData() == s);
    QCOMPARE(str2.capacity(), 0);

    // detach on non const access
    QVERIFY(str.data() != s);
    QVERIFY(str.capacity() >= str.length());

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
    QVERIFY(str2.capacity() >= str2.length());
}

void tst_QByteArray::userDefinedLiterals()
{
    QByteArray str = "abcd"_qba;

    QVERIFY(str.length() == 4);
    QCOMPARE(str.capacity(), 0);
    QVERIFY(str == "abcd");
    QVERIFY(!str.data_ptr()->isMutable());

    const char *s = str.constData();
    QByteArray str2 = str;
    QVERIFY(str2.constData() == s);
    QCOMPARE(str2.capacity(), 0);

    // detach on non const access
    QVERIFY(str.data() != s);
    QVERIFY(str.capacity() >= str.length());

    QVERIFY(str2.constData() == s);
    QVERIFY(str2.data() != s);
    QVERIFY(str2.capacity() >= str2.length());
}

void tst_QByteArray::toUpperLower_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("upper");
    QTest::addColumn<QByteArray>("lower");

    {
        QByteArray nonAscii(128, Qt::Uninitialized);
        char *data = nonAscii.data();
        for (unsigned char i = 0; i < 128; ++i)
            data[i] = i + 128;
        QTest::newRow("non-ASCII") << nonAscii << nonAscii << nonAscii;
    }

    QTest::newRow("null") << QByteArray() << QByteArray() << QByteArray();
    QTest::newRow("empty") << QByteArray("") << QByteArray("") << QByteArray("");
    QTest::newRow("literal") << QByteArrayLiteral("Hello World")
                             << QByteArrayLiteral("HELLO WORLD")
                             << QByteArrayLiteral("hello world");
    QTest::newRow("ascii") << QByteArray("Hello World, this is a STRING")
                           << QByteArray("HELLO WORLD, THIS IS A STRING")
                           << QByteArray("hello world, this is a string");
    QTest::newRow("nul") << QByteArray("a\0B", 3) << QByteArray("A\0B", 3) << QByteArray("a\0b", 3);
}

void tst_QByteArray::toUpperLower()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, upper);
    QFETCH(QByteArray, lower);
    QCOMPARE(lower.toLower(), lower);
    QCOMPARE(upper.toUpper(), upper);
    QCOMPARE(input.toUpper(), upper);
    QCOMPARE(input.toLower(), lower);

    QByteArray copy = input;
    QCOMPARE(std::move(copy).toUpper(), upper);
    copy = input;
    copy.detach();
    QCOMPARE(std::move(copy).toUpper(), upper);

    copy = input;
    QCOMPARE(std::move(copy).toLower(), lower);
    copy = input;
    copy.detach();
    QCOMPARE(std::move(copy).toLower(), lower);

    copy = lower;
    QCOMPARE(std::move(copy).toLower(), lower);
    copy = lower;
    copy.detach();
    QCOMPARE(std::move(copy).toLower(), lower);

    copy = upper;
    QCOMPARE(std::move(copy).toUpper(), upper);
    copy = upper;
    copy.detach();
    QCOMPARE(std::move(copy).toUpper(), upper);
}

void tst_QByteArray::isUpper()
{
    QVERIFY(!QByteArray().isUpper());
    QVERIFY(!QByteArray("").isUpper());
    QVERIFY(QByteArray("TEXT").isUpper());
    QVERIFY(!QByteArray("\xD0\xDE").isUpper()); // non-ASCII is neither upper nor lower
    QVERIFY(!QByteArray("\xD7").isUpper());
    QVERIFY(!QByteArray("\xDF").isUpper());
    QVERIFY(!QByteArray("text").isUpper());
    QVERIFY(!QByteArray("Text").isUpper());
    QVERIFY(!QByteArray("tExt").isUpper());
    QVERIFY(!QByteArray("teXt").isUpper());
    QVERIFY(!QByteArray("texT").isUpper());
    QVERIFY(!QByteArray("TExt").isUpper());
    QVERIFY(!QByteArray("teXT").isUpper());
    QVERIFY(!QByteArray("tEXt").isUpper());
    QVERIFY(!QByteArray("tExT").isUpper());
    QVERIFY(!QByteArray("@ABYZ[").isUpper());
    QVERIFY(!QByteArray("@abyz[").isUpper());
    QVERIFY(!QByteArray("`ABYZ{").isUpper());
    QVERIFY(!QByteArray("`abyz{").isUpper());
}

void tst_QByteArray::isLower()
{
    QVERIFY(!QByteArray().isLower());
    QVERIFY(!QByteArray("").isLower());
    QVERIFY(QByteArray("text").isLower());
    QVERIFY(!QByteArray("\xE0\xFF").isLower()); // non-ASCII is neither upper nor lower
    QVERIFY(!QByteArray("\xF7").isLower());
    QVERIFY(!QByteArray("Text").isLower());
    QVERIFY(!QByteArray("tExt").isLower());
    QVERIFY(!QByteArray("teXt").isLower());
    QVERIFY(!QByteArray("texT").isLower());
    QVERIFY(!QByteArray("TExt").isLower());
    QVERIFY(!QByteArray("teXT").isLower());
    QVERIFY(!QByteArray("tEXt").isLower());
    QVERIFY(!QByteArray("tExT").isLower());
    QVERIFY(!QByteArray("TEXT").isLower());
    QVERIFY(!QByteArray("@ABYZ[").isLower());
    QVERIFY(!QByteArray("@abyz[").isLower());
    QVERIFY(!QByteArray("`ABYZ{").isLower());
    QVERIFY(!QByteArray("`abyz{").isLower());
}

void tst_QByteArray::macTypes()
{
#ifndef Q_OS_MAC
    QSKIP("This is a Apple-only test");
#else
    extern void tst_QByteArray_macTypes(); // in qbytearray_mac.mm
    tst_QByteArray_macTypes();
#endif
}

void tst_QByteArray::stdString()
{
    std::string stdstr( "QByteArray" );

    const QByteArray stlqt = QByteArray::fromStdString(stdstr);
    QCOMPARE(stlqt.length(), int(stdstr.length()));
    QCOMPARE(stlqt.data(), stdstr.c_str());
    QCOMPARE(stlqt.toStdString(), stdstr);

    std::string utf8str( "Nøt æscii" );
    const QByteArray u8 = QByteArray::fromStdString(utf8str);
    const QByteArray l1 = QString::fromUtf8(u8).toLatin1();
    std::string l1str = l1.toStdString();
    QVERIFY(l1str.length() < utf8str.length());
}

void tst_QByteArray::emptyAndClear()
{
    QByteArray a;
    QVERIFY(a.isEmpty());
    a.clear();
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a.append("data");
    QVERIFY(!a.isEmpty());

    a.clear();
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::fill()
{
    QByteArray a;
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    // filling an empty QByteArray does nothing
    a.fill('a');
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    // filling empty QByteArray to 0 length does nothing
    a.fill('a', 0);
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a.fill('b', 5);
    QCOMPARE(a, QByteArray("bbbbb"));

    a.fill('c');
    QCOMPARE(a, QByteArray("ccccc"));

    a.fill('d', 2);
    QCOMPARE(a, QByteArray("dd"));

    // filling to 0 length empties the QByteArray
    a.fill('a', 0);
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::dataPointers()
{
    QByteArray a;
    const char *constPtr = a.constData();
    QCOMPARE(a.data(), constPtr); // does not detach on empty QBA.

    a = "abc"; // detaches
    const char *dataConstPtr = a.constData();
    QVERIFY(dataConstPtr != constPtr);

    QByteArray copy = a;
    QCOMPARE(copy.constData(), dataConstPtr);

    char *dataPtr = copy.data(); // detaches, as the QBA is not empty
    QVERIFY(dataPtr != dataConstPtr);

    *dataPtr = 'd';
    QCOMPARE(copy, QByteArray("dbc"));
    QCOMPARE(a, QByteArray("abc"));
}

void tst_QByteArray::truncate()
{
    QByteArray a;
    a.truncate(0);
    a.truncate(10);
    QVERIFY(a.isEmpty());
    QVERIFY(!a.isDetached());

    a = QByteArray("abcdef");
    a.truncate(4);
    QCOMPARE(a, QByteArray("abcd"));
    a.truncate(5);
    QCOMPARE(a, QByteArray("abcd"));

    a.truncate(-5);
    QVERIFY(a.isEmpty());
}

void tst_QByteArray::trimmed()
{
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, expected);

    QCOMPARE(source.trimmed(), expected);
    QByteArray copy = source;
    QCOMPARE(std::move(copy).trimmed(), expected);

    if (source.isEmpty())
        QVERIFY(!source.isDetached());
}

void tst_QByteArray::trimmed_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null") << QByteArray() << QByteArray();
    QTest::newRow("empty") << QByteArray("") << QByteArray("");
    QTest::newRow("no spaces") << QByteArray("a b\nc\td") << QByteArray("a b\nc\td");
    QTest::newRow("with spaces") << QByteArray("\t \v a b\r\nc \td\ve   f \r\n\f")
                                 << QByteArray("a b\r\nc \td\ve   f");
    QTest::newRow("all spaces") << QByteArray("\t \r \n \v \f") << QByteArray("");
}

void tst_QByteArray::simplified()
{
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, expected);

    QCOMPARE(source.simplified(), expected);
    QByteArray copy = source;
    QCOMPARE(std::move(copy).simplified(), expected);

    if (source.isEmpty())
        QVERIFY(!source.isDetached());
}

void tst_QByteArray::simplified_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null") << QByteArray() << QByteArray();
    QTest::newRow("empty") << QByteArray("") << QByteArray("");
    QTest::newRow("no extra spaces") << QByteArray("a bc d") << QByteArray("a bc d");
    QTest::newRow("with spaces") << QByteArray("\t \v a  b\r\nc\td \r\n\f")
                                 << QByteArray("a b c d");
    QTest::newRow("all spaces") << QByteArray("\t \r \n \v \f") << QByteArray("");
}

void tst_QByteArray::left()
{
    QByteArray a;
    QCOMPARE(a.left(0), QByteArray());
    QCOMPARE(a.left(10), QByteArray());
    QVERIFY(!a.isDetached());

    a = QByteArray("abcdefgh");
    const char *ptr = a.constData();
    QCOMPARE(a.left(5), QByteArray("abcde"));
    QCOMPARE(a.left(20), a);
    QCOMPARE(a.left(-5), QByteArray());
    // calling left() does not modify the source array
    QCOMPARE(a.constData(), ptr);
}

void tst_QByteArray::right()
{
    QByteArray a;
    QCOMPARE(a.right(0), QByteArray());
    QCOMPARE(a.right(10), QByteArray());
    QVERIFY(!a.isDetached());

    a = QByteArray("abcdefgh");
    const char *ptr = a.constData();
    QCOMPARE(a.right(5), QByteArray("defgh"));
    QCOMPARE(a.right(20), a);
    QCOMPARE(a.right(-5), QByteArray());
    // calling right() does not modify the source array
    QCOMPARE(a.constData(), ptr);
}

void tst_QByteArray::mid()
{
    QByteArray a;
    QCOMPARE(a.mid(0), QByteArray());
    QCOMPARE(a.mid(0, 10), QByteArray());
    QCOMPARE(a.mid(10), QByteArray());
    QVERIFY(!a.isDetached());

    a = QByteArray("abcdefgh");
    const char *ptr = a.constData();
    QCOMPARE(a.mid(2), QByteArray("cdefgh"));
    QCOMPARE(a.mid(2, 3), QByteArray("cde"));
    QCOMPARE(a.mid(20), QByteArray());
    QCOMPARE(a.mid(-5), QByteArray("abcdefgh"));
    QCOMPARE(a.mid(-5, 8), QByteArray("abc"));
    // calling mid() does not modify the source array
    QCOMPARE(a.constData(), ptr);
}

void tst_QByteArray::length()
{
    QFETCH(QByteArray, src);
    QFETCH(qsizetype, res);

    QCOMPARE(src.length(), res);
    QCOMPARE(src.size(), res);
    QCOMPARE(src.count(), res);
}

void tst_QByteArray::length_data()
{
    QTest::addColumn<QByteArray>("src");
    QTest::addColumn<qsizetype>("res");

    QTest::newRow("null") << QByteArray() << qsizetype(0);
    QTest::newRow("empty") << QByteArray("") << qsizetype(0);
    QTest::newRow("letters and digits") << QByteArray("abc123") << qsizetype(6);
    QTest::newRow("with space chars") << QByteArray(" abc\r\n123\t\v") << qsizetype(11);
    QTest::newRow("with '\\0'") << QByteArray("abc\0def", 7) << qsizetype(7);
    QTest::newRow("with '\\0' no size") << QByteArray("abc\0def") << qsizetype(3);
}

const char globalChar = '1';

QTEST_MAIN(tst_QByteArray)
#include "tst_qbytearray.moc"
