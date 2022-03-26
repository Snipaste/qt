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

#ifndef QCOAPINTERNALREQUEST_H
#define QCOAPINTERNALREQUEST_H

#include <private/qcoapinternalmessage_p.h>

#include <QtCoap/qcoapglobal.h>
#include <QtCoap/qcoapnamespace.h>
#include <private/qcoapconnection_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QCoapRequest;
class QCoapInternalRequestPrivate;
class Q_AUTOTEST_EXPORT QCoapInternalRequest : public QCoapInternalMessage
{
    Q_OBJECT
public:
    explicit QCoapInternalRequest(QObject *parent = nullptr);
    explicit QCoapInternalRequest(const QCoapRequest &request, QObject *parent = nullptr);

    bool isValid() const override;

    void initEmptyMessage(quint16 messageId, QCoapMessage::Type type);

    QByteArray toQByteArray() const;
    void setMessageId(quint16);
    void setToken(const QCoapToken&);
    void setToRequestBlock(uint blockNumber, uint blockSize);
    void setToSendBlock(uint blockNumber, uint blockSize);
    bool checkBlockNumber(uint blockNumber);

    using QCoapInternalMessage::addOption;
    void addOption(const QCoapOption &option) override;
    bool addUriOptions(QUrl uri, const QUrl &proxyUri = QUrl());

    QCoapToken token() const;
    QUrl targetUri() const;
    QtCoap::Method method() const;
    bool isObserve() const;
    bool isObserveCancelled() const;
    bool isMulticast() const;
    QCoapConnection *connection() const;
    uint retransmissionCounter() const;
    void setMethod(QtCoap::Method method);
    void setConnection(QCoapConnection *connection);
    void setObserveCancelled();

    void setTargetUri(QUrl targetUri);
    void setTimeout(uint timeout);
    void setMaxTransmissionWait(uint timeout);
    void setMulticastTimeout(uint responseDelay);
    void restartTransmission();
    void startMulticastTransmission();
    void stopTransmission();

Q_SIGNALS:
    void timeout(QCoapInternalRequest*);
    void maxTransmissionSpanReached(QCoapInternalRequest*);
    void multicastRequestExpired(QCoapInternalRequest*);

protected:
    QCoapOption uriHostOption(const QUrl &uri) const;
    QCoapOption blockOption(QCoapOption::OptionName name, uint blockNumber, uint blockSize) const;

private:
    Q_DECLARE_PRIVATE(QCoapInternalRequest)
};

class Q_AUTOTEST_EXPORT QCoapInternalRequestPrivate : public QCoapInternalMessagePrivate
{
public:
    QCoapInternalRequestPrivate() = default;

    QUrl targetUri;
    QtCoap::Method method = QtCoap::Method::Invalid;
    QCoapConnection *connection = nullptr;
    QByteArray fullPayload;

    uint timeout = 0;
    uint retransmissionCounter = 0;
    QTimer *timeoutTimer = nullptr;
    QTimer *maxTransmitWaitTimer = nullptr;
    QTimer *multicastExpireTimer = nullptr;

    bool observeCancelled = false;
    bool transmissionInProgress = false;

    Q_DECLARE_PUBLIC(QCoapInternalRequest)
};

QT_END_NAMESPACE

#endif // QCOAPINTERNALREQUEST_H
