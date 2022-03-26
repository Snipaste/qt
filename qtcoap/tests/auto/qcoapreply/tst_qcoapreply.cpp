/****************************************************************************
**
** Copyright (C) 2017 Witekio.
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
#include <QCoreApplication>

#include <QtCoap/qcoapreply.h>
#include <private/qcoapreply_p.h>
#include <private/qcoapnamespace_p.h>

class tst_QCoapReply : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateReply_data();
    void updateReply();
    void requestData();
    void abortRequest();
};

void tst_QCoapReply::updateReply_data()
{
    QTest::addColumn<QByteArray>("payload");
    QTest::addColumn<QtCoap::ResponseCode>("responseCode");
    QTest::addColumn<QtCoap::Error>("error");

    QTest::newRow("success")
            << QByteArray("Some data")
            << QtCoap::ResponseCode::Content
            << QtCoap::Error::Ok;
    QTest::newRow("content error")
            << QByteArray("Error")
            << QtCoap::ResponseCode::BadRequest
            << QtCoap::Error::Ok;
    QTest::newRow("finished error")
            << QByteArray("Error")
            << QtCoap::ResponseCode::Content
            << QtCoap::Error::BadRequest;
    QTest::newRow("content & finished errors")
            << QByteArray("2Errors")
            << QtCoap::ResponseCode::BadGateway
            << QtCoap::Error::BadRequest;
}

void tst_QCoapReply::updateReply()
{
    QFETCH(QByteArray, payload);
    QFETCH(QtCoap::ResponseCode, responseCode);
    QFETCH(QtCoap::Error, error);

    const QByteArray token = "\xAF\x01\xC2";
    const quint16 id = 645;

    QScopedPointer<QCoapReply> reply(QCoapReplyPrivate::createCoapReply(QCoapRequest()));
    QCoapMessage message;
    message.setToken(token);
    message.setMessageId(id);
    message.setPayload(payload);

    QSignalSpy spyReplyFinished(reply.data(), &QCoapReply::finished);
    QSignalSpy spyReplyNotified(reply.data(), &QCoapReply::notified);
    QSignalSpy spyReplyError(reply.data(), &QCoapReply::error);
    QSignalSpy spyReplyAborted(reply.data(), &QCoapReply::aborted);

    QMetaObject::invokeMethod(reply.data(), "_q_setContent",
                              Q_ARG(QHostAddress, QHostAddress()),
                              Q_ARG(QCoapMessage, message),
                              Q_ARG(QtCoap::ResponseCode, responseCode));
    QMetaObject::invokeMethod(reply.data(), "_q_setFinished",
                              Q_ARG(QtCoap::Error, error));

    QCOMPARE(spyReplyFinished.count(), 1);
    QCOMPARE(spyReplyNotified.count(), 0);
    QCOMPARE(spyReplyAborted.count(), 0);
    if (error != QtCoap::Error::Ok || QtCoap::isError(responseCode)) {
        QVERIFY(spyReplyError.count() > 0);
        QCOMPARE(reply->isSuccessful(), false);
    } else {
        QCOMPARE(spyReplyError.count(), 0);
        QCOMPARE(reply->isSuccessful(), true);
    }

    QCOMPARE(reply->readAll(), payload);
    QCOMPARE(reply->readAll(), QByteArray());
    QCOMPARE(reply->responseCode(), responseCode);
    QCOMPARE(reply->message().token(), token);
    QCOMPARE(reply->message().messageId(), id);
}

void tst_QCoapReply::requestData()
{
    QScopedPointer<QCoapReply> reply(QCoapReplyPrivate::createCoapReply(QCoapRequest()));
    QMetaObject::invokeMethod(reply.data(), "_q_setRunning",
                              Q_ARG(QCoapToken, "token"),
                              Q_ARG(QCoapMessageId, 543));

    QCOMPARE(reply->request().token(), QByteArray("token"));
    QCOMPARE(reply->request().messageId(), 543);
}

void tst_QCoapReply::abortRequest()
{
    QScopedPointer<QCoapReply> reply(QCoapReplyPrivate::createCoapReply(QCoapRequest()));
    QMetaObject::invokeMethod(reply.data(), "_q_setRunning",
                              Q_ARG(QCoapToken, "token"),
                              Q_ARG(QCoapMessageId, 543));

    QSignalSpy spyAborted(reply.data(), &QCoapReply::aborted);
    QSignalSpy spyFinished(reply.data(), &QCoapReply::finished);
    reply->abortRequest();

    QTRY_COMPARE_WITH_TIMEOUT(spyAborted.count(), 1, 1000);
    QList<QVariant> arguments = spyAborted.takeFirst();
    QTRY_COMPARE_WITH_TIMEOUT(spyFinished.count(), 1, 1000);
    QVERIFY(arguments.at(0).toByteArray() == "token");
}

QTEST_MAIN(tst_QCoapReply)

#include "tst_qcoapreply.moc"
