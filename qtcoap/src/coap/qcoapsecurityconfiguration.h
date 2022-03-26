/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QCOAPSECURITYCONFIGURATION_H
#define QCOAPSECURITYCONFIGURATION_H

#include <QtCoap/qcoapglobal.h>

#include <QtCore/qglobal.h>
#include <QtCore/QSharedDataPointer>
#include <QtNetwork/QSslCertificate>

QT_BEGIN_NAMESPACE

class QCoapPrivateKeyPrivate;
class Q_COAP_EXPORT QCoapPrivateKey
{
public:
    QCoapPrivateKey();
    QCoapPrivateKey(const QByteArray &key, QSsl::KeyAlgorithm algorithm,
                    QSsl::EncodingFormat format = QSsl::Pem,
                    const QByteArray &passPhrase = QByteArray());
    QCoapPrivateKey(const Qt::HANDLE &handle);

    QCoapPrivateKey(const QCoapPrivateKey& other);
    QCoapPrivateKey(QCoapPrivateKey&& other) noexcept;
    ~QCoapPrivateKey();

    QCoapPrivateKey &operator=(QCoapPrivateKey &&other) noexcept
    { swap(other); return *this; }
    QCoapPrivateKey &operator=(const QCoapPrivateKey &other);

    void swap(QCoapPrivateKey &other) noexcept
    { qSwap(d, other.d); }

    bool isNull() const;

    QByteArray key() const;
    Qt::HANDLE handle() const;
    QSsl::KeyAlgorithm algorithm() const;
    QSsl::EncodingFormat encodingFormat() const;
    QByteArray passPhrase() const;
private:
     QSharedDataPointer<QCoapPrivateKeyPrivate> d;
};

class QCoapSecurityConfigurationPrivate;
class Q_COAP_EXPORT QCoapSecurityConfiguration
{
public:
    QCoapSecurityConfiguration();
    QCoapSecurityConfiguration(const QCoapSecurityConfiguration &other);
    ~QCoapSecurityConfiguration();

    QCoapSecurityConfiguration(QCoapSecurityConfiguration &&other) noexcept;
    QCoapSecurityConfiguration &operator=(QCoapSecurityConfiguration &&other) noexcept
    { swap(other); return *this; }
    QCoapSecurityConfiguration &operator=(const QCoapSecurityConfiguration &other);

    void swap(QCoapSecurityConfiguration &other) noexcept
    { qSwap(d, other.d); }

    void setPreSharedKeyIdentity(const QByteArray &preSharedKeyIdentity);
    QByteArray preSharedKeyIdentity() const;

    void setPreSharedKey(const QByteArray &preSharedKey);
    QByteArray preSharedKey() const;

    void setDefaultCipherString(const QString &cipherString);
    QString defaultCipherString() const;

    void setCaCertificates(const QList<QSslCertificate> &certificates);
    QList<QSslCertificate> caCertificates() const;

    void setLocalCertificateChain(const QList<QSslCertificate> &localChain);
    QList<QSslCertificate> localCertificateChain() const;

    void setPrivateKey(const QCoapPrivateKey &key);
    QCoapPrivateKey privateKey() const;

private:
     QSharedDataPointer<QCoapSecurityConfigurationPrivate> d;
};

Q_DECLARE_SHARED(QCoapSecurityConfiguration)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCoapSecurityConfiguration)

#endif // QCOAPSECURITYCONFIGURATION_H
