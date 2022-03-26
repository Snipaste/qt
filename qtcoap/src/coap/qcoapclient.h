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

#ifndef QCOAPCLIENT_H
#define QCOAPCLIENT_H

#include <QtCore/qglobal.h>
#include <QtCoap/qcoapglobal.h>
#include <QtCoap/qcoapnamespace.h>
#include <QtCore/qobject.h>
#include <QtNetwork/qabstractsocket.h>

QT_BEGIN_NAMESPACE

class QCoapReply;
class QCoapResourceDiscoveryReply;
class QCoapRequest;
class QCoapProtocol;
class QCoapConnection;
class QCoapSecurityConfiguration;
class QCoapMessage;
class QIODevice;

class QCoapClientPrivate;
class Q_COAP_EXPORT QCoapClient : public QObject
{
    Q_OBJECT
public:
    explicit QCoapClient(QtCoap::SecurityMode securityMode = QtCoap::SecurityMode::NoSecurity,
                         QObject *parent = nullptr);
    ~QCoapClient();

    QCoapReply *get(const QCoapRequest &request);
    QCoapReply *get(const QUrl &url);
    QCoapReply *put(const QCoapRequest &request, const QByteArray &data = QByteArray());
    QCoapReply *put(const QCoapRequest &request, QIODevice *device);
    QCoapReply *put(const QUrl &url, const QByteArray &data = QByteArray());
    QCoapReply *post(const QCoapRequest &request, const QByteArray &data = QByteArray());
    QCoapReply *post(const QCoapRequest &request, QIODevice *device);
    QCoapReply *post(const QUrl &url, const QByteArray &data = QByteArray());
    QCoapReply *deleteResource(const QCoapRequest &request);
    QCoapReply *deleteResource(const QUrl &url);
    QCoapReply *observe(const QCoapRequest &request);
    QCoapReply *observe(const QUrl &request);
    void cancelObserve(QCoapReply *notifiedReply);
    void cancelObserve(const QUrl &url);
    void disconnect();

    QCoapResourceDiscoveryReply *discover(
            QtCoap::MulticastGroup group = QtCoap::MulticastGroup::AllCoapNodesIPv4,
            int port = QtCoap::DefaultPort,
            const QString &discoveryPath = QLatin1String("/.well-known/core"));
    QCoapResourceDiscoveryReply *discover(
            const QUrl &baseUrl,
            const QString &discoveryPath = QLatin1String("/.well-known/core"));

    void setSecurityConfiguration(const QCoapSecurityConfiguration &configuration);
    void setBlockSize(quint16 blockSize);
    void setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value);
    void setMaximumServerResponseDelay(uint responseDelay);
    void setAckTimeout(uint ackTimeout);
    void setAckRandomFactor(double ackRandomFactor);
    void setMaximumRetransmitCount(uint maximumRetransmitCount);
    void setMinimumTokenSize(int tokenSize);

Q_SIGNALS:
    void finished(QCoapReply *reply);
    void responseToMulticastReceived(QCoapReply *reply, const QCoapMessage &message,
                                     const QHostAddress &sender);
    void error(QCoapReply *reply, QtCoap::Error error);

protected:
    Q_DECLARE_PRIVATE(QCoapClient)
};

QT_END_NAMESPACE

#endif // QCOAPCLIENT_H
