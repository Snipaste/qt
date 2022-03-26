/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <time.h>
#include <qdatetime.h>
#include <private/qdatetime_p.h>

#ifdef Q_OS_WIN
#   include <qt_windows.h>
#endif

class tst_QDateTime : public QObject
{
    Q_OBJECT

public:
    tst_QDateTime();

public Q_SLOTS:
    void initTestCase();
private Q_SLOTS:
    void ctor();
    void operator_eq();
    void isNull();
    void isValid();
    void date();
    void time();
    void timeSpec();
    void toSecsSinceEpoch_data();
    void toSecsSinceEpoch();
    void daylightSavingsTimeChange_data();
    void daylightSavingsTimeChange();
    void springForward_data();
    void springForward();
    void setDate();
    void setTime_data();
    void setTime();
    void setTimeSpec_data();
    void setTimeSpec();
    void setSecsSinceEpoch();
    void setMSecsSinceEpoch_data();
    void setMSecsSinceEpoch();
    void fromSecsSinceEpoch();
    void fromMSecsSinceEpoch_data();
    void fromMSecsSinceEpoch();
#if QT_CONFIG(datestring)
    void toString_isoDate_data();
    void toString_isoDate();
    void toString_isoDate_extra();
    void toString_textDate_data();
    void toString_textDate();
    void toString_textDate_extra();
    void toString_rfcDate_data();
    void toString_rfcDate();
    void toString_enumformat();
    void toString_strformat();
#endif
    void addDays();
    void addInvalid();
    void addMonths();
    void addMonths_data();
    void addYears();
    void addYears_data();
    void addSecs_data();
    void addSecs();
    void addMSecs_data();
    void addMSecs();
    void toTimeSpec_data();
    void toTimeSpec();
    void toLocalTime_data();
    void toLocalTime();
    void toUTC_data();
    void toUTC();
    void daysTo();
    void secsTo_data();
    void secsTo();
    void msecsTo_data();
    void msecsTo();
    void operator_eqeq_data();
    void operator_eqeq();
    void operator_insert_extract_data();
    void operator_insert_extract();
    void currentDateTime();
    void currentDateTimeUtc();
    void currentDateTimeUtc2();
#if QT_CONFIG(datestring)
    void fromStringDateFormat_data();
    void fromStringDateFormat();
#  if QT_CONFIG(datetimeparser)
    void fromStringStringFormat_data();
    void fromStringStringFormat();
    void fromStringStringFormat_localTimeZone_data();
    void fromStringStringFormat_localTimeZone();
#  endif
#endif

    void offsetFromUtc();
    void setOffsetFromUtc();
    void toOffsetFromUtc();

    void zoneAtTime_data();
    void zoneAtTime();
    void timeZoneAbbreviation();

    void getDate();

    void fewDigitsInYear() const;
    void printNegativeYear() const;
#if QT_CONFIG(datetimeparser)
    void roundtripTextDate() const;
#endif
    void utcOffsetLessThan() const;

    void isDaylightTime() const;
    void daylightTransitions() const;
    void timeZones() const;
    void systemTimeZoneChange_data() const;
    void systemTimeZoneChange() const;

    void invalid_data() const;
    void invalid() const;
    void range() const;

    void macTypes();

private:
    enum { LocalTimeIsUtc = 0, LocalTimeAheadOfUtc = 1, LocalTimeBehindUtc = -1} localTimeType;
    int preZoneFix;
    bool zoneIsCET;

    class TimeZoneRollback
    {
        const QByteArray prior;
    public:
        // Save the previous timezone so we can restore it afterwards, otherwise
        // later tests may break:
        explicit TimeZoneRollback(const QByteArray &zone) : prior(qgetenv("TZ"))
        { reset(zone); }
        void reset(const QByteArray &zone)
        {
            qputenv("TZ", zone.constData());
            qTzSet();
        }
        ~TimeZoneRollback()
        {
            if (prior.isNull())
                qunsetenv("TZ");
            else
                qputenv("TZ", prior.constData());
            qTzSet();
        }
    };
};

Q_DECLARE_METATYPE(Qt::TimeSpec)
Q_DECLARE_METATYPE(Qt::DateFormat)

tst_QDateTime::tst_QDateTime()
{
    /*
      Due to some jurisdictions changing their zones and rules, it's possible
      for a non-CET zone to accidentally match CET at a few tested moments but
      be different a few years later or earlier.  This would lead to tests
      failing if run in the partially-aliasing zone (e.g. Algeria, Lybia).  So
      test thoroughly; ideally at every mid-winter or mid-summer in whose
      half-year any test below assumes zoneIsCET means what it says.  (Tests at
      or near a DST transition implicate both of the half-years that meet
      there.)  Years outside the +ve half of 32-bit time_t's range, however,
      might not be properly handled by our work-arounds for the MS backend and
      32-bit time_t; so don't probe them here.
    */
    const uint day = 24 * 3600; // in seconds
    zoneIsCET = (QDateTime(QDate(2038, 1, 19), QTime(4, 14, 7)).toSecsSinceEpoch() == 0x7fffffff
                 // Entries a year apart robustly differ by multiples of day.
                 && QDate(2015, 7, 1).startOfDay().toSecsSinceEpoch() == 1435701600
                 && QDate(2015, 1, 1).startOfDay().toSecsSinceEpoch() == 1420066800
                 && QDate(2013, 7, 1).startOfDay().toSecsSinceEpoch() == 1372629600
                 && QDate(2013, 1, 1).startOfDay().toSecsSinceEpoch() == 1356994800
                 && QDate(2012, 7, 1).startOfDay().toSecsSinceEpoch() == 1341093600
                 && QDate(2012, 1, 1).startOfDay().toSecsSinceEpoch() == 1325372400
                 && QDate(2008, 7, 1).startOfDay().toSecsSinceEpoch() == 1214863200
                 && QDate(2004, 1, 1).startOfDay().toSecsSinceEpoch() == 1072911600
                 && QDate(2000, 1, 1).startOfDay().toSecsSinceEpoch() == 946681200
                 && QDate(1990, 7, 1).startOfDay().toSecsSinceEpoch() == 646783200
                 && QDate(1990, 1, 1).startOfDay().toSecsSinceEpoch() == 631148400
                 && QDate(1979, 1, 1).startOfDay().toSecsSinceEpoch() == 283993200
                 // .toSecsSinceEpoch() returns -1 for everything before this:
                 && QDateTime(QDate(1970, 1, 1), QTime(1, 0)).toSecsSinceEpoch() == 0);
    // Use .toMSecsSinceEpoch() if you really need to test anything earlier.

    /*
      Zones which currently appear to be CET may have distinct offsets before
      the advent of time-zones. The date used here is the eve of the birth of
      Dr. William Hyde Wollaston, who first proposed a uniform national time,
      instead of local mean time:
    */
    preZoneFix = zoneIsCET ? QDate(1766, 8, 5).startOfDay().offsetFromUtc() - 3600 : 0;
    // Madrid, actually west of Greenwich, uses CET as if it were an hour east
    // of Greenwich; allow that the fix might be more than an hour, either way:
    Q_ASSERT(preZoneFix > -7200 && preZoneFix < 7200);
    // So it's OK to add it to a QTime() between 02:00 and 22:00, but otherwise
    // we must add it to the QDateTime constructed from it.

    /*
      Again, rule changes can cause a TZ to look like UTC at some sample dates
      but deviate at some date relevant to a test using localTimeType.  These
      tests mostly use years outside the 1970--2037 range, for which we trust
      our TZ data, so we can't helpfully be exhaustive.  Instead, scan a sample
      of years' starts and middles.
    */
    const int sampled = 3;
    // UTC starts of months in 2004, 2038 and 1970:
    qint64 jans[sampled] = { 12418 * day, 24837 * day, 0 };
    qint64 juls[sampled] = { 12600 * day, 25018 * day, 181 * day };
    localTimeType = LocalTimeIsUtc;
    for (int i = sampled; i-- > 0; ) {
        QDateTime jan = QDateTime::fromSecsSinceEpoch(jans[i]);
        QDateTime jul = QDateTime::fromSecsSinceEpoch(juls[i]);
        if (jan.date().year() < 1970 || jul.date().month() < 7) {
            localTimeType = LocalTimeBehindUtc;
            break;
        } else if (jan.time().hour() > 0 || jul.time().hour() > 0
                   || jan.date().day() > 1 || jul.date().day() > 1) {
            localTimeType = LocalTimeAheadOfUtc;
            break;
        }
    }
}

void tst_QDateTime::initTestCase()
{
    // Never construct a message like this in an i18n context...
    const char *typemsg1 = "exactly";
    const char *typemsg2 = "and therefore not";
    switch (localTimeType) {
    case LocalTimeIsUtc:
        break;
    case LocalTimeBehindUtc:
        typemsg1 = "behind";
        break;
    case LocalTimeAheadOfUtc:
        typemsg1 = "ahead of";
        typemsg2 = zoneIsCET ? "and is" : "but isn't";
        break;
    }

    qDebug() << "Current local time detected to be"
             << typemsg1
             << "UTC"
             << typemsg2
             << "the Central European timezone";
}

void tst_QDateTime::ctor()
{
    QDateTime dt1(QDate(2004, 1, 2), QTime(1, 2, 3));
    QCOMPARE(dt1.timeSpec(), Qt::LocalTime);
    QDateTime dt2(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::LocalTime);
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);
    QDateTime dt3(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::UTC);
    QCOMPARE(dt3.timeSpec(), Qt::UTC);

    QVERIFY(dt1 == dt2);
    if (zoneIsCET) {
        QVERIFY(dt1 != dt3);
        QVERIFY(dt1 < dt3);
        QVERIFY(dt1.addSecs(3600).toUTC() == dt3);
    }

    // Test OffsetFromUTC constructors
    QDate offsetDate(2013, 1, 1);
    QTime offsetTime(1, 2, 3);

    QDateTime offset1(offsetDate, offsetTime, Qt::OffsetFromUTC);
    QCOMPARE(offset1.timeSpec(), Qt::UTC);
    QCOMPARE(offset1.offsetFromUtc(), 0);
    QCOMPARE(offset1.date(), offsetDate);
    QCOMPARE(offset1.time(), offsetTime);

    QDateTime offset2(offsetDate, offsetTime, Qt::OffsetFromUTC, 0);
    QCOMPARE(offset2.timeSpec(), Qt::UTC);
    QCOMPARE(offset2.offsetFromUtc(), 0);
    QCOMPARE(offset2.date(), offsetDate);
    QCOMPARE(offset2.time(), offsetTime);

    QDateTime offset3(offsetDate, offsetTime, Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(offset3.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(offset3.offsetFromUtc(), 60 * 60);
    QCOMPARE(offset3.date(), offsetDate);
    QCOMPARE(offset3.time(), offsetTime);

    QDateTime offset4(offsetDate, QTime(0, 0), Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(offset4.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(offset4.offsetFromUtc(), 60 * 60);
    QCOMPARE(offset4.date(), offsetDate);
    QCOMPARE(offset4.time(), QTime(0, 0));
}

void tst_QDateTime::operator_eq()
{
    QVERIFY(QDateTime() != QDateTime(QDate(1970, 1, 1), QTime(0, 0))); // QTBUG-79006
    QDateTime dt1(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::UTC);
    QDateTime dt2(QDate(2005, 3, 11), QTime(0, 0), Qt::UTC);
    dt2 = dt1;
    QVERIFY(dt1 == dt2);
}

void tst_QDateTime::isNull()
{
    QDateTime dt1;
    QVERIFY(dt1.isNull());
    dt1.setDate(QDate());
    QVERIFY(dt1.isNull());
    dt1.setTime(QTime());
    QVERIFY(dt1.isNull());
    dt1.setTimeSpec(Qt::UTC);
    QVERIFY(dt1.isNull());   // maybe it should return false?

    dt1.setDate(QDate(2004, 1, 2));
    QVERIFY(!dt1.isNull());
    dt1.setTime(QTime(12, 34, 56));
    QVERIFY(!dt1.isNull());
    dt1.setTime(QTime());
    QVERIFY(!dt1.isNull());
}

void tst_QDateTime::isValid()
{
    QDateTime dt1;
    QVERIFY(!dt1.isValid());
    dt1.setDate(QDate());
    QVERIFY(!dt1.isValid());
    dt1.setTime(QTime());
    QVERIFY(!dt1.isValid());
    dt1.setTimeSpec(Qt::UTC);
    QVERIFY(!dt1.isValid());

    dt1.setDate(QDate(2004, 1, 2));
    QVERIFY(dt1.isValid());
    dt1.setDate(QDate());
    QVERIFY(!dt1.isValid());
    dt1.setTime(QTime(12, 34, 56));
    QVERIFY(!dt1.isValid());
    dt1.setTime(QTime());
    QVERIFY(!dt1.isValid());
}

void tst_QDateTime::date()
{
    QDateTime dt1(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::LocalTime);
    QCOMPARE(dt1.date(), QDate(2004, 3, 24));

    QDateTime dt2(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    QCOMPARE(dt2.date(), QDate(2004, 3, 25));

    QDateTime dt3(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::UTC);
    QCOMPARE(dt3.date(), QDate(2004, 3, 24));

    QDateTime dt4(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    QCOMPARE(dt4.date(), QDate(2004, 3, 25));
}

void tst_QDateTime::time()
{
    QDateTime dt1(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::LocalTime);
    QCOMPARE(dt1.time(), QTime(23, 45, 57));

    QDateTime dt2(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    QCOMPARE(dt2.time(), QTime(0, 45, 57));

    QDateTime dt3(QDate(2004, 3, 24), QTime(23, 45, 57), Qt::UTC);
    QCOMPARE(dt3.time(), QTime(23, 45, 57));

    QDateTime dt4(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    QCOMPARE(dt4.time(), QTime(0, 45, 57));
}

void tst_QDateTime::timeSpec()
{
    QDateTime dt1(QDate(2004, 1, 24), QTime(23, 45, 57));
    QCOMPARE(dt1.timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addDays(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addMonths(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addMonths(6).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addYears(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addSecs(0).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.addSecs(86400 * 185).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.toTimeSpec(Qt::LocalTime).timeSpec(), Qt::LocalTime);
    QCOMPARE(dt1.toTimeSpec(Qt::UTC).timeSpec(), Qt::UTC);

    QDateTime dt2(QDate(2004, 1, 24), QTime(23, 45, 57), Qt::LocalTime);
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);

    QDateTime dt3(QDate(2004, 1, 25), QTime(0, 45, 57), Qt::UTC);
    QCOMPARE(dt3.timeSpec(), Qt::UTC);

    QDateTime dt4 = QDateTime::currentDateTime();
    QCOMPARE(dt4.timeSpec(), Qt::LocalTime);
}

void tst_QDateTime::setDate()
{
    QDateTime dt1(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    dt1.setDate(QDate(2004, 6, 25));
    QCOMPARE(dt1.date(), QDate(2004, 6, 25));
    QCOMPARE(dt1.time(), QTime(0, 45, 57));
    QCOMPARE(dt1.timeSpec(), Qt::UTC);

    QDateTime dt2(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    dt2.setDate(QDate(2004, 6, 25));
    QCOMPARE(dt2.date(), QDate(2004, 6, 25));
    QCOMPARE(dt2.time(), QTime(0, 45, 57));
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);

    QDateTime dt3(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::UTC);
    dt3.setDate(QDate(4004, 6, 25));
    QCOMPARE(dt3.date(), QDate(4004, 6, 25));
    QCOMPARE(dt3.time(), QTime(0, 45, 57));
    QCOMPARE(dt3.timeSpec(), Qt::UTC);

    QDateTime dt4(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    dt4.setDate(QDate(4004, 6, 25));
    QCOMPARE(dt4.date(), QDate(4004, 6, 25));
    QCOMPARE(dt4.time(), QTime(0, 45, 57));
    QCOMPARE(dt4.timeSpec(), Qt::LocalTime);

    QDateTime dt5(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::UTC);
    dt5.setDate(QDate(1760, 6, 25));
    QCOMPARE(dt5.date(), QDate(1760, 6, 25));
    QCOMPARE(dt5.time(), QTime(0, 45, 57));
    QCOMPARE(dt5.timeSpec(), Qt::UTC);

    QDateTime dt6(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::LocalTime);
    dt6.setDate(QDate(1760, 6, 25));
    QCOMPARE(dt6.date(), QDate(1760, 6, 25));
    QCOMPARE(dt6.time(), QTime(0, 45, 57));
    QCOMPARE(dt6.timeSpec(), Qt::LocalTime);
}

void tst_QDateTime::setTime_data()
{
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<QTime>("newTime");

    QTest::newRow("data0") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << QTime(23, 11, 22);
    QTest::newRow("data1") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::LocalTime) << QTime(23, 11, 22);
    QTest::newRow("data2") << QDateTime(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::UTC) << QTime(23, 11, 22);
    QTest::newRow("data3") << QDateTime(QDate(4004, 3, 25), QTime(0, 45, 57), Qt::LocalTime) << QTime(23, 11, 22);
    QTest::newRow("data4") << QDateTime(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::UTC) << QTime(23, 11, 22);
    QTest::newRow("data5") << QDateTime(QDate(1760, 3, 25), QTime(0, 45, 57), Qt::LocalTime) << QTime(23, 11, 22);

    QTest::newRow("set on std/dst") << QDateTime::currentDateTime() << QTime(23, 11, 22);
}

void tst_QDateTime::setTime()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(QTime, newTime);

    const QDate expectedDate(dateTime.date());
    const Qt::TimeSpec expectedTimeSpec(dateTime.timeSpec());

    dateTime.setTime(newTime);

    QCOMPARE(dateTime.date(), expectedDate);
    QCOMPARE(dateTime.time(), newTime);
    QCOMPARE(dateTime.timeSpec(), expectedTimeSpec);
}

void tst_QDateTime::setTimeSpec_data()
{
    QTest::addColumn<QDateTime>("dateTime");
    QTest::addColumn<Qt::TimeSpec>("newTimeSpec");

    QTest::newRow("UTC => UTC") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << Qt::UTC;
    QTest::newRow("UTC => LocalTime") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << Qt::LocalTime;
    QTest::newRow("UTC => OffsetFromUTC") << QDateTime(QDate(2004, 3, 25), QTime(0, 45, 57), Qt::UTC) << Qt::OffsetFromUTC;
}

void tst_QDateTime::setTimeSpec()
{
    QFETCH(QDateTime, dateTime);
    QFETCH(Qt::TimeSpec, newTimeSpec);

    const QDate expectedDate(dateTime.date());
    const QTime expectedTime(dateTime.time());

    dateTime.setTimeSpec(newTimeSpec);

    QCOMPARE(dateTime.date(), expectedDate);
    QCOMPARE(dateTime.time(), expectedTime);
    if (newTimeSpec == Qt::OffsetFromUTC)
        QCOMPARE(dateTime.timeSpec(), Qt::UTC);
    else
        QCOMPARE(dateTime.timeSpec(), newTimeSpec);
}

void tst_QDateTime::setSecsSinceEpoch()
{
    QDateTime dt1;
    dt1.setSecsSinceEpoch(0);
    QCOMPARE(dt1.toUTC(), QDate(1970, 1, 1).startOfDay(Qt::UTC));
    QCOMPARE(dt1.timeSpec(), Qt::LocalTime);

    dt1.setTimeSpec(Qt::UTC);
    dt1.setSecsSinceEpoch(0);
    QCOMPARE(dt1, QDate(1970, 1, 1).startOfDay(Qt::UTC));
    QCOMPARE(dt1.timeSpec(), Qt::UTC);

    dt1.setSecsSinceEpoch(123456);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch(123456);
        QCOMPARE(dt2, QDateTime(QDate(1970, 1, 2), QTime(11, 17, 36), Qt::LocalTime));
    }

    dt1.setSecsSinceEpoch((uint)(quint32)-123456);
    QCOMPARE(dt1, QDateTime(QDate(2106, 2, 5), QTime(20, 10, 40), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch((uint)(quint32)-123456);
        QCOMPARE(dt2, QDateTime(QDate(2106, 2, 5), QTime(21, 10, 40), Qt::LocalTime));
    }

    dt1.setSecsSinceEpoch(1214567890);
    QCOMPARE(dt1, QDateTime(QDate(2008, 6, 27), QTime(11, 58, 10), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch(1214567890);
        QCOMPARE(dt2, QDateTime(QDate(2008, 6, 27), QTime(13, 58, 10), Qt::LocalTime));
    }

    dt1.setSecsSinceEpoch(0x7FFFFFFF);
    QCOMPARE(dt1, QDateTime(QDate(2038, 1, 19), QTime(3, 14, 7), Qt::UTC));
    if (zoneIsCET) {
        QDateTime dt2;
        dt2.setSecsSinceEpoch(0x7FFFFFFF);
        QCOMPARE(dt2, QDateTime(QDate(2038, 1, 19), QTime(4, 14, 7), Qt::LocalTime));
    }

    dt1 = QDateTime(QDate(2013, 1, 1), QTime(0, 0), Qt::OffsetFromUTC, 60 * 60);
    dt1.setSecsSinceEpoch(123456);
    QCOMPARE(dt1, QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36), Qt::UTC));
    QCOMPARE(dt1.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);

    // Only testing UTC; see fromSecsSinceEpoch() for fuller test.
    dt1.setTimeSpec(Qt::UTC);
    const qint64 maxSeconds = std::numeric_limits<qint64>::max() / 1000;
    dt1.setSecsSinceEpoch(maxSeconds);
    QVERIFY(dt1.isValid());
    dt1.setSecsSinceEpoch(-maxSeconds);
    QVERIFY(dt1.isValid());
    dt1.setSecsSinceEpoch(maxSeconds + 1);
    QVERIFY(!dt1.isValid());
    dt1.setSecsSinceEpoch(0);
    QVERIFY(dt1.isValid());
    dt1.setSecsSinceEpoch(-maxSeconds - 1);
    QVERIFY(!dt1.isValid());
}

void tst_QDateTime::setMSecsSinceEpoch_data()
{
    QTest::addColumn<qint64>("msecs");
    QTest::addColumn<QDateTime>("utc");
    QTest::addColumn<QDateTime>("cet");

    QTest::newRow("zero")
            << Q_INT64_C(0)
            << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(1, 0));
    QTest::newRow("+1ms")
            << Q_INT64_C(+1)
            << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0, 1), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(1, 0, 0, 1));
    QTest::newRow("+1s")
            << Q_INT64_C(+1000)
            << QDateTime(QDate(1970, 1, 1), QTime(0, 0, 1), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(1, 0, 1));
    QTest::newRow("-1ms")
            << Q_INT64_C(-1)
            << QDateTime(QDate(1969, 12, 31), QTime(23, 59, 59, 999), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(0, 59, 59, 999));
    QTest::newRow("-1s")
            << Q_INT64_C(-1000)
            << QDateTime(QDate(1969, 12, 31), QTime(23, 59, 59), Qt::UTC)
            << QDateTime(QDate(1970, 1, 1), QTime(0, 59, 59));
    QTest::newRow("123456789")
            << Q_INT64_C(123456789)
            << QDateTime(QDate(1970, 1, 2), QTime(10, 17, 36, 789), Qt::UTC)
            << QDateTime(QDate(1970, 1, 2), QTime(11, 17, 36, 789), Qt::LocalTime);
    QTest::newRow("-123456789")
            << Q_INT64_C(-123456789)
            << QDateTime(QDate(1969, 12, 30), QTime(13, 42, 23, 211), Qt::UTC)
            << QDateTime(QDate(1969, 12, 30), QTime(14, 42, 23, 211), Qt::LocalTime);
    QTest::newRow("post-32-bit-time_t")
            << (Q_INT64_C(1000) << 32)
            << QDateTime(QDate(2106, 2, 7), QTime(6, 28, 16), Qt::UTC)
            << QDateTime(QDate(2106, 2, 7), QTime(7, 28, 16));
    QTest::newRow("very-large")
            << (Q_INT64_C(123456) << 32)
            << QDateTime(QDate(18772, 8, 15), QTime(1, 8, 14, 976), Qt::UTC)
            << QDateTime(QDate(18772, 8, 15), QTime(3, 8, 14, 976));
    QTest::newRow("old min (Tue Nov 25 00:00:00 -4714)")
            << Q_INT64_C(-210866716800000)
            << QDateTime(QDate::fromJulianDay(1), QTime(0, 0), Qt::UTC)
            << QDateTime(QDate::fromJulianDay(1), QTime(1, 0)).addSecs(preZoneFix);
    QTest::newRow("old max (Tue Jun 3 21:59:59 5874898)")
            << Q_INT64_C(185331720376799999)
            << QDateTime(QDate::fromJulianDay(0x7fffffff), QTime(21, 59, 59, 999), Qt::UTC)
            << QDateTime(QDate::fromJulianDay(0x7fffffff), QTime(23, 59, 59, 999));
    QTest::newRow("min")
            << std::numeric_limits<qint64>::min()
            << QDateTime(QDate(-292275056, 5, 16), QTime(16, 47, 4, 192), Qt::UTC)
            << QDateTime(QDate(-292275056, 5, 16), QTime(17, 47, 4, 192).addSecs(preZoneFix));
    QTest::newRow("max")
            << std::numeric_limits<qint64>::max()
            << QDateTime(QDate(292278994, 8, 17), QTime(7, 12, 55, 807), Qt::UTC)
            << QDateTime(QDate(292278994, 8, 17), QTime(9, 12, 55, 807), Qt::LocalTime);
}

void tst_QDateTime::setMSecsSinceEpoch()
{
    QFETCH(qint64, msecs);
    QFETCH(QDateTime, utc);
    QFETCH(QDateTime, cet);

    QDateTime dt;
    dt.setTimeSpec(Qt::UTC);
    dt.setMSecsSinceEpoch(msecs);

    QCOMPARE(dt, utc);
    QCOMPARE(dt.date(), utc.date());
    QCOMPARE(dt.time(), utc.time());
    QCOMPARE(dt.timeSpec(), Qt::UTC);

    {
        QDateTime dt1 = QDateTime::fromMSecsSinceEpoch(msecs, Qt::UTC);
        QCOMPARE(dt1, utc);
        QCOMPARE(dt1.date(), utc.date());
        QCOMPARE(dt1.time(), utc.time());
        QCOMPARE(dt1.timeSpec(), Qt::UTC);
    }
    {
        QDateTime dt1(utc.date(), utc.time(), Qt::UTC);
        QCOMPARE(dt1, utc);
        QCOMPARE(dt1.date(), utc.date());
        QCOMPARE(dt1.time(), utc.time());
        QCOMPARE(dt1.timeSpec(), Qt::UTC);
    }
    {
        // used to fail to clear the ShortData bit, causing corruption
        QDateTime dt1 = dt.addDays(0);
        QCOMPARE(dt1, utc);
        QCOMPARE(dt1.date(), utc.date());
        QCOMPARE(dt1.time(), utc.time());
        QCOMPARE(dt1.timeSpec(), Qt::UTC);
    }

    if (zoneIsCET) {
        QCOMPARE(dt.toLocalTime(), cet);

        // Test converting from LocalTime to UTC back to LocalTime.
        QDateTime localDt;
        localDt.setTimeSpec(Qt::LocalTime);
        localDt.setMSecsSinceEpoch(msecs);

        // LocalTime will overflow for max
        if (msecs != std::numeric_limits<qint64>::max()
            //... or for min, if this CET zone is west of Greenwich (Europe/Madrid)
            && (preZoneFix >= -3600 || msecs != std::numeric_limits<qint64>::min())) {
            QCOMPARE(localDt, utc);
        }
        QCOMPARE(localDt.timeSpec(), Qt::LocalTime);

        // Compare result for LocalTime to TimeZone
        QDateTime dt2;
#if QT_CONFIG(timezone)
        QTimeZone europe("Europe/Oslo");
        dt2.setTimeZone(europe);
#endif
        dt2.setMSecsSinceEpoch(msecs);
        if (cet.date().year() >= 1970 || cet.date() == utc.date())
            QCOMPARE(dt2.date(), cet.date());

        // Don't compare the time if the date is too early: prior to the early
        // 20th century, timezones in Europe were not standardised. Limit to the
        // same year-range as we used when determining zoneIsCET:
        if (cet.date().year() >= 1970 && cet.date().year() <= 2037)
            QCOMPARE(dt2.time(), cet.time());
#if QT_CONFIG(timezone)
        QCOMPARE(dt2.timeSpec(), Qt::TimeZone);
        QCOMPARE(dt2.timeZone(), europe);
#endif
    }

    QCOMPARE(dt.toMSecsSinceEpoch(), msecs);
    QCOMPARE(qint64(dt.toSecsSinceEpoch()), msecs / 1000);

    QDateTime reference(QDate(1970, 1, 1), QTime(0, 0), Qt::UTC);
    QCOMPARE(dt, reference.addMSecs(msecs));

    // Tests that we correctly recognize when we fall off the extremities:
    if (msecs == std::numeric_limits<qint64>::max()) {
        QDateTime off(QDate(1970, 1, 1).startOfDay(Qt::OffsetFromUTC, 1));
        off.setMSecsSinceEpoch(msecs);
        QVERIFY(!off.isValid());
    } else if (msecs == std::numeric_limits<qint64>::min()) {
        QDateTime off(QDate(1970, 1, 1).startOfDay(Qt::OffsetFromUTC, -1));
        off.setMSecsSinceEpoch(msecs);
        QVERIFY(!off.isValid());
    }

    if ((localTimeType == LocalTimeAheadOfUtc && msecs == std::numeric_limits<qint64>::max())
        || (localTimeType == LocalTimeBehindUtc && msecs == std::numeric_limits<qint64>::min())) {
        QDateTime curt = QDate(1970, 1, 1).startOfDay(); // initially in short-form
        curt.setMSecsSinceEpoch(msecs); // Overflows due to offset
        QVERIFY(!curt.isValid());
    }
}

void tst_QDateTime::fromMSecsSinceEpoch_data()
{
    setMSecsSinceEpoch_data();
}

void tst_QDateTime::fromMSecsSinceEpoch()
{
    QFETCH(qint64, msecs);
    QFETCH(QDateTime, utc);
    QFETCH(QDateTime, cet);
    using Bound = std::numeric_limits<qint64>;
    if (msecs == Bound::min())
        qDebug() << "Local overflow:" << preZoneFix << Qt::hex;

    QDateTime dtLocal = QDateTime::fromMSecsSinceEpoch(msecs, Qt::LocalTime);
    QDateTime dtUtc = QDateTime::fromMSecsSinceEpoch(msecs, Qt::UTC);
    QDateTime dtOffset = QDateTime::fromMSecsSinceEpoch(msecs, Qt::OffsetFromUTC, 60*60);
    // LocalTime will overflow for "min" or "max" tests, depending on whether
    // you're East or West of Greenwich.  In UTC, we won't overflow. If we're
    // actually west of Greenwich but (e.g. Europe/Madrid) our zone claims east,
    // "min" can also overflow (case only caught if local time is CET).
    const bool localOverflow = (localTimeType == LocalTimeAheadOfUtc
                                ? msecs == Bound::max() || preZoneFix < -3600
                                : localTimeType == LocalTimeBehindUtc && msecs == Bound::min());
    if (!localOverflow)
        QCOMPARE(dtLocal, utc);

    QCOMPARE(dtUtc, utc);
    QCOMPARE(dtUtc.date(), utc.date());
    QCOMPARE(dtUtc.time(), utc.time());

    if (msecs == Bound::max()) { // Offset is positive, so overflows max
        QVERIFY(!dtOffset.isValid());
    } else {
        QCOMPARE(dtOffset, utc);
        QCOMPARE(dtOffset.offsetFromUtc(), 60*60);
        QCOMPARE(dtOffset.time(), utc.time().addMSecs(60*60*1000));
    }

    if (zoneIsCET) {
        QCOMPARE(dtLocal.toLocalTime(), cet);
        QCOMPARE(dtUtc.toLocalTime(), cet);
        if (msecs != Bound::max())
            QCOMPARE(dtOffset.toLocalTime(), cet);
    }

    if (!localOverflow)
        QCOMPARE(dtLocal.toMSecsSinceEpoch(), msecs);
    QCOMPARE(dtUtc.toMSecsSinceEpoch(), msecs);
    if (msecs != Bound::max())
        QCOMPARE(dtOffset.toMSecsSinceEpoch(), msecs);

    if (!localOverflow)
        QCOMPARE(qint64(dtLocal.toSecsSinceEpoch()), msecs / 1000);
    QCOMPARE(qint64(dtUtc.toSecsSinceEpoch()), msecs / 1000);
    if (msecs != Bound::max())
        QCOMPARE(qint64(dtOffset.toSecsSinceEpoch()), msecs / 1000);

    QDateTime reference(QDate(1970, 1, 1), QTime(0, 0), Qt::UTC);
    if (!localOverflow)
        QCOMPARE(dtLocal, reference.addMSecs(msecs));
    QCOMPARE(dtUtc, reference.addMSecs(msecs));
    if (msecs != Bound::max())
        QCOMPARE(dtOffset, reference.addMSecs(msecs));
}

void tst_QDateTime::fromSecsSinceEpoch()
{
    // Compare setSecsSinceEpoch()
    const qint64 maxSeconds = std::numeric_limits<qint64>::max() / 1000;
    const QDateTime early = QDateTime::fromSecsSinceEpoch(-maxSeconds, Qt::UTC);
    const QDateTime late = QDateTime::fromSecsSinceEpoch(maxSeconds, Qt::UTC);

    QVERIFY(late.isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(maxSeconds + 1, Qt::UTC).isValid());
    QVERIFY(early.isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(-maxSeconds - 1, Qt::UTC).isValid());

    // Local time: need to adjust for its zone offset
    const qint64 last = maxSeconds - qMax(late.addYears(-1).toLocalTime().offsetFromUtc(), 0);
    QVERIFY(QDateTime::fromSecsSinceEpoch(last).isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(last + 1).isValid());
    const qint64 first = -maxSeconds - qMin(early.addYears(1).toLocalTime().offsetFromUtc(), 0);
    QVERIFY(QDateTime::fromSecsSinceEpoch(first).isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(first - 1).isValid());

    // Use an offset for which .toUTC()'s return would flip the validity:
    QVERIFY(QDateTime::fromSecsSinceEpoch(maxSeconds - 7200, Qt::OffsetFromUTC, 7200).isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(maxSeconds - 7199, Qt::OffsetFromUTC, 7200).isValid());
    QVERIFY(QDateTime::fromSecsSinceEpoch(7200 - maxSeconds, Qt::OffsetFromUTC, -7200).isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(7199 - maxSeconds, Qt::OffsetFromUTC, -7200).isValid());

#if QT_CONFIG(timezone)
    // As for offset, use zones each side of UTC:
    const QTimeZone west("UTC-02:00"), east("UTC+02:00");
    QVERIFY(QDateTime::fromSecsSinceEpoch(maxSeconds, west).isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(maxSeconds + 1, east).isValid());
    QVERIFY(QDateTime::fromSecsSinceEpoch(-maxSeconds, east).isValid());
    QVERIFY(!QDateTime::fromSecsSinceEpoch(-maxSeconds - 1, west).isValid());
#endif // timezone
}

#if QT_CONFIG(datestring) // depends on, so implies, textdate
void tst_QDateTime::toString_isoDate_data()
{
    QTest::addColumn<QDateTime>("datetime");
    QTest::addColumn<Qt::DateFormat>("format");
    QTest::addColumn<QString>("expected");

    QTest::newRow("localtime")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34))
            << Qt::ISODate << QString("1978-11-09T13:28:34");
    QTest::newRow("UTC")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << Qt::ISODate << QString("1978-11-09T13:28:34Z");
    QDateTime dt(QDate(1978, 11, 9), QTime(13, 28, 34));
    dt.setOffsetFromUtc(19800);
    QTest::newRow("positive OffsetFromUTC")
            << dt << Qt::ISODate
            << QString("1978-11-09T13:28:34+05:30");
    dt.setOffsetFromUtc(-7200);
    QTest::newRow("negative OffsetFromUTC")
            << dt << Qt::ISODate
            << QString("1978-11-09T13:28:34-02:00");
    dt.setOffsetFromUtc(-900);
    QTest::newRow("negative non-integral OffsetFromUTC")
            << dt << Qt::ISODate
            << QString("1978-11-09T13:28:34-00:15");
    QTest::newRow("invalid") // ISODate < 2019 doesn't allow -ve year numbers; QTBUG-91070
            << QDateTime(QDate(-1, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << Qt::ISODate << QString();
    QTest::newRow("without-ms")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34, 20))
            << Qt::ISODate << QString("1978-11-09T13:28:34");
    QTest::newRow("with-ms")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34, 20))
            << Qt::ISODateWithMs << QString("1978-11-09T13:28:34.020");
}

void tst_QDateTime::toString_isoDate()
{
    QFETCH(const QDateTime, datetime);
    QFETCH(const Qt::DateFormat, format);
    QFETCH(const QString, expected);

    const QString result = datetime.toString(format);
    QCOMPARE(result, expected);

    const QDateTime resultDatetime = QDateTime::fromString(result, format);
    if (QByteArrayView(QTest::currentDataTag()) == "invalid") {
        QCOMPARE(resultDatetime, QDateTime());
    } else {
        const QDateTime when =
            QByteArrayView(QTest::currentDataTag()) == "without-ms"
            ? datetime.addMSecs(-datetime.time().msec()) : datetime;
        QCOMPARE(resultDatetime.toMSecsSinceEpoch(), when.toMSecsSinceEpoch());
        QCOMPARE(resultDatetime, when);
        QCOMPARE(resultDatetime.date(), when.date());
        QCOMPARE(resultDatetime.time(), when.time());
        QCOMPARE(resultDatetime.timeSpec(), when.timeSpec());
        QCOMPARE(resultDatetime.offsetFromUtc(), when.offsetFromUtc());
    }
}

void tst_QDateTime::toString_isoDate_extra()
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(0, Qt::UTC);
    QCOMPARE(dt.toString(Qt::ISODate), QLatin1String("1970-01-01T00:00:00Z"));
#if QT_CONFIG(timezone)
    QTimeZone PST("America/Vancouver");
    if (PST.isValid()) {
        dt = QDateTime::fromMSecsSinceEpoch(0, PST);
        QCOMPARE(dt.toString(Qt::ISODate), QLatin1String("1969-12-31T16:00:00-08:00"));
    } else {
        qDebug("Missed zone test: no America/Vancouver zone available");
    }
    QTimeZone CET("Europe/Berlin");
    if (CET.isValid()) {
        dt = QDateTime::fromMSecsSinceEpoch(0, CET);
        QCOMPARE(dt.toString(Qt::ISODate), QLatin1String("1970-01-01T01:00:00+01:00"));
    } else {
        qDebug("Missed zone test: no Europe/Berlin zone available");
    }
#endif // timezone
}

void tst_QDateTime::toString_textDate_data()
{
    QTest::addColumn<QDateTime>("datetime");
    QTest::addColumn<QString>("expected");

    const QString wednesdayJanuary = QLocale::c().dayName(3, QLocale::ShortFormat)
        + ' ' + QLocale::c().monthName(1, QLocale::ShortFormat);

    // ### Qt 7 GMT: change to UTC - see matching QDateTime::fromString() comment
    QTest::newRow("localtime")  << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::LocalTime)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013");
    QTest::newRow("utc")        << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::UTC)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013 GMT");
    QTest::newRow("offset+")    << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::OffsetFromUTC,
                                             10 * 60 * 60)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013 GMT+1000");
    QTest::newRow("offset-")    << QDateTime(QDate(2013, 1, 2), QTime(1, 2, 3), Qt::OffsetFromUTC,
                                             -10 * 60 * 60)
                                << wednesdayJanuary + QString(" 2 01:02:03 2013 GMT-1000");
    QTest::newRow("invalid")    << QDateTime()
                                << QString("");
}

void tst_QDateTime::toString_textDate()
{
    QFETCH(QDateTime, datetime);
    QFETCH(QString, expected);

    QString result = datetime.toString(Qt::TextDate);
    QCOMPARE(result, expected);

#if QT_CONFIG(datetimeparser)
    QDateTime resultDatetime = QDateTime::fromString(result, Qt::TextDate);
    QCOMPARE(resultDatetime, datetime);
    QCOMPARE(resultDatetime.date(), datetime.date());
    QCOMPARE(resultDatetime.time(), datetime.time());
    QCOMPARE(resultDatetime.timeSpec(), datetime.timeSpec());
    QCOMPARE(resultDatetime.offsetFromUtc(), datetime.offsetFromUtc());
#endif
}

void tst_QDateTime::toString_textDate_extra()
{
    // ### Qt 7 GMT: change to UTC - see matching QDateTime::fromString() comment
    auto endsWithGmt = [](const QDateTime &dt) {
        return dt.toString().endsWith(QLatin1String("GMT"));
    };
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(0, Qt::LocalTime);
    QVERIFY(!endsWithGmt(dt));
    dt = QDateTime::fromMSecsSinceEpoch(0, Qt::UTC).toLocalTime();
    QVERIFY(!endsWithGmt(dt));

#if QT_CONFIG(timezone)
    if (QTimeZone::systemTimeZone().offsetFromUtc(dt))
        QVERIFY(dt.toString() != QLatin1String("Thu Jan 1 00:00:00 1970"));
    else
        QCOMPARE(dt.toString(), QLatin1String("Thu Jan 1 00:00:00 1970"));

    QTimeZone PST("America/Vancouver");
    if (PST.isValid()) {
        dt = QDateTime::fromMSecsSinceEpoch(0, PST);
        QCOMPARE(dt.toString(), QLatin1String("Wed Dec 31 16:00:00 1969 UTC-08:00"));
        dt = dt.toLocalTime();
        QVERIFY(!endsWithGmt(dt));
    } else {
        qDebug("Missed zone test: no America/Vancouver zone available");
    }
    QTimeZone CET("Europe/Berlin");
    if (CET.isValid()) {
        dt = QDateTime::fromMSecsSinceEpoch(0, CET);
        QCOMPARE(dt.toString(), QLatin1String("Thu Jan 1 01:00:00 1970 UTC+01:00"));
        dt = dt.toLocalTime();
        QVERIFY(!endsWithGmt(dt));
    } else {
        qDebug("Missed zone test: no Europe/Berlin zone available");
    }
#else // timezone
    if (dt.offsetFromUtc())
        QVERIFY(dt.toString() != QLatin1String("Thu Jan 1 00:00:00 1970"));
    else
        QCOMPARE(dt.toString(), QLatin1String("Thu Jan 1 00:00:00 1970"));
#endif
    dt = QDateTime::fromMSecsSinceEpoch(0, Qt::UTC);
    QVERIFY(endsWithGmt(dt));
}

void tst_QDateTime::toString_rfcDate_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<QString>("formatted");

    if (zoneIsCET) {
        QTest::newRow("localtime")
                << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34))
                << QString("09 Nov 1978 13:28:34 +0100");
    }
    QTest::newRow("UTC")
            << QDateTime(QDate(1978, 11, 9), QTime(13, 28, 34), Qt::UTC)
            << QString("09 Nov 1978 13:28:34 +0000");
    QDateTime dt(QDate(1978, 11, 9), QTime(13, 28, 34));
    dt.setOffsetFromUtc(19800);
    QTest::newRow("positive OffsetFromUTC")
            << dt
            << QString("09 Nov 1978 13:28:34 +0530");
    dt.setOffsetFromUtc(-7200);
    QTest::newRow("negative OffsetFromUTC")
            << dt
            << QString("09 Nov 1978 13:28:34 -0200");
    QTest::newRow("invalid")
            << QDateTime(QDate(1978, 13, 9), QTime(13, 28, 34), Qt::UTC)
            << QString();
    QTest::newRow("999 milliseconds UTC")
            << QDateTime(QDate(2000, 1, 1), QTime(13, 28, 34, 999), Qt::UTC)
            << QString("01 Jan 2000 13:28:34 +0000");
}

void tst_QDateTime::toString_rfcDate()
{
    QFETCH(QDateTime, dt);
    QFETCH(QString, formatted);

    // Set to non-English locale to confirm still uses English
    QLocale oldLocale;
    QLocale::setDefault(QLocale("de_DE"));
    QString actual(dt.toString(Qt::RFC2822Date));
    QLocale::setDefault(oldLocale);
    QCOMPARE(actual, formatted);
}

void tst_QDateTime::toString_enumformat()
{
    QDateTime dt1(QDate(1995, 5, 20), QTime(12, 34, 56));

    QString str1 = dt1.toString(Qt::TextDate);
    QVERIFY(!str1.isEmpty()); // It's locale-dependent everywhere

    QString str2 = dt1.toString(Qt::ISODate);
    QCOMPARE(str2, QString("1995-05-20T12:34:56"));
}

void tst_QDateTime::toString_strformat()
{
    // Most tests are in QLocale, just test that the api works.
    QDate testDate(2013, 1, 1);
    QTime testTime(1, 2, 3);
    QDateTime testDateTime(testDate, testTime, Qt::UTC);
    QCOMPARE(testDate.toString("yyyy-MM-dd"), QString("2013-01-01"));
    QCOMPARE(testTime.toString("hh:mm:ss"), QString("01:02:03"));
    QCOMPARE(testDateTime.toString("yyyy-MM-dd hh:mm:ss t"), QString("2013-01-01 01:02:03 UTC"));
}
#endif // datestring

void tst_QDateTime::addDays()
{
    for (int pass = 0; pass < 2; ++pass) {
        QDateTime dt(QDate(2004, 1, 1), QTime(12, 34, 56), pass == 0 ? Qt::LocalTime : Qt::UTC);
        dt = dt.addDays(185);
        QVERIFY(dt.date().year() == 2004 && dt.date().month() == 7 && dt.date().day() == 4);
        QVERIFY(dt.time().hour() == 12 && dt.time().minute() == 34 && dt.time().second() == 56
               && dt.time().msec() == 0);
        QCOMPARE(dt.timeSpec(), (pass == 0 ? Qt::LocalTime : Qt::UTC));

        dt = dt.addDays(-185);
        QCOMPARE(dt.date(), QDate(2004, 1, 1));
        QCOMPARE(dt.time(), QTime(12, 34, 56));
    }

    QDateTime dt(QDate(1752, 9, 14), QTime(12, 34, 56));
    while (dt.date().year() < 8000) {
        int year = dt.date().year();
        if (QDate::isLeapYear(year + 1))
            dt = dt.addDays(366);
        else
            dt = dt.addDays(365);
        QCOMPARE(dt.date(), QDate(year + 1, 9, 14));
        QCOMPARE(dt.time(), QTime(12, 34, 56));
    }

    // Test preserves TimeSpec
    QDateTime dt1(QDate(2013, 1, 1), QTime(0, 0), Qt::UTC);
    QDateTime dt2 = dt1.addDays(2);
    QCOMPARE(dt2.date(), QDate(2013, 1, 3));
    QCOMPARE(dt2.time(), QTime(0, 0));
    QCOMPARE(dt2.timeSpec(), Qt::UTC);

    dt1 = QDateTime(QDate(2013, 1, 1), QTime(0, 0), Qt::LocalTime);
    dt2 = dt1.addDays(2);
    QCOMPARE(dt2.date(), QDate(2013, 1, 3));
    QCOMPARE(dt2.time(), QTime(0, 0));
    QCOMPARE(dt2.timeSpec(), Qt::LocalTime);

    dt1 = QDateTime(QDate(2013, 1, 1), QTime(0, 0), Qt::OffsetFromUTC, 60*60);
    dt2 = dt1.addDays(2);
    QCOMPARE(dt2.date(), QDate(2013, 1, 3));
    QCOMPARE(dt2.time(), QTime(0, 0));
    QCOMPARE(dt2.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt2.offsetFromUtc(), 60 * 60);

#if QT_CONFIG(timezone)
    const QTimeZone cet("Europe/Oslo");
    if (cet.isValid()) {
        dt1 = QDate(2022, 1, 10).startOfDay(cet);
        dt2 = dt1.addDays(2); // QTBUG-99668: should not assert
        QCOMPARE(dt2.date(), QDate(2022, 1, 12));
        QCOMPARE(dt2.time(), QTime(0, 0));
        QCOMPARE(dt2.timeSpec(), Qt::TimeZone);
        QCOMPARE(dt2.timeZone(), cet);
    }
#endif

    // test last second of 1969 *is* valid (despite being time_t(-1))
    dt1 = QDateTime(QDate(1970, 1, 1), QTime(23, 59, 59));
    dt2 = dt1.addDays(-1);
    QVERIFY(dt2.isValid());
}

void tst_QDateTime::addInvalid()
{
    QDateTime bad;
    QVERIFY(!bad.isValid());
    QVERIFY(bad.isNull());

    QDateTime offset = bad.addDays(2);
    QVERIFY(offset.isNull());
    offset = bad.addMonths(-1);
    QVERIFY(offset.isNull());
    offset = bad.addYears(23);
    QVERIFY(offset.isNull());
    offset = bad.addSecs(73);
    QVERIFY(offset.isNull());
    offset = bad.addMSecs(73);
    QVERIFY(offset.isNull());

    QDateTime bound = QDateTime::fromMSecsSinceEpoch(std::numeric_limits<qint64>::min(), Qt::UTC);
    QVERIFY(bound.isValid());
    offset = bound.addMSecs(-1);
    QVERIFY(!offset.isValid());
    offset = bound.addSecs(-1);
    QVERIFY(!offset.isValid());
    offset = bound.addDays(-1);
    QVERIFY(!offset.isValid());
    offset = bound.addMonths(-1);
    QVERIFY(!offset.isValid());
    offset = bound.addYears(-1);
    QVERIFY(!offset.isValid());

    bound.setMSecsSinceEpoch(std::numeric_limits<qint64>::max());
    QVERIFY(bound.isValid());
    offset = bound.addMSecs(1);
    QVERIFY(!offset.isValid());
    offset = bound.addSecs(1);
    QVERIFY(!offset.isValid());
    offset = bound.addDays(1);
    QVERIFY(!offset.isValid());
    offset = bound.addMonths(1);
    QVERIFY(!offset.isValid());
    offset = bound.addYears(1);
    QVERIFY(!offset.isValid());
}

void tst_QDateTime::addMonths_data()
{
    QTest::addColumn<int>("months");
    QTest::addColumn<QDate>("resultDate");

    QTest::newRow("-15") << -15 << QDate(2002, 10, 31);
    QTest::newRow("-14") << -14 << QDate(2002, 11, 30);
    QTest::newRow("-13") << -13 << QDate(2002, 12, 31);
    QTest::newRow("-12") << -12 << QDate(2003, 1, 31);

    QTest::newRow("-11") << -11 << QDate(2003, 2, 28);
    QTest::newRow("-10") << -10 << QDate(2003, 3, 31);
    QTest::newRow("-9") << -9 << QDate(2003, 4, 30);
    QTest::newRow("-8") << -8 << QDate(2003, 5, 31);
    QTest::newRow("-7") << -7 << QDate(2003, 6, 30);
    QTest::newRow("-6") << -6 << QDate(2003, 7, 31);
    QTest::newRow("-5") << -5 << QDate(2003, 8, 31);
    QTest::newRow("-4") << -4 << QDate(2003, 9, 30);
    QTest::newRow("-3") << -3 << QDate(2003, 10, 31);
    QTest::newRow("-2") << -2 << QDate(2003, 11, 30);
    QTest::newRow("-1") << -1 << QDate(2003, 12, 31);
    QTest::newRow("0") << 0 << QDate(2004, 1, 31);
    QTest::newRow("1") << 1 << QDate(2004, 2, 29);
    QTest::newRow("2") << 2 << QDate(2004, 3, 31);
    QTest::newRow("3") << 3 << QDate(2004, 4, 30);
    QTest::newRow("4") << 4 << QDate(2004, 5, 31);
    QTest::newRow("5") << 5 << QDate(2004, 6, 30);
    QTest::newRow("6") << 6 << QDate(2004, 7, 31);
    QTest::newRow("7") << 7 << QDate(2004, 8, 31);
    QTest::newRow("8") << 8 << QDate(2004, 9, 30);
    QTest::newRow("9") << 9 << QDate(2004, 10, 31);
    QTest::newRow("10") << 10 << QDate(2004, 11, 30);
    QTest::newRow("11") << 11 << QDate(2004, 12, 31);
    QTest::newRow("12") << 12 << QDate(2005, 1, 31);
    QTest::newRow("13") << 13 << QDate(2005, 2, 28);
    QTest::newRow("14") << 14 << QDate(2005, 3, 31);
    QTest::newRow("15") << 15 << QDate(2005, 4, 30);
}

void tst_QDateTime::addMonths()
{
    QFETCH(int, months);
    QFETCH(QDate, resultDate);

    QDate testDate(2004, 1, 31);
    QTime testTime(12, 34, 56);
    QDateTime start(testDate, testTime);
    QDateTime end = start.addMonths(months);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::LocalTime);

    start = QDateTime(testDate, testTime, Qt::UTC);
    end = start.addMonths(months);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::UTC);

    start = QDateTime(testDate, testTime, Qt::OffsetFromUTC, 60 * 60);
    end = start.addMonths(months);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(end.offsetFromUtc(), 60 * 60);
}

void tst_QDateTime::addYears_data()
{
    QTest::addColumn<int>("years1");
    QTest::addColumn<int>("years2");
    QTest::addColumn<QDate>("startDate");
    QTest::addColumn<QDate>("resultDate");

    QTest::newRow("0") << 0 << 0 << QDate(1752, 9, 14) << QDate(1752, 9, 14);
    QTest::newRow("4000 - 4000") << 4000 << -4000 << QDate(1752, 9, 14) << QDate(1752, 9, 14);
    QTest::newRow("10") << 10 << 0 << QDate(1752, 9, 14) << QDate(1762, 9, 14);
    QTest::newRow("0 leap year") << 0 << 0 << QDate(1760, 2, 29) << QDate(1760, 2, 29);
    QTest::newRow("1 leap year") << 1 << 0 << QDate(1760, 2, 29) << QDate(1761, 2, 28);
    QTest::newRow("2 leap year") << 2 << 0 << QDate(1760, 2, 29) << QDate(1762, 2, 28);
    QTest::newRow("3 leap year") << 3 << 0 << QDate(1760, 2, 29) << QDate(1763, 2, 28);
    QTest::newRow("4 leap year") << 4 << 0 << QDate(1760, 2, 29) << QDate(1764, 2, 29);

    QTest::newRow("toNegative1") << -2000 << 0 << QDate(1752, 9, 14) << QDate(-249, 9, 14);
    QTest::newRow("toNegative2") << -1752 << 0 << QDate(1752, 9, 14) << QDate(-1, 9, 14);
    QTest::newRow("toNegative3") << -1751 << 0 << QDate(1752, 9, 14) << QDate(1, 9, 14);
    QTest::newRow("toPositive1") << 2000 << 0 << QDate(-1752, 9, 14) << QDate(249, 9, 14);
    QTest::newRow("toPositive2") << 1752 << 0 << QDate(-1752, 9, 14) << QDate(1, 9, 14);
    QTest::newRow("toPositive3") << 1751 << 0 << QDate(-1752, 9, 14) << QDate(-1, 9, 14);
}

void tst_QDateTime::addYears()
{
    QFETCH(int, years1);
    QFETCH(int, years2);
    QFETCH(QDate, startDate);
    QFETCH(QDate, resultDate);

    QTime testTime(14, 25, 36);
    QDateTime start(startDate, testTime);
    QDateTime end = start.addYears(years1).addYears(years2);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::LocalTime);

    start = QDateTime(startDate, testTime, Qt::UTC);
    end = start.addYears(years1).addYears(years2);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::UTC);

    start = QDateTime(startDate, testTime, Qt::OffsetFromUTC, 60 * 60);
    end = start.addYears(years1).addYears(years2);
    QCOMPARE(end.date(), resultDate);
    QCOMPARE(end.time(), testTime);
    QCOMPARE(end.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(end.offsetFromUtc(), 60 * 60);
}

void tst_QDateTime::addMSecs_data()
{
    QTest::addColumn<QDateTime>("dt");
    QTest::addColumn<qint64>("nsecs");
    QTest::addColumn<QDateTime>("result");

    const QTime standardTime(12, 34, 56);
    const QTime daylightTime(13, 34, 56);
    const qint64 daySecs(86400);

    QTest::newRow("utc0")
        << QDateTime(QDate(2004, 1, 1), standardTime, Qt::UTC) << daySecs
        << QDateTime(QDate(2004, 1, 2), standardTime, Qt::UTC);
    QTest::newRow("utc1")
        << QDateTime(QDate(2004, 1, 1), standardTime, Qt::UTC) << (daySecs * 185)
        << QDateTime(QDate(2004, 7, 4), standardTime, Qt::UTC);
    QTest::newRow("utc2")
        << QDateTime(QDate(2004, 1, 1), standardTime, Qt::UTC) << (daySecs * 366)
        << QDateTime(QDate(2005, 1, 1), standardTime, Qt::UTC);
    QTest::newRow("utc3")
        << QDateTime(QDate(1760, 1, 1), standardTime, Qt::UTC) << daySecs
        << QDateTime(QDate(1760, 1, 2), standardTime, Qt::UTC);
    QTest::newRow("utc4")
        << QDateTime(QDate(1760, 1, 1), standardTime, Qt::UTC) << (daySecs * 185)
        << QDateTime(QDate(1760, 7, 4), standardTime, Qt::UTC);
    QTest::newRow("utc5")
        << QDateTime(QDate(1760, 1, 1), standardTime, Qt::UTC) << (daySecs * 366)
        << QDateTime(QDate(1761, 1, 1), standardTime, Qt::UTC);
    QTest::newRow("utc6")
        << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << daySecs
        << QDateTime(QDate(4000, 1, 2), standardTime, Qt::UTC);
    QTest::newRow("utc7")
        << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << (daySecs * 185)
        << QDateTime(QDate(4000, 7, 4), standardTime, Qt::UTC);
    QTest::newRow("utc8")
        << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << (daySecs * 366)
        << QDateTime(QDate(4001, 1, 1), standardTime, Qt::UTC);
    QTest::newRow("utc9")
        << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC) << qint64(0)
        << QDateTime(QDate(4000, 1, 1), standardTime, Qt::UTC);

    if (zoneIsCET) {
        QTest::newRow("cet0")
            << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << daySecs
            << QDateTime(QDate(2004, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet1")
            << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << (daySecs * 185)
            << QDateTime(QDate(2004, 7, 4), daylightTime, Qt::LocalTime);
        QTest::newRow("cet2")
            << QDateTime(QDate(2004, 1, 1), standardTime, Qt::LocalTime) << (daySecs * 366)
            << QDateTime(QDate(2005, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet3")
            << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << daySecs
            << QDateTime(QDate(1760, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet4")
            << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << (daySecs * 185)
            << QDateTime(QDate(1760, 7, 4), standardTime, Qt::LocalTime);
        QTest::newRow("cet5")
            << QDateTime(QDate(1760, 1, 1), standardTime, Qt::LocalTime) << (daySecs * 366)
            << QDateTime(QDate(1761, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet6")
            << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << daySecs
            << QDateTime(QDate(4000, 1, 2), standardTime, Qt::LocalTime);
        QTest::newRow("cet7")
            << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << (daySecs * 185)
            << QDateTime(QDate(4000, 7, 4), daylightTime, Qt::LocalTime);
        QTest::newRow("cet8")
            << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << (daySecs * 366)
            << QDateTime(QDate(4001, 1, 1), standardTime, Qt::LocalTime);
        QTest::newRow("cet9")
            << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime) << qint64(0)
            << QDateTime(QDate(4000, 1, 1), standardTime, Qt::LocalTime);
    }

    // Year sign change
    QTest::newRow("toNegative")
        << QDateTime(QDate(1, 1, 1), QTime(0, 0), Qt::UTC) << qint64(-1)
        << QDateTime(QDate(-1, 12, 31), QTime(23, 59, 59), Qt::UTC);
    QTest::newRow("toPositive")
        << QDateTime(QDate(-1, 12, 31), QTime(23, 59, 59), Qt::UTC) << qint64(1)
                                << QDateTime(QDate(1, 1, 1), QTime(0, 0), Qt::UTC);

    QTest::newRow("invalid") << QDateTime() << qint64(1) << QDateTime();

    // Check Offset details are preserved
    QTest::newRow("offset0")
        << QDateTime(QDate(2013, 1, 1), QTime(1, 2, 3), Qt::OffsetFromUTC, 60 * 60)
        << qint64(60 * 60)
        << QDateTime(QDate(2013, 1, 1), QTime(2, 2, 3), Qt::OffsetFromUTC, 60 * 60);
    // Check last second of 1969
    QTest::newRow("epoch-1s-utc")
        << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::UTC) << qint64(-1)
        << QDateTime(QDate(1969, 12, 31), QTime(23, 59, 59), Qt::UTC);
    QTest::newRow("epoch-1s-local")
        << QDateTime(QDate(1970, 1, 1), QTime(0, 0)) << qint64(-1)
        << QDateTime(QDate(1969, 12, 31), QTime(23, 59, 59));

    // Overflow and Underflow
    const qint64 maxSeconds = std::numeric_limits<qint64>::max() / 1000;
    QTest::newRow("after-last")
        << QDateTime::fromSecsSinceEpoch(maxSeconds, Qt::UTC) << qint64(1) << QDateTime();
    QTest::newRow("to-last")
        << QDateTime::fromSecsSinceEpoch(maxSeconds - 1, Qt::UTC) << qint64(1)
        << QDateTime::fromSecsSinceEpoch(maxSeconds, Qt::UTC);
    QTest::newRow("before-first")
        << QDateTime::fromSecsSinceEpoch(-maxSeconds, Qt::UTC) << qint64(-1) << QDateTime();
    QTest::newRow("to-first")
        << QDateTime::fromSecsSinceEpoch(1 - maxSeconds, Qt::UTC) << qint64(-1)
        << QDateTime::fromSecsSinceEpoch(-maxSeconds, Qt::UTC);
}

void tst_QDateTime::addSecs_data()
{
    addMSecs_data();

    const qint64 maxSeconds = std::numeric_limits<qint64>::max() / 1000;
    // Results would be representable, but the step isn't
    QTest::newRow("leap-up")
        << QDateTime::fromSecsSinceEpoch(-1, Qt::UTC) << 1 + maxSeconds << QDateTime();
    QTest::newRow("leap-down")
        << QDateTime::fromSecsSinceEpoch(1, Qt::UTC) << -1 - maxSeconds << QDateTime();
}

void tst_QDateTime::addSecs()
{
    QFETCH(const QDateTime, dt);
    QFETCH(const qint64, nsecs);
    QFETCH(const QDateTime, result);
    QDateTime test = dt.addSecs(nsecs);
    if (!result.isValid()) {
        QVERIFY(!test.isValid());
    } else {
        QCOMPARE(test, result);
        QCOMPARE(test.timeSpec(), dt.timeSpec());
        if (test.timeSpec() == Qt::OffsetFromUTC)
            QCOMPARE(test.offsetFromUtc(), dt.offsetFromUtc());
        QCOMPARE(result.addSecs(-nsecs), dt);
    }
}

void tst_QDateTime::addMSecs()
{
    QFETCH(const QDateTime, dt);
    QFETCH(const qint64, nsecs);
    QFETCH(const QDateTime, result);

    QDateTime test = dt.addMSecs(qint64(nsecs) * 1000);
    if (!result.isValid()) {
        QVERIFY(!test.isValid());
    } else {
        QCOMPARE(test, result);
        QCOMPARE(test.timeSpec(), dt.timeSpec());
        if (test.timeSpec() == Qt::OffsetFromUTC)
            QCOMPARE(test.offsetFromUtc(), dt.offsetFromUtc());
        QCOMPARE(result.addMSecs(qint64(-nsecs) * 1000), dt);
    }
}

void tst_QDateTime::toTimeSpec_data()
{
    QTest::addColumn<QDateTime>("fromUtc");
    QTest::addColumn<QDateTime>("fromLocal");

    QTime utcTime(4, 20, 30);
    QTime localStandardTime(5, 20, 30);
    QTime localDaylightTime(6, 20, 30);

    QTest::newRow("winter1") << QDateTime(QDate(2004, 1, 1), utcTime, Qt::UTC)
                          << QDateTime(QDate(2004, 1, 1), localStandardTime, Qt::LocalTime);
    QTest::newRow("winter2") << QDateTime(QDate(2004, 2, 29), utcTime, Qt::UTC)
                          << QDateTime(QDate(2004, 2, 29), localStandardTime, Qt::LocalTime);
    QTest::newRow("winter3")
        << QDateTime(QDate(1760, 2, 29), utcTime, Qt::UTC)
        << QDateTime(QDate(1760, 2, 29), localStandardTime.addSecs(preZoneFix));
    QTest::newRow("winter4") << QDateTime(QDate(6000, 2, 29), utcTime, Qt::UTC)
                          << QDateTime(QDate(6000, 2, 29), localStandardTime, Qt::LocalTime);

    // Test mktime boundaries (1970 - 2038) and adjustDate().
    QTest::newRow("1969/12/31 23:00 UTC")
        << QDateTime(QDate(1969, 12, 31), QTime(23, 0), Qt::UTC)
        << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("2037/12/31 23:00 UTC")
        << QDateTime(QDate(2037, 12, 31), QTime(23, 0), Qt::UTC)
        << QDateTime(QDate(2038, 1, 1), QTime(0, 0), Qt::LocalTime);

    QTest::newRow("-271821/4/20 00:00 UTC (JavaScript min date, start of day)")
        << QDateTime(QDate(-271821, 4, 20), QTime(0, 0), Qt::UTC)
        << QDateTime(QDate(-271821, 4, 20), QTime(1, 0)).addSecs(preZoneFix);
    QTest::newRow("-271821/4/20 23:00 UTC (JavaScript min date, end of day)")
        << QDateTime(QDate(-271821, 4, 20), QTime(23, 0), Qt::UTC)
        << QDateTime(QDate(-271821, 4, 21), QTime(0, 0)).addSecs(preZoneFix);

    if (zoneIsCET) {
        QTest::newRow("summer1") << QDateTime(QDate(2004, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(2004, 6, 30), localDaylightTime, Qt::LocalTime);
        QTest::newRow("summer2")
            << QDateTime(QDate(1760, 6, 30), utcTime, Qt::UTC)
            << QDateTime(QDate(1760, 6, 30), localStandardTime.addSecs(preZoneFix));
        QTest::newRow("summer3") << QDateTime(QDate(4000, 6, 30), utcTime, Qt::UTC)
                                 << QDateTime(QDate(4000, 6, 30), localDaylightTime, Qt::LocalTime);

        QTest::newRow("275760/9/23 00:00 UTC (JavaScript max date, start of day)")
            << QDateTime(QDate(275760, 9, 23), QTime(0, 0), Qt::UTC)
            << QDateTime(QDate(275760, 9, 23), QTime(2, 0), Qt::LocalTime);

        QTest::newRow("275760/9/23 22:00 UTC (JavaScript max date, end of day)")
            << QDateTime(QDate(275760, 9, 23), QTime(22, 0), Qt::UTC)
            << QDateTime(QDate(275760, 9, 24), QTime(0, 0), Qt::LocalTime);
    }

    QTest::newRow("msec") << QDateTime(QDate(4000, 6, 30), utcTime.addMSecs(1), Qt::UTC)
                       << QDateTime(QDate(4000, 6, 30), localDaylightTime.addMSecs(1), Qt::LocalTime);
}

void tst_QDateTime::toTimeSpec()
{
    if (zoneIsCET) {
        QFETCH(QDateTime, fromUtc);
        QFETCH(QDateTime, fromLocal);

        QDateTime utcToUtc = fromUtc.toTimeSpec(Qt::UTC);
        QDateTime localToLocal = fromLocal.toTimeSpec(Qt::LocalTime);
        QDateTime utcToLocal = fromUtc.toTimeSpec(Qt::LocalTime);
        QDateTime localToUtc = fromLocal.toTimeSpec(Qt::UTC);
        QDateTime utcToOffset = fromUtc.toTimeSpec(Qt::OffsetFromUTC);
        QDateTime localToOffset = fromLocal.toTimeSpec(Qt::OffsetFromUTC);

        QCOMPARE(utcToUtc, fromUtc);
        QCOMPARE(utcToUtc.date(), fromUtc.date());
        QCOMPARE(utcToUtc.time(), fromUtc.time());
        QCOMPARE(utcToUtc.timeSpec(), Qt::UTC);

        QCOMPARE(localToLocal, fromLocal);
        QCOMPARE(localToLocal.date(), fromLocal.date());
        QCOMPARE(localToLocal.time(), fromLocal.time());
        QCOMPARE(localToLocal.timeSpec(), Qt::LocalTime);

        QCOMPARE(utcToLocal, fromLocal);
        QCOMPARE(utcToLocal.date(), fromLocal.date());
        QCOMPARE(utcToLocal.time(), fromLocal.time());
        QCOMPARE(utcToLocal.timeSpec(), Qt::LocalTime);
        QCOMPARE(utcToLocal.toTimeSpec(Qt::UTC), fromUtc);

        QCOMPARE(localToUtc, fromUtc);
        QCOMPARE(localToUtc.date(), fromUtc.date());
        QCOMPARE(localToUtc.time(), fromUtc.time());
        QCOMPARE(localToUtc.timeSpec(), Qt::UTC);
        QCOMPARE(localToUtc.toTimeSpec(Qt::LocalTime), fromLocal);

        QCOMPARE(utcToUtc, localToUtc);
        QCOMPARE(utcToUtc.date(), localToUtc.date());
        QCOMPARE(utcToUtc.time(), localToUtc.time());
        QCOMPARE(utcToUtc.timeSpec(), Qt::UTC);

        QCOMPARE(utcToLocal, localToLocal);
        QCOMPARE(utcToLocal.date(), localToLocal.date());
        QCOMPARE(utcToLocal.time(), localToLocal.time());
        QCOMPARE(utcToLocal.timeSpec(), Qt::LocalTime);

        // OffsetToUTC becomes UTC
        QCOMPARE(utcToOffset, fromUtc);
        QCOMPARE(utcToOffset.date(), fromUtc.date());
        QCOMPARE(utcToOffset.time(), fromUtc.time());
        QCOMPARE(utcToOffset.timeSpec(), Qt::UTC);
        QCOMPARE(utcToOffset.toTimeSpec(Qt::UTC), fromUtc);

        QCOMPARE(localToOffset, fromUtc);
        QCOMPARE(localToOffset.date(), fromUtc.date());
        QCOMPARE(localToOffset.time(), fromUtc.time());
        QCOMPARE(localToOffset.timeSpec(), Qt::UTC);
        QCOMPARE(localToOffset.toTimeSpec(Qt::LocalTime), fromLocal);
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");
    }
}

void tst_QDateTime::toLocalTime_data()
{
    toTimeSpec_data();
}

void tst_QDateTime::toLocalTime()
{
    if (zoneIsCET) {
        QFETCH(QDateTime, fromUtc);
        QFETCH(QDateTime, fromLocal);

        QCOMPARE(fromLocal.toLocalTime(), fromLocal);
        QCOMPARE(fromUtc.toLocalTime(), fromLocal);
        QCOMPARE(fromUtc.toLocalTime(), fromLocal.toLocalTime());
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");
    }
}

void tst_QDateTime::toUTC_data()
{
    toTimeSpec_data();
}

void tst_QDateTime::toUTC()
{
    if (zoneIsCET) {
        QFETCH(QDateTime, fromUtc);
        QFETCH(QDateTime, fromLocal);

        QCOMPARE(fromUtc.toUTC(), fromUtc);
        QCOMPARE(fromLocal.toUTC(), fromUtc);
        QCOMPARE(fromUtc.toUTC(), fromLocal.toUTC());
    } else {
        QSKIP("Not tested with timezone other than Central European (CET/CEST)");
    }

    QDateTime dt = QDateTime::currentDateTime();
    if(dt.time().msec() == 0){
        dt.setTime(dt.time().addMSecs(1));
    }
    QString s = dt.toString("zzz");
    QString t = dt.toUTC().toString("zzz");
    QCOMPARE(s, t);
}

void tst_QDateTime::daysTo()
{
    QDateTime dt1(QDate(1760, 1, 2).startOfDay());
    QDateTime dt2(QDate(1760, 2, 2).startOfDay());
    QDateTime dt3(QDate(1760, 3, 2).startOfDay());

    QCOMPARE(dt1.daysTo(dt2), (qint64) 31);
    QCOMPARE(dt1.addDays(31), dt2);

    QCOMPARE(dt2.daysTo(dt3), (qint64) 29);
    QCOMPARE(dt2.addDays(29), dt3);

    QCOMPARE(dt1.daysTo(dt3), (qint64) 60);
    QCOMPARE(dt1.addDays(60), dt3);

    QCOMPARE(dt2.daysTo(dt1), (qint64) -31);
    QCOMPARE(dt2.addDays(-31), dt1);

    QCOMPARE(dt3.daysTo(dt2), (qint64) -29);
    QCOMPARE(dt3.addDays(-29), dt2);

    QCOMPARE(dt3.daysTo(dt1), (qint64) -60);
    QCOMPARE(dt3.addDays(-60), dt1);
}

void tst_QDateTime::secsTo_data()
{
    addSecs_data();

    QTest::newRow("disregard milliseconds #1")
        << QDateTime(QDate(2012, 3, 7), QTime(0, 58, 0, 0)) << qint64(60)
        << QDateTime(QDate(2012, 3, 7), QTime(0, 59, 0, 400));

    QTest::newRow("disregard milliseconds #2")
        << QDateTime(QDate(2012, 3, 7), QTime(0, 59, 0, 0)) << qint64(60)
        << QDateTime(QDate(2012, 3, 7), QTime(1, 0, 0, 400));
}

void tst_QDateTime::secsTo()
{
    QFETCH(const QDateTime, dt);
    QFETCH(const qint64, nsecs);
    QFETCH(const QDateTime, result);

    if (result.isValid()) {
        QCOMPARE(dt.secsTo(result), (qint64)nsecs);
        QCOMPARE(result.secsTo(dt), (qint64)-nsecs);
        QVERIFY((dt == result) == (0 == nsecs));
        QVERIFY((dt != result) == (0 != nsecs));
        QVERIFY((dt < result) == (0 < nsecs));
        QVERIFY((dt <= result) == (0 <= nsecs));
        QVERIFY((dt > result) == (0 > nsecs));
        QVERIFY((dt >= result) == (0 >= nsecs));
    } else {
        QVERIFY(dt.secsTo(result) == 0);
        QVERIFY(result.secsTo(dt) == 0);
    }
}

void tst_QDateTime::msecsTo_data()
{
    addMSecs_data();
}

void tst_QDateTime::msecsTo()
{
    QFETCH(const QDateTime, dt);
    QFETCH(const qint64, nsecs);
    QFETCH(const QDateTime, result);

    if (result.isValid()) {
        QCOMPARE(dt.msecsTo(result), qint64(nsecs) * 1000);
        QCOMPARE(result.msecsTo(dt), -qint64(nsecs) * 1000);
        QVERIFY((dt == result) == (0 == (qint64(nsecs) * 1000)));
        QVERIFY((dt != result) == (0 != (qint64(nsecs) * 1000)));
        QVERIFY((dt < result) == (0 < (qint64(nsecs) * 1000)));
        QVERIFY((dt <= result) == (0 <= (qint64(nsecs) * 1000)));
        QVERIFY((dt > result) == (0 > (qint64(nsecs) * 1000)));
        QVERIFY((dt >= result) == (0 >= (qint64(nsecs) * 1000)));
    } else {
        QVERIFY(dt.msecsTo(result) == 0);
        QVERIFY(result.msecsTo(dt) == 0);
    }
}

void tst_QDateTime::currentDateTime()
{
    time_t buf1, buf2;
    ::time(&buf1);
    QDateTime lowerBound;
    lowerBound.setSecsSinceEpoch(buf1);

    QDateTime dt1 = QDateTime::currentDateTime();
    QDateTime dt2 = QDateTime::currentDateTime().toLocalTime();
    QDateTime dt3 = QDateTime::currentDateTime().toUTC();

    ::time(&buf2);

    QDateTime upperBound;
    upperBound.setSecsSinceEpoch(buf2);
    // Note we must add 2 seconds here because time() may return up to
    // 1 second difference from the more accurate method used by QDateTime::currentDateTime()
    upperBound = upperBound.addSecs(2);

    QString details = QString("\n"
        "lowerBound: %1\n"
        "dt1:        %2\n"
        "dt2:        %3\n"
        "dt3:        %4\n"
        "upperBound: %5\n")
        .arg(lowerBound.toSecsSinceEpoch())
        .arg(dt1.toSecsSinceEpoch())
        .arg(dt2.toSecsSinceEpoch())
        .arg(dt3.toSecsSinceEpoch())
        .arg(upperBound.toSecsSinceEpoch());

    QVERIFY2(lowerBound < upperBound, qPrintable(details));

    QVERIFY2(lowerBound <= dt1, qPrintable(details));
    QVERIFY2(dt1 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt2, qPrintable(details));
    QVERIFY2(dt2 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt3, qPrintable(details));
    QVERIFY2(dt3 < upperBound, qPrintable(details));

    QVERIFY(dt1.timeSpec() == Qt::LocalTime);
    QVERIFY(dt2.timeSpec() == Qt::LocalTime);
    QVERIFY(dt3.timeSpec() == Qt::UTC);
}

void tst_QDateTime::currentDateTimeUtc()
{
    time_t buf1, buf2;
    ::time(&buf1);

    QDateTime lowerBound;
    lowerBound.setSecsSinceEpoch(buf1);

    QDateTime dt1 = QDateTime::currentDateTimeUtc();
    QDateTime dt2 = QDateTime::currentDateTimeUtc().toLocalTime();
    QDateTime dt3 = QDateTime::currentDateTimeUtc().toUTC();

    ::time(&buf2);

    QDateTime upperBound;
    upperBound.setSecsSinceEpoch(buf2);
    // Note we must add 2 seconds here because time() may return up to
    // 1 second difference from the more accurate method used by QDateTime::currentDateTime()
    upperBound = upperBound.addSecs(2);

    QString details = QString("\n"
        "lowerBound: %1\n"
        "dt1:        %2\n"
        "dt2:        %3\n"
        "dt3:        %4\n"
        "upperBound: %5\n")
        .arg(lowerBound.toSecsSinceEpoch())
        .arg(dt1.toSecsSinceEpoch())
        .arg(dt2.toSecsSinceEpoch())
        .arg(dt3.toSecsSinceEpoch())
        .arg(upperBound.toSecsSinceEpoch());

    QVERIFY2(lowerBound < upperBound, qPrintable(details));

    QVERIFY2(lowerBound <= dt1, qPrintable(details));
    QVERIFY2(dt1 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt2, qPrintable(details));
    QVERIFY2(dt2 < upperBound, qPrintable(details));
    QVERIFY2(lowerBound <= dt3, qPrintable(details));
    QVERIFY2(dt3 < upperBound, qPrintable(details));

    QVERIFY(dt1.timeSpec() == Qt::UTC);
    QVERIFY(dt2.timeSpec() == Qt::LocalTime);
    QVERIFY(dt3.timeSpec() == Qt::UTC);
}

void tst_QDateTime::currentDateTimeUtc2()
{
    QDateTime local, utc;
    qint64 msec;

    // check that we got all down to the same milliseconds
    int i = 20;
    bool ok = false;
    do {
        local = QDateTime::currentDateTime();
        utc = QDateTime::currentDateTimeUtc();
        msec = QDateTime::currentMSecsSinceEpoch();
        ok = local.time().msec() == utc.time().msec()
            && utc.time().msec() == (msec % 1000);
    } while (--i && !ok);

    if (!i)
        QSKIP("Failed to get the dates within 1 ms of each other");

    // seconds and milliseconds should be the same:
    QCOMPARE(utc.time().second(), local.time().second());
    QCOMPARE(utc.time().msec(), local.time().msec());
    QCOMPARE(msec % 1000, qint64(local.time().msec()));
    QCOMPARE(msec / 1000 % 60, qint64(local.time().second()));

    // the two dates should be equal, actually
    QCOMPARE(local.toUTC(), utc);
    QCOMPARE(utc.toLocalTime(), local);

    // and finally, the SecsSinceEpoch should equal our number
    QCOMPARE(qint64(utc.toSecsSinceEpoch()), msec / 1000);
    QCOMPARE(qint64(local.toSecsSinceEpoch()), msec / 1000);
    QCOMPARE(utc.toMSecsSinceEpoch(), msec);
    QCOMPARE(local.toMSecsSinceEpoch(), msec);
}

void tst_QDateTime::toSecsSinceEpoch_data()
{
    QTest::addColumn<QDate>("date");

    QTest::newRow("start-1800") << QDate(1800, 1, 1);
    QTest::newRow("start-1969") << QDate(1969, 1, 1);
    QTest::newRow("start-2002") << QDate(2002, 1, 1);
    QTest::newRow("mid-2002") << QDate(2002, 6, 1);
    QTest::newRow("start-2038") << QDate(2038, 1, 1);
    QTest::newRow("star-trek-1st-contact") << QDate(2063, 4, 5);
    QTest::newRow("start-2107") << QDate(2107, 1, 1);
}

void tst_QDateTime::toSecsSinceEpoch()
{
    const QTime noon(12, 0);
    QFETCH(const QDate, date);
    const QDateTime dateTime(date, noon);
    QVERIFY(dateTime.isValid());

    const qint64 asSecsSinceEpoch = dateTime.toSecsSinceEpoch();
    QCOMPARE(asSecsSinceEpoch, dateTime.toMSecsSinceEpoch() / 1000);
    QCOMPARE(QDateTime::fromSecsSinceEpoch(asSecsSinceEpoch), dateTime);
}

void tst_QDateTime::daylightSavingsTimeChange_data()
{
    QTest::addColumn<QDate>("inDST");
    QTest::addColumn<QDate>("outDST");
    QTest::addColumn<int>("days"); // from in to out; -ve if reversed
    QTest::addColumn<int>("months");

    QTest::newRow("Autumn") << QDate(2006, 8, 1) << QDate(2006, 12, 1)
                            << 122 << 4;

    QTest::newRow("Spring") << QDate(2006, 5, 1) << QDate(2006, 2, 1)
                            << -89 << -3;
}

void tst_QDateTime::daylightSavingsTimeChange()
{
    // This has grown from a regression test for an old bug where starting with
    // a date in DST and then moving to a date outside it (or vice-versa) caused
    // 1-hour jumps in time when addSecs() was called.
    //
    // The bug was caused by QDateTime knowing more than it lets show.
    // Internally, if it knows, QDateTime stores a flag indicating if the time is
    // DST or not. If it doesn't, it sets to "LocalUnknown".  The problem happened
    // because some functions did not reset the flag when moving in or out of DST.

    // WARNING: This only tests anything if there's a Daylight Savings Time change
    // in the current time-zone between inDST and outDST.
    // This is true for Central European Time and may be elsewhere.

    QFETCH(QDate, inDST);
    QFETCH(QDate, outDST);
    QFETCH(int, days);
    QFETCH(int, months);

    // First with simple construction
    QDateTime dt = QDateTime(outDST, QTime(0, 0, 0), Qt::LocalTime);
    int outDSTsecs = dt.toSecsSinceEpoch();

    dt.setDate(inDST);
    dt = dt.addSecs(1);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 0, 1)));

    // now using addDays:
    dt = dt.addDays(days).addSecs(1);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 2)));

    // ... and back again:
    dt = dt.addDays(-days).addSecs(1);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 0, 3)));

    // now using addMonths:
    dt = dt.addMonths(months).addSecs(1);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 4)));

    // ... and back again:
    dt = dt.addMonths(-months).addSecs(1);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 0, 5)));

    // now using fromSecsSinceEpoch
    dt = QDateTime::fromSecsSinceEpoch(outDSTsecs);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0, 0)));

    dt.setDate(inDST);
    dt = dt.addSecs(60);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 1, 0)));

    // using addMonths:
    dt = dt.addMonths(months).addSecs(60);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 2, 0)));
    // back again:
    dt = dt.addMonths(-months).addSecs(60);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 3, 0)));

    // using addDays:
    dt = dt.addDays(days).addSecs(60);
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 4, 0)));
    // back again:
    dt = dt.addDays(-days).addSecs(60);
    QCOMPARE(dt, QDateTime(inDST, QTime(0, 5, 0)));

    // Now use the result of a UTC -> LocalTime conversion
    dt = QDateTime(outDST, QTime(0, 0), Qt::LocalTime).toUTC();
    dt = QDateTime(dt.date(), dt.time(), Qt::UTC).toLocalTime();
    QCOMPARE(dt, QDateTime(outDST, QTime(0, 0)));

    // using addDays:
    dt = dt.addDays(-days).addSecs(3600);
    QCOMPARE(dt, QDateTime(inDST, QTime(1, 0)));
    // back again
    dt = dt.addDays(days).addSecs(3600);
    QCOMPARE(dt, QDateTime(outDST, QTime(2, 0)));

    // using addMonths:
    dt = dt.addMonths(-months).addSecs(3600);
    QCOMPARE(dt, QDateTime(inDST, QTime(3, 0)));
    // back again:
    dt = dt.addMonths(months).addSecs(3600);
    QCOMPARE(dt, QDateTime(outDST, QTime(4, 0)));

    // using setDate:
    dt.setDate(inDST);
    dt = dt.addSecs(3600);
    QCOMPARE(dt, QDateTime(inDST, QTime(5, 0)));
}

void tst_QDateTime::springForward_data()
{
    QTest::addColumn<QDate>("day"); // day of DST transition
    QTest::addColumn<QTime>("time"); // in the "missing hour"
    QTest::addColumn<int>("step"); // days to step; +ve from before, -ve from after
    QTest::addColumn<int>("adjust"); // minutes ahead of UTC on day stepped from

    /*
      Zone tests compare a summer and winter moment's SecsSinceEpoch to known values.
      This could in principle be flawed (two DST-using zones in the same
      hemisphere with the same DST and standard times but different transition
      times) but no actual example is known where this is a problem.  Please
      document any such conflicts, if discovered.

      See http://www.timeanddate.com/time/zones/ for data on more candidates to
      test.
     */

    uint winter = QDate(2015, 1, 1).startOfDay().toSecsSinceEpoch();
    uint summer = QDate(2015, 7, 1).startOfDay().toSecsSinceEpoch();

    if (winter == 1420066800 && summer == 1435701600) {
        QTest::newRow("CET from day before") << QDate(2015, 3, 29) << QTime(2, 30, 0) << 1 << 60;
        QTest::newRow("CET from day after") << QDate(2015, 3, 29) << QTime(2, 30, 0) << -1 << 120;
    } else if (winter == 1420063200 && summer == 1435698000) {
        // e.g. Finland, where our CI runs ...
        QTest::newRow("EET from day before") << QDate(2015, 3, 29) << QTime(3, 30, 0) << 1 << 120;
        QTest::newRow("EET from day after") << QDate(2015, 3, 29) << QTime(3, 30, 0) << -1 << 180;
    } else if (winter == 1420070400 && summer == 1435705200) {
        // Western European Time, WET/WEST; a.k.a. GMT/BST
        QTest::newRow("WET from day before") << QDate(2015, 3, 29) << QTime(1, 30, 0) << 1 << 0;
        QTest::newRow("WET from day after") << QDate(2015, 3, 29) << QTime(1, 30, 0) << -1 << 60;
    } else if (winter == 1420099200 && summer == 1435734000) {
        // Western USA, Canada: Pacific Time (e.g. US/Pacific)
        QTest::newRow("PT from day before") << QDate(2015, 3, 8) << QTime(2, 30, 0) << 1 << -480;
        QTest::newRow("PT from day after") << QDate(2015, 3, 8) << QTime(2, 30, 0) << -1 << -420;
    } else if (winter == 1420088400 && summer == 1435723200) {
        // Eastern USA, Canada: Eastern Time (e.g. US/Eastern)
        QTest::newRow("ET from day before") << QDate(2015, 3, 8) << QTime(2, 30, 0) << 1 << -300;
        QTest::newRow("ET from day after") << QDate(2015, 3, 8) << QTime(2, 30, 0) << -1 << -240;
    } else {
        // Includes the numbers you need to test for your zone, as above:
        QString msg(QString::fromLatin1("No spring forward test data for this TZ (%1, %2)"
                        ).arg(winter).arg(summer));
        QSKIP(qPrintable(msg));
    }
}

void tst_QDateTime::springForward()
{
    QFETCH(QDate, day);
    QFETCH(QTime, time);
    QFETCH(int, step);
    QFETCH(int, adjust);

    QDateTime direct = QDateTime(day.addDays(-step), time, Qt::LocalTime).addDays(step);
    if (direct.isValid()) { // mktime() may deem a time in the gap invalid
        QCOMPARE(direct.date(), day);
        QCOMPARE(direct.time().minute(), time.minute());
        QCOMPARE(direct.time().second(), time.second());
        int off = direct.time().hour() - time.hour();
        QVERIFY(off == 1 || off == -1);
        // Note: function doc claims always +1, but this should be reviewed !
    }

    // Repeat, but getting there via .toLocalTime():
    QDateTime detour = QDateTime(day.addDays(-step),
                                 time.addSecs(-60 * adjust),
                                 Qt::UTC).toLocalTime();
    QCOMPARE(detour.time(), time);
    detour = detour.addDays(step);
    // Insist on consistency:
    if (direct.isValid())
        QCOMPARE(detour, direct);
    else
        QVERIFY(!detour.isValid());
}

void tst_QDateTime::operator_eqeq_data()
{
    QTest::addColumn<QDateTime>("dt1");
    QTest::addColumn<QDateTime>("dt2");
    QTest::addColumn<bool>("expectEqual");
    QTest::addColumn<bool>("checkEuro");

    QDateTime dateTime1(QDate(2012, 6, 20), QTime(14, 33, 2, 500));
    QDateTime dateTime1a = dateTime1.addMSecs(1);
    QDateTime dateTime2(QDate(2012, 20, 6), QTime(14, 33, 2, 500)); // Invalid
    QDateTime dateTime2a = dateTime2.addMSecs(-1); // Still invalid
    QDateTime dateTime3(QDate(1970, 1, 1), QTime(0, 0), Qt::UTC); // UTC epoch
    QDateTime dateTime3a = dateTime3.addDays(1);
    QDateTime dateTime3b = dateTime3.addDays(-1);
    // Ensure that different times may be equal when considering timezone.
    QDateTime dateTime3c(dateTime3.addSecs(3600));
    dateTime3c.setOffsetFromUtc(3600);
    QDateTime dateTime3d(dateTime3.addSecs(-3600));
    dateTime3d.setOffsetFromUtc(-3600);
    QDateTime dateTime3e(dateTime3.date(), dateTime3.time()); // Local time's epoch

    QTest::newRow("data0") << dateTime1 << dateTime1 << true << false;
    QTest::newRow("data1") << dateTime2 << dateTime2 << true << false;
    QTest::newRow("data2") << dateTime1a << dateTime1a << true << false;
    QTest::newRow("data3") << dateTime1 << dateTime2 << false << false;
    QTest::newRow("data4") << dateTime1 << dateTime1a << false << false;
    QTest::newRow("data5") << dateTime2 << dateTime2a << true << false;
    QTest::newRow("data6") << dateTime2 << dateTime3 << false << false;
    QTest::newRow("data7") << dateTime3 << dateTime3a << false << false;
    QTest::newRow("data8") << dateTime3 << dateTime3b << false << false;
    QTest::newRow("data9") << dateTime3a << dateTime3b << false << false;
    QTest::newRow("data10") << dateTime3 << dateTime3c << true << false;
    QTest::newRow("data11") << dateTime3 << dateTime3d << true << false;
    QTest::newRow("data12") << dateTime3c << dateTime3d << true << false;
    if (localTimeType == LocalTimeIsUtc)
        QTest::newRow("data13") << dateTime3 << dateTime3e << true << false;
    // ... but a zone (sometimes) ahead of or behind UTC (e.g. Europe/London)
    // might agree with UTC about the epoch, all the same.

    QTest::newRow("invalid == invalid") << QDateTime() << QDateTime() << true << false;
    QTest::newRow("invalid != valid #1") << QDateTime() << dateTime1 << false << false;

    if (zoneIsCET) {
        QTest::newRow("data14")
            << QDateTime(QDate(2004, 1, 2), QTime(2, 2, 3), Qt::LocalTime)
            << QDateTime(QDate(2004, 1, 2), QTime(1, 2, 3), Qt::UTC) << true << true;
        QTest::newRow("local-fall-back") // Sun, 31 Oct 2004, 02:30, both ways round:
            << QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1099186200000))
            << QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1099182600000))
            << false << false;
    }
#if QT_CONFIG(timezone)
    const QTimeZone CET("Europe/Oslo");
    if (CET.isValid()) {
        QTest::newRow("CET-fall-back") // Sun, 31 Oct 2004, 02:30, both ways round:
            << QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1099186200000), CET)
            << QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1099182600000), CET)
            << false << false;
    }
#endif
}

void tst_QDateTime::operator_eqeq()
{
    QFETCH(QDateTime, dt1);
    QFETCH(QDateTime, dt2);
    QFETCH(bool, expectEqual);
    QFETCH(bool, checkEuro);

    QVERIFY(dt1 == dt1);
    QVERIFY(!(dt1 != dt1));

    QVERIFY(dt2 == dt2);
    QVERIFY(!(dt2 != dt2));

    QVERIFY(dt1 != QDateTime::currentDateTime());
    QVERIFY(dt2 != QDateTime::currentDateTime());

    QVERIFY(dt1.toUTC() == dt1.toUTC());

    bool equal = dt1 == dt2;
    QCOMPARE(equal, expectEqual);
    bool notEqual = dt1 != dt2;
    QCOMPARE(notEqual, !expectEqual);

    if (equal)
        QVERIFY(qHash(dt1) == qHash(dt2));

    if (checkEuro && zoneIsCET) {
        QVERIFY(dt1.toUTC() == dt2);
        QVERIFY(dt1 == dt2.toLocalTime());
    }
}

Q_DECLARE_METATYPE(QDataStream::Version)

void tst_QDateTime::operator_insert_extract_data()
{
    QTest::addColumn<int>("yearNumber");
    QTest::addColumn<QByteArray>("serialiseAs");
    QTest::addColumn<QByteArray>("deserialiseAs");
    QTest::addColumn<QDataStream::Version>("dataStreamVersion");

    const QByteArray westernAustralia("AWST-8AWDT-9,M10.5.0,M3.5.0/03:00:00");
    const QByteArray hawaii("HAW10");

    const QDataStream tmpDataStream;
    const int thisVersion = tmpDataStream.version();
    for (int version = QDataStream::Qt_1_0; version <= thisVersion; ++version) {
        const QDataStream::Version dataStreamVersion = static_cast<QDataStream::Version>(version);
        const QByteArray vN = QByteArray::number(dataStreamVersion);
        QTest::addRow("v%d WA => HAWAII %d", version, 2012)
            << 2012 << westernAustralia << hawaii << dataStreamVersion;
        QTest::addRow("v%d WA => WA %d", version, 2012)
            << 2012 << westernAustralia << westernAustralia << dataStreamVersion;
        QTest::addRow("v%d HAWAII => WA %d", version, -2012)
            << -2012 << hawaii << westernAustralia << dataStreamVersion;
        QTest::addRow("v%d HAWAII => HAWAII %d", version, 2012)
            << 2012 << hawaii << hawaii << dataStreamVersion;
    }
}

void tst_QDateTime::operator_insert_extract()
{
    QFETCH(int, yearNumber);
    QFETCH(QByteArray, serialiseAs);
    QFETCH(QByteArray, deserialiseAs);
    QFETCH(QDataStream::Version, dataStreamVersion);

    // Start off in a certain timezone.
    TimeZoneRollback useZone(serialiseAs);

    // It is important that dateTime is created after the time zone shift
    QDateTime dateTime(QDate(yearNumber, 8, 14), QTime(8, 0), Qt::LocalTime);
    QDateTime dateTimeAsUTC(dateTime.toUTC());

    QByteArray byteArray;
    {
        QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
        dataStream.setVersion(dataStreamVersion);
        if (dataStreamVersion == QDataStream::Qt_5_0) {
            // Qt 5 serialises as UTC and converts back to the stored timeSpec when
            // deserialising; we don't need to do it ourselves...
            dataStream << dateTime << dateTime;
        } else {
            // ... but other versions don't, so we have to here.
            dataStream << dateTimeAsUTC << dateTimeAsUTC;
            // We'll also make sure that a deserialised local datetime is the same
            // time of day (potentially different UTC time), regardless of which
            // timezone it was serialised in. E.g.: Tue Aug 14 08:00:00 2012
            // serialised in WST should be deserialised as Tue Aug 14 08:00:00 2012
            // HST.
            dataStream << dateTime;
        }
    }

    // Ensure that a change in timezone between serialisation and deserialisation
    // still results in identical UTC-converted datetimes.
    useZone.reset(deserialiseAs);
    QDateTime expectedLocalTime(dateTimeAsUTC.toLocalTime());
    {
        // Deserialise whole QDateTime at once.
        QDataStream dataStream(&byteArray, QIODevice::ReadOnly);
        dataStream.setVersion(dataStreamVersion);
        QDateTime deserialised;
        dataStream >> deserialised;

        if (dataStreamVersion == QDataStream::Qt_5_0) {
            // Ensure local time is still correct. Again, Qt 5 handles the timeSpec
            // conversion (in this case, UTC => LocalTime) for us when deserialising.
            QCOMPARE(deserialised, expectedLocalTime);
        } else {
            if (dataStreamVersion < QDataStream::Qt_4_0) {
                // Versions lower than Qt 4 don't serialise the timeSpec, instead
                // assuming that everything is LocalTime.
                deserialised.setTimeSpec(Qt::UTC);
            }
            // Qt 4.* versions do serialise the timeSpec, so we only need to convert from UTC here.
            deserialised = deserialised.toLocalTime();

            QCOMPARE(deserialised, expectedLocalTime);
        }
        // Sanity check UTC times (operator== already converts its operands to UTC before comparing).
        QCOMPARE(deserialised.toUTC(), expectedLocalTime.toUTC());

        // Deserialise each component individually.
        QDate deserialisedDate;
        dataStream >> deserialisedDate;
        QTime deserialisedTime;
        dataStream >> deserialisedTime;
        qint8 deserialisedSpec;
        if (dataStreamVersion >= QDataStream::Qt_4_0)
            dataStream >> deserialisedSpec;
        deserialised = QDateTime(deserialisedDate, deserialisedTime, Qt::UTC);
        if (dataStreamVersion >= QDataStream::Qt_4_0)
            deserialised = deserialised.toTimeSpec(static_cast<Qt::TimeSpec>(deserialisedSpec));
        // Ensure local time is still correct.
        QCOMPARE(deserialised, expectedLocalTime);
        // Sanity check UTC times.
        QCOMPARE(deserialised.toUTC(), expectedLocalTime.toUTC());

        if (dataStreamVersion != QDataStream::Qt_5_0) {
            // Deserialised local datetime should be the same time of day,
            // regardless of which timezone it was serialised in.
            QDateTime localDeserialized;
            dataStream >> localDeserialized;
            QCOMPARE(localDeserialized, dateTime);
        }
    }
}

#if QT_CONFIG(datestring)
void tst_QDateTime::fromStringDateFormat_data()
{
    QTest::addColumn<QString>("dateTimeStr");
    QTest::addColumn<Qt::DateFormat>("dateFormat");
    QTest::addColumn<QDateTime>("expected");

    // Test Qt::TextDate format.
    QTest::newRow("text date") << QString::fromLatin1("Tue Jun 17 08:00:10 2003")
        << Qt::TextDate << QDateTime(QDate(2003, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year 0999") << QString::fromLatin1("Tue Jun 17 08:00:10 0999")
        << Qt::TextDate << QDateTime(QDate(999, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year 999") << QString::fromLatin1("Tue Jun 17 08:00:10 999")
        << Qt::TextDate << QDateTime(QDate(999, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year 12345") << QString::fromLatin1("Tue Jun 17 08:00:10 12345")
        << Qt::TextDate << QDateTime(QDate(12345, 6, 17), QTime(8, 0, 10, 0), Qt::LocalTime);
    QTest::newRow("text date Year -4712") << QString::fromLatin1("Tue Jan 1 00:01:02 -4712")
        << Qt::TextDate << QDateTime(QDate(-4712, 1, 1), QTime(0, 1, 2, 0), Qt::LocalTime);
    QTest::newRow("text epoch")
        << QString::fromLatin1("Thu Jan 1 00:00:00 1970") << Qt::TextDate
        << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("text data1") << QString::fromLatin1("Thu Jan 2 12:34 1970")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 2), QTime(12, 34, 0), Qt::LocalTime);
    QTest::newRow("text epoch terse")
        << QString::fromLatin1("Thu Jan 1 00 1970") << Qt::TextDate << QDateTime();
    QTest::newRow("text epoch stray :00")
        << QString::fromLatin1("Thu Jan 1 00:00:00:00 1970") << Qt::TextDate << QDateTime();
    QTest::newRow("text epoch spaced")
        << QString::fromLatin1(" Thu   Jan   1    00:00:00    1970  ")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("text data6") << QString::fromLatin1("Thu Jan 1 00:00:00")
        << Qt::TextDate << QDateTime();
    QTest::newRow("text data7") << QString::fromLatin1("Thu Jan 1 1970 00:00:00")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("text bad offset") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 UTC+foo")
        << Qt::TextDate << QDateTime();
    QTest::newRow("text UTC early") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 UTC")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("text UTC-3 early") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 UTC-0300")
        << Qt::TextDate << QDateTime(QDate(1970, 1, 1), QTime(3, 12, 34), Qt::UTC);
    QTest::newRow("text UTC+3 early") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 UTC+0300")
        << Qt::TextDate << QDateTime(QDate(1969, 12, 31), QTime(21, 12, 34), Qt::UTC);
    QTest::newRow("text UTC+1 early") << QString::fromLatin1("Thu Jan 1 1970 00:12:34 UTC+0100")
        << Qt::TextDate << QDateTime(QDate(1969, 12, 31), QTime(23, 12, 34), Qt::UTC);
    // We produce use GMT as prefix, so need to parse it:
    QTest::newRow("text GMT early")
        << QString::fromLatin1("Thu Jan 1 00:12:34 1970 GMT") << Qt::TextDate
        << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("text GMT+3 early")
        << QString::fromLatin1("Thu Jan 1 00:12:34 1970 GMT+0300") << Qt::TextDate
        << QDateTime(QDate(1969, 12, 31), QTime(21, 12, 34), Qt::UTC);
    // ... and we match (only) it case-insensitively:
    QTest::newRow("text gmt early")
        << QString::fromLatin1("Thu Jan 1 00:12:34 1970 gmt") << Qt::TextDate
        << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);

    QTest::newRow("text empty") << QString::fromLatin1("")
        << Qt::TextDate << QDateTime();
    QTest::newRow("text too many parts") << QString::fromLatin1("Thu Jan 1 00:12:34 1970 UTC +0100")
        << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid month name") << QString::fromLatin1("Thu Jaz 1 1970 00:12:34")
        << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid date") << QString::fromLatin1("Thu Jan 32 1970 00:12:34")
        << Qt::TextDate << QDateTime();
    QTest::newRow("text pre-5.2 MS-Win format") // Support dropped in 6.2
        << QString::fromLatin1("Thu 1. Jan 00:00:00 1970") << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid day")
        << QString::fromLatin1("Thu Jan XX 1970 00:12:34") << Qt::TextDate << QDateTime();
    QTest::newRow("text misplaced day")
        << QString::fromLatin1("Thu 1 Jan 00:00:00 1970") << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid year end")
        << QString::fromLatin1("Thu Jan 1 00:00:00 19X0") << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid year early")
        << QString::fromLatin1("Thu Jan 1 19X0 00:00:00") << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid hour")
        << QString::fromLatin1("Thu Jan 1 1970 0X:00:00") << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid minute")
        << QString::fromLatin1("Thu Jan 1 1970 00:0X:00") << Qt::TextDate << QDateTime();
    QTest::newRow("text invalid second")
        << QString::fromLatin1("Thu Jan 1 1970 00:00:0X") << Qt::TextDate << QDateTime();
    QTest::newRow("text non-UTC offset")
        << QString::fromLatin1("Thu Jan 1 1970 00:00:00 DMT") << Qt::TextDate << QDateTime();
    QTest::newRow("text bad UTC offset")
        << QString::fromLatin1("Thu Jan 1 1970 00:00:00 UTCx0200") << Qt::TextDate << QDateTime();
    QTest::newRow("text bad UTC hour")
        << QString::fromLatin1("Thu Jan 1 1970 00:00:00 UTC+0X00") << Qt::TextDate << QDateTime();
    QTest::newRow("text bad UTC minute")
        << QString::fromLatin1("Thu Jan 1 1970 00:00:00 UTC+000X") << Qt::TextDate << QDateTime();

    QTest::newRow("text second fraction")
        << QString::fromLatin1("Mon May 6 2013 01:02:03.456")
        << Qt::TextDate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 3, 456));
    QTest::newRow("text max milli")
        << QString::fromLatin1("Mon May 6 2013 01:02:03.999499999")
        << Qt::TextDate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 3, 999));
    QTest::newRow("text milli wrap")
        << QString::fromLatin1("Mon May 6 2013 01:02:03.9995")
        << Qt::TextDate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 4));
    QTest::newRow("text last milli") // Special case, don't round up to invalid:
        << QString::fromLatin1("Mon May 6 2013 23:59:59.9999999999")
        << Qt::TextDate << QDateTime(QDate(2013, 5, 6), QTime(23, 59, 59, 999));
    QTest::newRow("text Sunday lunch")
        << QStringLiteral("Sun Dec 1 13:02:00 1974") << Qt::TextDate
        << QDateTime(QDate(1974, 12, 1), QTime(13, 2));

    // Test Qt::ISODate format.
    QTest::newRow("trailing space") // QTBUG-80445
        << QString("2000-01-02 03:04:05.678 ") << Qt::ISODate << QDateTime();

    // Invalid spaces (but keeping field widths correct):
    QTest::newRow("space before millis")
        << QString("2000-01-02 03:04:05. 678") << Qt::ISODate << QDateTime();
    QTest::newRow("space after seconds")
        << QString("2000-01-02 03:04:5 .678") << Qt::ISODate << QDateTime();
    QTest::newRow("space before seconds")
        << QString("2000-01-02 03:04: 5.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space after minutes")
        << QString("2000-01-02 03:4 :05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space before minutes")
        << QString("2000-01-02 03: 4:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space after hour")
        << QString("2000-01-02 3 :04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space before hour")
        << QString("2000-01-02  3:04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space after day")
        << QString("2000-01-2  03:04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space before day")
        << QString("2000-01- 2 03:04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space after month")
        << QString("2000-1 -02 03:04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space before month")
        << QString("2000- 1-02 03:04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("space after year")
        << QString("200 -01-02 03:04:05.678") << Qt::ISODate << QDateTime();

    // Spaces as separators:
    QTest::newRow("sec-milli space")
        << QString("2000-01-02 03:04:05 678") << Qt::ISODate
        << QDateTime();
    QTest::newRow("min-sec space")
        << QString("2000-01-02 03:04 05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("hour-min space")
        << QString("2000-01-02 03 04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("mon-day space")
        << QString("2000-01 02 03:04:05.678") << Qt::ISODate << QDateTime();
    QTest::newRow("year-mon space")
        << QString("2000 01-02 03:04:05.678") << Qt::ISODate << QDateTime();

    // Normal usage:
    QTest::newRow("ISO +01:00") << QString::fromLatin1("1987-02-13T13:24:51+01:00")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("ISO +00:01") << QString::fromLatin1("1987-02-13T13:24:51+00:01")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(13, 23, 51), Qt::UTC);
    QTest::newRow("ISO -01:00") << QString::fromLatin1("1987-02-13T13:24:51-01:00")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("ISO -00:01") << QString::fromLatin1("1987-02-13T13:24:51-00:01")
        << Qt::ISODate << QDateTime(QDate(1987, 2, 13), QTime(13, 25, 51), Qt::UTC);
    QTest::newRow("ISO +0000") << QString::fromLatin1("1970-01-01T00:12:34+0000")
        << Qt::ISODate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("ISO +00:00") << QString::fromLatin1("1970-01-01T00:12:34+00:00")
        << Qt::ISODate << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("ISO -03") << QString::fromLatin1("2014-12-15T12:37:09-03")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9), Qt::UTC);
    QTest::newRow("ISO zzz-03") << QString::fromLatin1("2014-12-15T12:37:09.745-03")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9, 745), Qt::UTC);
    QTest::newRow("ISO -3") << QString::fromLatin1("2014-12-15T12:37:09-3")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9), Qt::UTC);
    QTest::newRow("ISO zzz-3") << QString::fromLatin1("2014-12-15T12:37:09.745-3")
        << Qt::ISODate << QDateTime(QDate(2014, 12, 15), QTime(15, 37, 9, 745), Qt::UTC);
    QTest::newRow("ISO lower-case") << QString::fromLatin1("2005-06-28T07:57:30.002z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 2), Qt::UTC);
    // No time specified - defaults to Qt::LocalTime.
    QTest::newRow("ISO data3") << QString::fromLatin1("2002-10-01")
        << Qt::ISODate << QDateTime(QDate(2002, 10, 1), QTime(0, 0), Qt::LocalTime);
    // Excess digits in milliseconds, round correctly:
    QTest::newRow("ISO") << QString::fromLatin1("2005-06-28T07:57:30.0010000000Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 1), Qt::UTC);
    QTest::newRow("ISO rounding") << QString::fromLatin1("2005-06-28T07:57:30.0015Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 2), Qt::UTC);
    // ... and accept comma as separator:
    QTest::newRow("ISO with comma 1") << QString::fromLatin1("2005-06-28T07:57:30,0040000000Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 4), Qt::UTC);
    QTest::newRow("ISO with comma 2") << QString::fromLatin1("2005-06-28T07:57:30,0015Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 2), Qt::UTC);
    QTest::newRow("ISO with comma 3") << QString::fromLatin1("2005-06-28T07:57:30,0014Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 1), Qt::UTC);
    QTest::newRow("ISO with comma 4") << QString::fromLatin1("2005-06-28T07:57:30,1Z")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 100), Qt::UTC);
    QTest::newRow("ISO with comma 5") << QString::fromLatin1("2005-06-28T07:57:30,11")
        << Qt::ISODate << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 110), Qt::LocalTime);
    // 24:00:00 Should be next day according to ISO 8601 section 4.2.3.
    QTest::newRow("ISO 24:00") << QString::fromLatin1("2012-06-04T24:00:00")
        << Qt::ISODate << QDateTime(QDate(2012, 6, 5), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00 in DST") // Only special if TZ=America/Sao_Paulo
        << QString::fromLatin1("2008-10-18T24:00") << Qt::ISODate
        << QDateTime(QDate(2008, 10, 19),
                     QTime(QTimeZone::systemTimeZoneId() == "America/Sao_Paulo" ? 1 : 0, 0),
                     Qt::LocalTime);
    QTest::newRow("ISO 24:00 end of month") << QString::fromLatin1("2012-06-30T24:00:00")
        << Qt::ISODate << QDateTime(QDate(2012, 7, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00 end of year") << QString::fromLatin1("2012-12-31T24:00:00")
        << Qt::ISODate << QDateTime(QDate(2013, 1, 1), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00, fract ms") << QString::fromLatin1("2012-01-01T24:00:00.000")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 2), QTime(0, 0), Qt::LocalTime);
    QTest::newRow("ISO 24:00 end of year, fract ms") << QString::fromLatin1("2012-12-31T24:00:00.000")
        << Qt::ISODate << QDateTime(QDate(2013, 1, 1), QTime(0, 0), Qt::LocalTime);
    // Test fractional seconds.
    QTest::newRow("ISO .0 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.0")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .00 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.00")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .000 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.000")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .1 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,1")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 100), Qt::LocalTime);
    QTest::newRow("ISO .99 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,99")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 990), Qt::LocalTime);
    QTest::newRow("ISO .998 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,998")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 998), Qt::LocalTime);
    QTest::newRow("ISO .999 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,999")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 999), Qt::LocalTime);
    QTest::newRow("ISO .3335 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,3335")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 334), Qt::LocalTime);
    QTest::newRow("ISO .333333 of a second (comma)") << QString::fromLatin1("2012-01-01T08:00:00,333333")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 333), Qt::LocalTime);
    QTest::newRow("ISO .00009 of a second (period)") << QString::fromLatin1("2012-01-01T08:00:00.00009")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO second fraction") << QString::fromLatin1("2013-05-06T01:02:03.456")
        << Qt::ISODate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 3, 456));
    QTest::newRow("ISO max milli")
        << QString::fromLatin1("2013-05-06T01:02:03.999499999")
        << Qt::ISODate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 3, 999));
    QTest::newRow("ISO milli wrap")
        << QString::fromLatin1("2013-05-06T01:02:03.9995")
        << Qt::ISODate << QDateTime(QDate(2013, 5, 6), QTime(1, 2, 4));
    QTest::newRow("ISO last milli") // Does round up and overflow into new day:
        << QString::fromLatin1("2013-05-06T23:59:59.9999999999")
        << Qt::ISODate << QDate(2013, 5, 7).startOfDay();
    QTest::newRow("ISO no fraction specified")
        << QString::fromLatin1("2012-01-01T08:00:00.") << Qt::ISODate << QDateTime();
    // Test invalid characters (should ignore invalid characters at end of string).
    QTest::newRow("ISO invalid character at end") << QString::fromLatin1("2012-01-01T08:00:00!")
        << Qt::ISODate << QDateTime();
    QTest::newRow("ISO invalid character at front") << QString::fromLatin1("!2012-01-01T08:00:00")
        << Qt::ISODate << QDateTime();
    QTest::newRow("ISO invalid character both ends") << QString::fromLatin1("!2012-01-01T08:00:00!")
        << Qt::ISODate << QDateTime();
    QTest::newRow("ISO invalid character at front, 2 at back") << QString::fromLatin1("!2012-01-01T08:00:00..")
        << Qt::ISODate << QDateTime();
    QTest::newRow("ISO invalid character 2 at front") << QString::fromLatin1("!!2012-01-01T08:00:00")
        << Qt::ISODate << QDateTime();
    // Test fractional minutes.
    QTest::newRow("ISO .0 of a minute (period)") << QString::fromLatin1("2012-01-01T08:00.0")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .8 of a minute (period)") << QString::fromLatin1("2012-01-01T08:00.8")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 48, 0), Qt::LocalTime);
    QTest::newRow("ISO .99999 of a minute (period)") << QString::fromLatin1("2012-01-01T08:00.99999")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 59, 999), Qt::LocalTime);
    QTest::newRow("ISO .0 of a minute (comma)") << QString::fromLatin1("2012-01-01T08:00,0")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 0, 0), Qt::LocalTime);
    QTest::newRow("ISO .8 of a minute (comma)") << QString::fromLatin1("2012-01-01T08:00,8")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 48, 0), Qt::LocalTime);
    QTest::newRow("ISO .99999 of a minute (comma)") << QString::fromLatin1("2012-01-01T08:00,99999")
        << Qt::ISODate << QDateTime(QDate(2012, 1, 1), QTime(8, 0, 59, 999), Qt::LocalTime);
    QTest::newRow("ISO empty") << QString::fromLatin1("") << Qt::ISODate << QDateTime();
    QTest::newRow("ISO short") << QString::fromLatin1("2017-07-01T") << Qt::ISODate << QDateTime();
    QTest::newRow("ISO zoned date")
        << QString::fromLatin1("2017-07-01Z") << Qt::ISODate << QDateTime();
    QTest::newRow("ISO zoned empty time")
        << QString::fromLatin1("2017-07-01TZ") << Qt::ISODate << QDateTime();
    QTest::newRow("ISO mis-punctuated")
        << QString::fromLatin1("2018/01/30 ") << Qt::ISODate << QDateTime();

    // Test Qt::RFC2822Date format (RFC 2822).
    QTest::newRow("RFC 2822 +0100") << QString::fromLatin1("13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 after space +0100")
        << QString::fromLatin1(" 13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 with day +0100") << QString::fromLatin1("Fri, 13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 with day after space +0100")
        << QString::fromLatin1(" Fri, 13 Feb 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 -0100") << QString::fromLatin1("13 Feb 1987 13:24:51 -0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 with day -0100") << QString::fromLatin1("Fri, 13 Feb 1987 13:24:51 -0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("RFC 2822 +0000") << QString::fromLatin1("01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 with day +0000") << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 +0000") << QString::fromLatin1("01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 with day +0000") << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 2822 missing space before +0100")
        << QString::fromLatin1("Thu, 01 Jan 1970 00:12:34+0100") << Qt::RFC2822Date << QDateTime();
    // No timezone assume UTC
    QTest::newRow("RFC 2822 no timezone") << QString::fromLatin1("01 Jan 1970 00:12:34")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No time specified
    QTest::newRow("RFC 2822 date only") << QString::fromLatin1("01 Nov 2002")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 with day date only") << QString::fromLatin1("Fri, 01 Nov 2002")
        << Qt::RFC2822Date << QDateTime();

    QTest::newRow("RFC 2822 malformed time (truncated)")
        << QString::fromLatin1("01 Nov 2002 0:") << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 malformed time (hour)")
        << QString::fromLatin1("01 Nov 2002 7:35:21") << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 malformed time (minute)")
        << QString::fromLatin1("01 Nov 2002 07:5:21") << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 malformed time (second)")
        << QString::fromLatin1("01 Nov 2002 07:35:1") << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 malformed time (fraction-second)")
        << QString::fromLatin1("01 Nov 2002 07:35:15.200") << Qt::RFC2822Date << QDateTime();

    // Test invalid month, day, year
    QTest::newRow("RFC 2822 invalid month name") << QString::fromLatin1("13 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 invalid day") << QString::fromLatin1("36 Fev 1987 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 invalid year") << QString::fromLatin1("13 Fev 0000 13:24:51 +0100")
        << Qt::RFC2822Date << QDateTime();
    // Test invalid characters.
    QTest::newRow("RFC 2822 invalid character at end")
        << QString::fromLatin1("01 Jan 2012 08:00:00 +0100!")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 invalid character at front")
        << QString::fromLatin1("!01 Jan 2012 08:00:00 +0100")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 invalid character both ends")
        << QString::fromLatin1("!01 Jan 2012 08:00:00 +0100!")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 invalid character at front, 2 at back")
        << QString::fromLatin1("!01 Jan 2012 08:00:00 +0100..")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 2822 invalid character 2 at front")
        << QString::fromLatin1("!!01 Jan 2012 08:00:00 +0100")
        << Qt::RFC2822Date << QDateTime();
    // The common date text used by the "invalid character" tests, just to be
    // sure *it's* not what's invalid:
    QTest::newRow("RFC 2822 (not invalid)")
        << QString::fromLatin1("01 Jan 2012 08:00:00 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(2012, 1, 1), QTime(7, 0), Qt::UTC);

    // Test Qt::RFC2822Date format (RFC 850 and 1036, permissive).
    QTest::newRow("RFC 850 and 1036 +0100") << QString::fromLatin1("Fri Feb 13 13:24:51 1987 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 1036 after space +0100")
        << QString::fromLatin1(" Fri Feb 13 13:24:51 1987 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(12, 24, 51), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 -0100") << QString::fromLatin1("Fri Feb 13 13:24:51 1987 -0100")
        << Qt::RFC2822Date << QDateTime(QDate(1987, 2, 13), QTime(14, 24, 51), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 +0000") << QString::fromLatin1("Thu Jan 01 00:12:34 1970 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    QTest::newRow("RFC 850 and 1036 +0000") << QString::fromLatin1("Thu Jan 01 00:12:34 1970 +0000")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No timezone assume UTC
    QTest::newRow("RFC 850 and 1036 no timezone") << QString::fromLatin1("Thu Jan 01 00:12:34 1970")
        << Qt::RFC2822Date << QDateTime(QDate(1970, 1, 1), QTime(0, 12, 34), Qt::UTC);
    // No time specified
    QTest::newRow("RFC 850 and 1036 date only")
        << QString::fromLatin1("Fri Nov 01 2002")
        << Qt::RFC2822Date << QDateTime();
    // Test invalid characters.
    QTest::newRow("RFC 850 and 1036 invalid character at end")
        << QString::fromLatin1("Sun Jan 01 08:00:00 2012 +0100!")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character at front")
        << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0100")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character both ends")
        << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0100!")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character at front, 2 at back")
        << QString::fromLatin1("!Sun Jan 01 08:00:00 2012 +0100..")
        << Qt::RFC2822Date << QDateTime();
    QTest::newRow("RFC 850 and 1036 invalid character 2 at front")
        << QString::fromLatin1("!!Sun Jan 01 08:00:00 2012 +0100")
        << Qt::RFC2822Date << QDateTime();
    // Again, check the text in the "invalid character" tests isn't the source of invalidity:
    QTest::newRow("RFC 850 and 1036 (not invalid)")
        << QString::fromLatin1("Sun Jan 01 08:00:00 2012 +0100")
        << Qt::RFC2822Date << QDateTime(QDate(2012, 1, 1), QTime(7, 0), Qt::UTC);

    QTest::newRow("RFC empty") << QString::fromLatin1("") << Qt::RFC2822Date << QDateTime();
}

void tst_QDateTime::fromStringDateFormat()
{
    QFETCH(QString, dateTimeStr);
    QFETCH(Qt::DateFormat, dateFormat);
    QFETCH(QDateTime, expected);

    QDateTime dateTime = QDateTime::fromString(dateTimeStr, dateFormat);
    QCOMPARE(dateTime, expected);
}

# if QT_CONFIG(datetimeparser)
void tst_QDateTime::fromStringStringFormat_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QDateTime>("expected");

    const QDate defDate(1900, 1, 1);
    QTest::newRow("data0")
        << QString("101010") << QString("dMyy") << QDate(1910, 10, 10).startOfDay();
    QTest::newRow("data1") << QString("1020") << QString("sss") << QDateTime();
    QTest::newRow("data2")
        << QString("1010") << QString("sss") << QDateTime(defDate, QTime(0, 0, 10));
    QTest::newRow("data3") << QString("10hello20") << QString("ss'hello'ss") << QDateTime();
    QTest::newRow("data4") << QString("10") << QString("''") << QDateTime();
    QTest::newRow("data5") << QString("10") << QString("'") << QDateTime();
    QTest::newRow("data6") << QString("pm") << QString("ap") << QDateTime(defDate, QTime(12, 0));
    QTest::newRow("data7") << QString("foo") << QString("ap") << QDateTime();
    // Day non-conflict should not hide earlier year conflict (1963-03-01 was a
    // Friday; asking for Thursday moves this, without conflict, to the 7th):
    QTest::newRow("data8")
        << QString("77 03 1963 Thu") << QString("yy MM yyyy ddd") << QDateTime();
    QTest::newRow("data9")
        << QString("101010") << QString("dMyy") << QDate(1910, 10, 10).startOfDay();
    QTest::newRow("data10")
        << QString("101010") << QString("dMyy") << QDate(1910, 10, 10).startOfDay();
    QTest::newRow("data11")
        << QString("10 Oct 10") << QString("dd MMM yy") << QDate(1910, 10, 10).startOfDay();
    QTest::newRow("data12")
        << QString("Fri December 3 2004") << QString("ddd MMMM d yyyy")
        << QDate(2004, 12, 3).startOfDay();
    QTest::newRow("data13") << QString("30.02.2004") << QString("dd.MM.yyyy") << QDateTime();
    QTest::newRow("data14") << QString("32.01.2004") << QString("dd.MM.yyyy") << QDateTime();
    QTest::newRow("data15")
        << QString("Thu January 2004") << QString("ddd MMMM yyyy")
        << QDate(2004, 1, 1).startOfDay();
    QTest::newRow("data16") << QString("2005-06-28T07:57:30.001Z")
                            << QString("yyyy-MM-ddThh:mm:ss.zt")
                            << QDateTime(QDate(2005, 06, 28), QTime(07, 57, 30, 1), Qt::UTC);
    QTest::newRow("utc-time-spec-as:UTC+0")
        << QString("2005-06-28T07:57:30.001UTC+0") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 1), Qt::UTC);
    QTest::newRow("utc-time-spec-as:UTC-0")
        << QString("2005-06-28T07:57:30.001UTC-0") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime(QDate(2005, 6, 28), QTime(7, 57, 30, 1), Qt::UTC);
    QTest::newRow("offset-from-utc:UTC+1")
        << QString("2001-09-13T07:33:01.001 UTC+1") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime(QDate(2001, 9, 13), QTime(7, 33, 1, 1), Qt::OffsetFromUTC, 3600);
    QTest::newRow("offset-from-utc:UTC-11:01")
        << QString("2008-09-13T07:33:01.001 UTC-11:01") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime(QDate(2008, 9, 13), QTime(7, 33, 1, 1), Qt::OffsetFromUTC, -39660);
    QTest::newRow("offset-from-utc:UTC+02:57")
        << QString("2001-09-15T09:33:01.001UTC+02:57") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime(QDate(2001, 9, 15), QTime(9, 33, 1, 1), Qt::OffsetFromUTC, 10620);
    QTest::newRow("offset-from-utc:-03:00")  // RFC 3339 offset format
        << QString("2001-09-15T09:33:01.001-03:00") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime(QDate(2001, 9, 15), QTime(9, 33, 1, 1), Qt::OffsetFromUTC, -10800);
    QTest::newRow("offset-from-utc:+0205")  // ISO 8601 basic offset format
        << QString("2001-09-15T09:33:01.001+0205") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime(QDate(2001, 9, 15), QTime(9, 33, 1, 1), Qt::OffsetFromUTC, 7500);
    QTest::newRow("offset-from-utc:-0401")  // ISO 8601 basic offset format
        << QString("2001-09-15T09:33:01.001-0401") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime(QDate(2001, 9, 15), QTime(9, 33, 1, 1), Qt::OffsetFromUTC, -14460);
    QTest::newRow("offset-from-utc:+10")  // ISO 8601 basic (hour-only) offset format
        << QString("2001-09-15T09:33:01.001 +10") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime(QDate(2001, 9, 15), QTime(9, 33, 1, 1), Qt::OffsetFromUTC, 36000);
    QTest::newRow("offset-from-utc:UTC+10:00")  // Time-spec specifier at the beginning
        << QString("UTC+10:00 2008-10-13T07:33") << QString("t yyyy-MM-ddThh:mm")
        << QDateTime(QDate(2008, 10, 13), QTime(7, 33), Qt::OffsetFromUTC, 36000);
    QTest::newRow("offset-from-utc:UTC-03:30")  // Time-spec specifier in the middle
        << QString("2008-10-13 UTC-03:30 11.50") << QString("yyyy-MM-dd t hh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, -12600);
    QTest::newRow("offset-from-utc:UTC-2")  // Time-spec specifier joined with text/time
        << QString("2008-10-13 UTC-2Z11.50") << QString("yyyy-MM-dd tZhh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, -7200);
    QTest::newRow("offset-from-utc:followed-by-colon")
        << QString("2008-10-13 UTC-0100:11.50") << QString("yyyy-MM-dd t:hh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, -3600);
    QTest::newRow("offset-from-utc:late-colon")
        << QString("2008-10-13 UTC+05T:11.50") << QString("yyyy-MM-dd tT:hh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, 18000);
    QTest::newRow("offset-from-utc:merged-with-time")
        << QString("2008-10-13 UTC+010011.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, 3600);
    QTest::newRow("offset-from-utc:double-colon-delimiter")
        << QString("2008-10-13 UTC+12::11.50") << QString("yyyy-MM-dd t::hh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, 43200);
    QTest::newRow("offset-from-utc:3-digit-with-colon")
        << QString("2008-10-13 -4:30 11.50") << QString("yyyy-MM-dd t hh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, -16200);
    QTest::newRow("offset-from-utc:merged-with-time")
        << QString("2008-10-13 UTC+010011.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, 3600);
    QTest::newRow("offset-from-utc:with-colon-merged-with-time")
        << QString("2008-10-13 UTC+01:0011.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime(QDate(2008, 10, 13), QTime(11, 50), Qt::OffsetFromUTC, 3600);
    QTest::newRow("invalid-offset-from-utc:out-of-range")
        << QString("2001-09-15T09:33:01.001-50") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:single-digit-format")
        << QString("2001-09-15T09:33:01.001+5") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:three-digit-format")
        << QString("2001-09-15T09:33:01.001-701") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:three-digit-minutes")
        << QString("2001-09-15T09:33:01.001+11:570") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:single-digit-minutes")
        << QString("2001-09-15T09:33:01.001+11:5") << QString("yyyy-MM-ddThh:mm:ss.zt")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:invalid-sign-symbol")
        << QString("2001-09-15T09:33:01.001 ~11:30") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:symbol-in-hours")
        << QString("2001-09-15T09:33:01.001 UTC+o8:30") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:symbol-in-minutes")
        << QString("2001-09-15T09:33:01.001 UTC+08:3i") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:UTC+123")  // Invalid offset (UTC and 3 digit format)
        << QString("2001-09-15T09:33:01.001 UTC+123") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:UTC+00005")  // Invalid offset with leading zeroes
        << QString("2001-09-15T09:33:01.001 UTC+00005") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:three-digit-with-colon-delimiter")
        << QString("2008-10-13 +123:11.50") << QString("yyyy-MM-dd t:hh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:double-colon-as-part-of-offset")
        << QString("2008-10-13 UTC+12::11.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:single-colon-as-part-of-offset")
        << QString("2008-10-13 UTC+12::11.50") << QString("yyyy-MM-dd t:hh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:starts-with-colon")
        << QString("2008-10-13 UTC+:59 11.50") << QString("yyyy-MM-dd t hh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:empty-offset")
        << QString("2008-10-13 UTC+ 11.50") << QString("yyyy-MM-dd t hh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:time-section-instead-of-offset")
        << QString("2008-10-13 UTC+11.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:missing-minutes-if-colon")
        << QString("2008-10-13 +05: 11.50") << QString("yyyy-MM-dd t hh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:1-digit-minutes-if-colon")
        << QString("2008-10-13 UTC+05:1 11.50") << QString("yyyy-MM-dd t hh.mm")
        << QDateTime();
    QTest::newRow("invalid-time-spec:random-symbol")
        << QString("2001-09-15T09:33:01.001 $") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-time-spec:random-digit")
        << QString("2001-09-15T09:33:01.001 1") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:merged-with-time")
        << QString("2008-10-13 UTC+0111.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime();
    QTest::newRow("invalid-offset-from-utc:with-colon-3-digit-merged-with-time")
        << QString("2008-10-13 UTC+01:011.50") << QString("yyyy-MM-dd thh.mm")
        << QDateTime();
    QTest::newRow("invalid-time-spec:empty")
        << QString("2001-09-15T09:33:01.001 ") << QString("yyyy-MM-ddThh:mm:ss.z t")
        << QDateTime();
#if QT_CONFIG(timezone)
    QTimeZone southBrazil("America/Sao_Paulo");
    if (southBrazil.isValid()) {
        QTest::newRow("spring-forward-midnight")
            << QString("2008-10-19 23:45.678 America/Sao_Paulo") << QString("yyyy-MM-dd mm:ss.zzz t")
            // That's in the hour skipped - expect the matching time after the spring-forward, in DST:
            << QDateTime(QDate(2008, 10, 19), QTime(1, 23, 45, 678), southBrazil);
    }

    QTimeZone berlintz("Europe/Berlin");
    if (berlintz.isValid()) {
        QTest::newRow("begin-of-high-summer-time-with-tz")
            << QString("1947-05-11 03:23:45.678 Europe/Berlin")
            << QString("yyyy-MM-dd hh:mm:ss.zzz t")
            // That's in the hour skipped - expecting an invalid DateTime
            << QDateTime(QDate(1947, 05, 11), QTime(3, 23, 45, 678), berlintz);
    }
#endif
    QTest::newRow("late") << QString("9999-12-31T23:59:59.999Z")
                          << QString("yyyy-MM-ddThh:mm:ss.zZ")
                          << QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59, 999));
    // Separators match /([^aAdhHMmstyz]*)/
    QTest::newRow("oddly-separated") // To show broken-separator's format is valid.
        << QStringLiteral("2018 wilful long working block relief 12-19T21:09 cruel blurb encore flux")
        << QStringLiteral("yyyy wilful long working block relief MM-ddThh:mm cruel blurb encore flux")
        << QDateTime(QDate(2018, 12, 19), QTime(21, 9));
    QTest::newRow("broken-separator")
        << QStringLiteral("2018 wilful")
        << QStringLiteral("yyyy wilful long working block relief MM-ddThh:mm cruel blurb encore flux")
        << QDateTime();
    QTest::newRow("broken-terminator")
        << QStringLiteral("2018 wilful long working block relief 12-19T21:09 cruel")
        << QStringLiteral("yyyy wilful long working block relief MM-ddThh:mm cruel blurb encore flux")
        << QDateTime();

    // test unicode
    QTest::newRow("unicode handling") << QString(u8"2005🤣06🤣28T07🤣57🤣30.001Z")
        << QString(u8"yyyy🤣MM🤣ddThh🤣mm🤣ss.zt")
        << QDateTime(QDate(2005, 06, 28), QTime(07, 57, 30, 1), Qt::UTC);

    // QTBUG-84349
    QTest::newRow("QTBUG-84349: positive sign in month")
            << QStringLiteral("9922+221102233Z") << QStringLiteral("yyyyMMddHHmmsst")
            << QDateTime();

    // fuzzer test
    QTest::newRow("integer overflow found by fuzzer")
            << QStringLiteral("EEE1200000MUB") << QStringLiteral("t")
            << QDateTime();
}

void tst_QDateTime::fromStringStringFormat()
{
    QFETCH(QString, string);
    QFETCH(QString, format);
    QFETCH(QDateTime, expected);

    QDateTime dt = QDateTime::fromString(string, format);

    QCOMPARE(dt, expected);
    if (expected.isValid()) {
        QCOMPARE(dt.timeSpec(), expected.timeSpec());
#if QT_CONFIG(timezone)
        if (expected.timeSpec() == Qt::TimeZone)
            QCOMPARE(dt.timeZone(), expected.timeZone());
#endif
        // OffsetFromUTC needs an offset check - we may as well do it for all:
        QCOMPARE(dt.offsetFromUtc(), expected.offsetFromUtc());
    } else {
        QCOMPARE(dt.isValid(), expected.isValid());
        QCOMPARE(dt.toMSecsSinceEpoch(), expected.toMSecsSinceEpoch());
    }
}

void tst_QDateTime::fromStringStringFormat_localTimeZone_data()
{
    QTest::addColumn<QByteArray>("localTimeZone");
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QDateTime>("expected");

#if QT_CONFIG(timezone)
    QTimeZone etcGmtWithOffset("Etc/GMT+3");
    if (etcGmtWithOffset.isValid()) {
        QTest::newRow("local-timezone-with-offset:Etc/GMT+3") << QByteArrayLiteral("GMT")
            << QString("2008-10-13 Etc/GMT+3 11.50") << QString("yyyy-MM-dd t hh.mm")
            << QDateTime(QDate(2008, 10, 13), QTime(11, 50), etcGmtWithOffset);
    }
    QTimeZone gmtWithOffset("GMT-2");
    if (gmtWithOffset.isValid()) {
        QTest::newRow("local-timezone-with-offset:GMT-2") << QByteArrayLiteral("GMT")
            << QString("2008-10-13 GMT-2 11.50") << QString("yyyy-MM-dd t hh.mm")
            << QDateTime(QDate(2008, 10, 13), QTime(11, 50), gmtWithOffset);
    }
    QTimeZone gmt("GMT");
    if (gmt.isValid()) {
        QTest::newRow("local-timezone-with-offset:GMT") << QByteArrayLiteral("GMT")
            << QString("2008-10-13 GMT 11.50") << QString("yyyy-MM-dd t hh.mm")
            << QDateTime(QDate(2008, 10, 13), QTime(11, 50), gmt);
    }
    QTimeZone helsinki("Europe/Helsinki");
    if (helsinki.isValid()) {
        // QTBUG-96861: QAsn1Element::toDateTime() tripped over an assert in
        // QTimeZonePrivate::dataForLocalTime() on macOS and iOS.
        // The first 20m 11s of 1921-05-01 were skipped, so the parser's attempt
        // to construct a local time after scanning yyMM tripped up on the start
        // of the day, when the zone backend lacked transition data.
        QTest::newRow("Helsinki-joins-EET")
            << QByteArrayLiteral("Europe/Helsinki")
            << QString("210506000000Z") << QString("yyMMddHHmmsst")
            << QDateTime(QDate(1921, 5, 6), QTime(0, 0), Qt::UTC);
    }
#endif
}

void tst_QDateTime::fromStringStringFormat_localTimeZone()
{
    QFETCH(QByteArray, localTimeZone);
    TimeZoneRollback useZone(localTimeZone);  // enforce test's time zone
    fromStringStringFormat();  // call basic fromStringStringFormat test
}
# endif // datetimeparser
#endif // datestring

void tst_QDateTime::offsetFromUtc()
{
    /* Check default value. */
    QCOMPARE(QDateTime().offsetFromUtc(), 0);

    // Offset constructor
    QDateTime dt1(QDate(2013, 1, 1), QTime(1, 0), Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);
#if QT_CONFIG(timezone)
    QVERIFY(dt1.timeZone().isValid());
#endif
    dt1 = QDateTime(QDate(2013, 1, 1), QTime(1, 0), Qt::OffsetFromUTC, -60 * 60);
    QCOMPARE(dt1.offsetFromUtc(), -60 * 60);

    // UTC should be 0 offset
    QDateTime dt2(QDate(2013, 1, 1), QTime(0, 0), Qt::UTC);
    QCOMPARE(dt2.offsetFromUtc(), 0);

    // LocalTime should vary
    if (zoneIsCET) {
        // Time definitely in Standard Time so 1 hour ahead
        QDateTime dt3(QDate(2013, 1, 1), QTime(0, 0), Qt::LocalTime);
        QCOMPARE(dt3.offsetFromUtc(), 1 * 60 * 60);
        // Time definitely in Daylight Time so 2 hours ahead
        QDateTime dt4(QDate(2013, 6, 1), QTime(0, 0), Qt::LocalTime);
        QCOMPARE(dt4.offsetFromUtc(), 2 * 60 * 60);
     } else {
         QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
     }

#if QT_CONFIG(timezone)
    QDateTime dt5(QDate(2013, 1, 1), QTime(0, 0), QTimeZone("Pacific/Auckland"));
    QCOMPARE(dt5.offsetFromUtc(), 46800);

    QDateTime dt6(QDate(2013, 6, 1), QTime(0, 0), QTimeZone("Pacific/Auckland"));
    QCOMPARE(dt6.offsetFromUtc(), 43200);
#endif
}

void tst_QDateTime::setOffsetFromUtc()
{
    /* Basic tests. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setTimeSpec(Qt::LocalTime);

        dt.setOffsetFromUtc(0);
        QCOMPARE(dt.offsetFromUtc(), 0);
        QCOMPARE(dt.timeSpec(), Qt::UTC);

        dt.setOffsetFromUtc(-100);
        QCOMPARE(dt.offsetFromUtc(), -100);
        QCOMPARE(dt.timeSpec(), Qt::OffsetFromUTC);
    }

    /* Test detaching. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        QDateTime dt2(dt);
        int offset2 = dt2.offsetFromUtc();

        dt.setOffsetFromUtc(501);

        QCOMPARE(dt.offsetFromUtc(), 501);
        QCOMPARE(dt2.offsetFromUtc(), offset2);
    }

    /* Check copying. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setOffsetFromUtc(502);
        QCOMPARE(dt.offsetFromUtc(), 502);

        QDateTime dt2(dt);
        QCOMPARE(dt2.offsetFromUtc(), 502);
    }

    /* Check assignment. */
    {
        QDateTime dt(QDateTime::currentDateTime());
        dt.setOffsetFromUtc(502);
        QDateTime dt2;
        dt2 = dt;

        QCOMPARE(dt2.offsetFromUtc(), 502);
    }

    // Check spec persists
    QDateTime dt1(QDate(2013, 1, 1), QTime(0, 0), Qt::OffsetFromUTC, 60 * 60);
    dt1.setMSecsSinceEpoch(123456789);
    QCOMPARE(dt1.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);
    dt1.setSecsSinceEpoch(123456789);
    QCOMPARE(dt1.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt1.offsetFromUtc(), 60 * 60);

    // Check datastream serialises the offset seconds
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << dt1;
    }
    QDateTime dt2;
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> dt2;
    }
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt2.offsetFromUtc(), 60 * 60);
}

void tst_QDateTime::toOffsetFromUtc()
{
    QDateTime dt1(QDate(2013, 1, 1), QTime(0, 0), Qt::UTC);

    QDateTime dt2 = dt1.toOffsetFromUtc(60 * 60);
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::OffsetFromUTC);
    QCOMPARE(dt2.date(), QDate(2013, 1, 1));
    QCOMPARE(dt2.time(), QTime(1, 0));

    dt2 = dt1.toOffsetFromUtc(0);
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::UTC);
    QCOMPARE(dt2.date(), QDate(2013, 1, 1));
    QCOMPARE(dt2.time(), QTime(0, 0));

    dt2 = dt1.toTimeSpec(Qt::OffsetFromUTC);
    QCOMPARE(dt2, dt1);
    QCOMPARE(dt2.timeSpec(), Qt::UTC);
    QCOMPARE(dt2.date(), QDate(2013, 1, 1));
    QCOMPARE(dt2.time(), QTime(0, 0));
}

void tst_QDateTime::zoneAtTime_data()
{
    QTest::addColumn<QByteArray>("ianaID");
    QTest::addColumn<QDate>("date");
    QTest::addColumn<int>("offset");
#define ADDROW(name, zone, date, offset) \
    QTest::newRow(name) << QByteArray(zone) << (date) << (offset)

    // Check DST handling around epoch:
    {
        QDate epoch(1970, 1, 1);
        ADDROW("epoch:UTC", "UTC", epoch, 0);
        // Paris and Berlin skipped DST around 1970; but Rome used it.
        ADDROW("epoch:CET", "Europe/Rome", epoch, 3600);
        ADDROW("epoch:PST", "America/Vancouver", epoch, -8 * 3600);
        ADDROW("epoch:EST", "America/New_York", epoch, -5 * 3600);
    }
    {
        // QDateTime now takes account of DST even before the epoch.
        QDate summer69(1969, 8, 15); // Woodstock started
        ADDROW("summer69:UTC", "UTC", summer69, 0);
        ADDROW("summer69:CET", "Europe/Rome", summer69, 2 * 3600);
        ADDROW("summer69:PST", "America/Vancouver", summer69, -7 * 3600);
        ADDROW("summer69:EST", "America/New_York", summer69, -4 * 3600);
    }
    {
        // ... and has always taken it into account since:
        QDate summer70(1970, 8, 26); // Isle of Wight festival
        ADDROW("summer70:UTC", "UTC", summer70, 0);
        ADDROW("summer70:CET", "Europe/Rome", summer70, 2 * 3600);
        ADDROW("summer70:PST", "America/Vancouver", summer70, -7 * 3600);
        ADDROW("summer70:EST", "America/New_York", summer70, -4 * 3600);
    }

#ifdef Q_OS_ANDROID // QTBUG-68835; gets offset 0 for the affected tests.
# define NONANDROIDROW(name, zone, date, offset)
#else
# define NONANDROIDROW(name, zone, date, offset) ADDROW(name, zone, date, offset)
#endif

#ifndef Q_OS_WIN
    // Bracket a few noteworthy transitions:
    ADDROW("before:ACWST", "Australia/Eucla", QDate(1974, 10, 26), 31500); // 8:45
    NONANDROIDROW("after:ACWST", "Australia/Eucla", QDate(1974, 10, 27), 35100); // 9:45
    NONANDROIDROW("before:NPT", "Asia/Kathmandu", QDate(1985, 12, 31), 19800); // 5:30
    ADDROW("after:NPT", "Asia/Kathmandu", QDate(1986, 1, 1), 20700); // 5:45
    // The two that have skipped a day (each):
    NONANDROIDROW("before:LINT", "Pacific/Kiritimati", QDate(1994, 12, 30), -36000);
    ADDROW("after:LINT", "Pacific/Kiritimati", QDate(1995, 1, 2), 14 * 3600);
    ADDROW("after:WST", "Pacific/Apia", QDate(2011, 12, 31), 14 * 3600);
#endif // MS lacks ACWST, NPT; doesn't grok date-line crossings; and Windows 7 lacks LINT.
    ADDROW("before:WST", "Pacific/Apia", QDate(2011, 12, 29), -36000);
#undef ADDROW
}

void tst_QDateTime::zoneAtTime()
{
#if QT_CONFIG(timezone)
    QFETCH(QByteArray, ianaID);
    QFETCH(QDate, date);
    QFETCH(int, offset);
    const QTime noon(12, 0);

    QTimeZone zone(ianaID);
    QVERIFY(zone.isValid());
    QCOMPARE(QDateTime(date, noon, zone).offsetFromUtc(), offset);
    QCOMPARE(zone.offsetFromUtc(QDateTime(date, noon, zone)), offset);
#else
    QSKIP("Needs timezone feature enabled");
#endif
}

void tst_QDateTime::timeZoneAbbreviation()
{
    QDateTime dt1(QDate(2013, 1, 1), QTime(1, 0), Qt::OffsetFromUTC, 60 * 60);
    QCOMPARE(dt1.timeZoneAbbreviation(), QString("UTC+01:00"));
    QDateTime dt2(QDate(2013, 1, 1), QTime(1, 0), Qt::OffsetFromUTC, -60 * 60);
    QCOMPARE(dt2.timeZoneAbbreviation(), QString("UTC-01:00"));

    QDateTime dt3(QDate(2013, 1, 1), QTime(0, 0), Qt::UTC);
    QCOMPARE(dt3.timeZoneAbbreviation(), QString("UTC"));

    // LocalTime should vary
    if (zoneIsCET) {
        // Time definitely in Standard Time
        QDateTime dt4(QDate(2013, 1, 1), QTime(0, 0), Qt::LocalTime);
#ifdef Q_OS_WIN
        QEXPECT_FAIL("", "Windows only reports long name (QTBUG-32759)", Continue);
#endif
        QCOMPARE(dt4.timeZoneAbbreviation(), QStringLiteral("CET"));
        // Time definitely in Daylight Time
        QDateTime dt5(QDate(2013, 6, 1), QTime(0, 0), Qt::LocalTime);
#ifdef Q_OS_WIN
        QEXPECT_FAIL("", "Windows only reports long name (QTBUG-32759)", Continue);
#endif
        QCOMPARE(dt5.timeZoneAbbreviation(), QStringLiteral("CEST"));
    } else {
        qDebug("(Skipped some CET-only tests)");
    }

#if QT_CONFIG(timezone)
    const QTimeZone berlin("Europe/Berlin");
    const QDateTime jan(QDate(2013, 1, 1).startOfDay(berlin));
    const QDateTime jul(QDate(2013, 7, 1).startOfDay(berlin));

    QCOMPARE(jan.timeZoneAbbreviation(), berlin.abbreviation(jan));
    QCOMPARE(jul.timeZoneAbbreviation(), berlin.abbreviation(jul));
#endif
}

void tst_QDateTime::getDate()
{
    {
    int y = -33, m = -44, d = -55;
    QDate date;
    date.getDate(&y, &m, &d);
    QCOMPARE(date.year(), y);
    QCOMPARE(date.month(), m);
    QCOMPARE(date.day(), d);

    date.getDate(0, 0, 0);
    }

    {
    int y = -33, m = -44, d = -55;
    QDate date(1998, 5, 24);
    date.getDate(0, &m, 0);
    date.getDate(&y, 0, 0);
    date.getDate(0, 0, &d);

    QCOMPARE(date.year(), y);
    QCOMPARE(date.month(), m);
    QCOMPARE(date.day(), d);
    }
}

void tst_QDateTime::fewDigitsInYear() const
{
    const QDateTime three(QDate(300, 10, 11).startOfDay());
    QCOMPARE(three.toString(QLatin1String("yyyy-MM-dd")), QString::fromLatin1("0300-10-11"));

    const QDateTime two(QDate(20, 10, 11).startOfDay());
    QCOMPARE(two.toString(QLatin1String("yyyy-MM-dd")), QString::fromLatin1("0020-10-11"));

    const QDateTime yyTwo(QDate(30, 10, 11).startOfDay());
    QCOMPARE(yyTwo.toString(QLatin1String("yy-MM-dd")), QString::fromLatin1("30-10-11"));

    const QDateTime yyOne(QDate(4, 10, 11).startOfDay());
    QCOMPARE(yyOne.toString(QLatin1String("yy-MM-dd")), QString::fromLatin1("04-10-11"));
}

void tst_QDateTime::printNegativeYear() const
{
    {
        QDateTime date(QDate(-20, 10, 11).startOfDay());
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0020"));
    }

    {
        QDateTime date(QDate(-3, 10, 11).startOfDay());
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0003"));
    }

    {
        QDateTime date(QDate(-400, 10, 11).startOfDay());
        QVERIFY(date.isValid());
        QCOMPARE(date.toString(QLatin1String("yyyy")), QString::fromLatin1("-0400"));
    }
}

#if QT_CONFIG(datetimeparser)
void tst_QDateTime::roundtripTextDate() const
{
    /* This code path should not result in warnings. */
    const QDateTime now(QDateTime::currentDateTime());
    // TextDate drops millis:
    const QDateTime theDateTime(now.addMSecs(-now.time().msec()));
    QCOMPARE(QDateTime::fromString(theDateTime.toString(Qt::TextDate), Qt::TextDate), theDateTime);
}
#endif

void tst_QDateTime::utcOffsetLessThan() const
{
    QDateTime dt1(QDate(2002, 10, 10), QTime(0, 0));
    QDateTime dt2(dt1);

    dt1.setOffsetFromUtc(-(2 * 60 * 60)); // Minus two hours.
    dt2.setOffsetFromUtc(-(3 * 60 * 60)); // Minus three hours.

    QVERIFY(dt1 != dt2);
    QVERIFY(!(dt1 == dt2));
    QVERIFY(dt1 < dt2);
    QVERIFY(!(dt2 < dt1));
}

void tst_QDateTime::isDaylightTime() const
{
    QDateTime utc1(QDate(2012, 1, 1), QTime(0, 0), Qt::UTC);
    QVERIFY(!utc1.isDaylightTime());
    QDateTime utc2(QDate(2012, 6, 1), QTime(0, 0), Qt::UTC);
    QVERIFY(!utc2.isDaylightTime());

    QDateTime offset1(QDate(2012, 1, 1), QTime(0, 0), Qt::OffsetFromUTC, 1 * 60 * 60);
    QVERIFY(!offset1.isDaylightTime());
    QDateTime offset2(QDate(2012, 6, 1), QTime(0, 0), Qt::OffsetFromUTC, 1 * 60 * 60);
    QVERIFY(!offset2.isDaylightTime());

    if (zoneIsCET) {
        QDateTime cet1(QDate(2012, 1, 1), QTime(0, 0));
        QVERIFY(!cet1.isDaylightTime());
        QDateTime cet2(QDate(2012, 6, 1), QTime(0, 0));
        QVERIFY(cet2.isDaylightTime());
    } else {
        QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
    }
}

void tst_QDateTime::daylightTransitions() const
{
    if (zoneIsCET) {
        // CET transitions occur at 01:00:00 UTC on last Sunday in March and October
        // 2011-03-27 02:00:00 CET  became 03:00:00 CEST at msecs = 1301187600000
        // 2011-10-30 03:00:00 CEST became 02:00:00 CET  at msecs = 1319936400000
        // 2012-03-25 02:00:00 CET  became 03:00:00 CEST at msecs = 1332637200000
        // 2012-10-28 03:00:00 CEST became 02:00:00 CET  at msecs = 1351386000000
        const qint64 daylight2012 = 1332637200000;
        const qint64 standard2012 = 1351386000000;
        const qint64 msecsOneHour = 3600000;

        // Test for correct behviour for StandardTime -> DaylightTime transition, i.e. missing hour

        // Test setting date, time in missing hour will be invalid

        QDateTime before(QDate(2012, 3, 25), QTime(1, 59, 59, 999));
        QVERIFY(before.isValid());
        QCOMPARE(before.date(), QDate(2012, 3, 25));
        QCOMPARE(before.time(), QTime(1, 59, 59, 999));
        QCOMPARE(before.toMSecsSinceEpoch(), daylight2012 - 1);

        QDateTime missing(QDate(2012, 3, 25), QTime(2, 0));
        QVERIFY(!missing.isValid());
        QCOMPARE(missing.date(), QDate(2012, 3, 25));
        QCOMPARE(missing.time(), QTime(2, 0));
        // datetimeparser relies on toMSecsSinceEpoch to still work:
        QCOMPARE(missing.toMSecsSinceEpoch(), daylight2012);

        QDateTime after(QDate(2012, 3, 25), QTime(3, 0));
        QVERIFY(after.isValid());
        QCOMPARE(after.date(), QDate(2012, 3, 25));
        QCOMPARE(after.time(), QTime(3, 0));
        QCOMPARE(after.toMSecsSinceEpoch(), daylight2012);

        // Test round-tripping of msecs

        before.setMSecsSinceEpoch(daylight2012 - 1);
        QVERIFY(before.isValid());
        QCOMPARE(before.date(), QDate(2012, 3, 25));
        QCOMPARE(before.time(), QTime(1, 59, 59, 999));
        QCOMPARE(before.toMSecsSinceEpoch(), daylight2012 -1);

        after.setMSecsSinceEpoch(daylight2012);
        QVERIFY(after.isValid());
        QCOMPARE(after.date(), QDate(2012, 3, 25));
        QCOMPARE(after.time(), QTime(3, 0));
        QCOMPARE(after.toMSecsSinceEpoch(), daylight2012);

        // Test changing time spec re-validates the date/time

        QDateTime utc(QDate(2012, 3, 25), QTime(2, 00, 0), Qt::UTC);
        QVERIFY(utc.isValid());
        QCOMPARE(utc.date(), QDate(2012, 3, 25));
        QCOMPARE(utc.time(), QTime(2, 0));
        utc.setTimeSpec(Qt::LocalTime);
        QVERIFY(!utc.isValid());
        QCOMPARE(utc.date(), QDate(2012, 3, 25));
        QCOMPARE(utc.time(), QTime(2, 0));
        utc.setTimeSpec(Qt::UTC);
        QVERIFY(utc.isValid());
        QCOMPARE(utc.date(), QDate(2012, 3, 25));
        QCOMPARE(utc.time(), QTime(2, 0));

        // Test date maths, if result falls in missing hour then becomes next
        // hour (or is always invalid; mktime() may reject gap-times).

        QDateTime test(QDate(2011, 3, 25), QTime(2, 0));
        QVERIFY(test.isValid());
        test = test.addYears(1);
        const bool handled = test.isValid();
#define CHECK_SPRING_FORWARD(test) \
            if (test.isValid()) { \
                QCOMPARE(test.date(), QDate(2012, 3, 25)); \
                QCOMPARE(test.time(), QTime(3, 0)); \
            } else { \
                QVERIFY(!handled); \
            }
        CHECK_SPRING_FORWARD(test);

        test = QDateTime(QDate(2012, 2, 25), QTime(2, 0));
        QVERIFY(test.isValid());
        test = test.addMonths(1);
        CHECK_SPRING_FORWARD(test);

        test = QDateTime(QDate(2012, 3, 24), QTime(2, 0));
        QVERIFY(test.isValid());
        test = test.addDays(1);
        CHECK_SPRING_FORWARD(test);

        test = QDateTime(QDate(2012, 3, 25), QTime(1, 0));
        QVERIFY(test.isValid());
        QCOMPARE(test.toMSecsSinceEpoch(), daylight2012 - msecsOneHour);
        test = test.addMSecs(msecsOneHour);
        CHECK_SPRING_FORWARD(test);
        if (handled)
            QCOMPARE(test.toMSecsSinceEpoch(), daylight2012);
#undef CHECK_SPRING_FORWARD

        // Test for correct behviour for DaylightTime -> StandardTime transition, i.e. second occurrence

        // Test setting date and time in first and second occurrence will be valid

        // 1 hour before transition is 2:00:00 FirstOccurrence
        QDateTime hourBefore(QDate(2012, 10, 28), QTime(2, 0));
        QVERIFY(hourBefore.isValid());
        QCOMPARE(hourBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(hourBefore.time(), QTime(2, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(hourBefore.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // 1 msec before transition is 2:59:59.999 FirstOccurrence
        QDateTime msecBefore(QDate(2012, 10, 28), QTime(2, 59, 59, 999));
        QVERIFY(msecBefore.isValid());
        QCOMPARE(msecBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(msecBefore.time(), QTime(2, 59, 59, 999));
#if defined(Q_OS_DARWIN) || defined(Q_OS_WIN) || defined(Q_OS_QNX) || defined(Q_OS_ANDROID)
        // Win and Mac uses SecondOccurrence here
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_MAC
        QCOMPARE(msecBefore.toMSecsSinceEpoch(), standard2012 - 1);

        // At transition is 2:00:00 SecondOccurrence
        QDateTime atTran(QDate(2012, 10, 28), QTime(2, 0));
        QVERIFY(atTran.isValid());
        QCOMPARE(atTran.date(), QDate(2012, 10, 28));
        QCOMPARE(atTran.time(), QTime(2, 0));
#ifndef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(atTran.toMSecsSinceEpoch(), standard2012);

        // 59:59.999 after transition is 2:59:59.999 SecondOccurrence
        QDateTime afterTran(QDate(2012, 10, 28), QTime(2, 59, 59, 999));
        QVERIFY(afterTran.isValid());
        QCOMPARE(afterTran.date(), QDate(2012, 10, 28));
        QCOMPARE(afterTran.time(), QTime(2, 59, 59, 999));
#ifdef __GLIBCXX__
        // Linux (i.e. glibc) mktime bug reuses last calculation
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_UNIX
        QCOMPARE(afterTran.toMSecsSinceEpoch(), standard2012 + msecsOneHour - 1);

        // 1 hour after transition is 3:00:00 FirstOccurrence
        QDateTime hourAfter(QDate(2012, 10, 28), QTime(3, 0));
        QVERIFY(hourAfter.isValid());
        QCOMPARE(hourAfter.date(), QDate(2012, 10, 28));
        QCOMPARE(hourAfter.time(), QTime(3, 0));
        QCOMPARE(hourAfter.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Test round-tripping of msecs

        // 1 hour before transition is 2:00:00 FirstOccurrence
        hourBefore.setMSecsSinceEpoch(standard2012 - msecsOneHour);
        QVERIFY(hourBefore.isValid());
        QCOMPARE(hourBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(hourBefore.time(), QTime(2, 0));
        QCOMPARE(hourBefore.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // 1 msec before transition is 2:59:59.999 FirstOccurrence
        msecBefore.setMSecsSinceEpoch(standard2012 - 1);
        QVERIFY(msecBefore.isValid());
        QCOMPARE(msecBefore.date(), QDate(2012, 10, 28));
        QCOMPARE(msecBefore.time(), QTime(2, 59, 59, 999));
        QCOMPARE(msecBefore.toMSecsSinceEpoch(), standard2012 - 1);

        // At transition is 2:00:00 SecondOccurrence
        atTran.setMSecsSinceEpoch(standard2012);
        QVERIFY(atTran.isValid());
        QCOMPARE(atTran.date(), QDate(2012, 10, 28));
        QCOMPARE(atTran.time(), QTime(2, 0));
        QCOMPARE(atTran.toMSecsSinceEpoch(), standard2012);

        // 59:59.999 after transition is 2:59:59.999 SecondOccurrence
        afterTran.setMSecsSinceEpoch(standard2012 + msecsOneHour - 1);
        QVERIFY(afterTran.isValid());
        QCOMPARE(afterTran.date(), QDate(2012, 10, 28));
        QCOMPARE(afterTran.time(), QTime(2, 59, 59, 999));
        QCOMPARE(afterTran.toMSecsSinceEpoch(), standard2012 + msecsOneHour - 1);

        // 1 hour after transition is 3:00:00 FirstOccurrence
        hourAfter.setMSecsSinceEpoch(standard2012 + msecsOneHour);
        QVERIFY(hourAfter.isValid());
        QCOMPARE(hourAfter.date(), QDate(2012, 10, 28));
        QCOMPARE(hourAfter.time(), QTime(3, 0));
        QCOMPARE(hourAfter.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Test date maths, result is always FirstOccurrence

        // Add year to get to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 28), QTime(2, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add year to get to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 28), QTime(3, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(3, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Add year to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 30));
        QCOMPARE(test.time(), QTime(2, 0));

        // Add year to tran SecondOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0)); // TODO SecondOccurrence
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 30));
        QCOMPARE(test.time(), QTime(2, 0));

        // Add year to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(3, 0));
        test = test.addYears(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 30));
        QCOMPARE(test.time(), QTime(3, 0));


        // Add month to get to tran FirstOccurrence
        test = QDateTime(QDate(2012, 9, 28), QTime(2, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add month to get to after tran FirstOccurrence
        test = QDateTime(QDate(2012, 9, 28), QTime(3, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(3, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Add month to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 11, 30));
        QCOMPARE(test.time(), QTime(2, 0));

        // Add month to tran SecondOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0)); // TODO SecondOccurrence
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 11, 30));
        QCOMPARE(test.time(), QTime(2, 0));

        // Add month to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(3, 0));
        test = test.addMonths(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 11, 30));
        QCOMPARE(test.time(), QTime(3, 0));


        // Add day to get to tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 27), QTime(2, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add day to get to after tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 27), QTime(3, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(3, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

        // Add day to tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 10, 31));
        QCOMPARE(test.time(), QTime(2, 0));

        // Add day to tran SecondOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(2, 0)); // TODO SecondOccurrence
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 10, 31));
        QCOMPARE(test.time(), QTime(2, 0));

        // Add day to after tran FirstOccurrence
        test = QDateTime(QDate(2011, 10, 30), QTime(3, 0));
        test = test.addDays(1);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2011, 10, 31));
        QCOMPARE(test.time(), QTime(3, 0));


        // Add hour to get to tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 28), QTime(1, 0));
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
        QCOMPARE(test.time(), QTime(2, 0));
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 - msecsOneHour);

        // Add hour to tran FirstOccurrence to get to tran SecondOccurrence
        test = QDateTime(QDate(2012, 10, 28), QTime(2, 0));
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.time(), QTime(2, 0));
#ifdef Q_OS_WIN
        // Windows uses SecondOccurrence
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012);

        // Add hour to tran SecondOccurrence to get to after tran FirstOccurrence
        test = QDateTime(QDate(2012, 10, 28), QTime(2, 0)); // TODO SecondOccurrence
        test = test.addMSecs(msecsOneHour);
        QVERIFY(test.isValid());
        QCOMPARE(test.date(), QDate(2012, 10, 28));
#if defined(Q_OS_DARWIN) || defined(Q_OS_QNX) || defined(Q_OS_ANDROID)
        // Mac uses FirstOccurrence, Windows uses SecondOccurrence, Linux uses last calculation
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.time(), QTime(3, 0));
#if defined(Q_OS_DARWIN) || defined(Q_OS_QNX) || defined(Q_OS_ANDROID)
        // Mac uses FirstOccurrence, Windows uses SecondOccurrence, Linux uses last calculation
        QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
#endif // Q_OS_WIN
        QCOMPARE(test.toMSecsSinceEpoch(), standard2012 + msecsOneHour);

    } else {
        QSKIP("You must test using Central European (CET/CEST) time zone, e.g. TZ=Europe/Oslo");
    }
}

void tst_QDateTime::timeZones() const
{
#if QT_CONFIG(timezone)
    QTimeZone invalidTz = QTimeZone("Vulcan/ShiKahr");
    QCOMPARE(invalidTz.isValid(), false);
    QDateTime invalidDateTime = QDateTime(QDate(2000, 1, 1), QTime(0, 0), invalidTz);
    QCOMPARE(invalidDateTime.isValid(), false);
    QCOMPARE(invalidDateTime.date(), QDate(2000, 1, 1));
    QCOMPARE(invalidDateTime.time(), QTime(0, 0));

    QTimeZone nzTz = QTimeZone("Pacific/Auckland");
    QTimeZone nzTzOffset = QTimeZone(12 * 3600);

    // During Standard Time NZ is +12:00
    QDateTime utcStd(QDate(2012, 6, 1), QTime(0, 0), Qt::UTC);
    QDateTime nzStd(QDate(2012, 6, 1), QTime(12, 0), nzTz);
    QDateTime nzStdOffset(QDate(2012, 6, 1), QTime(12, 0), nzTzOffset);

    QCOMPARE(nzStd.isValid(), true);
    QCOMPARE(nzStd.timeSpec(), Qt::TimeZone);
    QCOMPARE(nzStd.date(), QDate(2012, 6, 1));
    QCOMPARE(nzStd.time(), QTime(12, 0));
    QVERIFY(nzStd.timeZone() == nzTz);
    QCOMPARE(nzStd.timeZone().id(), QByteArray("Pacific/Auckland"));
    QCOMPARE(nzStd.offsetFromUtc(), 43200);
    QCOMPARE(nzStd.isDaylightTime(), false);
    QCOMPARE(nzStd.toMSecsSinceEpoch(), utcStd.toMSecsSinceEpoch());

    QCOMPARE(nzStdOffset.isValid(), true);
    QCOMPARE(nzStdOffset.timeSpec(), Qt::TimeZone);
    QCOMPARE(nzStdOffset.date(), QDate(2012, 6, 1));
    QCOMPARE(nzStdOffset.time(), QTime(12, 0));
    QVERIFY(nzStdOffset.timeZone() == nzTzOffset);
    QCOMPARE(nzStdOffset.timeZone().id(), QByteArray("UTC+12"));
    QCOMPARE(nzStdOffset.offsetFromUtc(), 43200);
    QCOMPARE(nzStdOffset.isDaylightTime(), false);
    QCOMPARE(nzStdOffset.toMSecsSinceEpoch(), utcStd.toMSecsSinceEpoch());

    // During Daylight Time NZ is +13:00
    QDateTime utcDst(QDate(2012, 1, 1), QTime(0, 0), Qt::UTC);
    QDateTime nzDst(QDate(2012, 1, 1), QTime(13, 0), nzTz);

    QCOMPARE(nzDst.isValid(), true);
    QCOMPARE(nzDst.date(), QDate(2012, 1, 1));
    QCOMPARE(nzDst.time(), QTime(13, 0));
    QCOMPARE(nzDst.offsetFromUtc(), 46800);
    QCOMPARE(nzDst.isDaylightTime(), true);
    QCOMPARE(nzDst.toMSecsSinceEpoch(), utcDst.toMSecsSinceEpoch());

    QDateTime utc = nzStd.toUTC();
    QCOMPARE(utc.date(), utcStd.date());
    QCOMPARE(utc.time(), utcStd.time());

    utc = nzDst.toUTC();
    QCOMPARE(utc.date(), utcDst.date());
    QCOMPARE(utc.time(), utcDst.time());

    // Crash test, QTBUG-80146:
    QVERIFY(!nzStd.toTimeZone(QTimeZone()).isValid());

    // Sydney is 2 hours behind New Zealand
    QTimeZone ausTz = QTimeZone("Australia/Sydney");
    QDateTime aus = nzStd.toTimeZone(ausTz);
    QCOMPARE(aus.date(), QDate(2012, 6, 1));
    QCOMPARE(aus.time(), QTime(10, 0));

    QDateTime dt1(QDate(2012, 6, 1), QTime(0, 0), Qt::UTC);
    QCOMPARE(dt1.timeSpec(), Qt::UTC);
    dt1.setTimeZone(nzTz);
    QCOMPARE(dt1.timeSpec(), Qt::TimeZone);
    QCOMPARE(dt1.date(), QDate(2012, 6, 1));
    QCOMPARE(dt1.time(), QTime(0, 0));
    QCOMPARE(dt1.timeZone(), nzTz);

    QDateTime dt2 = QDateTime::fromSecsSinceEpoch(1338465600, nzTz);
    QCOMPARE(dt2.date(), dt1.date());
    QCOMPARE(dt2.time(), dt1.time());
    QCOMPARE(dt2.timeSpec(), dt1.timeSpec());
    QCOMPARE(dt2.timeZone(), dt1.timeZone());

    QDateTime dt3 = QDateTime::fromMSecsSinceEpoch(1338465600000, nzTz);
    QCOMPARE(dt3.date(), dt1.date());
    QCOMPARE(dt3.time(), dt1.time());
    QCOMPARE(dt3.timeSpec(), dt1.timeSpec());
    QCOMPARE(dt3.timeZone(), dt1.timeZone());

    // The start of year 1 should be *describable* in any zone (QTBUG-78051)
    dt3 = QDateTime(QDate(1, 1, 1), QTime(0, 0), ausTz);
    QVERIFY(dt3.isValid());
    // Likewise the end of year -1 (a.k.a. 1 BCE).
    dt3 = dt3.addMSecs(-1);
    QVERIFY(dt3.isValid());
    QCOMPARE(dt3, QDateTime(QDate(-1, 12, 31), QTime(23, 59, 59, 999), ausTz));

    // Check datastream serialises the time zone
    QByteArray tmp;
    {
        QDataStream ds(&tmp, QIODevice::WriteOnly);
        ds << dt1;
    }
    QDateTime dt4;
    {
        QDataStream ds(&tmp, QIODevice::ReadOnly);
        ds >> dt4;
    }
    QCOMPARE(dt4, dt1);
    QCOMPARE(dt4.timeSpec(), Qt::TimeZone);
    QCOMPARE(dt4.timeZone(), nzTz);

    // Check handling of transition times
    QTimeZone cet("Europe/Oslo");

    // Standard Time to Daylight Time 2013 on 2013-03-31 is 2:00 local time / 1:00 UTC
    qint64 stdToDstMSecs = 1364691600000;

    // Test MSecs to local
    // - Test 1 msec before tran = 01:59:59.999
    QDateTime beforeDst = QDateTime::fromMSecsSinceEpoch(stdToDstMSecs - 1, cet);
    QCOMPARE(beforeDst.date(), QDate(2013, 3, 31));
    QCOMPARE(beforeDst.time(), QTime(1, 59, 59, 999));
    // - Test at tran = 03:00:00
    QDateTime atDst = QDateTime::fromMSecsSinceEpoch(stdToDstMSecs, cet);
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(3, 0));

    // Test local to MSecs
    // - Test 1 msec before tran = 01:59:59.999
    beforeDst = QDateTime(QDate(2013, 3, 31), QTime(1, 59, 59, 999), cet);
    QCOMPARE(beforeDst.toMSecsSinceEpoch(), stdToDstMSecs - 1);
    // - Test at tran = 03:00:00
    atDst = QDateTime(QDate(2013, 3, 31), QTime(3, 0), cet);
    QCOMPARE(atDst.toMSecsSinceEpoch(), stdToDstMSecs);
    // - Test transition hole, setting 03:00:00 is valid
    atDst = QDateTime(QDate(2013, 3, 31), QTime(3, 0), cet);
    QVERIFY(atDst.isValid());
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(3, 0));
    QCOMPARE(atDst.toMSecsSinceEpoch(), stdToDstMSecs);
    // - Test transition hole, setting 02:00:00 is invalid
    atDst = QDateTime(QDate(2013, 3, 31), QTime(2, 0), cet);
    QVERIFY(!atDst.isValid());
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(2, 0));
    // - Test transition hole, setting 02:59:59.999 is invalid
    atDst = QDateTime(QDate(2013, 3, 31), QTime(2, 59, 59, 999), cet);
    QVERIFY(!atDst.isValid());
    QCOMPARE(atDst.date(), QDate(2013, 3, 31));
    QCOMPARE(atDst.time(), QTime(2, 59, 59, 999));

    // Standard Time to Daylight Time 2013 on 2013-10-27 is 3:00 local time / 1:00 UTC
    qint64 dstToStdMSecs = 1382835600000;

    // Test MSecs to local
    // - Test 1 hour before tran = 02:00:00 local first occurrence
    QDateTime hourBeforeStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs - 3600000, cet);
    QCOMPARE(hourBeforeStd.date(), QDate(2013, 10, 27));
    QCOMPARE(hourBeforeStd.time(), QTime(2, 0));
    // - Test 1 msec before tran = 02:59:59.999 local first occurrence
    QDateTime msecBeforeStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs - 1, cet);
    QCOMPARE(msecBeforeStd.date(), QDate(2013, 10, 27));
    QCOMPARE(msecBeforeStd.time(), QTime(2, 59, 59, 999));
    // - Test at tran = 03:00:00 local becomes 02:00:00 local second occurrence
    QDateTime atStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs, cet);
    QCOMPARE(atStd.date(), QDate(2013, 10, 27));
    QCOMPARE(atStd.time(), QTime(2, 0));
    // - Test 59 mins after tran = 02:59:59.999 local second occurrence
    QDateTime afterStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs + 3600000 -1, cet);
    QCOMPARE(afterStd.date(), QDate(2013, 10, 27));
    QCOMPARE(afterStd.time(), QTime(2, 59, 59, 999));
    // - Test 1 hour after tran = 03:00:00 local
    QDateTime hourAfterStd = QDateTime::fromMSecsSinceEpoch(dstToStdMSecs + 3600000, cet);
    QCOMPARE(hourAfterStd.date(), QDate(2013, 10, 27));
    QCOMPARE(hourAfterStd.time(), QTime(3, 00, 00));

    // Test local to MSecs
    // - Test first occurrence 02:00:00 = 1 hour before tran
    hourBeforeStd = QDateTime(QDate(2013, 10, 27), QTime(2, 0), cet);
    QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
    QCOMPARE(hourBeforeStd.toMSecsSinceEpoch(), dstToStdMSecs - 3600000);
    // - Test first occurrence 02:59:59.999 = 1 msec before tran
    msecBeforeStd = QDateTime(QDate(2013, 10, 27), QTime(2, 59, 59, 999), cet);
    QEXPECT_FAIL("", "QDateTime doesn't properly support Daylight Transitions", Continue);
    QCOMPARE(msecBeforeStd.toMSecsSinceEpoch(), dstToStdMSecs - 1);
    // - Test second occurrence 02:00:00 = at tran
    atStd = QDateTime(QDate(2013, 10, 27), QTime(2, 0), cet);
    QCOMPARE(atStd.toMSecsSinceEpoch(), dstToStdMSecs);
    // - Test second occurrence 03:00:00 = 59 mins after tran
    afterStd = QDateTime(QDate(2013, 10, 27), QTime(2, 59, 59, 999), cet);
    QCOMPARE(afterStd.toMSecsSinceEpoch(), dstToStdMSecs + 3600000 - 1);
    // - Test 03:00:00 = 1 hour after tran
    hourAfterStd = QDateTime(QDate(2013, 10, 27), QTime(3, 0), cet);
    QCOMPARE(hourAfterStd.toMSecsSinceEpoch(), dstToStdMSecs + 3600000);

    // Test Time Zone that has transitions but no future transitions afer a given date
    QTimeZone sgt("Asia/Singapore");
    QDateTime future(QDate(2015, 1, 1), QTime(0, 0), sgt);
    QVERIFY(future.isValid());
    QCOMPARE(future.offsetFromUtc(), 28800);
#else
    QSKIP("Needs timezone feature enabled");
#endif
}

void tst_QDateTime::systemTimeZoneChange_data() const
{
    QTest::addColumn<QDate>("date");
    QTest::newRow("short") << QDate(1970, 1, 1);
    QTest::newRow("2012") << QDate(2012, 6, 1); // short on 64-bit, pimpled on 32-bit
    QTest::newRow("pimpled") << QDate(1150000, 6, 1);
}

void tst_QDateTime::systemTimeZoneChange() const
{
    QFETCH(const QDate, date);
    const QTime early(2, 15, 30);

    // Start out in Brisbane time:
    TimeZoneRollback useZone(QByteArray("AEST-10:00"));
    if (QDateTime(date, early, Qt::LocalTime).offsetFromUtc() != 600 * 60)
        QSKIP("Test depends on system support for changing zone to AEST-10:00");
#if QT_CONFIG(timezone)
    QVERIFY(QTimeZone::systemTimeZone().isValid());
#endif

    const QDateTime localDate = QDateTime(date, early, Qt::LocalTime);
    const QDateTime utcDate = QDateTime(date, early, Qt::UTC);
    const qint64 localMsecs = localDate.toMSecsSinceEpoch();
    const qint64 utcMsecs = utcDate.toMSecsSinceEpoch();
#if QT_CONFIG(timezone)
    const QTimeZone aest("Australia/Brisbane"); // no transitions since 1992
    // Check that Australia/Brisbane is known:
    QVERIFY(aest.isValid());
    const QDateTime tzDate = QDateTime(date, early, aest);

    // Check we got the right zone !
    QVERIFY(tzDate.timeZone().isValid());
    QCOMPARE(tzDate.timeZone(), aest);
    const qint64 tzMsecs = tzDate.toMSecsSinceEpoch();
#endif

    // Change to Indian time
    useZone.reset(QByteArray("IST-05:30"));
    if (QDateTime(date, early, Qt::LocalTime).offsetFromUtc() != 330 * 60)
        QSKIP("Test depends on system support for changing zone to IST-05:30");
#if QT_CONFIG(timezone)
    QVERIFY(QTimeZone::systemTimeZone().isValid());
#endif

    QCOMPARE(localDate, QDateTime(date, early, Qt::LocalTime));
    // Note: localDate.toMSecsSinceEpoch == localMsecs, unchanged, iff localDate is pimpled.
    QVERIFY(localMsecs != QDateTime(date, early, Qt::LocalTime).toMSecsSinceEpoch());
    QCOMPARE(utcDate, QDateTime(date, early, Qt::UTC));
    QCOMPARE(utcDate.toMSecsSinceEpoch(), utcMsecs);
#if QT_CONFIG(timezone)
    QCOMPARE(tzDate.toMSecsSinceEpoch(), tzMsecs);
    QCOMPARE(tzDate.timeZone(), aest);
    QCOMPARE(tzDate, QDateTime(date, early, aest));
#endif
}

void tst_QDateTime::invalid_data() const
{
    QTest::addColumn<QDateTime>("when");
    QTest::addColumn<Qt::TimeSpec>("spec");
    QTest::addColumn<bool>("goodZone");
    QTest::newRow("default") << QDateTime() << Qt::LocalTime << true;

    QDateTime invalidDate = QDateTime(QDate(0, 0, 0), QTime(-1, -1, -1));
    QTest::newRow("simple") << invalidDate << Qt::LocalTime << true;
    QTest::newRow("UTC") << invalidDate.toUTC() << Qt::UTC << true;
    QTest::newRow("offset")
        << invalidDate.toOffsetFromUtc(3600) << Qt::OffsetFromUTC << true;
#if QT_CONFIG(timezone)
    QTest::newRow("CET")
        << invalidDate.toTimeZone(QTimeZone("Europe/Oslo")) << Qt::TimeZone << true;

    // Crash tests, QTBUG-80146:
    QTest::newRow("nozone+construct")
        << QDateTime(QDate(1970, 1, 1), QTime(12, 0), QTimeZone()) << Qt::TimeZone << false;
    QTest::newRow("nozone+fromMSecs")
        << QDateTime::fromMSecsSinceEpoch(42, QTimeZone()) << Qt::TimeZone << false;
    QDateTime valid(QDate(1970, 1, 1), QTime(12, 0), Qt::UTC);
    QTest::newRow("tonozone") << valid.toTimeZone(QTimeZone()) << Qt::TimeZone << false;
#endif
}

void tst_QDateTime::invalid() const
{
    QFETCH(QDateTime, when);
    QFETCH(Qt::TimeSpec, spec);
    QFETCH(bool, goodZone);
    QVERIFY(!when.isValid());
    QCOMPARE(when.timeSpec(), spec);
    QCOMPARE(when.timeZoneAbbreviation(), QString());
    if (!goodZone)
        QCOMPARE(when.toMSecsSinceEpoch(), 0);
    QVERIFY(!when.isDaylightTime());
#if QT_CONFIG(timezone)
    QCOMPARE(when.timeZone().isValid(), goodZone);
#endif
}

void tst_QDateTime::range() const
{
    using Bounds = std::numeric_limits<qint64>;
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(Bounds::min() + 1, Qt::UTC).date().year(),
             int(QDateTime::YearRange::First));
    QCOMPARE(QDateTime::fromMSecsSinceEpoch(Bounds::max() - 1, Qt::UTC).date().year(),
             int(QDateTime::YearRange::Last));
    constexpr qint64 millisPerDay = 24 * 3600 * 1000;
    constexpr qint64 wholeDays = Bounds::max() / millisPerDay;
    constexpr qint64 millisRemainder = Bounds::max() % millisPerDay;
    QVERIFY(QDateTime(QDate(1970, 1, 1).addDays(wholeDays),
                      QTime::fromMSecsSinceStartOfDay(millisRemainder),
                      Qt::UTC).isValid());
    QVERIFY(!QDateTime(QDate(1970, 1, 1).addDays(wholeDays),
                       QTime::fromMSecsSinceStartOfDay(millisRemainder + 1),
                       Qt::UTC).isValid());
    QVERIFY(QDateTime(QDate(1970, 1, 1).addDays(-wholeDays - 1),
                      QTime::fromMSecsSinceStartOfDay(3600 * 24000 - millisRemainder - 1),
                      Qt::UTC).isValid());
    QVERIFY(!QDateTime(QDate(1970, 1, 1).addDays(-wholeDays - 1),
                       QTime::fromMSecsSinceStartOfDay(3600 * 24000 - millisRemainder - 2),
                       Qt::UTC).isValid());
}

void tst_QDateTime::macTypes()
{
#ifndef Q_OS_MAC
    QSKIP("This is a Apple-only test");
#else
    extern void tst_QDateTime_macTypes(); // in qdatetime_mac.mm
    tst_QDateTime_macTypes();
#endif
}

QTEST_APPLESS_MAIN(tst_QDateTime)
#include "tst_qdatetime.moc"
