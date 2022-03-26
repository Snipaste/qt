/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef STATE_H
#define STATE_H

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

#include "qstatemachineqmlglobals_p.h"
#include "childrenprivate_p.h"

#include <QtCore/qproperty.h>
#include <QtStateMachine/QState>
#include <QtQml/QQmlParserStatus>
#include <QtQml/QQmlListProperty>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class Q_STATEMACHINEQML_PRIVATE_EXPORT State : public QState, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> children READ children
               NOTIFY childrenChanged BINDABLE bindableChildren)
    Q_CLASSINFO("DefaultProperty", "children")
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)

public:
    explicit State(QState *parent = 0);

    void classBegin() override {}
    void componentComplete() override;

    QQmlListProperty<QObject> children();
    QBindable<QQmlListProperty<QObject>> bindableChildren() const;

Q_SIGNALS:
    void childrenChanged();

private:
    // See the childrenActualCalculation for the mutable explanation
    mutable ChildrenPrivate<State, ChildrenMode::StateOrTransition> m_children;
    friend ChildrenPrivate<State, ChildrenMode::StateOrTransition>;
    void childrenContentChanged();
    QQmlListProperty<QObject> childrenActualCalculation() const;
    Q_OBJECT_COMPUTED_PROPERTY(State, QQmlListProperty<QObject>, m_childrenComputedProperty,
                               &State::childrenActualCalculation);
};

QT_END_NAMESPACE

#endif
