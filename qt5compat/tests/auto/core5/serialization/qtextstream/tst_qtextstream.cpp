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

#include <QtTest/QtTest>

#include <QBuffer>
#include <QStringConverter>
#include <QStringRef>
#include <QTextStream>

class tst_QTextStream : public QObject
{
    Q_OBJECT

private slots:
    // text write operators
    void stringref_write_operator_ToDevice();
};

void tst_QTextStream::stringref_write_operator_ToDevice()
{
    QBuffer buf;
    buf.open(QBuffer::WriteOnly);
    QTextStream stream(&buf);
    stream.setEncoding(QStringConverter::Latin1);
    stream.setAutoDetectUnicode(true);

    const QString expected = "No explicit lengthExplicit length";

    stream << QStringRef(&expected).left(18);
    stream << QStringRef(&expected).mid(18);
    stream.flush();
    QCOMPARE(buf.buffer().constData(), "No explicit lengthExplicit length");
}

QTEST_MAIN(tst_QTextStream)
#include "tst_qtextstream.moc"
