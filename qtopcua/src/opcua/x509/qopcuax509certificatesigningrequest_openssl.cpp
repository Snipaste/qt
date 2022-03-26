/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt OPC UA module.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopcuax509certificatesigningrequest.h"
#include "openssl_symbols_p.h"
#include "qopcuakeypair_p.h"
#include "qopcuax509utils_p.h"
#include "qopcuax509distinguishedname.h"
#include "qopcuax509extensionsubjectalternativename.h"
#include "qopcuax509certificatesigningrequest_p.h"
#include <QOpcUaX509ExtensionBasicConstraints>
#include <QOpcUaX509ExtensionKeyUsage>
#include <QOpcUaX509ExtensionExtendedKeyUsage>


QT_BEGIN_NAMESPACE

QOpcUaX509CertificateSigningRequestPrivate::QOpcUaX509CertificateSigningRequestPrivate()
{

}

QOpcUaX509CertificateSigningRequestPrivate::~QOpcUaX509CertificateSigningRequestPrivate()
{
    qDeleteAll(m_extensions);
}

void QOpcUaX509CertificateSigningRequestPrivate::setMessageDigest(QOpcUaX509CertificateSigningRequest::MessageDigest digest)
{
    m_messageDigest = digest;
}

QOpcUaX509CertificateSigningRequest::MessageDigest QOpcUaX509CertificateSigningRequestPrivate::messageDigest() const
{
    return m_messageDigest;
}

void QOpcUaX509CertificateSigningRequestPrivate::addExtension(QOpcUaX509Extension *extension)
{
    m_extensions.append(extension);
}

void QOpcUaX509CertificateSigningRequestPrivate::setSubject(const QOpcUaX509DistinguishedName &subject)
{
    m_subject = subject;
}

QOpcUaX509CertificateSigningRequest::Encoding QOpcUaX509CertificateSigningRequestPrivate::encoding() const
{
    return m_encoding;
}

void QOpcUaX509CertificateSigningRequestPrivate::setEncoding(QOpcUaX509CertificateSigningRequest::Encoding encoding)
{
    m_encoding = encoding;
}

const QOpcUaX509DistinguishedName &QOpcUaX509CertificateSigningRequestPrivate::subject() const
{
    return m_subject;
}

static X509_EXTENSION *createExtension(QOpcUaX509Extension *extension)
{
    X509_EXTENSION *ex = nullptr;

    if (const auto *san = dynamic_cast<const QOpcUaX509ExtensionSubjectAlternativeName *>(extension)) {
        QStringList data;

        for (const auto &pair : qAsConst(san->entries())) {
            QString prefix;
            if (pair.first == QOpcUaX509ExtensionSubjectAlternativeName::Type::DNS)
                prefix = QLatin1String("DNS:");
            else if (pair.first == QOpcUaX509ExtensionSubjectAlternativeName::Type::Email)
                    prefix = QLatin1String("EMAIL:");
            else if (pair.first == QOpcUaX509ExtensionSubjectAlternativeName::Type::IP)
                    prefix = QLatin1String("IP:");
            else if (pair.first == QOpcUaX509ExtensionSubjectAlternativeName::Type::URI)
                    prefix = QLatin1String("URI:");
            else {
                qCWarning(lcSsl()) << "Invalid SubjectAlternativeName type";
                return nullptr;
            }

            if (pair.second.isEmpty() || pair.second.contains(QChar::fromLatin1(','))) {
                qCWarning(lcSsl()) << "Invalid SubjectAlternativeName value";
                return nullptr;
            }

            data.append(prefix + pair.second);
        }

        ex = q_X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, data.join(QLatin1Char(',')).toUtf8().data());
        if (!ex) {
            qCWarning(lcSsl()) << "Failed to create X509 extension" << data;
            return nullptr;
        }
        q_X509_EXTENSION_set_critical(ex, san->critical() ? 1 : 0);
    } else if (const auto *bc = dynamic_cast<const QOpcUaX509ExtensionBasicConstraints *>(extension)) {
        QString data = QLatin1String("CA:") + QLatin1String(bc->ca() ? "true" : "false");
        if (bc->ca() && bc->pathLength() >= 0)
            data.append(QLatin1String(",pathlen:") + QString::number(bc->pathLength()));

        ex = q_X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, data.toUtf8().data());
        if (!ex) {
            qCWarning(lcSsl()) << "Failed to create X509 extension" << data;
            return nullptr;
        }
        q_X509_EXTENSION_set_critical(ex, bc->critical() ? 1 : 0);
    } else if (const auto *ku = dynamic_cast<const QOpcUaX509ExtensionKeyUsage *>(extension)) {
        QStringList data;

        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::DigitalSignature))
            data.append(QLatin1String("Digital Signature"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::NonRepudiation))
            data.append(QLatin1String("Non Repudiation"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::KeyEncipherment))
            data.append(QLatin1String("Key Encipherment"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::DataEncipherment))
            data.append(QLatin1String("Data Encipherment"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::KeyAgreement))
            data.append(QLatin1String("Key Agreement"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::CertificateSigning))
            data.append(QLatin1String("Certificate Sign"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::CrlSigning))
            data.append(QLatin1String("CRL Sign"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::EnciptherOnly))
            data.append(QLatin1String("Encipther Only"));
        if (ku->keyUsage(QOpcUaX509ExtensionKeyUsage::KeyUsage::DecipherOnly))
            data.append(QLatin1String("Decipher Only"));

        ex = q_X509V3_EXT_conf_nid(NULL, NULL, NID_key_usage, data.join(QLatin1Char(',')).toUtf8().data());
        if (!ex) {
            qCWarning(lcSsl()) << "Failed to create X509 extension" << data;
            return nullptr;
        }
        q_X509_EXTENSION_set_critical(ex, ku->critical() ? 1 : 0);
    } else if (const auto *eku = dynamic_cast<const QOpcUaX509ExtensionExtendedKeyUsage *>(extension)) {
        QStringList data;

        if (eku->keyUsage(QOpcUaX509ExtensionExtendedKeyUsage::KeyUsage::TlsWebServerAuthentication))
            data.append(QLatin1String("SSL Server"));
        if (eku->keyUsage(QOpcUaX509ExtensionExtendedKeyUsage::KeyUsage::TlsWebClientAuthentication))
            data.append(QLatin1String("SSL Client"));
        if (eku->keyUsage(QOpcUaX509ExtensionExtendedKeyUsage::KeyUsage::SignExecutableCode))
            data.append(QLatin1String("Object Signing"));
        if (eku->keyUsage(QOpcUaX509ExtensionExtendedKeyUsage::KeyUsage::EmailProtection))
            data.append(QLatin1String("S/MIME"));

       // NID_ext_key_usage
        ex = q_X509V3_EXT_conf_nid(NULL, NULL, NID_ext_key_usage, data.join(QLatin1Char(',')).toUtf8().data());
        if (!ex) {
            qCWarning(lcSsl()) << "Failed to create X509 extension" << data;
            return nullptr;
        }
        q_X509_EXTENSION_set_critical(ex, eku->critical() ? 1 : 0);
    } else {
        qCWarning(lcSsl) << "Unknown X509 extension";
        return nullptr;
    }
    return ex;
}

static bool setSubjectName(X509_NAME *subject, const QOpcUaX509DistinguishedName &dn) {
    QList<QOpcUaX509DistinguishedName::Type> entries = {
        QOpcUaX509DistinguishedName::Type::CommonName,
        QOpcUaX509DistinguishedName::Type::CountryName,
        QOpcUaX509DistinguishedName::Type::LocalityName,
        QOpcUaX509DistinguishedName::Type::StateOrProvinceName,
        QOpcUaX509DistinguishedName::Type::OrganizationName,
    };

    for (const auto &type : entries) {
        const auto value = dn.entry(type);
        if (value.isEmpty())
            continue;

        ASN1_OBJECT *obj = q_OBJ_txt2obj(QOpcUaX509DistinguishedName::typeToOid(type).toLatin1().constData(), 1 /* no names allowed */);
        if (!obj) {
            qCWarning(lcSsl()) << "Invalid distinguishedName type";
            return false;
        }

        if (!q_X509_NAME_add_entry_by_OBJ(subject, obj, MBSTRING_UTF8, (const unsigned char*)(value.toUtf8().constData()), -1, -1, 0)) {
            qCWarning(lcSsl) << "Failed to set CSR entry:" << getOpenSslError();
            return false;
        }
    }
    return true;
}

// Creates the request and returns a PEM encoded byte array
QByteArray QOpcUaX509CertificateSigningRequestPrivate::createRequest(const QOpcUaKeyPair &privateKey)
{
    if (!privateKey.hasPrivateKey()) {
        qCWarning(lcSsl) << "Key has no private key";
        return QByteArray();
    }

    auto keyData = privateKey.d_func()->m_keyData;

    X509_REQ *req = q_X509_REQ_new();
    if (!req) {
        qCWarning(lcSsl) << "Failed to create CSR:" << getOpenSslError();
        return QByteArray();
    }
    Deleter<X509_REQ> reqDeleter(req, q_X509_REQ_free);

    if (!q_X509_REQ_set_version(req, 0 /* version */)) {
        qCWarning(lcSsl) << "Failed to set CSR version:" << getOpenSslError();
        return QByteArray();
    }

    X509_NAME *subj = q_X509_REQ_get_subject_name(req);
    if (!subj) {
        qCWarning(lcSsl) << "Invalid subject pointer";
        return QByteArray();
    }

    if (!setSubjectName(subj, m_subject)) {
        qCWarning(lcSsl) << "Failed to set subject";
        return QByteArray();
    }

    if (m_extensions.size() > 0) {
        auto exts = q_sk_X509_EXTENSION_new_null();

        for (auto extension : m_extensions) {
            auto ex = createExtension(extension);
            if (ex)
                q_sk_X509_EXTENSION_push(exts, ex); // returns void
        }
        if (q_X509_REQ_add_extensions(req, (STACK_OF(X509_EXTENSION) *)exts) == 0) {
            qCWarning(lcSsl) << "Failed to add X509 extensions";
            return QByteArray();
        }
        q_sk_X509_EXTENSION_pop_free(exts, (void(*)(void*))q_X509_EXTENSION_free); // frees the whole stack, returns void
    } // end of for loop

    if (!q_X509_REQ_set_pubkey(req, keyData)) {
        qCWarning(lcSsl) << "Failed to set public key:" << getOpenSslError();
        return QByteArray();
    }

    const EVP_MD *digest = nullptr;
    if (m_messageDigest == QOpcUaX509CertificateSigningRequest::MessageDigest::SHA256)
        digest = q_EVP_sha256();

    if (!digest) {
        qCWarning(lcSsl) << "Invalid message digest";
        return QByteArray();
    }

    if (q_X509_REQ_sign(req, keyData, digest) <= 0) {
        qCWarning(lcSsl) << "Failed to sign CSR:" << getOpenSslError();
        return QByteArray();
    }

    BIO *bio = q_BIO_new(q_BIO_s_mem());
    if (!bio) {
        qCWarning(lcSsl) << "Failed to allocate a buffer:" << getOpenSslError();
        return QByteArray();
    }
    Deleter<BIO> bioDeleter(bio, q_BIO_free_all);

    int result = 0;

    if (m_encoding == QOpcUaX509CertificateSigningRequest::Encoding::PEM) {
        // Some CAs require to use q_PEM_write_bio_X509_REW_NEW
        result = q_PEM_write_bio_X509_REQ(bio, req);
    } else if (m_encoding == QOpcUaX509CertificateSigningRequest::Encoding::DER) {
        result = q_i2d_X509_REQ_bio(bio, req);
    }
    if (result != 1) {
        qCWarning(lcSsl) << "Failed to export certificate request";
        return QByteArray();
    }

    char *buf;
    int length = q_BIO_get_mem_data(bio, &buf);
    QByteArray data(buf, length);
    return data;
}

QByteArray QOpcUaX509CertificateSigningRequestPrivate::createSelfSignedCertificate(const QOpcUaKeyPair &privateKey, int validityInDays)
{
    if (!privateKey.hasPrivateKey()) {
        qCWarning(lcSsl) << "Key has no private key";
        return QByteArray();
    }

    auto keyData = privateKey.d_func()->m_keyData;

    X509 *x509 = q_X509_new();
    if (!x509)
        return QByteArray();

    Deleter<X509> x509Deleter(x509, q_X509_free);

    if (!q_X509_set_version(x509, 2 /* version */)) {
        qCWarning(lcSsl) << "Failed to set version";
        return QByteArray();
    }
    q_X509_gmtime_adj(q_X509_getm_notBefore(x509), 0); // current time
    q_X509_gmtime_adj(q_X509_getm_notAfter(x509), (long)60 * 60 * 24 * validityInDays);

    if (!q_X509_set_pubkey(x509, keyData)) {
        qCWarning(lcSsl) << "Failed to set public key:" << getOpenSslError();
        return QByteArray();
    }

    X509_NAME *subj = q_X509_get_subject_name(x509);
    if (!subj) {
        qCWarning(lcSsl) << "Invalid subject pointer";
        return QByteArray();
    }

    if (!setSubjectName(subj, m_subject)) {
        qCWarning(lcSsl) << "Failed to set subject";
        return QByteArray();
    }

    X509_NAME *issuer = q_X509_get_issuer_name(x509);
    if (!issuer) {
        qCWarning(lcSsl) << "Invalid issuer pointer";
        return QByteArray();
    }

    if (!setSubjectName(issuer, m_subject)) {
        qCWarning(lcSsl) << "Failed to set issuer";
        return QByteArray();
    }

    for (auto extension : m_extensions) {
        auto ex = createExtension(extension);
        if (ex) {
            if (!q_X509_add_ext(x509, ex, -1)) {
                qCWarning(lcSsl) << "Failed to add extension";
                return QByteArray();
            }
            q_X509_EXTENSION_free(ex);
        } else {
            qCWarning(lcSsl) << "Invalid extension";
            return QByteArray();
        }
    }

    // Hash of public key
    unsigned char publicKeyHash[SHA_DIGEST_LENGTH];
    unsigned int len;
    if (!q_X509_pubkey_digest(x509, q_EVP_sha1(), publicKeyHash, &len)) {
        qCWarning(lcSsl) << "Failed to hash public key";
        return QByteArray();
    }

    // Set subject key identifier
    ASN1_OCTET_STRING *subjectKeyIdentifier = q_ASN1_OCTET_STRING_new();
    if (!subjectKeyIdentifier) {
        qCWarning(lcSsl) << "Failed to allocate ASN1 string";
        return QByteArray();
    }
    Deleter<ASN1_OCTET_STRING> subjectKeyIdentifierDeleter(subjectKeyIdentifier, q_ASN1_OCTET_STRING_free);

    if (!q_ASN1_OCTET_STRING_set(subjectKeyIdentifier, publicKeyHash, SHA_DIGEST_LENGTH)) {
        qCWarning(lcSsl) << "Failed set ASN1 string";
        return QByteArray();
    }

    if (!q_X509_add1_ext_i2d(x509, NID_subject_key_identifier, subjectKeyIdentifier, 0, X509V3_ADD_DEFAULT)) {
        qCWarning(lcSsl) << "Failed to add subject key identifier extension";
        return QByteArray();
    }

    // Set serial number
    unsigned char subjHash[SHA_DIGEST_LENGTH];
    unsigned char finalHash[SHA_DIGEST_LENGTH];

    if (!q_X509_NAME_digest(subj, q_EVP_sha1(), subjHash, &len)) {
        qCWarning(lcSsl) << "failed";
        return QByteArray();
    }
    for (unsigned int i = 0; i < len; i++)
        finalHash[i] = subjHash[i] ^ publicKeyHash[i];

    ASN1_INTEGER *serial_num = q_ASN1_INTEGER_new();
    if (!serial_num) {
        qCWarning(lcSsl) << "Failed to allocate ASN1 integer";
        return QByteArray();
    }
    Deleter<ASN1_OCTET_STRING> serial_numDeleter(serial_num, q_ASN1_INTEGER_free);

    if (!q_ASN1_OCTET_STRING_set(serial_num, finalHash, len)) {
        qCWarning(lcSsl) << "Failed to set ASN1 integer";
        return QByteArray();
    }
    if (!q_X509_set_serialNumber(x509, serial_num)) {
        qCWarning(lcSsl) << "Failed to set serial number";
        return QByteArray();
    }

    // Set authority key identifier
    AUTHORITY_KEYID *akid = q_AUTHORITY_KEYID_new();
    if (!akid) {
        qCWarning(lcSsl) << "Failed to allocate authority key id";
        return QByteArray();
    }
    Deleter<AUTHORITY_KEYID> akidDeleter(akid, q_AUTHORITY_KEYID_free);

    akid->issuer = q_GENERAL_NAMES_new();
    if (!akid->issuer) {
        qCWarning(lcSsl) << "Failed to set authority key id";
        return QByteArray();
    }

    GENERAL_NAME *generalName = q_GENERAL_NAME_new();
    if (!generalName) {
        qCWarning(lcSsl) << "Failed to set authority key id";
        return QByteArray();
    }
    generalName->type = GEN_DIRNAME;
    generalName->d.directoryName = q_X509_NAME_dup(q_X509_get_subject_name(x509));

#if QT_CONFIG(opensslv11)
    q_sk_GENERAL_NAME_push((OPENSSL_STACK*)akid->issuer, generalName);
#else
    q_sk_GENERAL_NAME_push(akid->issuer, generalName);
#endif
    akid->keyid = (ASN1_OCTET_STRING*)q_X509_get_ext_d2i(x509, NID_subject_key_identifier, NULL, NULL);
    akid->serial = q_ASN1_INTEGER_dup(q_X509_get_serialNumber(x509));

    if (!q_X509_add1_ext_i2d(x509, NID_authority_key_identifier, akid, 0, X509V3_ADD_DEFAULT)) {
        qCWarning(lcSsl) << "Failed to add authority key id extension";
        return QByteArray();
    }

    const EVP_MD *digest = nullptr;
    if (m_messageDigest == QOpcUaX509CertificateSigningRequest::MessageDigest::SHA256)
        digest = q_EVP_sha256();

    if (!digest) {
        qCWarning(lcSsl) << "Invalid message digest";
        return QByteArray();
    }

    if (q_X509_sign(x509, keyData, digest) <= 0) {
        qCWarning(lcSsl) << "Failed to sign certificate:" << getOpenSslError();
        return QByteArray();
    }

    BIO *bio = q_BIO_new(q_BIO_s_mem());
    if (!bio) {
        qCWarning(lcSsl) << "Failed to allocate a buffer:" << getOpenSslError();
        return QByteArray();
    }
    Deleter<BIO> bioDeleter(bio, q_BIO_free_all);

    int result = 0;

    if (m_encoding == QOpcUaX509CertificateSigningRequest::Encoding::PEM) {
        result = q_PEM_write_bio_X509(bio, x509);
    } else if (m_encoding == QOpcUaX509CertificateSigningRequest::Encoding::DER) {
        result = q_i2d_X509_bio(bio, x509);
    }
    if (result != 1) {
        qCWarning(lcSsl) << "Failed to export certificate";
        return QByteArray();
    }

    char *buf;
    int length = q_BIO_get_mem_data(bio, &buf);
    QByteArray data(buf, length);
    return data;
}

QT_END_NAMESPACE

