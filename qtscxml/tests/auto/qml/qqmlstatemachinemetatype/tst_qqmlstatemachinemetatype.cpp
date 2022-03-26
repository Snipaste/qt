/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>
#include <qqmlengine.h>
#include <qqmlcomponent.h>

#include <private/qqmlmetatype_p.h>
#include <private/qqmlengine_p.h>

class tst_qqmlstatemachinemetatype : public QObject
{
    Q_OBJECT

private slots:
    void unregisterAttachedProperties()
    {
        qmlClearTypeRegistrations();
        const QUrl dummy("qrc:///doesnotexist.qml");

        QQmlEngine e;
        QQmlComponent c(&e);

        // The extra import shuffles the type IDs around, so that we
        // get a different ID for the attached properties. If the attached
        // properties aren't properly cleared, this will crash.
        c.setData("import QtQml.StateMachine 1.0 \n"
                  "import QtQuick 2.2 \n"
                  "Item { KeyNavigation.up: null }", dummy);

        const QQmlType attachedType = QQmlMetaType::qmlType("QtQuick/KeyNavigation",
                                                            QTypeRevision::fromVersion(2, 2));
        QCOMPARE(attachedType.attachedPropertiesType(QQmlEnginePrivate::get(&e)),
                 attachedType.metaObject());

        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj);
    }
};

QTEST_MAIN(tst_qqmlstatemachinemetatype)

#include "tst_qqmlstatemachinemetatype.moc"
