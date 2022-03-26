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

#ifndef OPCUAREADRESULT_H
#define OPCUAREADRESULT_H

#include <QObject>
#include <QDateTime>
#include "qopcuatype.h"
#include "opcuastatus.h"

QT_BEGIN_NAMESPACE

class QOpcUaReadResult;
class QOpcUaClient;
class OpcUaReadResultData;
class OpcUaReadResult
{
    Q_GADGET
    Q_PROPERTY(QOpcUa::NodeAttribute attribute READ attribute)
    Q_PROPERTY(QString indexRange READ indexRange)
    Q_PROPERTY(QString nodeId READ nodeId)
    Q_PROPERTY(QString namespaceName READ namespaceName)
    Q_PROPERTY(const QDateTime &serverTimestamp READ serverTimestamp)
    Q_PROPERTY(const QDateTime &sourceTimestamp READ sourceTimestamp)
    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(OpcUaStatus status READ status)

public:
    OpcUaReadResult();
    OpcUaReadResult(const OpcUaReadResult &other);
    OpcUaReadResult(const QOpcUaReadResult &other, const QOpcUaClient *client);
    OpcUaReadResult &operator=(const OpcUaReadResult &rhs);
    ~OpcUaReadResult();

    const QString &indexRange() const;
    const QString &nodeId() const;
    QOpcUa::NodeAttribute attribute() const;
    const QString &namespaceName() const;
    const QDateTime &serverTimestamp() const;
    const QDateTime &sourceTimestamp() const;
    const QVariant &value() const;

    OpcUaStatus status() const;

private:
    QSharedDataPointer<OpcUaReadResultData> data;
};


QT_END_NAMESPACE

Q_DECLARE_METATYPE(OpcUaReadResult)

#endif // OPCUAREADRESULT_H
