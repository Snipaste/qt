/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/private/qpointingdevice_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_MouseAreaInterop : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_MouseAreaInterop()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void dragHandlerInSiblingStealingGrabFromMouseAreaViaMouse();
    void dragHandlerInSiblingStealingGrabFromMouseAreaViaTouch_data();
    void dragHandlerInSiblingStealingGrabFromMouseAreaViaTouch();
    void hoverHandlerDoesntHoverOnPress();

private:
    void createView(QScopedPointer<QQuickView> &window, const char *fileName);
    QPointingDevice *touchDevice = QTest::createTouchDevice();
};

void tst_MouseAreaInterop::createView(QScopedPointer<QQuickView> &window, const char *fileName)
{
    window.reset(new QQuickView);
    window->setSource(testFileUrl(fileName));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    QQuickViewTestUtils::centerOnScreen(window.data());
    QQuickViewTestUtils::moveMouseAway(window.data());

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));
    QVERIFY(window->rootObject() != nullptr);
}

void tst_MouseAreaInterop::dragHandlerInSiblingStealingGrabFromMouseAreaViaMouse()
{
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragTakeOverFromSibling.qml");
    QQuickView * window = windowPtr.data();

    QPointer<QQuickPointerHandler> handler = window->rootObject()->findChild<QQuickPointerHandler*>();
    QVERIFY(handler);
    QQuickMouseArea *ma = window->rootObject()->findChild<QQuickMouseArea*>();
    QVERIFY(ma);

    QPoint p1(150, 150);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(window->mouseGrabberItem(), ma);
    QCOMPARE(ma->pressed(), true);

    // Start dragging
    // DragHandler keeps monitoring, due to its passive grab,
    // and eventually steals the exclusive grab from MA
    int dragStoleGrab = 0;
    auto devPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());
    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(dragThreshold / 2, 0);
        QTest::mouseMove(window, p1);

        if (!dragStoleGrab && devPriv->pointById(0)->exclusiveGrabber == handler)
            dragStoleGrab = i;
    }
    if (dragStoleGrab)
        qCDebug(lcPointerTests, "DragHandler stole the grab after %d events", dragStoleGrab);
    QVERIFY(dragStoleGrab > 1);
    QCOMPARE(handler->active(), true);
    QCOMPARE(ma->pressed(), false);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, p1);
    QCOMPARE(handler->active(), false);
}

void tst_MouseAreaInterop::dragHandlerInSiblingStealingGrabFromMouseAreaViaTouch_data()
{
    QTest::addColumn<bool>("preventStealing");

    QTest::newRow("allow stealing") << false;
    QTest::newRow("prevent stealing") << true;
}

void tst_MouseAreaInterop::dragHandlerInSiblingStealingGrabFromMouseAreaViaTouch() // QTBUG-77624 and QTBUG-79163
{
    QFETCH(bool, preventStealing);

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QScopedPointer<QQuickView> windowPtr;
    createView(windowPtr, "dragTakeOverFromSibling.qml");
    QQuickView * window = windowPtr.data();
    auto devPriv = QPointingDevicePrivate::get(touchDevice);

    QPointer<QQuickPointerHandler> handler = window->rootObject()->findChild<QQuickPointerHandler*>();
    QVERIFY(handler);
    QQuickMouseArea *ma = window->rootObject()->findChild<QQuickMouseArea*>();
    QVERIFY(ma);
    ma->setPreventStealing(preventStealing);

    QPoint p1(150, 150);
    QTest::QTouchEventSequence touch = QTest::touchEvent(window, touchDevice);

    touch.press(1, p1).commit();
    QQuickTouchUtils::flush(window);
    QTRY_VERIFY(!devPriv->activePoints.isEmpty());
    qCDebug(lcPointerTests) << "active point after press:" << devPriv->activePoints.values().first().eventPoint;
    auto epd = devPriv->queryPointById(1);
    QVERIFY(epd);
    QVERIFY(epd->passiveGrabbers.contains(handler.data()));
    QCOMPARE(epd->exclusiveGrabber, ma);
    QCOMPARE(ma->pressed(), true);

    // Start dragging
    // DragHandler keeps monitoring, due to its passive grab,
    // and eventually steals the exclusive grab from MA if MA allows it
    int dragStoleGrab = 0;
    for (int i = 0; i < 4; ++i) {
        p1 += QPoint(dragThreshold / 2, 0);
        touch.move(1, p1).commit();
        QQuickTouchUtils::flush(window);
        if (!dragStoleGrab && epd->exclusiveGrabber == handler)
            dragStoleGrab = i;
    }
    if (dragStoleGrab)
        qCDebug(lcPointerTests, "DragHandler stole the grab after %d events", dragStoleGrab);
    if (preventStealing) {
        QCOMPARE(dragStoleGrab, 0);
        QCOMPARE(handler->active(), false);
        QCOMPARE(ma->pressed(), true);
    } else {
        QVERIFY(dragStoleGrab > 1);
        QCOMPARE(handler->active(), true);
        QCOMPARE(ma->pressed(), false);
    }

    touch.release(1, p1).commit();
    QQuickTouchUtils::flush(window);
    QCOMPARE(handler->active(), false);
}

void tst_MouseAreaInterop::hoverHandlerDoesntHoverOnPress() // QTBUG-72843
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverHandlerInGrandparentOfHoverableItem.qml")));

    QPointer<QQuickHoverHandler> handler = window.rootObject()->findChild<QQuickHoverHandler*>();
    QVERIFY(handler);
    QQuickMouseArea *ma = window.rootObject()->findChild<QQuickMouseArea*>();
    QVERIFY(ma);
    QPoint p = ma->mapToScene(ma->boundingRect().center()).toPoint();

    // move the mouse below the "button" but within HoverHandler's region of interest
    QTest::mouseMove(&window, p + QPoint(0, 50));
    QTRY_COMPARE(handler->isHovered(), true);
    // move the mouse into the "button"
    QTest::mouseMove(&window, p);
    // current behavior: the mouse is still within the HoverHandler's region of interest, but MouseArea is obstructing.
    QTRY_COMPARE(handler->isHovered(), false);
    QCOMPARE(ma->hovered(), true);

    // So HoverHandler is no longer hovered (unfortunately).  Clicking should not change it.
    QSignalSpy hoveredChangedSpy(handler, SIGNAL(hoveredChanged()));
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_COMPARE(ma->pressed(), true);
    QCOMPARE(handler->isHovered(), false);
    QCOMPARE(hoveredChangedSpy.count(), 0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_COMPARE(ma->pressed(), false);
    QCOMPARE(handler->isHovered(), false);
    QCOMPARE(hoveredChangedSpy.count(), 0);
}

QTEST_MAIN(tst_MouseAreaInterop)

#include "tst_mousearea_interop.moc"
