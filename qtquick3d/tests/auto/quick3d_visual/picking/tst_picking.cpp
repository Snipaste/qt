/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <QSignalSpy>
#include <QTest>
#include <QtQuick/QQuickItem>

#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>
#include <QtQuick3D/private/qquick3ditem2d_p.h>

#include "../shared/util.h"

class tst_Picking : public QQuick3DDataTest
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() override;
    void test_object_picking();

private:
    QQuickItem *find2DChildIn3DNode(QQuickView *view, const QString &objectName, const QString &itemName);

};

void tst_Picking::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

QQuickItem *tst_Picking::find2DChildIn3DNode(QQuickView *view, const QString &objectName, const QString &itemName)
{
    QQuick3DNode *obj = view->rootObject()->findChild<QQuick3DNode*>(objectName);
    if (!obj)
        return nullptr;
    QQuickItem *subsceneRoot = obj->findChild<QQuickItem *>();
    if (!subsceneRoot)
        subsceneRoot = obj->findChild<QQuick3DItem2D *>()->contentItem();
    if (!subsceneRoot)
        return nullptr;
    QObject *child = subsceneRoot->findChild<QObject *>(itemName);
    return static_cast<QQuickItem *>(child);
}

void tst_Picking::test_object_picking()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("picking.qml"), QSize(400, 400)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QQuick3DViewport *view3d = view->findChild<QQuick3DViewport *>(QStringLiteral("view"));
    QVERIFY(view3d);
    QQuick3DModel *model1 = view3d->findChild<QQuick3DModel *>(QStringLiteral("model1"));
    QVERIFY(model1);
    QQuick3DModel *model2 = view3d->findChild<QQuick3DModel *>(QStringLiteral("model2"));
    QVERIFY(model2);
    QQuickItem *item2d = qmlobject_cast<QQuickItem *>(find2DChildIn3DNode(view.data(), "item2dNode", "item2d"));
    QVERIFY(item2d);

    // Pick nearest based on viewport position

    // Center of model1
    auto result = view3d->pick(200, 200);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Upper right corner of model1
    result = view3d->pick(250, 150);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Just outside model1's upper right corner, so should hit the model behind (model2)
    result = view3d->pick(251, 151);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model2);

    // Upper right corner of model2
    result = view3d->pick(300, 100);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model2);

    // Just outside model2's upper right corner, so there should be no hit
    result = view3d->pick(301, 99);
    QCOMPARE(result.objectHit(), nullptr);

    // Enable the 2D item
    item2d->setEnabled(true);
    QTest::qWait(100);
    // Then picking on top of model1 should not pick it anymore
    result = view3d->pick(200, 200);
    QVERIFY(result.objectHit() != model1);
    // Hide the 2D item
    item2d->setVisible(false);
    QTest::qWait(100);
    // Then picking on top of model1 should pick it again
    result = view3d->pick(200, 200);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Pick all based on viewport position

    // Center of model1, bottom left corner of model2
    auto resultList = view3d->pickAll(200, 200);
    QCOMPARE(resultList.count(), 2);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);

    // Top right corner of model1, center of model2
    resultList = view3d->pickAll(250, 150);
    QCOMPARE(resultList.count(), 2);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);

    // Just outside model1's upper right corner, so should hit the model behind (model2)
    resultList = view3d->pickAll(251, 151);
    QCOMPARE(resultList.count(), 1);
    QCOMPARE(resultList[0].objectHit(), model2);

    // Just outside model2's upper right corner, so there should be no hit
    resultList = view3d->pickAll(301, 99);
    QVERIFY(resultList.isEmpty());

    // Ray based picking one result

    // Down the z axis from 0,0,100 (towards 0,0,0)
    QVector3D origin(0.0f, 0.0f, 100.0f);
    QVector3D direction(0.0f, 0.0f, -1.0f);
    result = view3d->rayPick(origin, direction);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model1);

    // Up the z axis from 0, 0, -100 (towards 0,0,0)
    origin = QVector3D(0.0f, 0.0f, -100.0f);
    direction = QVector3D(0.0f, 0.0f, 1.0f);
    result = view3d->rayPick(origin, direction);
    QVERIFY(result.objectHit() != nullptr);
    QCOMPARE(result.objectHit(), model2);

    // Up the z axis from 0, 0, -100 (towards 0,0,-1000)
    origin = QVector3D(0.0f, 0.0f, -100.0f);
    direction = QVector3D(0.0f, 0.0f, -1.0f);
    result = view3d->rayPick(origin, direction);
    QVERIFY(result.objectHit() == nullptr);

    // Ray based picking all results

    // Down the z axis from 0,0,100 (towards 0,0,0)
    origin = QVector3D(0.0f, 0.0f, 100.0f);
    direction = QVector3D(0.0f, 0.0f, -1.0f);
    resultList = view3d->rayPickAll(origin, direction);
    QCOMPARE(resultList.count(), 2);
    QCOMPARE(resultList[0].objectHit(), model1);
    QCOMPARE(resultList[1].objectHit(), model2);

    // Up the z axis from 0, 0, -100 (towards 0,0,0)
    origin = QVector3D(0.0f, 0.0f, -100.0f);
    direction = QVector3D(0.0f, 0.0f, 1.0f);
    resultList = view3d->rayPickAll(origin, direction);
    QCOMPARE(resultList.count(), 2);
    QCOMPARE(resultList[0].objectHit(), model2);
    QCOMPARE(resultList[1].objectHit(), model1);

    // Up the z axis from 0, 0, -100 (towards 0,0,-1000)
    origin = QVector3D(0.0f, 0.0f, -100.0f);
    direction = QVector3D(0.0f, 0.0f, -1.0f);
    resultList = view3d->rayPickAll(origin, direction);
    QVERIFY(resultList.isEmpty());
}

QTEST_MAIN(tst_Picking)
#include "tst_picking.moc"
