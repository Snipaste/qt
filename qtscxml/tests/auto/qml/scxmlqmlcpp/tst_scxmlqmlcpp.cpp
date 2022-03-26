/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
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

#include <QtTest>
#include <QtTest/private/qpropertytesthelper_p.h>
#include <QtScxml/QScxmlStateMachine>
#include <QtScxml/QScxmlNullDataModel>
#include <QtScxmlQml/private/eventconnection_p.h>
#include <QtScxmlQml/private/invokedservices_p.h>
#include <QtScxmlQml/private/statemachineloader_p.h>
#include "topmachine.h"
#include <functional>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <memory>

class tst_scxmlqmlcpp: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void eventConnectionStateMachineBinding();
    void eventConnectionEventsBinding();

    void invokedServicesStateMachineBinding();
    void invokedServicesChildrenBinding();

    void stateMachineLoaderInitialValuesBinding();
    void stateMachineLoaderSourceStateMachineBinding();
    void stateMachineLoaderDatamodelBinding();

private:
    QScxmlEventConnection m_eventConnection;
    QScxmlInvokedServices m_invokedServices;
    QScxmlStateMachineLoader m_stateMachineLoader;
    std::unique_ptr<QScxmlStateMachine> m_stateMachine1;
    std::unique_ptr<QScxmlStateMachine> m_stateMachine2;
};

void tst_scxmlqmlcpp::initTestCase()
{
    m_stateMachine1.reset(QScxmlStateMachine::fromFile("no_real_file_needed"));
    m_stateMachine2.reset(QScxmlStateMachine::fromFile("no_real_file_needed"));
}

void tst_scxmlqmlcpp::eventConnectionStateMachineBinding()
{
    QCOMPARE(m_eventConnection.bindableStateMachine().value(), nullptr);
    QTestPrivate::testReadWritePropertyBasics<QScxmlEventConnection, QScxmlStateMachine*>(
               m_eventConnection, m_stateMachine1.get(), m_stateMachine2.get(),  "stateMachine");
    m_eventConnection.setStateMachine(nullptr); // tidy up
}

void tst_scxmlqmlcpp::eventConnectionEventsBinding()
{
    QStringList eventList1{{"event1"},{"event2"}};
    QStringList eventList2{{"event3"},{"event4"}};
    QCOMPARE(m_eventConnection.events(), QStringList());
    QTestPrivate::testReadWritePropertyBasics<QScxmlEventConnection, QStringList>(
                m_eventConnection, eventList1, eventList2, "events");
    m_eventConnection.setEvents(QStringList()); // tidy up
}

void tst_scxmlqmlcpp::invokedServicesStateMachineBinding()
{
    QCOMPARE(m_invokedServices.stateMachine(), nullptr);
    QTestPrivate::testReadWritePropertyBasics<QScxmlInvokedServices, QScxmlStateMachine*>(
                m_invokedServices, m_stateMachine1.get(), m_stateMachine2.get(), "stateMachine");
    m_invokedServices.setStateMachine(nullptr); // tidy up
}

void tst_scxmlqmlcpp::invokedServicesChildrenBinding()
{
    TopMachine topSm;
    QScxmlInvokedServices invokedServices;
    invokedServices.setStateMachine(&topSm);
    QCOMPARE(invokedServices.children().count(), 0);
    QCOMPARE(topSm.invokedServices().count(), 0);
    // at some point during the topSm execution there are 3 invoked services
    // of the same name ('3' filters out as '1' at QML binding)
    topSm.start();
    QTRY_COMPARE(topSm.invokedServices().count(), 3);
    QCOMPARE(invokedServices.children().count(), 1);
    // after completion invoked services drop back to 0
    QTRY_COMPARE(topSm.invokedServices().count(), 0);
    QCOMPARE(invokedServices.children().count(), 0);
    // bind *to* the invokedservices property and check that we observe same changes
    // during the topSm execution
    QProperty<qsizetype> serviceCounter;
    serviceCounter.setBinding([&](){ return invokedServices.children().count(); });
    QCOMPARE(serviceCounter, 0);
    topSm.start();
    QTRY_COMPARE(serviceCounter, 1);
    QCOMPARE(topSm.invokedServices().count(), 3);
}

void tst_scxmlqmlcpp::stateMachineLoaderInitialValuesBinding()
{
    QVariantMap values1{{"key1","value1"}, {"key2","value2"}};
    QVariantMap values2{{"key3","value3"}, {"key4","value4"}};
    QTestPrivate::testReadWritePropertyBasics<QScxmlStateMachineLoader, QVariantMap>(
                m_stateMachineLoader, values1, values2, "initialValues");
    m_stateMachineLoader.setInitialValues(QVariantMap()); // tidy up
}

void tst_scxmlqmlcpp::stateMachineLoaderSourceStateMachineBinding()
{
    // Test source and stateMachine together as they interact with each other

    QUrl source1(QStringLiteral("qrc:///statemachine.scxml"));
    QUrl source2(QStringLiteral("qrc:///topmachine.scxml"));
    // The 'setSource' of the statemachineloader assumes a valid qml context
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
                "import QtQuick;\n"
                "import QtScxml;\n"
                "Item { StateMachineLoader { objectName: \'sml\'; } }",
                QUrl());
    std::unique_ptr<QObject> root(component.create());
    QScxmlStateMachineLoader *sml =
            qobject_cast<QScxmlStateMachineLoader*>(root->findChild<QObject*>("sml"));
    QVERIFY(sml != nullptr);

    // -- StateMachineLoader::source
    QTestPrivate::testReadWritePropertyBasics<QScxmlStateMachineLoader, QUrl>(
                *sml, source1, source2, "source");
    if (QTest::currentTestFailed()) {
        qWarning() << "QScxmlStateMachineLoader::source property testing failed";
        return;
    }

    // -- StateMachineLoader::stateMachine
    // The statemachine can be set indirectly by setting the 'source'
    QSignalSpy smSpy(sml, &QScxmlStateMachineLoader::stateMachineChanged);
    QUrl sourceNonexistent(QStringLiteral("qrc:///file_doesnt_exist.scxml"));
    QUrl sourceBroken(QStringLiteral("qrc:///brokenstatemachine.scxml"));

    QVERIFY(sml->stateMachine() != nullptr);
    QTest::ignoreMessage(QtWarningMsg,
                        "<Unknown File>:3:8: QML StateMachineLoader: Cannot open 'qrc:///file_doesnt_exist.scxml' for reading.");
    sml->setSource(sourceNonexistent);
    QVERIFY(sml->stateMachine() == nullptr);
    QCOMPARE(smSpy.count(), 1);
    QTest::ignoreMessage(QtWarningMsg,
                        "<Unknown File>:3:8: QML StateMachineLoader: :/brokenstatemachine.scxml:59:1: error: initial state 'working' not found for <scxml> element");
    QTest::ignoreMessage(QtWarningMsg,
                        "SCXML document has errors");
    QTest::ignoreMessage(QtWarningMsg,
                        "<Unknown File>:3:8: QML StateMachineLoader: Something went wrong while parsing 'qrc:///brokenstatemachine.scxml':\n");
    sml->setSource(sourceBroken);
    QVERIFY(sml->stateMachine() == nullptr);
    QCOMPARE(smSpy.count(), 1);

    QProperty<bool> hasStateMachine([&](){ return sml->stateMachine() ? true : false; });
    QVERIFY(hasStateMachine == false);
    sml->setSource(source1);
    QCOMPARE(smSpy.count(), 2);
    QVERIFY(hasStateMachine == true);
}

void tst_scxmlqmlcpp::stateMachineLoaderDatamodelBinding()
{
    QScxmlNullDataModel model1;
    QScxmlNullDataModel model2;
    QTestPrivate::testReadWritePropertyBasics<QScxmlStateMachineLoader,QScxmlDataModel*>
            (m_stateMachineLoader, &model1, &model2, "dataModel");
    m_stateMachineLoader.setDataModel(nullptr); // tidy up
}

QTEST_MAIN(tst_scxmlqmlcpp)
#include "tst_scxmlqmlcpp.moc"
