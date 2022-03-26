/****************************************************************************
**
** Copyright (C) 2018 Witekio.
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCoap module.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>

#include <QtCoap/qcoapoption.h>

class tst_QCoapOption : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructAndAssign();
    void constructWithQByteArray();
    void constructWithQString();
    void constructWithInteger();
    void constructWithUtf8Characters();
};

void tst_QCoapOption::constructAndAssign()
{
    QCoapOption option1;
    QCOMPARE(option1.name(), QCoapOption::Invalid);
    QCOMPARE(option1.uintValue(), 0);
    QVERIFY(option1.stringValue().isEmpty());
    QVERIFY(option1.opaqueValue().isEmpty());

    QCoapOption option2(QCoapOption::Size1, 1);
    QCOMPARE(option2.name(), QCoapOption::Size1);
    QCOMPARE(option2.uintValue(), 1);

    // Copy-construction
    QCoapOption option3(option2);
    QCOMPARE(option3.name(), QCoapOption::Size1);
    QCOMPARE(option3.uintValue(), 1);

    // Move-construction
    QCoapOption option4(std::move(option2));
    QCOMPARE(option4.name(), QCoapOption::Size1);
    QCOMPARE(option4.uintValue(), 1);

    // Copy-assignment
    option4 = option1;
    QCOMPARE(option4.name(), QCoapOption::Invalid);
    QCOMPARE(option4.uintValue(), 0);

    // Move-assignment
    option4 = std::move(option3);
    QCOMPARE(option4.name(), QCoapOption::Size1);
    QCOMPARE(option4.uintValue(), 1);

    // Assign to a moved-from
    option2 = option4;
    QCOMPARE(option2.name(), QCoapOption::Size1);
    QCOMPARE(option2.uintValue(), 1);
}

void tst_QCoapOption::constructWithQByteArray()
{
    QByteArray ba = "some data";
    QCoapOption option(QCoapOption::LocationPath, ba);

    QCOMPARE(option.opaqueValue(), ba);
}

void tst_QCoapOption::constructWithQString()
{
    QString str = "some data";
    QCoapOption option(QCoapOption::LocationPath, str);

    QCOMPARE(option.opaqueValue(), str.toUtf8());
}

void tst_QCoapOption::constructWithInteger()
{
    quint32 value = 64000;
    QCoapOption option(QCoapOption::Size1, value);

    QCOMPARE(option.uintValue(), value);
}

void tst_QCoapOption::constructWithUtf8Characters()
{
    QByteArray ba = "\xc3\xa9~\xce\xbb\xe2\x82\xb2";
    QCoapOption option(QCoapOption::LocationPath, ba);

    QCOMPARE(option.opaqueValue(), ba);
}

QTEST_APPLESS_MAIN(tst_QCoapOption)

#include "tst_qcoapoption.moc"
