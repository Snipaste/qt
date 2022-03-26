/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickselectionrectangle_p.h"
#include "qquickselectionrectangle_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquickhoverhandler_p.h>

#include <QtQuick/private/qquicktableview_p_p.h>

#include "qquickscrollview_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype SelectionRectangle
    \inherits Control
//!     \instantiates QQuickSelectionRectangle
    \inqmlmodule QtQuick.Controls
    \since 6.2
    \ingroup utilities
    \brief Used to select table cells inside a TableView.

    \image qtquickcontrols2-selectionrectangle.png

    SelectionRectangle is used for selecting table cells in a TableView. It lets
    the user start a selection by doing a pointer drag inside the viewport, or by
    doing a long press on top of a cell.

    For a SelectionRectangle to be able to select cells, TableView must have
    an ItemSelectionModel assigned. The ItemSelectionModel will store any
    selections done on the model, and can be used for querying
    which cells that the user has selected.

    The following example shows how you can make a SelectionRectangle target
    a TableView:

    \snippet qtquickcontrols2-selectionrectangle.qml 0

    \note A SelectionRectangle itself is not shown as part of a selection. Only the
    delegates (like topLeftHandle and bottomRightHandle) are used.
    You should also consider \l {Selecting items}{rendering the TableView delegate as selected}.

    \sa TableView, TableView::selectionModel, ItemSelectionModel
*/

/*!
    \qmlproperty Item QtQuick.Controls::SelectionRectangle::target

    This property holds the TableView on which the
    SelectionRectangle should act.
*/

/*!
    \qmlproperty bool QtQuick.Controls::SelectionRectangle::active
    \readonly

    This property is \c true while the user is performing a
    selection. The selection will be active from the time the
    the user starts to select, and until the selection is
    removed again, for example from tapping inside the viewport.
*/

/*!
    \qmlproperty bool QtQuick.Controls::SelectionRectangle::dragging
    \readonly

    This property is \c true whenever the user is doing a pointer drag or
    a handle drag to adjust the selection rectangle.
*/

/*!
    \qmlproperty Component QtQuick.Controls::SelectionRectangle::topLeftHandle

    This property holds the delegate that will be shown on the center of the
    top-left corner of the selection rectangle. When a handle is
    provided, the user can drag it to adjust the selection.

    Set this property to \c null if you don't want a selection handle on the top-left.

    \sa bottomRightHandle
*/

/*!
    \qmlproperty Component QtQuick.Controls::SelectionRectangle::bottomRightHandle

    This property holds the delegate that will be shown on the center of the
    top-left corner of the selection rectangle. When a handle is
    provided, the user can drag it to adjust the selection.

    Set this property to \c null if you don't want a selection handle on the bottom-right.

    \sa topLeftHandle
*/

/*!
    \qmlproperty enumeration QtQuick.Controls::SelectionRectangle::selectionMode

    This property holds when a selection should start.

    \value SelectionRectangle.Drag A selection will start by doing a pointer drag inside the viewport
    \value SelectionRectangle.PressAndHold A selection will start by doing a press and hold on top a cell
    \value SelectionRectangle.Auto SelectionRectangle will choose which mode to use based on the target
        and the platform. This normally means \c PressAndHold on touch based platforms, and \c Drag on desktop.
        However, \c Drag will only be used if it doesn't conflict with flicking. This means that
        TableView will need to be configured with \c interactive set to \c false, or placed
        inside a ScrollView (where flicking, by default, is off for mouse events), for \c Drag to be chosen.

    The default value is \c Auto.
*/

/*!
    \qmlattachedproperty SelectionRectangle QtQuick::SelectionRectangle::control

    This attached property holds the SelectionRectangle that manages the delegate instance.
    It is attached to each handle instance.
*/

/*!
    \qmlattachedproperty bool QtQuick::SelectionRectangle::dragging

    This attached property will be \c true if the user is dragging on the handle.
    It is attached to each handle instance.
*/

QQuickSelectionRectanglePrivate::QQuickSelectionRectanglePrivate()
    : QQuickControlPrivate()
{
    m_tapHandler = new QQuickTapHandler();
    m_dragHandler = new QQuickDragHandler();
    m_dragHandler->setTarget(nullptr);

    QObject::connect(&m_scrollTimer, &QTimer::timeout, [&]{
        if (m_topLeftHandle && m_draggedHandle == m_topLeftHandle)
            m_selectable->setSelectionStartPos(m_scrollToPoint);
        else
            m_selectable->setSelectionEndPos(m_scrollToPoint);
        updateHandles();
        const QSizeF dist = m_selectable->scrollTowardsSelectionPoint(m_scrollToPoint, m_scrollSpeed);
        m_scrollToPoint.rx() += dist.width() > 0 ? m_scrollSpeed.width() : -m_scrollSpeed.width();
        m_scrollToPoint.ry() += dist.height() > 0 ? m_scrollSpeed.height() : -m_scrollSpeed.height();
        m_scrollSpeed = QSizeF(qAbs(dist.width() * 0.007), qAbs(dist.height() * 0.007));
    });

    QObject::connect(m_tapHandler, &QQuickTapHandler::tapped, [=](QEventPoint) {
        m_selectable->clearSelection();
        updateActiveState(false);
    });

    QObject::connect(m_tapHandler, &QQuickTapHandler::longPressed, [=]() {
        if (handleUnderPos(m_tapHandler->point().pressPosition()) != nullptr) {
            // Don't allow press'n'hold to start a new
            // selection if it started on top of a handle.
            return;
        }
        if (!m_alwaysAcceptPressAndHold) {
            if (m_selectionMode == QQuickSelectionRectangle::Auto) {
                // In Auto mode, we only accept press and hold from touch
                if (m_tapHandler->point().device()->pointerType() != QPointingDevice::PointerType::Finger)
                    return;
            } else if (m_selectionMode != QQuickSelectionRectangle::PressAndHold) {
                return;
            }
        }

        const QPointF pos = m_tapHandler->point().position();
        m_selectable->clearSelection();
        m_selectable->setSelectionStartPos(pos);
        m_selectable->setSelectionEndPos(pos);
        updateHandles();
        updateActiveState(true);
    });

    QObject::connect(m_dragHandler, &QQuickDragHandler::activeChanged, [=]() {
        const QPointF pos = m_dragHandler->centroid().position();
        if (m_dragHandler->active()) {
            m_selectable->clearSelection();
            m_selectable->setSelectionStartPos(pos);
            m_selectable->setSelectionEndPos(pos);
            m_draggedHandle = nullptr;
            updateHandles();
            updateActiveState(true);
            updateDraggingState(true);
        } else {
            m_scrollTimer.stop();
            m_selectable->normalizeSelection();
            updateDraggingState(false);
        }
    });

    QObject::connect(m_dragHandler, &QQuickDragHandler::centroidChanged, [=]() {
        if (!m_dragging)
            return;
        const QPointF pos = m_dragHandler->centroid().position();
        m_selectable->setSelectionEndPos(pos);
        updateHandles();
        scrollTowardsPos(pos);
    });
}

void QQuickSelectionRectanglePrivate::scrollTowardsPos(const QPointF &pos)
{
    m_scrollToPoint = pos;
    if (m_scrollTimer.isActive())
        return;

    const QSizeF dist = m_selectable->scrollTowardsSelectionPoint(m_scrollToPoint, m_scrollSpeed);
    if (!dist.isNull())
        m_scrollTimer.start(1);
}

QQuickItem *QQuickSelectionRectanglePrivate::handleUnderPos(const QPointF &pos)
{
    const auto handlerTarget = m_selectable->selectionPointerHandlerTarget();
    if (m_topLeftHandle) {
        const QPointF localPos = m_topLeftHandle->mapFromItem(handlerTarget, pos);
        if (m_topLeftHandle->contains(localPos))
            return m_topLeftHandle;
    }

    if (m_bottomRightHandle) {
        const QPointF localPos = m_bottomRightHandle->mapFromItem(handlerTarget, pos);
        if (m_bottomRightHandle->contains(localPos))
            return m_bottomRightHandle;
    }

    return nullptr;
}

void QQuickSelectionRectanglePrivate::updateDraggingState(bool dragging)
{
    if (dragging != m_dragging) {
        m_dragging = dragging;
        emit q_func()->draggingChanged();
    }

    if (auto attached = getAttachedObject(m_draggedHandle))
        attached->setDragging(dragging);
}

void QQuickSelectionRectanglePrivate::updateActiveState(bool active)
{
    if (active == m_active)
        return;

    m_active = active;
    emit q_func()->activeChanged();
}

QQuickItem *QQuickSelectionRectanglePrivate::createHandle(QQmlComponent *delegate, Qt::Corner corner)
{
    Q_Q(QQuickSelectionRectangle);

    // Incubate the handle
    QObject *obj = delegate->beginCreate(QQmlEngine::contextForObject(q));
    QQuickItem *handleItem = qobject_cast<QQuickItem*>(obj);
    const auto handlerTarget = m_selectable->selectionPointerHandlerTarget();
    handleItem->setParentItem(handlerTarget);
    if (auto attached = getAttachedObject(handleItem))
        attached->setControl(q);
    delegate->completeCreate();
    if (handleItem->z() == 0)
        handleItem->setZ(100);

    // Add pointer handlers to it
    QQuickDragHandler *dragHandler = new QQuickDragHandler();
    dragHandler->setTarget(nullptr);
    dragHandler->setParent(handleItem);
    QQuickItemPrivate::get(handleItem)->addPointerHandler(dragHandler);

    QQuickHoverHandler *hoverHandler = new QQuickHoverHandler();
    hoverHandler->setTarget(nullptr);
    hoverHandler->setParent(handleItem);
    hoverHandler->setCursorShape(Qt::SizeFDiagCursor);
    QQuickItemPrivate::get(handleItem)->addPointerHandler(hoverHandler);

    QObject::connect(dragHandler, &QQuickDragHandler::activeChanged, [=]() {
        if (dragHandler->active()) {
            const QPointF localPos = dragHandler->centroid().position();
            const QPointF pos = handleItem->mapToItem(handleItem->parentItem(), localPos);
            if (corner == Qt::TopLeftCorner)
                m_selectable->setSelectionStartPos(pos);
            else
                m_selectable->setSelectionEndPos(pos);

            m_draggedHandle = handleItem;
            updateHandles();
            updateDraggingState(true);
            QGuiApplication::setOverrideCursor(Qt::SizeFDiagCursor);
        } else {
            m_scrollTimer.stop();
            m_selectable->normalizeSelection();
            updateDraggingState(false);
            QGuiApplication::restoreOverrideCursor();
        }
    });

    QObject::connect(dragHandler, &QQuickDragHandler::centroidChanged, [=]() {
        if (!m_dragging)
            return;

        const QPointF localPos = dragHandler->centroid().position();
        const QPointF pos = handleItem->mapToItem(handleItem->parentItem(), localPos);
        if (corner == Qt::TopLeftCorner)
            m_selectable->setSelectionStartPos(pos);
        else
            m_selectable->setSelectionEndPos(pos);

        updateHandles();
        scrollTowardsPos(pos);
    });

    return handleItem;
}

void QQuickSelectionRectanglePrivate::updateHandles()
{
    if (!m_selectable)
        return;

    const QRectF rect = m_selectable->selectionRectangle().normalized();

    if (!m_topLeftHandle && m_topLeftHandleDelegate)
        m_topLeftHandle = createHandle(m_topLeftHandleDelegate, Qt::TopLeftCorner);

    if (!m_bottomRightHandle && m_bottomRightHandleDelegate)
        m_bottomRightHandle = createHandle(m_bottomRightHandleDelegate, Qt::BottomRightCorner);

    if (m_topLeftHandle) {
        m_topLeftHandle->setX(rect.x() - (m_topLeftHandle->width() / 2));
        m_topLeftHandle->setY(rect.y() - (m_topLeftHandle->height() / 2));
    }

    if (m_bottomRightHandle) {
        m_bottomRightHandle->setX(rect.x() + rect.width() - (m_bottomRightHandle->width() / 2));
        m_bottomRightHandle->setY(rect.y() + rect.height() - (m_bottomRightHandle->height() / 2));
    }
}

void QQuickSelectionRectanglePrivate::connectToTarget()
{
    // To support QuickSelectionRectangle::Auto, we need to listen for changes to the target
    if (const auto flickable = qobject_cast<QQuickFlickable *>(m_target)) {
        connect(flickable, &QQuickFlickable::interactiveChanged, this, &QQuickSelectionRectanglePrivate::updateSelectionMode);
    }
}

void QQuickSelectionRectanglePrivate::updateSelectionMode()
{
    Q_Q(QQuickSelectionRectangle);

    const bool enabled = q->isEnabled();
    m_tapHandler->setEnabled(enabled);

    if (m_selectionMode == QQuickSelectionRectangle::Auto) {
        if (qobject_cast<QQuickScrollView *>(m_target->parentItem())) {
            // ScrollView allows flicking with touch, but not with mouse. So we do
            // the same here: you can drag to select with a mouse, but not with touch.
            m_dragHandler->setAcceptedDevices(QInputDevice::DeviceType::Mouse);
            m_dragHandler->setEnabled(enabled);
        } else if (const auto flickable = qobject_cast<QQuickFlickable *>(m_target)) {
            m_dragHandler->setEnabled(enabled && !flickable->isInteractive());
        } else {
            m_dragHandler->setAcceptedDevices(QInputDevice::DeviceType::Mouse);
            m_dragHandler->setEnabled(enabled);
        }
    } else if (m_selectionMode == QQuickSelectionRectangle::Drag) {
        m_dragHandler->setAcceptedDevices(QInputDevice::DeviceType::AllDevices);
        m_dragHandler->setEnabled(enabled);
    } else {
        m_dragHandler->setEnabled(false);
    }

    // If you can't select using a drag, we always accept a PressAndHold
    m_alwaysAcceptPressAndHold = !m_dragHandler->enabled();
}

QQuickSelectionRectangleAttached *QQuickSelectionRectanglePrivate::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickSelectionRectangle>(object);
    return static_cast<QQuickSelectionRectangleAttached *>(attachedObject);
}

// --------------------------------------------------------

QQuickSelectionRectangle::QQuickSelectionRectangle(QQuickItem *parent)
    : QQuickControl(*(new QQuickSelectionRectanglePrivate), parent)
{
    Q_D(QQuickSelectionRectangle);

    QObject::connect(this, &QQuickItem::enabledChanged, [=]() {
        d->m_scrollTimer.stop();
        d->updateSelectionMode();
        d->updateDraggingState(false);
        d->updateActiveState(false);
    });
}

QQuickItem *QQuickSelectionRectangle::target() const
{
    return d_func()->m_target;
}

void QQuickSelectionRectangle::setTarget(QQuickItem *target)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_target == target)
        return;

    if (d->m_selectable) {
        d->m_scrollTimer.stop();
        d->m_tapHandler->setParent(nullptr);
        d->m_dragHandler->setParent(nullptr);
        d->m_target->disconnect(this);
    }

    d->m_target = target;
    d->m_selectable = nullptr;

    if (d->m_target) {
        d->m_selectable = dynamic_cast<QQuickSelectable *>(QObjectPrivate::get(d->m_target.data()));
        if (!d->m_selectable)
            qmlWarning(this) << "the assigned target is not supported by the control";
    }

    if (d->m_selectable) {
        const auto handlerTarget = d->m_selectable->selectionPointerHandlerTarget();
        d->m_dragHandler->setParent(handlerTarget);
        d->m_tapHandler->setParent(handlerTarget);
        QQuickItemPrivate::get(handlerTarget)->addPointerHandler(d->m_tapHandler);
        QQuickItemPrivate::get(handlerTarget)->addPointerHandler(d->m_dragHandler);
        d->connectToTarget();
        d->updateSelectionMode();
    }

    emit targetChanged();
}

bool QQuickSelectionRectangle::active()
{
    return d_func()->m_active;
}

bool QQuickSelectionRectangle::dragging()
{
    return d_func()->m_dragging;
}

QQuickSelectionRectangle::SelectionMode QQuickSelectionRectangle::selectionMode() const
{
    return d_func()->m_selectionMode;
}

void QQuickSelectionRectangle::setSelectionMode(QQuickSelectionRectangle::SelectionMode selectionMode)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_selectionMode == selectionMode)
        return;

    d->m_selectionMode = selectionMode;

    if (d->m_target)
        d->updateSelectionMode();

    emit selectionModeChanged();
}

QQmlComponent *QQuickSelectionRectangle::topLeftHandle() const
{
    return d_func()->m_topLeftHandleDelegate;
}

void QQuickSelectionRectangle::setTopLeftHandle(QQmlComponent *topLeftHandle)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_topLeftHandleDelegate == topLeftHandle)
        return;

    d->m_topLeftHandleDelegate = topLeftHandle;
    emit topLeftHandleChanged();
}

QQmlComponent *QQuickSelectionRectangle::bottomRightHandle() const
{
    return d_func()->m_bottomRightHandleDelegate;
}

void QQuickSelectionRectangle::setBottomRightHandle(QQmlComponent *bottomRightHandle)
{
    Q_D(QQuickSelectionRectangle);
    if (d->m_bottomRightHandleDelegate == bottomRightHandle)
        return;

    d->m_bottomRightHandleDelegate = bottomRightHandle;
    emit bottomRightHandleChanged();
}

QQuickSelectionRectangleAttached *QQuickSelectionRectangle::qmlAttachedProperties(QObject *obj)
{
    return new QQuickSelectionRectangleAttached(obj);
}

QQuickSelectionRectangleAttached::QQuickSelectionRectangleAttached(QObject *parent)
    : QObject(parent)
{
}

QQuickSelectionRectangle *QQuickSelectionRectangleAttached::control() const
{
    return m_control;
}

void QQuickSelectionRectangleAttached::setControl(QQuickSelectionRectangle *control)
{
    if (m_control == control)
        return;

    m_control = control;
    emit controlChanged();
}

bool QQuickSelectionRectangleAttached::dragging() const
{
    return m_dragging;
}

void QQuickSelectionRectangleAttached::setDragging(bool dragging)
{
    if (m_dragging == dragging)
        return;

    m_dragging = dragging;
    emit draggingChanged();
}

QT_END_NAMESPACE
