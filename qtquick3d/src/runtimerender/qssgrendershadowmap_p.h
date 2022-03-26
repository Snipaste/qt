/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDER_SHADOW_MAP_H
#define QSSG_RENDER_SHADOW_MAP_H

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

#include <QtGui/QMatrix4x4>
#include <QtGui/QVector3D>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiContext;
class QSSGRenderContextInterface;

class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiTexture;

enum class ShadowMapModes
{
    VSM, ///< variance shadow mapping
    CUBE, ///< cubemap omnidirectional shadows
};

struct QSSGShadowMapEntry
{
    QSSGShadowMapEntry();

    static QSSGShadowMapEntry withRhiDepthMap(quint32 lightIdx,
                                              ShadowMapModes mode,
                                              QRhiTexture *depthMap,
                                              QRhiTexture *depthCopy,
                                              QRhiRenderBuffer *depthStencil);

    static QSSGShadowMapEntry withRhiDepthCubeMap(quint32 lightIdx,
                                                  ShadowMapModes mode,
                                                  QRhiTexture *depthCube,
                                                  QRhiTexture *cubeCopy,
                                                  QRhiRenderBuffer *depthStencil);

    void destroyRhiResources();

    quint32 m_lightIndex; ///< the light index it belongs to
    ShadowMapModes m_shadowMapMode; ///< shadow map method

    // RHI resources
    QRhiTexture *m_rhiDepthMap = nullptr; // shadow map (VSM)
    QRhiTexture *m_rhiDepthCopy = nullptr; // for blur pass (VSM)
    QRhiTexture *m_rhiDepthCube = nullptr; // shadow cube map (CUBE)
    QRhiTexture *m_rhiCubeCopy = nullptr; // for blur pass (CUBE)
    QRhiRenderBuffer *m_rhiDepthStencil = nullptr; // depth/stencil
    QVarLengthArray<QRhiTextureRenderTarget *, 6> m_rhiRenderTargets; // texture RT
    QRhiRenderPassDescriptor *m_rhiRenderPassDesc = nullptr; // texture RT renderpass descriptor
    QRhiTextureRenderTarget *m_rhiBlurRenderTarget0 = nullptr; // texture RT for blur X (targets depthCopy or cubeCopy)
    QRhiTextureRenderTarget *m_rhiBlurRenderTarget1 = nullptr; // texture RT for blur Y (targets depthMap or depthCube)
    QRhiRenderPassDescriptor *m_rhiBlurRenderPassDesc = nullptr; // blur needs its own because no depth/stencil

    QMatrix4x4 m_lightVP; ///< light view projection matrix
    QMatrix4x4 m_lightCubeView[6]; ///< light cubemap view matrices
    QMatrix4x4 m_lightView; ///< light view transform
};

class QSSGRenderShadowMap
{
    typedef QVector<QSSGShadowMapEntry> TShadowMapEntryList;

public:
    QAtomicInt ref;
    const QSSGRenderContextInterface &m_context;

    QSSGRenderShadowMap(const QSSGRenderContextInterface &inContext);
    ~QSSGRenderShadowMap();

    void addShadowMapEntry(qint32 lightIdx,
                           qint32 width,
                           qint32 height,
                           ShadowMapModes mode);

    QSSGShadowMapEntry *shadowMapEntry(int lightIdx);

    qint32 shadowMapEntryCount() { return m_shadowMapList.size(); }

private:
    TShadowMapEntryList m_shadowMapList;
};

QT_END_NAMESPACE

#endif
