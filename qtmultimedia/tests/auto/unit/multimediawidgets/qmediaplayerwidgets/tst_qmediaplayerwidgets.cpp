/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

//TESTED_COMPONENT=src/multimedia

#include <QtTest/QtTest>
#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>

#include <qgraphicsvideoitem.h>
#include <qvideosink.h>
#include <qmediaplayer.h>
#include <private/qplatformmediaplayer_p.h>

#include "qvideosink.h"
#include "qmockintegration_p.h"

QT_USE_NAMESPACE

class tst_QMediaPlayerWidgets: public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void testSetVideoOutput();
    void testSetVideoOutputNoService();
    void testSetVideoOutputNoControl();

private:
    QMockIntegration *mockIntegration;
};

void tst_QMediaPlayerWidgets::initTestCase()
{
    mockIntegration = new QMockIntegration;
}

void tst_QMediaPlayerWidgets::cleanupTestCase()
{
    delete mockIntegration;
}

void tst_QMediaPlayerWidgets::init()
{
}

void tst_QMediaPlayerWidgets::cleanup()
{
}

void tst_QMediaPlayerWidgets::testSetVideoOutput()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;

    QMediaPlayer player;

    player.setVideoOutput(&widget);
//    QVERIFY(widget.mediaSource() == &player);

    player.setVideoOutput(&item);
//    QVERIFY(widget.mediaSource() == nullptr);
//    QVERIFY(item.mediaSource() == &player);

    player.setVideoOutput(reinterpret_cast<QVideoWidget *>(0));
//    QVERIFY(item.mediaSource() == nullptr);

    player.setVideoOutput(&widget);
//    QVERIFY(widget.mediaSource() == &player);

    player.setVideoOutput(reinterpret_cast<QGraphicsVideoItem *>(0));
//    QVERIFY(widget.mediaSource() == nullptr);

    player.setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);

    player.setVideoOutput(reinterpret_cast<QVideoSink *>(0));
//    QVERIFY(mockService->rendererControl->surface() == nullptr);

    player.setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);

    player.setVideoOutput(&widget);
//    QVERIFY(mockService->rendererControl->surface() == nullptr);
//    QVERIFY(widget.mediaSource() == &player);

    player.setVideoOutput(&surface);
//    QVERIFY(mockService->rendererControl->surface() == &surface);
//    QVERIFY(widget.mediaSource() == nullptr);
}


void tst_QMediaPlayerWidgets::testSetVideoOutputNoService()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;

    mockIntegration->setFlags(QMockIntegration::NoPlayerInterface);
    QMediaPlayer player;
    mockIntegration->setFlags({});

    player.setVideoOutput(&widget);

    player.setVideoOutput(&item);

    player.setVideoOutput(&surface);
    // Nothing we can verify here other than it doesn't assert.
}

void tst_QMediaPlayerWidgets::testSetVideoOutputNoControl()
{
    QVideoWidget widget;
    QGraphicsVideoItem item;
    QVideoSink surface;

    QMediaPlayer player;

    player.setVideoOutput(&widget);

    player.setVideoOutput(&item);

    player.setVideoOutput(&surface);
}

QTEST_MAIN(tst_QMediaPlayerWidgets)
#include "tst_qmediaplayerwidgets.moc"
