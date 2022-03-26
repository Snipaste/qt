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

#ifndef SIGNALTRANSITION_H
#define SIGNALTRANSITION_H

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

#include <QtStateMachine/QSignalTransition>
#include <QtCore/QVariant>
#include <QtQml/QJSValue>

#include <QtQml/qqmlscriptstring.h>
#include <QtQml/qqmlparserstatus.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlrefcount_p.h>
#include <private/qqmlboundsignal_p.h>
#include <QtCore/private/qproperty_p.h>

QT_BEGIN_NAMESPACE

class Q_STATEMACHINEQML_PRIVATE_EXPORT SignalTransition : public QSignalTransition, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QJSValue signal READ signal WRITE setSignal
               NOTIFY qmlSignalChanged BINDABLE bindableSignal)
    Q_PROPERTY(QQmlScriptString guard READ guard WRITE setGuard
               NOTIFY guardChanged BINDABLE bindableGuard)
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)
    QML_CUSTOMPARSER

public:
    explicit SignalTransition(QState *parent = nullptr);

    QQmlScriptString guard() const;
    void setGuard(const QQmlScriptString &guard);
    QBindable<QQmlScriptString> bindableGuard();

    bool eventTest(QEvent *event) override;
    void onTransition(QEvent *event) override;

    const QJSValue &signal();
    void setSignal(const QJSValue &signal);
    QBindable<QJSValue> bindableSignal();

    Q_INVOKABLE void invoke();

Q_SIGNALS:
    void guardChanged();
    void invokeYourself();
    /*!
     * \internal
     */
    void qmlSignalChanged();

private:
    void classBegin() override { m_complete = false; }
    void componentComplete() override { m_complete = true; connectTriggered(); }
    void connectTriggered();

    friend class SignalTransitionParser;

    Q_OBJECT_COMPAT_PROPERTY(SignalTransition, QJSValue, m_signal, &SignalTransition::setSignal,
                             &SignalTransition::qmlSignalChanged);
    Q_OBJECT_BINDABLE_PROPERTY(SignalTransition, QQmlScriptString,
                               m_guard, &SignalTransition::guardChanged);
    bool m_complete;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> m_compilationUnit;
    QList<const QV4::CompiledData::Binding *> m_bindings;
    QQmlRefPointer<QQmlBoundSignalExpression> m_signalExpression;
};

class SignalTransitionParser : public QQmlCustomParser
{
public:
    void verifyBindings(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &props) override;
    void applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings) override;
};

template<>
inline QQmlCustomParser *qmlCreateCustomParser<SignalTransition>()
{
    return new SignalTransitionParser;
}

QT_END_NAMESPACE

#endif
