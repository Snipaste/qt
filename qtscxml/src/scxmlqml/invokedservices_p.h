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

#ifndef INVOKEDSERVICES_P_H
#define INVOKEDSERVICES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qscxmlqmlglobals_p.h"

#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqml.h>
#include <QtScxml/qscxmlstatemachine.h>
#include <QtCore/qproperty.h>
#include <private/qproperty_p.h>

QT_BEGIN_NAMESPACE

class Q_SCXMLQML_PRIVATE_EXPORT QScxmlInvokedServices : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QScxmlStateMachine *stateMachine READ stateMachine WRITE setStateMachine
               NOTIFY stateMachineChanged BINDABLE bindableStateMachine)
    Q_PROPERTY(QVariantMap children READ children NOTIFY childrenChanged BINDABLE bindableChildren)
    Q_PROPERTY(QQmlListProperty<QObject> qmlChildren READ qmlChildren)
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "qmlChildren")
    QML_NAMED_ELEMENT(InvokedServices)
    QML_ADDED_IN_VERSION(5,8)

public:
    QScxmlInvokedServices(QObject *parent = nullptr);

    QVariantMap children();
    QBindable<QVariantMap> bindableChildren();

    QScxmlStateMachine *stateMachine() const;
    void setStateMachine(QScxmlStateMachine *stateMachine);
    QBindable<QScxmlStateMachine*> bindableStateMachine();

    QQmlListProperty<QObject> qmlChildren();

Q_SIGNALS:
    void childrenChanged();
    void stateMachineChanged();

private:
    void classBegin() override;
    void componentComplete() override;
    QVariantMap childrenActualCalculation() const;

    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QScxmlInvokedServices, QScxmlStateMachine*, m_stateMachine,
                                      &QScxmlInvokedServices::setStateMachine,
                                      &QScxmlInvokedServices::stateMachineChanged, nullptr);
    Q_OBJECT_COMPUTED_PROPERTY(QScxmlInvokedServices, QVariantMap,
                               m_children, &QScxmlInvokedServices::childrenActualCalculation);
    QMetaObject::Connection m_serviceConnection;
    QList<QObject *> m_qmlChildren;
};

QT_END_NAMESPACE

#endif // INVOKEDSERVICES_P_H
