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

#ifndef QCOAPREPLY_P_H
#define QCOAPREPLY_P_H

#include <QtCoap/qcoapreply.h>
#include <private/qcoapmessage_p.h>
#include <private/qiodevice_p.h>

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

class QHostAddress;
class Q_AUTOTEST_EXPORT QCoapReplyPrivate : public QIODevicePrivate
{
public:
    QCoapReplyPrivate(const QCoapRequest &request);

    void _q_setRunning(const QCoapToken &, QCoapMessageId);
    virtual void _q_setContent(const QHostAddress &sender, const QCoapMessage &, QtCoap::ResponseCode);
    void _q_setNotified();
    void _q_setObserveCancelled();
    void _q_setFinished(QtCoap::Error = QtCoap::Error::Ok);
    void _q_setError(QtCoap::ResponseCode code);
    void _q_setError(QtCoap::Error);

    static QCoapReply *createCoapReply(const QCoapRequest &request, QObject *parent = nullptr);

    QCoapRequest request;
    QCoapMessage message;
    QtCoap::ResponseCode responseCode = QtCoap::ResponseCode::InvalidCode;
    QtCoap::Error error = QtCoap::Error::Ok;
    bool isRunning = false;
    bool isFinished = false;
    bool isAborted = false;

    Q_DECLARE_PUBLIC(QCoapReply)
};

QT_END_NAMESPACE

#endif // QCOAPREPLY_P_H
