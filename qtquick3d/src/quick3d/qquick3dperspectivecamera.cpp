/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquick3dperspectivecamera_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtMath>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include "qquick3dutils_p.h"

#include "qquick3dnode_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype PerspectiveCamera
    \inherits Camera
    \inqmlmodule QtQuick3D
    \brief Defines a Perspective Camera for viewing the content of a 3D scene.

    A \l Camera defines how the content of the 3D scene is projected onto a 2D surface,
    such as a View3D. A scene needs at least one \l Camera in order to visualize its
    contents.

    It is possible to position and rotate the \l Camera like any other spatial \l{QtQuick3D::Node}{Node} in
    the scene. The \l{QtQuick3D::Node}{Node}'s location and orientation determines where the \l Camera is in
    the scene, and what direction it is facing. The default orientation of the \l Camera
    has its forward vector pointing along the negative Z axis and its up vector along
    the positive Y axis.

    \image perspectivecamera.png

    PerspectiveCamera is the standard \l Camera type. It gives a realistic projection of the
    scene, where distant objects are perceived as smaller. The frustum is defined by
    the fieldOfView property as well as near and far clip planes.

    The following example creates a PerspectiveCamera at position [0, 200, 300] in the scene, a
    field of view of 90 degrees and with a 30 degree downward pitch.
    \code
    PerspectiveCamera {
        position: Qt.vector3d(0, 200, 300)
        eulerRotation.x: -30
        fieldOfView: 90
    }
    \endcode

    \sa {Qt Quick 3D - View3D Example}, OrthographicCamera, FrustumCamera, CustomCamera
*/

/*!
    \internal
*/
QQuick3DPerspectiveCamera::QQuick3DPerspectiveCamera(QQuick3DNodePrivate &dd, QQuick3DNode *parent)
    : QQuick3DCamera(dd, parent)
{}

/*!
    \internal
*/
QQuick3DPerspectiveCamera::QQuick3DPerspectiveCamera(QQuick3DNode *parent)
    : QQuick3DPerspectiveCamera(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::PerspectiveCamera)), parent)
{}

/*!
    \qmlproperty real PerspectiveCamera::clipNear

    This property defines the near clip plane of the PerspectiveCamera's frustum. Geometry which
    is closer to the \l Camera than the near clip plane will not be visible.

    The default value is 10.0.
*/

float QQuick3DPerspectiveCamera::clipNear() const
{
    return m_clipNear;
}

/*!
    \qmlproperty real PerspectiveCamera::clipFar

    This property defines the far clip plane of the PerspectiveCamera's frustum. Geometry which
    is further away from the \l Camera than the far clip plane will not be visible.

    The default value is 10000.0.
*/

float QQuick3DPerspectiveCamera::clipFar() const
{
    return m_clipFar;
}

/*!
   \qmlproperty enumeration PerspectiveCamera::FieldOfViewOrientation

   This enum type specifies the orientation in which camera field of view is given.

   \value PerspectiveCamera.Vertical
          The provided field of view is vertical, meaning the field of view is the angle between
          the line traced from the camera to the center top of the viewport and the line from
          the camera to the center bottom of the viewport. The horizontal aspect ratio will be
          adjusted to maintain aspect ratio. This is the default orientation.
   \value PerspectiveCamera.Horizontal
          The provided field of view is horizontal, meaning the field of view is the angle between
          the line traced from the camera to the center left side of the viewport and the line from
          the camera to the center right side of the viewport. The vertical aspect ratio will be
          adjusted to maintain aspect ratio.
 */

/*!
    \qmlproperty real PerspectiveCamera::fieldOfView

    This property holds the field of view of the camera in degrees. This can be either the
    vertical or horizontal field of view depending on whether the fieldOfViewOrientation property
    is set to \c {PerspectiveCamera.Vertical} or \c {PerspectiveCamera.Horizontal}.

    The default value is 60.0.
 */

float QQuick3DPerspectiveCamera::fieldOfView() const
{
    return m_fieldOfView;
}

/*!
    \qmlproperty enumeration PerspectiveCamera::fieldOfViewOrientation

    This property determines if the field of view property reflects the vertical or the horizontal
    field of view.

    It can be either of the following two values:
    \list
    \li PerspectiveCamera.Vertical - The provided field of view is vertical, meaning the field of view is the angle between
          the line traced from the camera to the center top of the viewport and the line from
          the camera to the center bottom of the viewport. The horizontal aspect ratio will be
          adjusted to maintain aspect ratio. This is the default orientation.
    \li PerspectiveCamera.Horizontal - The provided field of view is horizontal, meaning the field of view is the angle between
          the line traced from the camera to the center left side of the viewport and the line from
          the camera to the center right side of the viewport. The vertical aspect ratio will be
          adjusted to maintain aspect ratio.
    \endlist

    The default value is \c {PerspectiveCamera.Vertical}.
*/

QQuick3DPerspectiveCamera::FieldOfViewOrientation QQuick3DPerspectiveCamera::fieldOfViewOrientation() const
{
    return m_fieldOfViewOrientation;
}

void QQuick3DPerspectiveCamera::setClipNear(float clipNear)
{
    if (qFuzzyCompare(m_clipNear, clipNear))
        return;

    m_clipNear = clipNear;
    emit clipNearChanged();
    update();
}

void QQuick3DPerspectiveCamera::setClipFar(float clipFar)
{
    if (qFuzzyCompare(m_clipFar, clipFar))
        return;

    m_clipFar = clipFar;
    emit clipFarChanged();
    update();
}

void QQuick3DPerspectiveCamera::setFieldOfView(float fieldOfView)
{
    if (qFuzzyCompare(m_fieldOfView, fieldOfView))
        return;

    m_fieldOfView = fieldOfView;
    emit fieldOfViewChanged();
    update();
}

void QQuick3DPerspectiveCamera::setFieldOfViewOrientation(QQuick3DPerspectiveCamera::FieldOfViewOrientation
                                                          fieldOfViewOrientation)
{
    if (m_fieldOfViewOrientation == fieldOfViewOrientation)
        return;

    m_fieldOfViewOrientation = fieldOfViewOrientation;
    emit fieldOfViewOrientationChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DPerspectiveCamera::updateSpatialNode(QSSGRenderGraphObject *node)
{
    QSSGRenderCamera *camera = static_cast<QSSGRenderCamera *>(QQuick3DCamera::updateSpatialNode(node));
    if (camera) {
        const bool changed = ((qUpdateIfNeeded(camera->clipNear, m_clipNear)
                               | qUpdateIfNeeded(camera->clipFar, m_clipFar)
                               | qUpdateIfNeeded(camera->fov, qDegreesToRadians(m_fieldOfView))
                               | qUpdateIfNeeded(camera->fovHorizontal, m_fieldOfViewOrientation == QQuick3DPerspectiveCamera::FieldOfViewOrientation::Horizontal)) != 0);
        if (changed)
            camera->flags.setFlag(QSSGRenderNode::Flag::CameraDirty);
    }

    return camera;
}

QT_END_NAMESPACE
