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

#ifndef EVENTCONNECTION_P_H
#define EVENTCONNECTION_P_H

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

#include <QtScxml/qscxmlstatemachine.h>
#include <QtScxml/qscxmlevent.h>
#include <QtCore/qobject.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>
#include "QtCore/qproperty.h"
#include <private/qproperty_p.h>

QT_BEGIN_NAMESPACE

// QScxmlEvent is used as signal parameter, and defined in the cpp lib
struct Q_SCXMLQML_PRIVATE_EXPORT QScxmlEventForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QScxmlEvent)
    QML_ADDED_IN_VERSION(5,8)
};

class Q_SCXMLQML_PRIVATE_EXPORT QScxmlEventConnection : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QStringList events READ events WRITE setEvents NOTIFY eventsChanged
               BINDABLE bindableEvents)
    Q_PROPERTY(QScxmlStateMachine *stateMachine READ stateMachine WRITE setStateMachine
               NOTIFY stateMachineChanged BINDABLE bindableStateMachine)
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(EventConnection)
    QML_ADDED_IN_VERSION(5,8)

public:
    QScxmlEventConnection(QObject *parent = nullptr);

    QStringList events() const;
    void setEvents(const QStringList &events);
    QBindable<QStringList> bindableEvents();

    QScxmlStateMachine *stateMachine() const;
    void setStateMachine(QScxmlStateMachine *stateMachine);
    QBindable<QScxmlStateMachine*> bindableStateMachine();

Q_SIGNALS:
    void eventsChanged();
    void stateMachineChanged();

    void occurred(const QScxmlEvent &event);

private:
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QScxmlEventConnection, QScxmlStateMachine*, m_stateMachine,
                                      &QScxmlEventConnection::setStateMachine,
                                      &QScxmlEventConnection::stateMachineChanged, nullptr);
    Q_OBJECT_COMPAT_PROPERTY(QScxmlEventConnection, QStringList, m_events,
                             &QScxmlEventConnection::setEvents,
                             &QScxmlEventConnection::eventsChanged);

    QList<QMetaObject::Connection> m_connections;

    void doConnect();
    void classBegin() override;
    void componentComplete() override;
};

QT_END_NAMESPACE

#endif // EVENTCONNECTION_P_H
