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

#ifndef QQUICK3DPICKRESULT_P_H
#define QQUICK3DPICKRESULT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3D/qquick3dobject.h>
#include <QObject>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include "qquick3dmodel_p.h"
#include "qquick3dcamera_p.h"

QT_BEGIN_NAMESPACE
class QQuick3DModel;

class Q_QUICK3D_EXPORT QQuick3DPickResult
{
    Q_GADGET
    Q_PROPERTY(QQuick3DModel* objectHit READ objectHit CONSTANT)
    Q_PROPERTY(float distance READ distance CONSTANT)
    Q_PROPERTY(QVector2D uvPosition READ uvPosition CONSTANT)
    Q_PROPERTY(QVector3D scenePosition READ scenePosition CONSTANT)
    Q_PROPERTY(QVector3D position READ position CONSTANT)
    Q_PROPERTY(QVector3D normal READ normal CONSTANT)
    Q_PROPERTY(QVector3D sceneNormal READ sceneNormal CONSTANT)

public:

    QQuick3DPickResult();
    explicit QQuick3DPickResult(QQuick3DModel *hitObject,
                                float distanceFromCamera,
                                const QVector2D &uvPosition,
                                const QVector3D &scenePosition,
                                const QVector3D &position,
                                const QVector3D &normal);
    QQuick3DModel *objectHit() const;
    float distance() const;
    QVector2D uvPosition() const;
    QVector3D scenePosition() const;
    QVector3D position() const;
    QVector3D normal() const;
    QVector3D sceneNormal() const;

private:
    QQuick3DModel *m_objectHit;
    float m_distance;
    QVector2D m_uvPosition;
    QVector3D m_scenePosition;
    QVector3D m_position;
    QVector3D m_normal;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuick3DPickResult)

#endif // QQUICK3DPICKRESULT_P_H
