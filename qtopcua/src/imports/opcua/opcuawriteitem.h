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

#ifndef OPCUAWRITEITEM_H
#define OPCUAWRITEITEM_H

#include <QObject>
#include <QDateTime>
#include <QtCore/qshareddata.h>
#include "opcuastatus.h"
#include "qopcuatype.h"

QT_BEGIN_NAMESPACE

class OpcUaWriteItemData;
class OpcUaWriteItem
{
    Q_GADGET
    Q_PROPERTY(QString nodeId READ nodeId WRITE setNodeId)
    Q_PROPERTY(QVariant ns READ namespaceIdentifier WRITE setNamespaceIdentifier)
    Q_PROPERTY(QOpcUa::NodeAttribute attribute READ attribute WRITE setAttribute)
    Q_PROPERTY(QString indexRange READ indexRange WRITE setIndexRange)
    Q_PROPERTY(QVariant value READ value WRITE setValue)
    Q_PROPERTY(QOpcUa::Types valueType READ valueType WRITE setValueType)
    Q_PROPERTY(QDateTime sourceTimestamp READ sourceTimestamp WRITE setSourceTimestamp)
    Q_PROPERTY(QDateTime serverTimestamp READ serverTimestamp WRITE setServerTimestamp)
    Q_PROPERTY(OpcUaStatus::Status statusCode READ statusCode WRITE setStatusCode)

public:
    OpcUaWriteItem();
    OpcUaWriteItem(const OpcUaWriteItem &other);
    OpcUaWriteItem &operator=(const OpcUaWriteItem &rhs);
    ~OpcUaWriteItem();

    const QString &nodeId() const;
    void setNodeId(const QString &nodeId);

    const QVariant &namespaceIdentifier() const;
    void setNamespaceIdentifier(const QVariant &namespaceIdentifier);

    QOpcUa::NodeAttribute attribute() const;
    void setAttribute(QOpcUa::NodeAttribute attribute);

    const QString &indexRange() const;
    void setIndexRange(const QString &indexRange);

    const QVariant &value() const;
    void setValue(const QVariant &value);

    QOpcUa::Types valueType() const;
    void setValueType(QOpcUa::Types type);

    const QDateTime &sourceTimestamp() const;
    void setSourceTimestamp(const QDateTime &sourceTimestamp);

    const QDateTime &serverTimestamp() const;
    void setServerTimestamp(const QDateTime &serverTimestamp);

    OpcUaStatus::Status statusCode() const;
    bool hasStatusCode() const;
    void setStatusCode(OpcUaStatus::Status statusCode);

private:
    QSharedDataPointer<OpcUaWriteItemData> data;
};

class OpcUaWriteItemFactory : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE OpcUaWriteItem create();
};

QT_END_NAMESPACE

#endif // OPCUAWRITEITEM_H
