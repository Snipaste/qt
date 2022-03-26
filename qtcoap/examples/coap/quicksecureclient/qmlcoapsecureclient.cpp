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

#include "qmlcoapsecureclient.h"

#include <QCoapReply>
#include <QFile>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QUrl>

Q_LOGGING_CATEGORY(lcCoapClient, "qt.coap.client")

QmlCoapSecureClient::QmlCoapSecureClient(QObject *parent)
    : QObject(parent)
    , m_coapClient(nullptr)
    , m_securityMode(QtCoap::SecurityMode::NoSecurity)
{
}

QmlCoapSecureClient::~QmlCoapSecureClient()
{
    if (m_coapClient) {
        delete m_coapClient;
        m_coapClient = nullptr;
    }
}

static QString errorMessage(QtCoap::Error errorCode)
{
    const auto error = QMetaEnum::fromType<QtCoap::Error>().valueToKey(static_cast<int>(errorCode));
    return QString("Request failed with error: %1\n").arg(error);
}

void QmlCoapSecureClient::setSecurityMode(QtCoap::SecurityMode mode)
{
    // Create a new client, if the security mode has changed
    if (m_coapClient && mode != m_securityMode) {
        delete m_coapClient;
        m_coapClient = nullptr;
    }

    if (!m_coapClient) {
        m_coapClient = new QCoapClient(mode);
        m_securityMode = mode;

        connect(m_coapClient, &QCoapClient::finished, this,
                [this](QCoapReply *reply) {
                    if (!reply)
                        emit finished("Something went wrong, received a null reply");
                    else if (reply->errorReceived() != QtCoap::Error::Ok)
                        emit finished(errorMessage(reply->errorReceived()));
                    else
                        emit finished(reply->message().payload());
                });

        connect(m_coapClient, &QCoapClient::error, this,
                [this](QCoapReply *, QtCoap::Error errorCode) {
                    emit finished(errorMessage(errorCode));
                });
    }
}

void QmlCoapSecureClient::sendGetRequest(const QString &host, const QString &path, int port)
{
    if (!m_coapClient)
        return;

    m_coapClient->setSecurityConfiguration(m_configuration);

    QUrl url;
    url.setHost(host);
    url.setPath(path);
    url.setPort(port);
    m_coapClient->get(url);
}

void
QmlCoapSecureClient::setSecurityConfiguration(const QString &preSharedKey, const QString &identity)
{
    QCoapSecurityConfiguration configuration;
    configuration.setPreSharedKey(preSharedKey.toUtf8());
    configuration.setPreSharedKeyIdentity(identity.toUtf8());
    m_configuration = configuration;
}

void QmlCoapSecureClient::setSecurityConfiguration(const QString &localCertificatePath,
                                                   const QString &caCertificatePath,
                                                   const QString &privateKeyPath)
{
    QCoapSecurityConfiguration configuration;

    const auto localCerts =
            QSslCertificate::fromPath(QUrl(localCertificatePath).toLocalFile(), QSsl::Pem,
                                      QSslCertificate::PatternSyntax::FixedString);
    if (localCerts.isEmpty())
        qCWarning(lcCoapClient, "The specified local certificate file is not valid.");
    else
        configuration.setLocalCertificateChain(localCerts.toVector());

    const auto caCerts = QSslCertificate::fromPath(QUrl(caCertificatePath).toLocalFile(), QSsl::Pem,
                                                   QSslCertificate::PatternSyntax::FixedString);
    if (caCerts.isEmpty())
        qCWarning(lcCoapClient, "The specified CA certificate file is not valid.");
    else
        configuration.setCaCertificates(caCerts.toVector());

    QFile privateKey(QUrl(privateKeyPath).toLocalFile());
    if (privateKey.open(QIODevice::ReadOnly)) {
        QCoapPrivateKey key(privateKey.readAll(), QSsl::Ec);
        configuration.setPrivateKey(key);
    } else {
        qCWarning(lcCoapClient) << "Unable to read the specified private key file"
                                << privateKeyPath;
    }
    m_configuration = configuration;
}

void QmlCoapSecureClient::disconnect()
{
    if (m_coapClient)
        m_coapClient->disconnect();
}
