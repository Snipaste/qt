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

#include <QTest>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QtGui/private/qrhi_p.h>

#include "../shared/util.h"

class tst_MultiWindow : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void separateCubeSceneInMultipleWindows_data();
    void separateCubeSceneInMultipleWindows();
    void cubeMultiViewportSameWindow();
    void cubeMultiViewportMultiWindow();
};

void tst_MultiWindow::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

const int FUZZ = 5;

void tst_MultiWindow::separateCubeSceneInMultipleWindows_data()
{
    QTest::addColumn<int>("viewCount");
    QTest::newRow("2 windows") << 2;
    QTest::newRow("3 windows") << 3;
    QTest::newRow("4 windows") << 4;
}

// Open 2-4 windows, each with an independent source QML scene.
void tst_MultiWindow::separateCubeSceneInMultipleWindows()
{
    QFETCH(int, viewCount);
    QVector<QQuickView *> views;
    views.resize(viewCount);

    for (int i = 0; i < viewCount; ++i) {
        views[i] = createView(QLatin1String("texturedcube.qml"), QSize(640, 480));
        views[i]->setGeometry(i * 100, i * 100, 640, 480);
        QVERIFY(views[i]);
    }

    for (int i = 0; i < viewCount; ++i)
        QVERIFY(QTest::qWaitForWindowExposed(views[i]));

    for (int i = 0; i < viewCount; ++i) {
        const QImage result = grab(views[i]);
        if (result.isNull())
            return;
        QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(59, 192, 77), FUZZ));
    }

    qDeleteAll(views);
}

// Have a single window with 4 View3Ds rendering the same scene via importScene.
// Then change the (shared) Texture's source to be a QQuickItem.
void tst_MultiWindow::cubeMultiViewportSameWindow()
{
    QScopedPointer<QQuickView> view(createView(QLatin1String("texturedcube_multiviewport.qml"), QSize(640, 480)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QImage result = grab(view.data());
    if (result.isNull())
        return;

    const qreal dpr = view->devicePixelRatio();

    // First the cube is textured with a Qt logo.
    QVERIFY(comparePixel(result, 25, 25, dpr, QColor::fromRgb(0, 0, 0), FUZZ));
    QVERIFY(comparePixel(result, 160, 128, dpr, QColor::fromRgb(59, 192, 77), FUZZ));

    QVERIFY(comparePixel(result, 25 + 320, 25, dpr, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixel(result, 160 + 320, 128, dpr, QColor::fromRgb(59, 192, 77), FUZZ));

    QVERIFY(comparePixel(result, 25, 25 + 240, dpr, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixel(result, 160, 128 + 240, dpr, QColor::fromRgb(59, 192, 77), FUZZ));

    QVERIFY(comparePixel(result, 25 + 320, 25 + 240, dpr, QColor::fromRgb(0, 0, 255), FUZZ));
    QVERIFY(comparePixel(result, 160 + 320, 128 + 240, dpr, QColor::fromRgb(59, 192, 77), FUZZ));

    QMetaObject::invokeMethod(view->rootObject(), "changeToSourceItemBasedTexture");

    result = grab(view.data());
    if (result.isNull())
        return;

    // Now the texture is a filled red rectangle.
    QVERIFY(comparePixel(result, 25, 25, dpr, QColor::fromRgb(0, 0, 0), FUZZ));
    QVERIFY(comparePixel(result, 160, 128, dpr, QColor::fromRgb(239, 0, 0), FUZZ));

    QVERIFY(comparePixel(result, 25 + 320, 25, dpr, QColor::fromRgb(255, 0, 0), FUZZ));
    QVERIFY(comparePixel(result, 160 + 320, 128, dpr, QColor::fromRgb(239, 0, 0), FUZZ));

    QVERIFY(comparePixel(result, 25, 25 + 240, dpr, QColor::fromRgb(0, 128, 0), FUZZ));
    QVERIFY(comparePixel(result, 160, 128 + 240, dpr, QColor::fromRgb(239, 0, 0), FUZZ));

    QVERIFY(comparePixel(result, 25 + 320, 25 + 240, dpr, QColor::fromRgb(0, 0, 255), FUZZ));
    QVERIFY(comparePixel(result, 160 + 320, 128 + 240, dpr, QColor::fromRgb(239, 0, 0), FUZZ));
}

// Two View3Ds in two separate windows, importing the same scene.
void tst_MultiWindow::cubeMultiViewportMultiWindow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("texturedcube_multiviewport_multiwindow.qml"));
    QObject *obj = component.create();
    QScopedPointer<QObject> cleanup(obj);
    QVERIFY(obj);

    QQuickWindow *window1 = qobject_cast<QQuickWindow*>(obj);
    QVERIFY(window1);

    QQuickWindow *window2 = window1->findChild<QQuickWindow *>("window2");
    QVERIFY(window2);

    QVERIFY(QTest::qWaitForWindowExposed(window1));
    QVERIFY(QTest::qWaitForWindowExposed(window2));

    QImage result = grab(window1);
    if (result.isNull())
        return;

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(59, 192, 77), FUZZ));

    result = grab(window2);
    if (result.isNull())
        return;

    QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(59, 192, 77), FUZZ));

    {
        QQuick3DTestMessageHandler msgCatcher;

        QMetaObject::invokeMethod(window1, "changeToSourceItemBasedTexture");

        result = grab(window1);
        if (result.isNull())
            return;

        QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(239, 0, 0), FUZZ));

        result = grab(window2);
        if (result.isNull())
            return;

        // Have to check if we are using the threaded render loop
        // since using a Texture with sourceItem from multiple windows
        // is only a problem with separate render threads.
        QRhi *rhi = static_cast<QRhi *>(window1->rendererInterface()->getResource(window1, QSGRendererInterface::RhiResource));
        QVERIFY(rhi);
        if (rhi->thread() != QThread::currentThread()) {
            QVERIFY(msgCatcher.messageString().contains(QLatin1String("Cannot use QSGTexture")));
        } else {
            QVERIFY(comparePixelNormPos(result, 0.5, 0.5, QColor::fromRgb(239, 0, 0), FUZZ));
        }
    }
}

QTEST_MAIN(tst_MultiWindow)
#include "tst_multiwindow.moc"
