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

#include "qopcuaelementoperand.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpcUaElementOperand
    \inmodule QtOpcUa
    \brief The OPC UA ElementOperand type.

    The ElementOperand is defined in OPC-UA part 4, 7.4.4.2.
    It is used to identify another element in the filter by its index
    (the first element has the index 0).

    This is required to create complex filters, for example to reference
    the two operands of the AND operation in ((Severity > 500) AND (Message == "TestString")).
    The first step is to create content filter elements for the two conditions (Severity > 500)
    and (Message == "TestString"). A third content filter element is required to create an AND
    combination of the two conditions. It consists of the AND operator and two element operands
    with the indices of the two conditions created before:

    \code
    QOpcUaMonitoringParameters::EventFilter filter;
    ...
    // setup select clauses
    ...
    QOpcUaContentFilterElement condition1;
    QOpcUaContentFilterElement condition2;
    QOpcUaContentFilterElement condition3;
    condition1 << QOpcUaContentFilterElement::FilterOperator::GreaterThan << QOpcUaSimpleAttributeOperand("Severity") <<
                    QOpcUaLiteralOperand(quint16(500), QOpcUa::Types::UInt16);
    condition2 << QOpcUaContentFilterElement::FilterOperator::Equals << QOpcUaSimpleAttributeOperand("Message") <<
                    QOpcUaLiteralOperand("TestString", QOpcUa::Types::String);
    condition3 << QOpcUaContentFilterElement::FilterOperator::And << QOpcUaElementOperand(0) << QOpcUaElementOperand(1);
    filter << condition1 << condition2 << condition3;
    \endcode
*/

class QOpcUaElementOperandData : public QSharedData
{
public:
    quint32 index{0};
};

QOpcUaElementOperand::QOpcUaElementOperand()
    : data(new QOpcUaElementOperandData)
{
}

/*!
    Constructs an element operand from \a rhs.
*/
QOpcUaElementOperand::QOpcUaElementOperand(const QOpcUaElementOperand &rhs)
    : data(rhs.data)
{
}

/*!
    Constructs an element operand with index \a index.
*/
QOpcUaElementOperand::QOpcUaElementOperand(quint32 index)
    : data(new QOpcUaElementOperandData)
{
    setIndex(index);
}

/*!
    Sets the values from \a rhs in this element operand.
*/
QOpcUaElementOperand &QOpcUaElementOperand::operator=(const QOpcUaElementOperand &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

/*!
    Converts this element operand to \l QVariant.
*/
QOpcUaElementOperand::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

QOpcUaElementOperand::~QOpcUaElementOperand()
{
}

/*!
    Returns the index of the filter element that is going to be used as operand.
*/
quint32 QOpcUaElementOperand::index() const
{
    return data->index;
}

/*!
    Sets the index of the filter element that is going to be used as operand to \a index.
*/
void QOpcUaElementOperand::setIndex(quint32 index)
{
    data->index = index;
}

QT_END_NAMESPACE
