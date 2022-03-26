/****************************************************************************
**
** Copyright (C) 2016 Centria research and development
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNfc module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnearfieldtarget_android_p.h"
#include "android/androidjninfc_p.h"
#include "qdebug.h"

#define NDEFTECHNOLOGY              QStringLiteral("android.nfc.tech.Ndef")
#define NDEFFORMATABLETECHNOLOGY    QStringLiteral("android.nfc.tech.NdefFormatable")
#define ISODEPTECHNOLOGY            QStringLiteral("android.nfc.tech.IsoDep")
#define NFCATECHNOLOGY              QStringLiteral("android.nfc.tech.NfcA")
#define NFCBTECHNOLOGY              QStringLiteral("android.nfc.tech.NfcB")
#define NFCFTECHNOLOGY              QStringLiteral("android.nfc.tech.NfcF")
#define NFCVTECHNOLOGY              QStringLiteral("android.nfc.tech.NfcV")
#define MIFARECLASSICTECHNOLOGY     QStringLiteral("android.nfc.tech.MifareClassic")
#define MIFARECULTRALIGHTTECHNOLOGY QStringLiteral("android.nfc.tech.MifareUltralight")

#define MIFARETAG   QStringLiteral("com.nxp.ndef.mifareclassic")
#define NFCTAGTYPE1 QStringLiteral("org.nfcforum.ndef.type1")
#define NFCTAGTYPE2 QStringLiteral("org.nfcforum.ndef.type2")
#define NFCTAGTYPE3 QStringLiteral("org.nfcforum.ndef.type3")
#define NFCTAGTYPE4 QStringLiteral("org.nfcforum.ndef.type4")

QNearFieldTargetPrivateImpl::QNearFieldTargetPrivateImpl(QJniObject intent,
                                                         const QByteArray uid,
                                                         QObject *parent)
:   QNearFieldTargetPrivate(parent),
    targetIntent(intent),
    targetUid(uid)
{
    updateTechList();
    updateType();
    setupTargetCheckTimer();
}

QNearFieldTargetPrivateImpl::~QNearFieldTargetPrivateImpl()
{
    releaseIntent();
    Q_EMIT targetDestroyed(targetUid);
}

QByteArray QNearFieldTargetPrivateImpl::uid() const
{
    return targetUid;
}

QNearFieldTarget::Type QNearFieldTargetPrivateImpl::type() const
{
    return tagType;
}

QNearFieldTarget::AccessMethods QNearFieldTargetPrivateImpl::accessMethods() const
{
    QNearFieldTarget::AccessMethods result = QNearFieldTarget::UnknownAccess;

    if (techList.contains(NDEFTECHNOLOGY)
            || techList.contains(NDEFFORMATABLETECHNOLOGY))
        result |= QNearFieldTarget::NdefAccess;

    if (techList.contains(ISODEPTECHNOLOGY)
            || techList.contains(NFCATECHNOLOGY)
            || techList.contains(NFCBTECHNOLOGY)
            || techList.contains(NFCFTECHNOLOGY)
            || techList.contains(NFCVTECHNOLOGY))
        result |= QNearFieldTarget::TagTypeSpecificAccess;

    return result;
}

bool QNearFieldTargetPrivateImpl::disconnect()
{
    if (!tagTech.isValid())
        return false;
    QJniEnvironment env;
    bool connected = tagTech.callMethod<jboolean>("isConnected");
    if (!connected)
        return false;
    auto methodId = env.findMethod(tagTech.objectClass(), "close", "()V");
    if (!methodId)
        return false;
    env->CallVoidMethod(tagTech.object(), methodId);
    return !env.checkAndClearExceptions();
}

bool QNearFieldTargetPrivateImpl::hasNdefMessage()
{
    return techList.contains(NDEFTECHNOLOGY);
}

QNearFieldTarget::RequestId QNearFieldTargetPrivateImpl::readNdefMessages()
{
    // Making sure that target has NDEF messages
    if (!hasNdefMessage())
        return QNearFieldTarget::RequestId();

    // Making sure that target is still in range
    QNearFieldTarget::RequestId requestId(new QNearFieldTarget::RequestIdPrivate);
    if (!targetIntent.isValid()) {
        reportError(QNearFieldTarget::TargetOutOfRangeError, requestId);
        return requestId;
    }

    // Getting Ndef technology object
    if (!setTagTechnology({NDEFTECHNOLOGY})) {
        reportError(QNearFieldTarget::UnsupportedError, requestId);
        return requestId;
    }

    // Connect
    if (!connect()) {
        reportError(QNearFieldTarget::ConnectionError, requestId);
        return requestId;
    }

    // Get NdefMessage object
    QJniObject ndefMessage = tagTech.callObjectMethod("getNdefMessage", "()Landroid/nfc/NdefMessage;");
    if (!ndefMessage.isValid()) {
        reportError(QNearFieldTarget::NdefReadError, requestId);
        return requestId;
    }

    // Convert to byte array
    QJniObject ndefMessageBA = ndefMessage.callObjectMethod("toByteArray", "()[B");
    QByteArray ndefMessageQBA = jbyteArrayToQByteArray(ndefMessageBA.object<jbyteArray>());

    // Sending QNdefMessage, requestCompleted and exit.
    QNdefMessage qNdefMessage = QNdefMessage::fromByteArray(ndefMessageQBA);
    QMetaObject::invokeMethod(this, [this, qNdefMessage]() {
        Q_EMIT this->q_ptr->ndefMessageRead(qNdefMessage);
    }, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, [this, requestId]() {
        Q_EMIT this->requestCompleted(requestId);
    }, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, [this, qNdefMessage, requestId]() {
        //TODO This is an Android specific signal in NearFieldTarget.
        //     We need to check if it is still necessary.
        Q_EMIT this->ndefMessageRead(qNdefMessage, requestId);
    }, Qt::QueuedConnection);
    return requestId;
}

int QNearFieldTargetPrivateImpl::maxCommandLength() const
{
    QJniObject tagTech;
    if (techList.contains(ISODEPTECHNOLOGY))
        tagTech = getTagTechnology(ISODEPTECHNOLOGY);
    else if (techList.contains(NFCATECHNOLOGY))
        tagTech = getTagTechnology(NFCATECHNOLOGY);
    else if (techList.contains(NFCBTECHNOLOGY))
        tagTech = getTagTechnology(NFCBTECHNOLOGY);
    else if (techList.contains(NFCFTECHNOLOGY))
        tagTech = getTagTechnology(NFCFTECHNOLOGY);
    else if (techList.contains(NFCVTECHNOLOGY))
        tagTech = getTagTechnology(NFCVTECHNOLOGY);
    else
        return 0;

    return tagTech.callMethod<jint>("getMaxTransceiveLength");
}

QNearFieldTarget::RequestId QNearFieldTargetPrivateImpl::sendCommand(const QByteArray &command)
{
    if (command.size() == 0 || command.size() > maxCommandLength()) {
        Q_EMIT error(QNearFieldTarget::InvalidParametersError, QNearFieldTarget::RequestId());
        return QNearFieldTarget::RequestId();
    }

    // Making sure that target has commands
    if (!(accessMethods() & QNearFieldTarget::TagTypeSpecificAccess))
        return QNearFieldTarget::RequestId();

    QJniEnvironment env;

    if (!setTagTechnology({ISODEPTECHNOLOGY, NFCATECHNOLOGY, NFCBTECHNOLOGY, NFCFTECHNOLOGY, NFCVTECHNOLOGY})) {
        Q_EMIT error(QNearFieldTarget::UnsupportedError, QNearFieldTarget::RequestId());
        return QNearFieldTarget::RequestId();
    }

    // Connecting
    QNearFieldTarget::RequestId requestId = QNearFieldTarget::RequestId(new QNearFieldTarget::RequestIdPrivate());
    if (!connect()) {
        reportError(QNearFieldTarget::ConnectionError, requestId);
        return requestId;
    }

    // Making QByteArray
    QByteArray ba(command);
    jbyteArray jba = env->NewByteArray(ba.size());
    env->SetByteArrayRegion(jba, 0, ba.size(), reinterpret_cast<jbyte*>(ba.data()));

    // Writing
    QJniObject myNewVal = tagTech.callObjectMethod("transceive", "([B)[B", jba);
    if (!myNewVal.isValid()) {
        // Some devices (Samsung, Huawei) throw an exception when the card is lost:
        // "android.nfc.TagLostException: Tag was lost". But there seems to be a bug that
        // isConnected still reports true. So we need to invalidate the target as soon as
        // possible and treat the card as lost.
        handleTargetLost();

        reportError(QNearFieldTarget::CommandError, requestId);
        return requestId;
    }
    QByteArray result = jbyteArrayToQByteArray(myNewVal.object<jbyteArray>());
    env->DeleteLocalRef(jba);

    setResponseForRequest(requestId, result, false);

    QMetaObject::invokeMethod(this, [this, requestId]() {
        Q_EMIT this->requestCompleted(requestId);
    }, Qt::QueuedConnection);

    return requestId;
}

QNearFieldTarget::RequestId QNearFieldTargetPrivateImpl::writeNdefMessages(const QList<QNdefMessage> &messages)
{
    if (messages.size() == 0)
        return QNearFieldTarget::RequestId();

    if (messages.size() > 1)
        qWarning("QNearFieldTarget::writeNdefMessages: Android supports writing only one NDEF message per tag.");

    QJniEnvironment env;
    const char *writeMethod;

    if (!setTagTechnology({NDEFFORMATABLETECHNOLOGY, NDEFTECHNOLOGY}))
        return QNearFieldTarget::RequestId();

    // Getting write method
    if (selectedTech == NDEFFORMATABLETECHNOLOGY)
        writeMethod = "format";
    else
        writeMethod = "writeNdefMessage";

    // Connecting
    QNearFieldTarget::RequestId requestId = QNearFieldTarget::RequestId(new QNearFieldTarget::RequestIdPrivate());
    if (!connect()) {
        reportError(QNearFieldTarget::ConnectionError, requestId);
        return requestId;
    }

    // Making NdefMessage object
    const QNdefMessage &message = messages.first();
    QByteArray ba = message.toByteArray();
    QJniObject jba = env->NewByteArray(ba.size());
    env->SetByteArrayRegion(jba.object<jbyteArray>(), 0, ba.size(), reinterpret_cast<jbyte*>(ba.data()));
    QJniObject jmessage = QJniObject("android/nfc/NdefMessage", "([B)V", jba.object<jbyteArray>());
    if (!jmessage.isValid()) {
        reportError(QNearFieldTarget::UnknownError, requestId);
        return requestId;
    }

    // Writing
    auto methodId = env.findMethod(tagTech.objectClass(), writeMethod, "(Landroid/nfc/NdefMessage;)V");
    if (methodId)
        env->CallVoidMethod(tagTech.object(), methodId, jmessage.object<jobject>());
    if (!methodId || env.checkAndClearExceptions()) {
        reportError(QNearFieldTarget::NdefWriteError, requestId);
        return requestId;
    }

    QMetaObject::invokeMethod(this, [this, requestId]() {
        Q_EMIT this->requestCompleted(requestId);
    }, Qt::QueuedConnection);
    return requestId;
}

void QNearFieldTargetPrivateImpl::setIntent(QJniObject intent)
{
    if (targetIntent == intent)
        return;

    releaseIntent();
    targetIntent = intent;
    if (targetIntent.isValid()) {
        // Updating tech list and type in case of there is another tag with same UID as one before.
        updateTechList();
        updateType();
        targetCheckTimer->start();
    }
}

void QNearFieldTargetPrivateImpl::checkIsTargetLost()
{
    if (!targetIntent.isValid() || !setTagTechnology({selectedTech})) {
        handleTargetLost();
        return;
    }

    QJniEnvironment env;
    bool connected = false;
    auto methodId = env.findMethod(tagTech.objectClass(), "isConnected", "()Z");
    if (methodId)
        connected = env->CallBooleanMethod(tagTech.object(), methodId);
    if (!methodId || env.checkAndClearExceptions()) {
        handleTargetLost();
        return;
    }

    if (connected)
        return;

    methodId = env.findMethod(tagTech.objectClass(), "connect", "()V");
    if (methodId)
        env->CallVoidMethod(tagTech.object(), methodId);
    if (!methodId || env.checkAndClearExceptions()) {
        handleTargetLost();
        return;
    }
    methodId = env.findMethod(tagTech.objectClass(), "close", "()V");
    if (methodId)
        env->CallVoidMethod(tagTech.object(), methodId);
    if (!methodId || env.checkAndClearExceptions())
        handleTargetLost();
}

void QNearFieldTargetPrivateImpl::releaseIntent()
{
    targetCheckTimer->stop();

    targetIntent = QJniObject();
}

void QNearFieldTargetPrivateImpl::updateTechList()
{
    if (!targetIntent.isValid())
        return;

    // Getting tech list
    QJniEnvironment env;
    QJniObject tag = AndroidNfc::getTag(targetIntent);
    Q_ASSERT_X(tag.isValid(), "updateTechList", "could not get Tag object");

    QJniObject techListArray = tag.callObjectMethod("getTechList", "()[Ljava/lang/String;");
    if (!techListArray.isValid()) {
        handleTargetLost();
        return;
    }

    // Converting tech list array to QStringList.
    techList.clear();
    jsize techCount = env->GetArrayLength(techListArray.object<jobjectArray>());
    for (jsize i = 0; i < techCount; ++i) {
        QJniObject tech = env->GetObjectArrayElement(techListArray.object<jobjectArray>(), i);
        techList.append(tech.callObjectMethod<jstring>("toString").toString());
    }
}

void QNearFieldTargetPrivateImpl::updateType()
{
    tagType = getTagType();
}

QNearFieldTarget::Type QNearFieldTargetPrivateImpl::getTagType() const
{
    if (techList.contains(NDEFTECHNOLOGY)) {
        QJniObject ndef = getTagTechnology(NDEFTECHNOLOGY);
        QString qtype = ndef.callObjectMethod("getType", "()Ljava/lang/String;").toString();

        if (qtype.compare(MIFARETAG) == 0)
            return QNearFieldTarget::MifareTag;
        if (qtype.compare(NFCTAGTYPE1) == 0)
            return QNearFieldTarget::NfcTagType1;
        if (qtype.compare(NFCTAGTYPE2) == 0)
            return QNearFieldTarget::NfcTagType2;
        if (qtype.compare(NFCTAGTYPE3) == 0)
            return QNearFieldTarget::NfcTagType3;
        if (qtype.compare(NFCTAGTYPE4) == 0)
            return QNearFieldTarget::NfcTagType4;
        return QNearFieldTarget::ProprietaryTag;
    } else if (techList.contains(NFCATECHNOLOGY)) {
        if (techList.contains(MIFARECLASSICTECHNOLOGY))
            return QNearFieldTarget::MifareTag;

        // Checking ATQA/SENS_RES
        // xxx0 0000  xxxx xxxx: Identifies tag Type 1 platform
        QJniObject nfca = getTagTechnology(NFCATECHNOLOGY);
        QJniObject atqaBA = nfca.callObjectMethod("getAtqa", "()[B");
        QByteArray atqaQBA = jbyteArrayToQByteArray(atqaBA.object<jbyteArray>());
        if (atqaQBA.isEmpty())
            return QNearFieldTarget::ProprietaryTag;
        if ((atqaQBA[0] & 0x1F) == 0x00)
            return QNearFieldTarget::NfcTagType1;

        // Checking SAK/SEL_RES
        // xxxx xxxx  x00x x0xx: Identifies tag Type 2 platform
        // xxxx xxxx  x01x x0xx: Identifies tag Type 4 platform
        jshort sakS = nfca.callMethod<jshort>("getSak");
        if ((sakS & 0x0064) == 0x0000)
            return QNearFieldTarget::NfcTagType2;
        else if ((sakS & 0x0064) == 0x0020)
            return QNearFieldTarget::NfcTagType4A;
        return QNearFieldTarget::ProprietaryTag;
    } else if (techList.contains(NFCBTECHNOLOGY)) {
        return QNearFieldTarget::NfcTagType4B;
    } else if (techList.contains(NFCFTECHNOLOGY)) {
        return QNearFieldTarget::NfcTagType3;
    }

    return QNearFieldTarget::ProprietaryTag;
}

void QNearFieldTargetPrivateImpl::setupTargetCheckTimer()
{
    targetCheckTimer = new QTimer(this);
    targetCheckTimer->setInterval(1000);
    QObject::connect(targetCheckTimer, &QTimer::timeout, this, &QNearFieldTargetPrivateImpl::checkIsTargetLost);
    targetCheckTimer->start();
}

void QNearFieldTargetPrivateImpl::handleTargetLost()
{
    releaseIntent();
    Q_EMIT targetLost(this);
}

QJniObject QNearFieldTargetPrivateImpl::getTagTechnology(const QString &tech) const
{
    QString techClass(tech);
    techClass.replace(QLatin1Char('.'), QLatin1Char('/'));

    // Getting requested technology
    QJniObject tag = AndroidNfc::getTag(targetIntent);
    Q_ASSERT_X(tag.isValid(), "getTagTechnology", "could not get Tag object");

    const QString sig = QString::fromUtf8("(Landroid/nfc/Tag;)L%1;");
    QJniObject tagTech = QJniObject::callStaticObjectMethod(techClass.toUtf8().constData(), "get",
            sig.arg(techClass).toUtf8().constData(), tag.object<jobject>());

    return tagTech;
}

bool QNearFieldTargetPrivateImpl::setTagTechnology(const QStringList &technologies)
{
    for (const QString &tech : technologies) {
        if (techList.contains(tech)) {
            if (selectedTech == tech) {
                return true;
            }
            selectedTech = tech;
            tagTech = getTagTechnology(tech);
            return tagTech.isValid();
        }
    }

    return false;
}

bool QNearFieldTargetPrivateImpl::connect()
{
    if (!tagTech.isValid())
        return false;

    QJniEnvironment env;
    auto methodId = env.findMethod(tagTech.objectClass(), "isConnected", "()Z");
    bool connected = false;
    if (methodId)
        connected = env->CallBooleanMethod(tagTech.object(), methodId);
    if (!methodId || env.checkAndClearExceptions())
        return false;

    if (connected)
        return true;

    setCommandTimeout(2000);
    methodId = env.findMethod(tagTech.objectClass(), "connect", "()V");
    if (!methodId)
        return false;
    env->CallVoidMethod(tagTech.object(), methodId);
    return !env.checkAndClearExceptions();
}

bool QNearFieldTargetPrivateImpl::setCommandTimeout(int timeout)
{
    if (!tagTech.isValid())
        return false;

    QJniEnvironment env;
    auto methodId = env.findMethod(tagTech.objectClass(), "setTimeout", "(I)V");
    if (methodId)
        env->CallVoidMethod(tagTech.object(), methodId, timeout);
    return methodId && !env.checkAndClearExceptions();
}

QByteArray QNearFieldTargetPrivateImpl::jbyteArrayToQByteArray(const jbyteArray &byteArray) const
{
    QJniEnvironment env;
    QByteArray resultArray;
    jsize len = env->GetArrayLength(byteArray);
    resultArray.resize(len);
    env->GetByteArrayRegion(byteArray, 0, len, reinterpret_cast<jbyte*>(resultArray.data()));
    return resultArray;
}
