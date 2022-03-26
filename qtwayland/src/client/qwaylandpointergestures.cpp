/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwaylandpointergestures_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandinputdevice_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandPointerGestures::QWaylandPointerGestures(QWaylandDisplay *display, uint id, uint version)
    : zwp_pointer_gestures_v1(display->wl_registry(), id, qMin(version, uint(1)))
{
}

QWaylandPointerGestureSwipe *
    QWaylandPointerGestures::createPointerGestureSwipe(QWaylandInputDevice *device)
{
    return new QWaylandPointerGestureSwipe(device);
}

QWaylandPointerGesturePinch *
    QWaylandPointerGestures::createPointerGesturePinch(QWaylandInputDevice *device)
{
    return new QWaylandPointerGesturePinch(device);
}

QWaylandPointerGestureSwipe::QWaylandPointerGestureSwipe(QWaylandInputDevice *p)
    : mParent(p)
{
}

QWaylandPointerGestureSwipe::~QWaylandPointerGestureSwipe()
{
    destroy();
}

void QWaylandPointerGestureSwipe::zwp_pointer_gesture_swipe_v1_begin(uint32_t serial, uint32_t time,
                                                                     struct ::wl_surface *surface,
                                                                     uint32_t fingers)
{
#ifndef QT_NO_GESTURES
    mParent->mSerial = serial;
    mFocus = QWaylandWindow::fromWlSurface(surface);
    mFingers = fingers;

    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_begin @ "
                               << pointer->mSurfacePos << "fingers" << fingers;

    auto e = QWaylandPointerGestureSwipeEvent(mFocus, Qt::GestureStarted, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF());

    mFocus->handleSwipeGesture(mParent, e);
#endif
}

void QWaylandPointerGestureSwipe::zwp_pointer_gesture_swipe_v1_update(uint32_t time,
                                                                      wl_fixed_t dx, wl_fixed_t dy)
{
#ifndef QT_NO_GESTURES
    const auto* pointer = mParent->pointer();

    const QPointF delta = QPointF(wl_fixed_to_double(dx), wl_fixed_to_double(dy));
    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_update @ "
                               << pointer->mSurfacePos << "delta" << delta;

    auto e = QWaylandPointerGestureSwipeEvent(mFocus, Qt::GestureUpdated, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers, delta);

    mFocus->handleSwipeGesture(mParent, e);
#endif
}

void QWaylandPointerGestureSwipe::zwp_pointer_gesture_swipe_v1_end(uint32_t serial, uint32_t time,
                                                                   int32_t cancelled)
{
#ifndef QT_NO_GESTURES
    mParent->mSerial = serial;
    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_end @ "
                               << pointer->mSurfacePos << (cancelled ? "CANCELED" : "");

    auto gestureType = cancelled ? Qt::GestureFinished : Qt::GestureCanceled;

    auto e = QWaylandPointerGestureSwipeEvent(mFocus, gestureType, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF());

    mFocus->handleSwipeGesture(mParent, e);

    mFocus.clear();
    mFingers = 0;
#endif
}

QWaylandPointerGesturePinch::QWaylandPointerGesturePinch(QWaylandInputDevice *p)
    : mParent(p)
{
}

QWaylandPointerGesturePinch::~QWaylandPointerGesturePinch()
{
    destroy();
}

void QWaylandPointerGesturePinch::zwp_pointer_gesture_pinch_v1_begin(uint32_t serial, uint32_t time,
                                                                     struct ::wl_surface *surface,
                                                                     uint32_t fingers)
{
#ifndef QT_NO_GESTURES
    mParent->mSerial = serial;
    mFocus = QWaylandWindow::fromWlSurface(surface);
    mFingers = fingers;
    mLastScale = 1;

    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_pinch_v1_begin @ "
                               << pointer->mSurfacePos << "fingers" << fingers;

    auto e = QWaylandPointerGesturePinchEvent(mFocus, Qt::GestureStarted, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF(), 0, 0);

    mFocus->handlePinchGesture(mParent, e);
#endif
}

void QWaylandPointerGesturePinch::zwp_pointer_gesture_pinch_v1_update(uint32_t time,
                                                                      wl_fixed_t dx, wl_fixed_t dy,
                                                                      wl_fixed_t scale,
                                                                      wl_fixed_t rotation)
{
#ifndef QT_NO_GESTURES
    const auto* pointer = mParent->pointer();

    const qreal rscale = wl_fixed_to_double(scale);
    const qreal rot = wl_fixed_to_double(rotation);
    const QPointF delta = QPointF(wl_fixed_to_double(dx), wl_fixed_to_double(dy));
    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_pinch_v1_update @ "
                               << pointer->mSurfacePos << "delta" << delta
                               << "scale" << mLastScale << "->" << rscale
                               << "delta" << rscale - mLastScale << "rot" << rot;

    auto e = QWaylandPointerGesturePinchEvent(mFocus, Qt::GestureUpdated, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              delta, rscale - mLastScale, rot);

    mFocus->handlePinchGesture(mParent, e);

    mLastScale = rscale;
#endif
}

void QWaylandPointerGesturePinch::zwp_pointer_gesture_pinch_v1_end(uint32_t serial, uint32_t time,
                                                                   int32_t cancelled)
{
#ifndef QT_NO_GESTURES
    mParent->mSerial = serial;
    const auto* pointer = mParent->pointer();

    qCDebug(lcQpaWaylandInput) << "zwp_pointer_gesture_swipe_v1_end @ "
                               << pointer->mSurfacePos << (cancelled ? "CANCELED" : "");

    auto gestureType = cancelled ? Qt::GestureFinished : Qt::GestureCanceled;

    auto e = QWaylandPointerGesturePinchEvent(mFocus, gestureType, time,
                                              pointer->mSurfacePos, pointer->mGlobalPos, mFingers,
                                              QPointF(), 0, 0);

    mFocus->handlePinchGesture(mParent, e);

    mFocus.clear();
    mFingers = 0;
    mLastScale = 1;
#endif
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
