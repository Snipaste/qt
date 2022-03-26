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

#include <QTest>
#include <private/qdatetimeparser_p.h>

QT_BEGIN_NAMESPACE

// access to needed members in QDateTimeParser
class QDTPUnitTestParser : public QDateTimeParser
{
public:
    QDTPUnitTestParser() : QDateTimeParser(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit) { }

    // forward data structures
    using QDateTimeParser::ParsedSection;
    using QDateTimeParser::State;

    // function to manipulate private internals
    void setText(QString text) { m_text = text; }

    // forwarding of methods
    using QDateTimeParser::parseSection;
};

bool operator==(const QDTPUnitTestParser::ParsedSection &a,
                const QDTPUnitTestParser::ParsedSection &b)
{
    return a.value == b.value && a.used == b.used && a.zeroes == b.zeroes && a.state == b.state;
}

// pretty printing for ParsedSection
char *toString(const QDTPUnitTestParser::ParsedSection &section)
{
    using QTest::toString;
    return toString(QByteArray("ParsedSection(") + "state=" + QByteArray::number(section.state)
                    + ", value=" + QByteArray::number(section.value)
                    + ", used=" + QByteArray::number(section.used)
                    + ", zeros=" + QByteArray::number(section.zeroes) + ")");
}

QT_END_NAMESPACE

class tst_QDateTimeParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseSection_data();
    void parseSection();

    void intermediateYear_data();
    void intermediateYear();
};

void tst_QDateTimeParser::parseSection_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("sectionIndex");
    QTest::addColumn<int>("offset");
    QTest::addColumn<QDTPUnitTestParser::ParsedSection>("expected");

    using ParsedSection = QDTPUnitTestParser::ParsedSection;
    using State = QDTPUnitTestParser::State;
    QTest::newRow("short-year-begin")
        << "yyyy_MM_dd" << "200_12_15" << 0 << 0
        << ParsedSection(State::Intermediate ,200, 3, 0);

    QTest::newRow("short-year-middle")
        << "MM-yyyy-dd" << "12-200-15" << 1 << 3
        << ParsedSection(State::Intermediate, 200, 3, 0);

    QTest::newRow("negative-year-middle-5")
        << "MM-yyyy-dd" << "12--2000-15" << 1 << 3
        << ParsedSection(State::Acceptable, -2000, 5, 0);

    QTest::newRow("short-negative-year-middle-4")
        << "MM-yyyy-dd" << "12--200-15" << 1 << 3
        << ParsedSection(State::Intermediate, -200, 4, 0);

    QTest::newRow("short-negative-year-middle-3")
        << "MM-yyyy-dd" << "12--20-15" << 1 << 3
        << ParsedSection(State::Intermediate, -20, 3, 0);

    QTest::newRow("short-negative-year-middle-2")
        << "MM-yyyy-dd" << "12--2-15" << 1 << 3
        << ParsedSection(State::Intermediate, -2, 2, 0);

    QTest::newRow("short-negative-year-middle-1")
        << "MM-yyyy-dd" << "12---15"  << 1 << 3
        << ParsedSection(State::Intermediate, 0, 1, 0);

    // Here the -15 will be understood as year, with separator and day omitted,
    // although it could equally be read as month and day with missing year.
    QTest::newRow("short-negative-year-middle-0")
        << "MM-yyyy-dd" << "12--15"  << 1 << 3
        << ParsedSection(State::Intermediate, -15, 3, 0);
}

void tst_QDateTimeParser::parseSection()
{
    QFETCH(QString, format);
    QFETCH(QString, input);
    QFETCH(int, sectionIndex);
    QFETCH(int, offset);
    QFETCH(QDTPUnitTestParser::ParsedSection, expected);

    QDTPUnitTestParser testParser;

    QVERIFY(testParser.parseFormat(format));
    QDateTime val(QDate(1900, 1, 1).startOfDay());

    testParser.setText(input);
    auto result = testParser.parseSection(val, sectionIndex, offset);
    QCOMPARE(result, expected);
}

void tst_QDateTimeParser::intermediateYear_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QDate>("expected");

    QTest::newRow("short-year-begin")
        << "yyyy_MM_dd" << "200_12_15" << QDate(200, 12, 15);
    QTest::newRow("short-year-mid")
        << "MM_yyyy_dd" << "12_200_15" << QDate(200, 12, 15);
    QTest::newRow("short-year-end")
        << "MM_dd_yyyy" << "12_15_200" << QDate(200, 12, 15);
}

void tst_QDateTimeParser::intermediateYear()
{
    QFETCH(QString, format);
    QFETCH(QString, input);
    QFETCH(QDate, expected);

    QDateTimeParser testParser(QMetaType::QDateTime, QDateTimeParser::DateTimeEdit);

    QVERIFY(testParser.parseFormat(format));

    QDateTime val(QDate(1900, 1, 1).startOfDay());
    const QDateTimeParser::StateNode tmp = testParser.parse(input, -1, val, false);
    QCOMPARE(tmp.state, QDateTimeParser::Intermediate);
    QCOMPARE(tmp.value, expected.startOfDay());
}

QTEST_APPLESS_MAIN(tst_QDateTimeParser)

#include "tst_qdatetimeparser.moc"
