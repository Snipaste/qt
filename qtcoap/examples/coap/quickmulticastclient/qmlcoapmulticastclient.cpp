/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtCoap module.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmlcoapmulticastclient.h"

#include <QCoapResourceDiscoveryReply>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcCoapClient, "qt.coap.client")

QmlCoapMulticastClient::QmlCoapMulticastClient(QObject *parent)
    : QCoapClient(QtCoap::SecurityMode::NoSecurity, parent)
{
    connect(this, &QCoapClient::finished, this,
            [this](QCoapReply *reply) {
                    if (reply)
                        emit finished(static_cast<int>(reply->errorReceived()));
                    else
                        qCWarning(lcCoapClient, "Something went wrong, received a null reply");
            });
}

void QmlCoapMulticastClient::discover(const QString &host, int port, const QString &discoveryPath)
{
    QUrl url;
    url.setHost(host);
    url.setPort(port);

    QCoapResourceDiscoveryReply *discoverReply = QCoapClient::discover(url, discoveryPath);
    if (discoverReply) {
        connect(discoverReply, &QCoapResourceDiscoveryReply::discovered,
                this, &QmlCoapMulticastClient::onDiscovered);
    } else {
        qCWarning(lcCoapClient, "Discovery request failed.");
    }
}

void QmlCoapMulticastClient::discover(QtCoap::MulticastGroup group, int port,
                                      const QString &discoveryPath)
{
    QCoapResourceDiscoveryReply *discoverReply = QCoapClient::discover(group, port, discoveryPath);
    if (discoverReply) {
        connect(discoverReply, &QCoapResourceDiscoveryReply::discovered,
                this, &QmlCoapMulticastClient::onDiscovered);
    } else {
        qCWarning(lcCoapClient, "Discovery request failed.");
    }
}

void QmlCoapMulticastClient::onDiscovered(QCoapResourceDiscoveryReply *reply,
                                          const QList<QCoapResource> &resources)
{
    Q_UNUSED(reply)
    for (auto resource : resources)
        emit discovered(resource);
}
