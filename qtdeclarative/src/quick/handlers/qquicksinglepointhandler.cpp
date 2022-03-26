/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquicksinglepointhandler_p.h"
#include "qquicksinglepointhandler_p_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcTouchTarget)

/*!
    \qmltype SinglePointHandler
    \qmlabstract
    \preliminary
    \instantiates QQuickSinglePointHandler
    \inherits PointerDeviceHandler
    \inqmlmodule QtQuick
    \brief Abstract handler for single-point Pointer Events.

    An intermediate class (not registered as a QML type)
    for the most common handlers: those which expect only a single point.
    wantsPointerEvent() will choose the first point which is inside the
    \l target item, and return true as long as the event contains that point.
    Override handleEventPoint() to implement a single-point handler.
*/

QQuickSinglePointHandler::QQuickSinglePointHandler(QQuickItem *parent)
  : QQuickPointerDeviceHandler(*(new QQuickSinglePointHandlerPrivate), parent)
{
}

QQuickSinglePointHandler::QQuickSinglePointHandler(QQuickSinglePointHandlerPrivate &dd, QQuickItem *parent)
  : QQuickPointerDeviceHandler(dd, parent)
{
}

bool QQuickSinglePointHandler::wantsPointerEvent(QPointerEvent *event)
{
    Q_D(QQuickSinglePointHandler);
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;

    if (d->pointInfo.id() != -1) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        // If we no longer want it, cancel the grab.
        int candidatePointCount = 0;
        bool missing = true;
        QEventPoint *point = nullptr;
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &p = event->point(i);
            const bool found = (p.id() == d->pointInfo.id());
            if (found)
                missing = false;
            if (wantsEventPoint(event, p)) {
                ++candidatePointCount;
                if (found)
                    point = &p;
            }
        }
        if (missing)
            qCWarning(lcTouchTarget) << this << "pointId" << Qt::hex << d->pointInfo.id()
                << "is missing from current event, but was neither canceled nor released";
        if (point) {
            if (candidatePointCount == 1 || (candidatePointCount > 1 && d->ignoreAdditionalPoints)) {
                point->setAccepted();
                return true;
            } else {
                cancelAllGrabs(event, *point);
            }
        } else {
            return false;
        }
    } else {
        // We have not yet chosen a point; choose the first one for which wantsEventPoint() returns true.
        int candidatePointCount = 0;
        QEventPoint *chosen = nullptr;
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &p = event->point(i);
            if (!event->exclusiveGrabber(p) && wantsEventPoint(event, p)) {
                ++candidatePointCount;
                if (!chosen) {
                    chosen = &p;
                    break;
                }
            }
        }
        if (chosen && candidatePointCount == 1) {
            setPointId(chosen->id());
            chosen->setAccepted();
        }
    }
    return d->pointInfo.id() != -1;
}

void QQuickSinglePointHandler::handlePointerEventImpl(QPointerEvent *event)
{
    Q_D(QQuickSinglePointHandler);
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QEventPoint *currentPoint = const_cast<QEventPoint *>(event->pointById(d->pointInfo.id()));
    Q_ASSERT(currentPoint);
    d->pointInfo.reset(event, *currentPoint);
    handleEventPoint(event, *currentPoint);
    emit pointChanged();
}

void QQuickSinglePointHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    if (point.state() != QEventPoint::Released)
        return;

    const Qt::MouseButtons releasedButtons = static_cast<QSinglePointEvent *>(event)->buttons();
    if ((releasedButtons & acceptedButtons()) == Qt::NoButton) {
        setExclusiveGrab(event, point, false);
        d_func()->reset();
    }
}

void QQuickSinglePointHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point)
{
    Q_D(QQuickSinglePointHandler);
    if (grabber != this)
        return;
    switch (transition) {
    case QPointingDevice::GrabExclusive:
        d->pointInfo.m_sceneGrabPosition = point.sceneGrabPosition();
        setActive(true);
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        break;
    case QPointingDevice::GrabPassive:
        d->pointInfo.m_sceneGrabPosition = point.sceneGrabPosition();
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        break;
    case QPointingDevice::OverrideGrabPassive:
        return; // don't emit
    case QPointingDevice::UngrabPassive:
    case QPointingDevice::UngrabExclusive:
    case QPointingDevice::CancelGrabPassive:
    case QPointingDevice::CancelGrabExclusive:
        // the grab is lost or relinquished, so the point is no longer relevant
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        d->reset();
        break;
    }
}

void QQuickSinglePointHandler::setIgnoreAdditionalPoints(bool v)
{
    Q_D(QQuickSinglePointHandler);
    d->ignoreAdditionalPoints = v;
}

void QQuickSinglePointHandler::moveTarget(QPointF pos, QEventPoint &point)
{
    Q_D(QQuickSinglePointHandler);
    target()->setPosition(pos);
    d->pointInfo.m_scenePosition = point.scenePosition();
    d->pointInfo.m_position = target()->mapFromScene(d->pointInfo.m_scenePosition);
}

void QQuickSinglePointHandler::setPointId(int id)
{
    Q_D(QQuickSinglePointHandler);
    d->pointInfo.m_id = id;
}

QQuickHandlerPoint QQuickSinglePointHandler::point() const
{
    Q_D(const QQuickSinglePointHandler);
    return d->pointInfo;
}

/*!
    \readonly
    \qmlproperty HandlerPoint QtQuick::SinglePointHandler::point

    The event point currently being handled. When no point is currently being
    handled, this object is reset to default values (all coordinates are 0).
*/

QQuickSinglePointHandlerPrivate::QQuickSinglePointHandlerPrivate()
  : QQuickPointerDeviceHandlerPrivate()
{
}

void QQuickSinglePointHandlerPrivate::reset()
{
    Q_Q(QQuickSinglePointHandler);
    q->setActive(false);
    pointInfo.reset();
}

QT_END_NAMESPACE
