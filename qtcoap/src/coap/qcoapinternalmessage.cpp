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

#include "qcoapinternalmessage_p.h"
#include "qcoaprequest_p.h"
#include <QtCoap/qcoaprequest.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCoapExchange, "qt.coap.exchange")

/*!
    \internal

    Destructor of the private class.
 */
QCoapInternalMessagePrivate::~QCoapInternalMessagePrivate()
{
}

/*!
    \internal

    \class QCoapInternalMessage
    \brief The QCoapInternalMessage class contains data related to
    a received message or a message to send. It contains an instance of
    QCoapMessage and other data for the block management.

    \reentrant

    The QCoapInternalMessage class is inherited by QCoapInternalRequest and
    QCoapInternalReply that are used internally to manage requests to send
    and receive replies.

    \sa QCoapInternalReply, QCoapInternalRequest, QCoapMessage
*/

/*!
    \internal

    Constructs a new QCoapInternalMessage and sets \a parent as the parent
    object.
 */
QCoapInternalMessage::QCoapInternalMessage(QObject *parent) :
    QObject(*new QCoapInternalMessagePrivate, parent)
{
}

/*!
    \internal

    Constructs a new QCoapInternalMessage with the given \a message and sets
    \a parent as the parent object.
 */
QCoapInternalMessage::QCoapInternalMessage(const QCoapMessage &message, QObject *parent) :
    QCoapInternalMessage(parent)
{
    Q_D(QCoapInternalMessage);
    d->message = message;
}

/*!
    \internal
    Constructs a new QCoapInternalMessage with \a dd as the d_ptr.
    This constructor must be used when subclassing internally
    the QCoapInternalMessage class.
*/
QCoapInternalMessage::QCoapInternalMessage(QCoapInternalMessagePrivate &dd, QObject *parent):
    QObject(dd, parent)
{
}

/*!
    \internal
    Set block information from a descriptive block option. See
    \l {https://tools.ietf.org/html/rfc7959#section-2.3}{RFC 7959}.

    \note For block-wise transfer, the size of the block is expressed by a power
    of two. See
    \l{https://tools.ietf.org/html/rfc7959#section-2.2}{'Structure of a Block Option'}
    in RFC 7959 for more information.
*/
void QCoapInternalMessage::setFromDescriptiveBlockOption(const QCoapOption &option)
{
    Q_D(QCoapInternalMessage);

    const auto value = option.opaqueValue();
    const quint8 *optionData = reinterpret_cast<const quint8 *>(value.data());
    const quint8 lastByte = optionData[option.length() - 1];
    quint32 blockNumber = 0;

    for (int i = 0; i < option.length() - 1; ++i)
        blockNumber = (blockNumber << 8) | optionData[i];

    blockNumber = (blockNumber << 4) | (lastByte >> 4);
    d->currentBlockNumber = blockNumber;
    d->hasNextBlock = ((lastByte & 0x8) == 0x8);
    d->blockSize = static_cast<uint>(1u << ((lastByte & 0x7) + 4));

    if (d->blockSize > 1024)
        qCWarning(lcCoapExchange, "Received a block size larger than 1024, something may be wrong.");
}

/*!
    \internal
    \overload

    Adds the CoAP option with the given \a name and \a value.
*/
void QCoapInternalMessage::addOption(QCoapOption::OptionName name, const QByteArray &value)
{
    QCoapOption option(name, value);
    addOption(option);
}

/*!
    \internal
    \overload

    Adds the CoAP option with the given \a name and \a value.
*/
void QCoapInternalMessage::addOption(QCoapOption::OptionName name, quint32 value)
{
    QCoapOption option(name, value);
    addOption(option);
}

/*!
    \internal

    Adds the given CoAP \a option.
*/
void QCoapInternalMessage::addOption(const QCoapOption &option)
{
    Q_D(QCoapInternalMessage);
    d->message.addOption(option);
}

/*!
    \internal

    Removes the option with the given \a name.
*/
void QCoapInternalMessage::removeOption(QCoapOption::OptionName name)
{
    Q_D(QCoapInternalMessage);
    d->message.removeOption(name);
}

/*!
    \internal

    Returns a pointer to the message.
*/
QCoapMessage *QCoapInternalMessage::message()
{
    Q_D(QCoapInternalMessage);
    return &(d->message);
}

/*!
    \internal

    Returns a const pointer to the message.
*/
const QCoapMessage *QCoapInternalMessage::message() const
{
    Q_D(const QCoapInternalMessage);
    return &(d->message);
}

/*!
    \internal

    Returns the block number
*/
uint QCoapInternalMessage::currentBlockNumber() const
{
    Q_D(const QCoapInternalMessage);
    return d->currentBlockNumber;
}

/*!
    \internal

    Returns \c true if it has a next block, \c false otherwise.
*/
bool QCoapInternalMessage::hasMoreBlocksToReceive() const
{
    Q_D(const QCoapInternalMessage);
    return d->hasNextBlock;
}

/*!
    \internal

    Returns the size of the block.
*/
uint QCoapInternalMessage::blockSize() const
{
    Q_D(const QCoapInternalMessage);
    return d->blockSize;
}

/*!
    \internal

    Returns \c true if the message is considered valid.

    \sa isUrlValid()
*/
bool QCoapInternalMessage::isValid() const
{
    return true;
}

/*!
    \internal

    Returns \c true if URL is considered valid.

    \sa QCoapRequest::isUrlValid()
*/
bool QCoapInternalMessage::isUrlValid(const QUrl &url)
{
    return QCoapRequestPrivate::isUrlValid(url);
}

QT_END_NAMESPACE
