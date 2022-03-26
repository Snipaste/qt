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

#ifndef COAPHANDLER_H
#define COAPHANDLER_H

#include <QObject>
#include <QCoapClient>
#include <QCoapMessage>
#include <QCoapResource>
#include <qcoapnamespace.h>

QT_BEGIN_NAMESPACE

class QCoapReply;
class QCoapResourceDiscoveryReply;
class QCoapResource;

QT_END_NAMESPACE

class CoapHandler : public QObject
{
    Q_OBJECT
public:
    explicit CoapHandler(QObject *parent = nullptr);

    bool runGet(const QUrl &url);
    bool runPost(const QUrl &url);
    bool runPut(const QUrl &url);
    bool runDelete(const QUrl &url);
    bool runObserve(const QUrl &url);
    bool runDiscover(const QUrl &url);

public Q_SLOTS:
    void onFinished(QCoapReply *reply);
    void onNotified(QCoapReply *reply, QCoapMessage message);
    void onDiscovered(QCoapResourceDiscoveryReply *reply, QList<QCoapResource> resources);
    void onResponseToMulticast(QCoapReply *reply, const QCoapMessage& message,
                               const QHostAddress &sender);
    void onError(QCoapReply *reply, QtCoap::Error error);

private:
    QCoapClient m_coapClient;
};

#endif // COAPHANDLER_H
