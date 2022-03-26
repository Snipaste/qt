/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
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

#include "invokedservices_p.h"
#include <QtScxml/qscxmlinvokableservice.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype InvokedServices
    \instantiates QScxmlInvokedServices
    \inqmlmodule QtScxml
    \since QtScxml 5.8

    \brief Provices access to the services invoked by state machines.

    Makes the invoked services easily accessible by their names, without
    constantly iterating through QScxmlStateMachine::invokedServices.

    The services are called from state machines via the mechanism described in
    \l{SCXML Specification - 6.4 <invoke>}.
*/

QScxmlInvokedServices::QScxmlInvokedServices(QObject *parent) : QObject(parent)
{
}

/*!
    \qmlproperty var InvokedServices::children

    The services invoked by the state machine.
*/

QVariantMap QScxmlInvokedServices::children()
{
    return m_children.value();
}

QVariantMap QScxmlInvokedServices::childrenActualCalculation() const
{
    QVariantMap ret;
    if (m_stateMachine.value()) {
        const QList<QScxmlInvokableService *> children = m_stateMachine->invokedServices();
        for (QScxmlInvokableService *service : children)
            ret.insert(service->name(), QVariant::fromValue(service));
    }
    return ret;
}

QBindable<QVariantMap> QScxmlInvokedServices::bindableChildren()
{
    return &m_children;
}

void QScxmlInvokedServices::classBegin()
{
}

/*!
    \qmlproperty ScxmlStateMachine InvokedServices::stateMachine

    The state machine that invoked the services.
*/

QScxmlStateMachine *QScxmlInvokedServices::stateMachine() const
{
    return m_stateMachine;
}

void QScxmlInvokedServices::setStateMachine(QScxmlStateMachine *stateMachine)
{
    if (stateMachine == m_stateMachine.value()) {
        m_stateMachine.removeBindingUnlessInWrapper();
        return;
    }

    QObject::disconnect(m_serviceConnection);
    m_stateMachine = stateMachine;

    if (m_stateMachine.value()) {
        m_serviceConnection = QObject::connect(
                    m_stateMachine.value(), &QScxmlStateMachine::invokedServicesChanged,
                    [this](){
            m_children.notify();
            emit childrenChanged();
        });
    }
    m_stateMachine.notify();
    m_children.notify();
    emit childrenChanged();
}

QBindable<QScxmlStateMachine*> QScxmlInvokedServices::bindableStateMachine()
{
    return &m_stateMachine;
}

/*!
    \qmlproperty list<QtObject> InvokedServices::qmlChildren

    A list of additional QtObject types nested in this type.
*/

QQmlListProperty<QObject> QScxmlInvokedServices::qmlChildren()
{
    return QQmlListProperty<QObject>(this, &m_qmlChildren);
}

void QScxmlInvokedServices::componentComplete()
{
    if (!m_stateMachine.value())
        setStateMachine(qobject_cast<QScxmlStateMachine *>(parent()));
}

QT_END_NAMESPACE
