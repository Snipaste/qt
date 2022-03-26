/****************************************************************************
**
** Copyright (C) 2016 Ford Motor Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtStateMachineQml/private/signaltransition_p.h>
#include <QtStateMachineQml/private/timeouttransition_p.h>
#include <QtStateMachineQml/private/statemachine_p.h>
#include <QtStateMachineQml/private/finalstate_p.h>
#include <QtStateMachineQml/private/state_p.h>
#include <QtQml/qqmlscriptstring.h>

#include <QTest>
#include <QtTest/private/qpropertytesthelper_p.h>
#include "../../shared/util.h"
#include "../../shared/bindableqmlutils.h"

class tst_qqmlstatemachine : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlstatemachine();

private slots:
    void tst_cppObjectSignal();
    void tst_bindings();
};


class CppObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ObjectState objectState READ objectState WRITE setObjectState NOTIFY objectStateChanged)
    Q_ENUMS(ObjectState)
public:
    enum ObjectState {
        State0,
        State1,
        State2
    };

public:
    CppObject() {}

    ObjectState objectState() const { return m_objectState; }
    void setObjectState(ObjectState objectState) { m_objectState = objectState; emit objectStateChanged();}

signals:
    void objectStateChanged();
    void mySignal(int signalState);

private:
    ObjectState m_objectState = State0;
};

tst_qqmlstatemachine::tst_qqmlstatemachine()
{
    QVERIFY(-1 != qmlRegisterUncreatableType<CppObject>("CppObjectEnum", 1, 0, "CppObject", QString()));
}

void tst_qqmlstatemachine::tst_cppObjectSignal()
{
    CppObject cppObject;
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("cppsignal.qml"));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("_cppObject", &cppObject);
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject != nullptr);

    // wait for state machine to start
    QTRY_VERIFY(rootObject->property("running").toBool());

    // emit signal from cpp
    emit cppObject.mySignal(CppObject::State1);

    // check if the signal was propagated
    QTRY_COMPARE(cppObject.objectState(), CppObject::State1);

    // emit signal from cpp
    emit cppObject.mySignal(CppObject::State2);

    // check if the signal was propagated
    QTRY_COMPARE(cppObject.objectState(), CppObject::State2);

    // wait for state machine to finish
    QTRY_VERIFY(!rootObject->property("running").toBool());
}

void tst_qqmlstatemachine::tst_bindings()
{
    SignalTransition signalTransition;
    // Generating QQmlScriptString requires proper qml context setup, and here we
    // use same the element that we are testing to create the testing material
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QLatin1String("qrc:///data/signaltransition.qml")));
    std::unique_ptr<QObject> obj(component.create());
    SignalTransition *st1 = qobject_cast<SignalTransition*>(obj->findChild<QObject*>("st1"));
    SignalTransition *st2 = qobject_cast<SignalTransition*>(obj->findChild<QObject*>("st2"));
    QVERIFY(st1 && st2 && (st1->guard() != st2->guard()));

    // -- SignalTransition::guard
    QTestPrivate::testReadWritePropertyBasics<SignalTransition, QQmlScriptString>(
        signalTransition, st1->guard(), st2->guard(), "guard");
    if (QTest::currentTestFailed()) {
        qWarning() << "SignalTransition::guard bindable test failed.";
        return;
    }

    // -- SignalTransition::signal
    // We use QML to create the test material (QJSValues that contain valid methods)
    QVariant signal1;
    QVariant signal2;
    QMetaObject::invokeMethod(obj.get(), "getSignal1", Q_RETURN_ARG(QVariant, signal1));
    QMetaObject::invokeMethod(obj.get(), "getSignal2", Q_RETURN_ARG(QVariant, signal2));
    // QJSValue does not implement operator== so we supply own comparator
    QTestPrivate::testReadWritePropertyBasics<SignalTransition, QJSValue>(
                *st1, signal1.value<QJSValue>(), signal2.value<QJSValue>(), "signal",
                [](QJSValue d1, QJSValue d2) { return d1.strictlyEquals(d2); });
    if (QTest::currentTestFailed()) {
        qWarning() << "SignalTransition::signal bindable test failed.";
        return;
    }

    // -- TimeoutTransition::timeout
    TimeoutTransition timeoutTransition;
    QCOMPARE(timeoutTransition.timeout(), 1000); // the initialvalue
    int timeout1{100};
    int timeout2{200};
    QTestPrivate::testReadWritePropertyBasics<TimeoutTransition, int>(
                timeoutTransition, timeout1, timeout2, "timeout");
    if (QTest::currentTestFailed()) {
        qWarning() << "TimeoutTransition::timeout bindable test failed.";
        return;
    }

    // -- FinalState::children
    FinalState finalState;
    QObject object1;
    QObject object2;
    testManipulableQmlListBasics<FinalState, QObject*>(
                finalState, &object1, &object2, "children");

    // -- State::children
    State state;
    testManipulableQmlListBasics<State, QObject*>(
                state, &object1, &object2, "children");

    // -- StateMachine::children
    StateMachine stateMachine;
    testManipulableQmlListBasics<StateMachine, QObject*>(
                stateMachine, &object1, &object2, "children");
}

QTEST_MAIN(tst_qqmlstatemachine)

#include "tst_qqmlstatemachine.moc"
