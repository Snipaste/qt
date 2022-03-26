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

#ifndef QSSGMATERIAL_H
#define QSSGMATERIAL_H

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
#include <QtQuick3D/private/qquick3dtexture_p.h>

#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

class QQuick3DSceneManager;
class Q_QUICK3D_EXPORT QQuick3DMaterial : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DTexture *lightProbe READ lightProbe WRITE setLightProbe NOTIFY lightProbeChanged)
    Q_PROPERTY(CullMode cullMode READ cullMode WRITE setCullMode NOTIFY cullModeChanged)
    Q_PROPERTY(DepthDrawMode depthDrawMode READ depthDrawMode WRITE setDepthDrawMode NOTIFY depthDrawModeChanged)

    QML_NAMED_ELEMENT(Material)
    QML_UNCREATABLE("Material is Abstract")

public:
    enum CullMode {
        BackFaceCulling = 1,
        FrontFaceCulling = 2,
        NoCulling = 3
    };
    Q_ENUM(CullMode)

    enum TextureChannelMapping {
        R = 0,
        G,
        B,
        A,
    };
    Q_ENUM(TextureChannelMapping)

    enum DepthDrawMode {
        OpaqueOnlyDepthDraw = 0,
        AlwaysDepthDraw,
        NeverDepthDraw,
        OpaquePrePassDepthDraw,
    };
    Q_ENUM(DepthDrawMode)

    ~QQuick3DMaterial() override;

    QQuick3DTexture *lightProbe() const;

    CullMode cullMode() const;

    DepthDrawMode depthDrawMode() const;

public Q_SLOTS:
    void setLightProbe(QQuick3DTexture *lightProbe);
    void setCullMode(QQuick3DMaterial::CullMode cullMode);
    void setDepthDrawMode(QQuick3DMaterial::DepthDrawMode depthDrawMode);

Q_SIGNALS:
    void lightProbeChanged(QQuick3DTexture *lightProbe);
    void cullModeChanged(QQuick3DMaterial::CullMode cullMode);
    void depthDrawModeChanged(QQuick3DMaterial::DepthDrawMode depthDrawMode);

protected:
    explicit QQuick3DMaterial(QQuick3DObjectPrivate &dd, QQuick3DObject *parent = nullptr);
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void itemChange(ItemChange, const ItemChangeData &) override;

    QHash<QByteArray, QMetaObject::Connection> m_connections;

private:
    void updateSceneManager(QQuick3DSceneManager *sceneManager);
    QQuick3DTexture *m_iblProbe = nullptr;

    CullMode m_cullMode = CullMode::BackFaceCulling;
    DepthDrawMode m_depthDrawMode = DepthDrawMode::OpaqueOnlyDepthDraw;
};

QT_END_NAMESPACE

#endif // QSSGMATERIAL_H
