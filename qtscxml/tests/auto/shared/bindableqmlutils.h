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

#ifndef BINDABLEQMLUTILS_H
#define BINDABLEQMLUTILS_H

#include <QtQml/QQmlListReference>
#include <QtQml/QQmlProperty>
#include <QObject>
#include <QtTest>

// This is a helper function to test basics of typical bindable
// and manipulable QQmlListProperty
// "TestedClass" is the class type we are testing
// "TestedData" is the data type that the list contains
// "testedClass" is an instance of the class we are interested testing
// "data1" is a value that can be set and removed from the list
// "data2" is a value that can be set and removed from the list
// "propertyName" is the name of the property we are interested in testing
template<typename TestedClass, typename TestedData>
void testManipulableQmlListBasics(TestedClass& testedClass,
                                TestedData data1, TestedData data2,
                                const char* propertyName)
{
    // Get the property we are testing
    const QMetaObject *metaObject = testedClass.metaObject();
    QMetaProperty metaProperty = metaObject->property(metaObject->indexOfProperty(propertyName));

    // Generate a string to help identify failures (as this is a generic template)
    QString id(metaObject->className());
    id.append(QStringLiteral("::"));
    id.append(propertyName);

    // Fail gracefully if preconditions to use this helper function are not met:
    QQmlListReference listRef(&testedClass, propertyName);
    QVERIFY2(metaProperty.isBindable(), qPrintable(id));
    QVERIFY2(listRef.isManipulable(), qPrintable(id));
    QVERIFY2(data1 != data2, qPrintable(id));

    // Create a signal spy for the property changed -signal if such exists
    QScopedPointer<QSignalSpy> changedSpy(nullptr);
    if (metaProperty.hasNotifySignal())
        changedSpy.reset(new QSignalSpy(&testedClass, metaProperty.notifySignal()));

    // Modify values without binding (verify property basics still work)
    QVERIFY2(listRef.count() == 0, qPrintable(id));
    listRef.append(data1);
    QVERIFY2(listRef.count() == 1, qPrintable(id));
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 1, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));
    QVERIFY2(listRef.at(0) == data1, qPrintable(id));
    listRef.clear();
    QVERIFY2(listRef.count() == 0, qPrintable(id));
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 2, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));

    // Bind to the property and verify that the bindings reflect the listproperty changes
    QProperty<bool> data1InList([&](){
        QQmlListReference list(&testedClass, propertyName);
        for (int i = 0; i < list.count(); i++) {
            if (list.at(i) == data1)
                return true;
        }
        return false;
    });
    QProperty<bool> data2InList([&](){
        QQmlListReference list(&testedClass, propertyName);
        for (int i = 0; i < list.count(); i++) {
            if (list.at(i) == data2)
                return true;
        }
        return false;
    });

    // initial state
    QVERIFY2(!data1InList, qPrintable(id));
    QVERIFY2(!data2InList, qPrintable(id));

    listRef.append(data1);
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 3, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));
    QVERIFY2(data1InList, qPrintable(id));
    QVERIFY2(!data2InList, qPrintable(id));

    listRef.append(data2);
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 4, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));
    QVERIFY2(data1InList, qPrintable(id));
    QVERIFY2(data2InList, qPrintable(id));
    QVERIFY2(listRef.count() == 2, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));

    listRef.clear();
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 5, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));
    QVERIFY2(!data1InList, qPrintable(id));
    QVERIFY2(!data2InList, qPrintable(id));
    QVERIFY2(listRef.count() == 0, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));

    listRef.append(data1);
    listRef.replace(0, data2);
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 7, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));
    QVERIFY2(!data1InList, qPrintable(id));
    QVERIFY2(data2InList, qPrintable(id));
    QVERIFY2(listRef.count() == 1, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));

    listRef.removeLast();
    if (changedSpy)
        QVERIFY2(changedSpy->count() == 8, qPrintable(id + ", actual: " + QString::number(changedSpy->count())));
    QVERIFY2(!data1InList, qPrintable(id));
    QVERIFY2(!data2InList, qPrintable(id));

    QVERIFY2(listRef.count() == 0, qPrintable(id + ", actual: " + QString::number(listRef.count())));
}

#endif // BINDABLEQMLUTILS_H
