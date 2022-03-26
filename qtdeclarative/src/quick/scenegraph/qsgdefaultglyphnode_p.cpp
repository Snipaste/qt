/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qsgdefaultglyphnode_p_p.h"
#include <private/qsgmaterialshader_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qfontengine_p.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgdefaultrendercontext_p.h>

#include <private/qrawfont_p.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE
#define GL_FRAMEBUFFER_SRGB_CAPABLE 0x8DBA
#endif

static inline QVector4D qsg_premultiply(const QVector4D &c, float globalOpacity)
{
    float o = c.w() * globalOpacity;
    return QVector4D(c.x() * o, c.y() * o, c.z() * o, o);
}

#if 0
static inline qreal qt_sRGB_to_linear_RGB(qreal f)
{
    return f > 0.04045 ? qPow((f + 0.055) / 1.055, 2.4) : f / 12.92;
}

static inline QVector4D qt_sRGB_to_linear_RGB(const QVector4D &color)
{
    return QVector4D(qt_sRGB_to_linear_RGB(color.x()),
                     qt_sRGB_to_linear_RGB(color.y()),
                     qt_sRGB_to_linear_RGB(color.z()),
                     color.w());
}

static inline qreal fontSmoothingGamma()
{
    static qreal fontSmoothingGamma = QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::FontSmoothingGamma).toReal();
    return fontSmoothingGamma;
}
#endif

class QSGTextMaskRhiShader : public QSGMaterialShader
{
public:
    QSGTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat);

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    QFontEngine::GlyphFormat m_glyphFormat;
};

QSGTextMaskRhiShader::QSGTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat)
    : m_glyphFormat(glyphFormat)
{
    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/textmask.vert.qsb"));
    setShaderFileName(FragmentStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/textmask.frag.qsb"));
}

enum UbufOffset {
    ModelViewMatrixOffset = 0,
    ProjectionMatrixOffset = ModelViewMatrixOffset + 64,
    ColorOffset = ProjectionMatrixOffset + 64,
    TextureScaleOffset = ColorOffset + 16,
    DprOffset = TextureScaleOffset + 8,

    // + 1 float padding (vec4 must be aligned to 16)
    StyleColorOffset = DprOffset + 4 + 4,
    ShiftOffset = StyleColorOffset + 16
};

bool QSGTextMaskRhiShader::updateUniformData(RenderState &state,
                                             QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    // updateUniformData() is called before updateSampledImage() by the
    // renderer. Hence updating the glyph cache stuff here.
    const bool updated = mat->ensureUpToDate();
    Q_ASSERT(mat->texture());
    Q_ASSERT(oldMat == nullptr || oldMat->texture());

    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= DprOffset + 4);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 mv = state.modelViewMatrix();
        memcpy(buf->data() + ModelViewMatrixOffset, mv.constData(), 64);
        const QMatrix4x4 p = state.projectionMatrix();
        memcpy(buf->data() + ProjectionMatrixOffset, p.constData(), 64);

        changed = true;
    }

    QRhiTexture *oldRtex = oldMat ? oldMat->texture()->rhiTexture() : nullptr;
    QRhiTexture *newRtex = mat->texture()->rhiTexture();
    if (updated || !oldMat || oldRtex != newRtex) {
        const QVector2D textureScale = QVector2D(1.0f / mat->rhiGlyphCache()->width(),
                                                 1.0f / mat->rhiGlyphCache()->height());
        memcpy(buf->data() + TextureScaleOffset, &textureScale, 8);
        changed = true;
    }

    if (!oldMat) {
        float dpr = state.devicePixelRatio();
        memcpy(buf->data() + DprOffset, &dpr, 4);
    }

    // move texture uploads/copies onto the renderer's soon-to-be-committed list
    mat->rhiGlyphCache()->commitResourceUpdates(state.resourceUpdateBatch());

    return changed;
}

void QSGTextMaskRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                              QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);
    if (binding != 1)
        return;

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTexture *t = mat->texture();
    t->setFiltering(QSGTexture::Nearest);
    *texture = t;
}

class QSG8BitTextMaskRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG8BitTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat, bool alphaTexture)
        : QSGTextMaskRhiShader(glyphFormat)
    {
        if (alphaTexture)
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/8bittextmask_a.frag.qsb"));
        else
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/8bittextmask.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSG8BitTextMaskRhiShader::updateUniformData(RenderState &state,
                                                 QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= ColorOffset + 16);

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        const QVector4D color = qsg_premultiply(mat->color(), state.opacity());
        memcpy(buf->data() + ColorOffset, &color, 16);
        changed = true;
    }

    return changed;
}

class QSG24BitTextMaskRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG24BitTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskRhiShader(glyphFormat)
    {
        setFlag(UpdatesGraphicsPipelineState, true);
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/24bittextmask.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                     QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

// ### gamma correction (sRGB) Unsurprisingly, the GL approach is not portable
// to anything else - it just does not work that way, there is no opt-in/out
// switch and magic winsys-provided maybe-sRGB buffers. When requesting an sRGB
// QRhiSwapChain (which we do not do), it is full sRGB, with the sRGB
// framebuffer update and blending always on... Could we do gamma correction in
// the shader for text? (but that's bad for blending?)

bool QSG24BitTextMaskRhiShader::updateUniformData(RenderState &state,
                                                  QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= ColorOffset + 16);

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        // shader takes vec4 but uses alpha only; coloring happens via the blend constant
        const QVector4D color = qsg_premultiply(mat->color(), state.opacity());
        memcpy(buf->data() + ColorOffset, &color, 16);
        changed = true;
    }

    return changed;
}

bool QSG24BitTextMaskRhiShader::updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(oldMaterial);
    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);

    ps->blendEnable = true;
    ps->srcColor = GraphicsPipelineState::ConstantColor;
    ps->dstColor = GraphicsPipelineState::OneMinusSrcColor;

    QVector4D color = qsg_premultiply(mat->color(), state.opacity());
    //        if (useSRGB())
    //            color = qt_sRGB_to_linear_RGB(color);

    // this is dynamic state but it's - magic! - taken care of by the renderer
    ps->blendConstant = QColor::fromRgbF(color.x(), color.y(), color.z(), color.w());

    return true;
}

class QSG32BitColorTextRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG32BitColorTextRhiShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskRhiShader(glyphFormat)
    {
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/32bitcolortext.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSG32BitColorTextRhiShader::updateUniformData(RenderState &state,
                                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= ColorOffset + 16);

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        // shader takes vec4 but uses alpha only
        const QVector4D color(0, 0, 0, mat->color().w() * state.opacity());
        memcpy(buf->data() + ColorOffset, &color, 16);
        changed = true;
    }

    return changed;
}

class QSGStyledTextRhiShader : public QSG8BitTextMaskRhiShader
{
public:
    QSGStyledTextRhiShader(QFontEngine::GlyphFormat glyphFormat, bool alphaTexture)
        : QSG8BitTextMaskRhiShader(glyphFormat, alphaTexture)
    {
        setShaderFileName(VertexStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext.vert.qsb"));
        if (alphaTexture)
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext_a.frag.qsb"));
        else
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext.frag.qsb"));
    }

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSGStyledTextRhiShader::updateUniformData(RenderState &state,
                                               QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSG8BitTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGStyledTextMaterial *mat = static_cast<QSGStyledTextMaterial *>(newMaterial);
    QSGStyledTextMaterial *oldMat = static_cast<QSGStyledTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= ShiftOffset + 8);

    if (oldMat == nullptr || mat->styleColor() != oldMat->styleColor() || state.isOpacityDirty()) {
        const QVector4D styleColor = qsg_premultiply(mat->styleColor(), state.opacity());
        memcpy(buf->data() + StyleColorOffset, &styleColor, 16);
        changed = true;
    }

    if (oldMat == nullptr || oldMat->styleShift() != mat->styleShift()) {
        const QVector2D v = mat->styleShift();
        memcpy(buf->data() + ShiftOffset, &v, 8);
        changed = true;
    }

    return changed;
}

class QSGOutlinedTextRhiShader : public QSGStyledTextRhiShader
{
public:
    QSGOutlinedTextRhiShader(QFontEngine::GlyphFormat glyphFormat, bool alphaTexture)
        : QSGStyledTextRhiShader(glyphFormat, alphaTexture)
    {
        setShaderFileName(VertexStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext.vert.qsb"));
        if (alphaTexture)
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext_a.frag.qsb"));
        else
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext.frag.qsb"));
    }
};


// ***** common material stuff

QSGTextMaskMaterial::QSGTextMaskMaterial(QSGRenderContext *rc, const QVector4D &color, const QRawFont &font, QFontEngine::GlyphFormat glyphFormat)
    : m_rc(qobject_cast<QSGDefaultRenderContext *>(rc))
    , m_texture(nullptr)
    , m_glyphCache(nullptr)
    , m_font(font)
    , m_color(color)
{
    init(glyphFormat);
}

QSGTextMaskMaterial::~QSGTextMaskMaterial()
{
    delete m_texture;
}

void QSGTextMaskMaterial::setColor(const QVector4D &color)
{
    if (m_color == color)
        return;

    m_color = color;

    // If it is an RGB cache, then the pen color is actually part of the cache key
    // so it has to be updated
    if (m_glyphCache != nullptr && m_glyphCache->glyphFormat() == QFontEngine::Format_ARGB)
        updateCache(QFontEngine::Format_ARGB);
}

void QSGTextMaskMaterial::init(QFontEngine::GlyphFormat glyphFormat)
{
    Q_ASSERT(m_font.isValid());

    setFlag(Blending, true);

    Q_ASSERT(m_rc);
    m_rhi = m_rc->rhi();

    updateCache(glyphFormat);
}

void QSGTextMaskMaterial::updateCache(QFontEngine::GlyphFormat glyphFormat)
{
    // The following piece of code will read/write to the font engine's caches,
    // potentially from different threads. However, this is safe because this
    // code is only called from QQuickItem::updatePaintNode() which is called
    // only when the GUI is blocked, and multiple threads will call it in
    // sequence. See also QSGRenderContext::invalidate

    QRawFontPrivate *fontD = QRawFontPrivate::get(m_font);
    if (QFontEngine *fontEngine = fontD->fontEngine) {
        if (glyphFormat == QFontEngine::Format_None) {
            glyphFormat = fontEngine->glyphFormat != QFontEngine::Format_None
                        ? fontEngine->glyphFormat
                        : QFontEngine::Format_A32;
        }

        qreal devicePixelRatio;
        void *cacheKey;
        Q_ASSERT(m_rhi);
        cacheKey = m_rhi;
        // Get the dpr the modern way. This value retrieved via the
        // rendercontext matches what RenderState::devicePixelRatio()
        // exposes to the material shaders later on.
        devicePixelRatio = m_rc->currentDevicePixelRatio();

        QTransform glyphCacheTransform = QTransform::fromScale(devicePixelRatio, devicePixelRatio);
        if (!fontEngine->supportsTransformation(glyphCacheTransform))
            glyphCacheTransform = QTransform();

        QColor color = glyphFormat == QFontEngine::Format_ARGB ? QColor::fromRgbF(m_color.x(), m_color.y(), m_color.z(), m_color.w()) : QColor();
        m_glyphCache = fontEngine->glyphCache(cacheKey, glyphFormat, glyphCacheTransform, color);
        if (!m_glyphCache || int(m_glyphCache->glyphFormat()) != glyphFormat) {
            m_glyphCache = new QSGRhiTextureGlyphCache(m_rc, glyphFormat, glyphCacheTransform, color);
            fontEngine->setGlyphCache(cacheKey, m_glyphCache.data());
            m_rc->registerFontengineForCleanup(fontEngine);
        }
    }
}

void QSGTextMaskMaterial::populate(const QPointF &p,
                                   const QVector<quint32> &glyphIndexes,
                                   const QVector<QPointF> &glyphPositions,
                                   QSGGeometry *geometry,
                                   QRectF *boundingRect,
                                   QPointF *baseLine,
                                   const QMargins &margins)
{
    Q_ASSERT(m_font.isValid());
    QPointF position(p.x(), p.y() - m_font.ascent());
    QVector<QFixedPoint> fixedPointPositions;
    const int glyphPositionsSize = glyphPositions.size();
    fixedPointPositions.reserve(glyphPositionsSize);
    for (int i=0; i < glyphPositionsSize; ++i)
        fixedPointPositions.append(QFixedPoint::fromPointF(position + glyphPositions.at(i)));

    QTextureGlyphCache *cache = glyphCache();

    QRawFontPrivate *fontD = QRawFontPrivate::get(m_font);
    cache->populate(fontD->fontEngine, glyphIndexes.size(), glyphIndexes.constData(),
                    fixedPointPositions.data());
    cache->fillInPendingGlyphs();

    int margin = fontD->fontEngine->glyphMargin(cache->glyphFormat());

    qreal glyphCacheScaleX = cache->transform().m11();
    qreal glyphCacheScaleY = cache->transform().m22();
    qreal glyphCacheInverseScaleX = 1.0 / glyphCacheScaleX;
    qreal glyphCacheInverseScaleY = 1.0 / glyphCacheScaleY;
    qreal scaledMargin = margin * glyphCacheInverseScaleX;

    Q_ASSERT(geometry->indexType() ==  QSGGeometry::UnsignedShortType);
    geometry->allocate(glyphIndexes.size() * 4, glyphIndexes.size() * 6);
    QVector4D *vp = (QVector4D *)geometry->vertexDataAsTexturedPoint2D();
    Q_ASSERT(geometry->sizeOfVertex() == sizeof(QVector4D));
    ushort *ip = geometry->indexDataAsUShort();

    bool supportsSubPixelPositions = fontD->fontEngine->supportsHorizontalSubPixelPositions();
    for (int i=0; i<glyphIndexes.size(); ++i) {
         QPointF glyphPosition = glyphPositions.at(i) + position;
         QFixed subPixelPosition;
         if (supportsSubPixelPositions)
             subPixelPosition = fontD->fontEngine->subPixelPositionForX(QFixed::fromReal(glyphPosition.x()));

         QTextureGlyphCache::GlyphAndSubPixelPosition glyph(glyphIndexes.at(i),
                                                            QFixedPoint(subPixelPosition, 0));
         const QTextureGlyphCache::Coord &c = cache->coords.value(glyph);

         // On a retina screen the glyph positions are not pre-scaled (as opposed to
         // eg. the raster paint engine). To ensure that we get the same behavior as
         // the raster engine (and CoreText itself) when it comes to rounding of the
         // coordinates, we need to apply the scale factor before rounding, and then
         // apply the inverse scale to get back to the coordinate system of the node.

         qreal x = (qFloor(glyphPosition.x() * glyphCacheScaleX) * glyphCacheInverseScaleX) +
                        (c.baseLineX * glyphCacheInverseScaleX) - scaledMargin;
         qreal y = (qRound(glyphPosition.y() * glyphCacheScaleY) * glyphCacheInverseScaleY) -
                        (c.baseLineY * glyphCacheInverseScaleY) - scaledMargin;

         qreal w = c.w * glyphCacheInverseScaleX;
         qreal h = c.h * glyphCacheInverseScaleY;

         *boundingRect |= QRectF(x + scaledMargin, y + scaledMargin, w, h);

         float cx1 = x - margins.left();
         float cx2 = x + w + margins.right();
         float cy1 = y - margins.top();
         float cy2 = y + h + margins.bottom();

         float tx1 = c.x - margins.left();
         float tx2 = c.x + c.w + margins.right();
         float ty1 = c.y - margins.top();
         float ty2 = c.y + c.h + margins.bottom();

         if (baseLine->isNull())
             *baseLine = glyphPosition;

         vp[4 * i + 0] = QVector4D(cx1, cy1, tx1, ty1);
         vp[4 * i + 1] = QVector4D(cx2, cy1, tx2, ty1);
         vp[4 * i + 2] = QVector4D(cx1, cy2, tx1, ty2);
         vp[4 * i + 3] = QVector4D(cx2, cy2, tx2, ty2);

         int o = i * 4;
         ip[6 * i + 0] = o + 0;
         ip[6 * i + 1] = o + 2;
         ip[6 * i + 2] = o + 3;
         ip[6 * i + 3] = o + 3;
         ip[6 * i + 4] = o + 1;
         ip[6 * i + 5] = o + 0;
    }
}

QSGMaterialType *QSGTextMaskMaterial::type() const
{
    static QSGMaterialType argb, rgb, gray;
    switch (glyphCache()->glyphFormat()) {
    case QFontEngine::Format_ARGB:
        return &argb;
    case QFontEngine::Format_A32:
        return &rgb;
    case QFontEngine::Format_A8:
    default:
        return &gray;
    }
}

QTextureGlyphCache *QSGTextMaskMaterial::glyphCache() const
{
    return static_cast<QTextureGlyphCache *>(m_glyphCache.data());
}

QSGRhiTextureGlyphCache *QSGTextMaskMaterial::rhiGlyphCache() const
{
    return static_cast<QSGRhiTextureGlyphCache *>(glyphCache());
}

QSGMaterialShader *QSGTextMaskMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    const QFontEngine::GlyphFormat glyphFormat = gc->glyphFormat();
    switch (glyphFormat) {
    case QFontEngine::Format_ARGB:
        return new QSG32BitColorTextRhiShader(glyphFormat);
    case QFontEngine::Format_A32:
        return new QSG24BitTextMaskRhiShader(glyphFormat);
    case QFontEngine::Format_A8:
    default:
        return new QSG8BitTextMaskRhiShader(glyphFormat, gc->eightBitFormatIsAlphaSwizzled());
    }
}

static inline int qsg_colorDiff(const QVector4D &a, const QVector4D &b)
{
    if (a.x() != b.x())
        return a.x() > b.x() ? 1 : -1;
    if (a.y() != b.y())
        return a.y() > b.y() ? 1 : -1;
    if (a.z() != b.z())
        return a.z() > b.z() ? 1 : -1;
    if (a.w() != b.w())
        return a.w() > b.w() ? 1 : -1;
    return 0;
}

int QSGTextMaskMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGTextMaskMaterial *other = static_cast<const QSGTextMaskMaterial *>(o);
    if (m_glyphCache != other->m_glyphCache)
        return m_glyphCache.data() < other->m_glyphCache.data() ? -1 : 1;
    return qsg_colorDiff(m_color, other->m_color);
}

bool QSGTextMaskMaterial::ensureUpToDate()
{
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    QSize glyphCacheSize(gc->width(), gc->height());
    if (glyphCacheSize != m_size) {
        if (m_texture)
            delete m_texture;
        m_texture = new QSGPlainTexture;
        m_texture->setTexture(gc->texture());
        m_texture->setTextureSize(QSize(gc->width(), gc->height()));
        m_texture->setOwnsTexture(false);
        m_size = glyphCacheSize;
        return true;
    }
    return false;
}


QSGStyledTextMaterial::QSGStyledTextMaterial(QSGRenderContext *rc, const QRawFont &font)
    : QSGTextMaskMaterial(rc, QVector4D(), font, QFontEngine::Format_A8)
{
}

QSGMaterialType *QSGStyledTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGStyledTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    return new QSGStyledTextRhiShader(gc->glyphFormat(), gc->eightBitFormatIsAlphaSwizzled());
}

int QSGStyledTextMaterial::compare(const QSGMaterial *o) const
{
    const QSGStyledTextMaterial *other = static_cast<const QSGStyledTextMaterial *>(o);

    if (m_styleShift != other->m_styleShift)
        return m_styleShift.y() - other->m_styleShift.y();

    int diff = qsg_colorDiff(m_styleColor, other->m_styleColor);
    if (diff == 0)
        return QSGTextMaskMaterial::compare(o);
    return diff;
}


QSGOutlinedTextMaterial::QSGOutlinedTextMaterial(QSGRenderContext *rc, const QRawFont &font)
    : QSGStyledTextMaterial(rc, font)
{
}

QSGMaterialType *QSGOutlinedTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGOutlinedTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    return new QSGOutlinedTextRhiShader(gc->glyphFormat(), gc->eightBitFormatIsAlphaSwizzled());
}

QT_END_NAMESPACE
