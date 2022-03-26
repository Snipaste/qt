/****************************************************************************
**
** Copyright (C) 2015 basysKom GmbH, opensource@basyskom.com
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtOpcUa module of the Qt Toolkit.
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

#ifndef QOPCUAATTRIBUTEOPERAND_H
#define QOPCUAATTRIBUTEOPERAND_H

#include <QtOpcUa/qopcuaglobal.h>
#include <QtOpcUa/qopcuatype.h>

#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QOpcUaRelativePathElement;

class QVariant;
class QOpcUaRelativePathElement;

// OPC-UA part 4, 7.4.4.4
class QOpcUaAttributeOperandData;
class Q_OPCUA_EXPORT QOpcUaAttributeOperand
{
public:
    QOpcUaAttributeOperand();
    QOpcUaAttributeOperand(const QOpcUaAttributeOperand &);
    QOpcUaAttributeOperand &operator=(const QOpcUaAttributeOperand &);
    operator QVariant() const;
    ~QOpcUaAttributeOperand();

    QString nodeId() const;
    void setNodeId(const QString &nodeId);

    QString alias() const;
    void setAlias(const QString &alias);

    QList<QOpcUaRelativePathElement> browsePath() const;
    QList<QOpcUaRelativePathElement> &browsePathRef();
    void setBrowsePath(const QList<QOpcUaRelativePathElement> &browsePath);

    QOpcUa::NodeAttribute attributeId() const;
    void setAttributeId(QOpcUa::NodeAttribute attributeId);

    QString indexRange() const;
    void setIndexRange(const QString &indexRange);

private:
    QSharedDataPointer<QOpcUaAttributeOperandData> data;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QOpcUaAttributeOperand)

#endif // QOPCUAATTRIBUTEOPERAND_H
