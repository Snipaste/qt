/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "qvideowindow_p.h"
#include <QPlatformSurfaceEvent>
#include <qfile.h>
#include <qpainter.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

static QSurface::SurfaceType platformSurfaceType()
{
#if defined(Q_OS_DARWIN)
    return QSurface::MetalSurface;
#elif defined (Q_OS_WIN)
    return QSurface::Direct3DSurface;
#endif

    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        return QSurface::RasterSurface;
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface)
        || QCoreApplication::testAttribute(Qt::AA_ForceRasterWidgets))
        return QSurface::RasterSurface;

    return QSurface::RasterGLSurface;
}

QVideoWindowPrivate::QVideoWindowPrivate(QVideoWindow *q)
    : q(q),
    m_sink(new QVideoSink)
{
    Q_ASSERT(q);

    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering)) {
        auto surfaceType = ::platformSurfaceType();
        q->setSurfaceType(surfaceType);
        switch (surfaceType) {
        case QSurface::RasterSurface:
        case QSurface::OpenVGSurface:
            // can't use those surfaces, need to render in SW
            m_graphicsApi = QRhi::Null;
            break;
        case QSurface::OpenGLSurface:
        case QSurface::RasterGLSurface:
            m_graphicsApi = QRhi::OpenGLES2;
            break;
        case QSurface::VulkanSurface:
            m_graphicsApi = QRhi::Vulkan;
            break;
        case QSurface::MetalSurface:
            m_graphicsApi = QRhi::Metal;
            break;
        case QSurface::Direct3DSurface:
            m_graphicsApi = QRhi::D3D11;
            break;
        }
    }

    QObject::connect(m_sink.get(), &QVideoSink::videoFrameChanged, q, &QVideoWindow::setVideoFrame);
}

QVideoWindowPrivate::~QVideoWindowPrivate()
{
    freeTextures();
}

static const float g_quad[] = {
    // 4 clockwise rotation of texture vertexes (the second pair)
    // Rotation 0
    -1.f, -1.f,   0.f, 0.f,
    -1.f, 1.f,    0.f, 1.f,
    1.f, -1.f,    1.f, 0.f,
    1.f, 1.f,     1.f, 1.f,
    // Rotation 90
    -1.f, -1.f,   0.f, 1.f,
    -1.f, 1.f,    1.f, 1.f,
    1.f, -1.f,    0.f, 0.f,
    1.f, 1.f,     1.f, 0.f,

    // Rotation 180
    -1.f, -1.f,   1.f, 1.f,
    -1.f, 1.f,    1.f, 0.f,
    1.f, -1.f,    0.f, 1.f,
    1.f, 1.f,     0.f, 0.f,
    // Rotation 270
    -1.f, -1.f,   1.f, 0.f,
    -1.f, 1.f,    0.f, 0.f,
    1.f, -1.f,    1.f, 1.f,
    1.f, 1.f,     0.f, 1.f
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void QVideoWindowPrivate::initRhi()
{
    if (m_graphicsApi == QRhi::Null)
        return;

    QRhi::Flags rhiFlags = {};//QRhi::EnableDebugMarkers | QRhi::EnableProfiling;

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface(q->format()));
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = q;
        params.format = q->format();
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params, rhiFlags));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = q->vulkanInstance();
        params.window = q;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params, rhiFlags));
    }
#endif

#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D11, &params, rhiFlags));
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (m_graphicsApi == QRhi::Metal) {
        QRhiMetalInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Metal, &params, rhiFlags));
    }
#endif
    if (!m_rhi)
        return;

    m_swapChain.reset(m_rhi->newSwapChain());
    m_swapChain->setWindow(q);
    m_renderPass.reset(m_swapChain->newCompatibleRenderPassDescriptor());
    m_swapChain->setRenderPassDescriptor(m_renderPass.get());

    m_vertexBuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(g_quad)));
    m_vertexBuf->create();
    m_vertexBufReady = false;

    m_uniformBuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 64 + 4 + 4));
    m_uniformBuf->create();

    m_textureSampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_textureSampler->create();

    m_shaderResourceBindings.reset(m_rhi->newShaderResourceBindings());
    m_subtitleResourceBindings.reset(m_rhi->newShaderResourceBindings());

    m_subtitleUniformBuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 64 + 4 + 4));
    m_subtitleUniformBuf->create();

    Q_ASSERT(NVideoFrameSlots >= m_rhi->resourceLimit(QRhi::FramesInFlight));
}

void QVideoWindowPrivate::setupGraphicsPipeline(QRhiGraphicsPipeline *pipeline, QRhiShaderResourceBindings *bindings, QVideoFrameFormat::PixelFormat fmt)
{

    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = getShader(QVideoTextureHelper::vertexShaderFileName(fmt));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QVideoTextureHelper::fragmentShaderFileName(fmt));
    Q_ASSERT(fs.isValid());
    pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(bindings);
    pipeline->setRenderPassDescriptor(m_renderPass.get());
    pipeline->create();
}

void QVideoWindowPrivate::updateTextures(QRhiResourceUpdateBatch *rub)
{
    m_texturesDirty = false;

    auto fmt = m_currentFrame.pixelFormat();
    if (fmt == QVideoFrameFormat::Format_Invalid)
        // We render a 1x1 black pixel when we don't have a video
        fmt = QVideoFrameFormat::Format_RGBA8888;

    auto textureDesc = QVideoTextureHelper::textureDescription(fmt);

    m_frameSize = m_currentFrame.isValid() ? m_currentFrame.size() : QSize(1, 1);

    freeTextures();
    if (m_currentFrame.isValid()) {
        QVideoTextureHelper::updateRhiTextures(m_currentFrame, m_rhi.get(), rub, m_frameTextures);
    } else {
        Q_ASSERT(fmt == QVideoFrameFormat::Format_RGBA8888);
        QImage img(QSize(1, 1), QImage::Format_RGBA8888);
        img.fill(Qt::black);
        m_frameTextures[0] = m_rhi->newTexture(QRhiTexture::RGBA8, m_frameSize, 1);
        m_frameTextures[0]->create();
        rub->uploadTexture(m_frameTextures[0], img);
    }

    QRhiShaderResourceBinding bindings[4];
    auto *b = bindings;
    *(b++) = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                   m_uniformBuf.get());
    for (int i = 0; i < textureDesc->nplanes; ++i)
        (*b++) = QRhiShaderResourceBinding::sampledTexture(i + 1, QRhiShaderResourceBinding::FragmentStage,
                                                           m_frameTextures[i], m_textureSampler.get());
    m_shaderResourceBindings->setBindings(bindings, b);
    m_shaderResourceBindings->create();

    if (fmt != format) {
        format = fmt;
        if (!m_graphicsPipeline)
            m_graphicsPipeline.reset(m_rhi->newGraphicsPipeline());

        setupGraphicsPipeline(m_graphicsPipeline.get(), m_shaderResourceBindings.get(), format);
    }
}

void QVideoWindowPrivate::updateSubtitle(QRhiResourceUpdateBatch *rub, const QSize &frameSize)
{
    m_subtitleDirty = false;
    m_hasSubtitle = !m_currentFrame.subtitleText().isEmpty();
    if (!m_hasSubtitle)
        return;

    m_subtitleLayout.update(frameSize, m_currentFrame.subtitleText());
    QSize size = m_subtitleLayout.bounds.size().toSize();

    QImage img = m_subtitleLayout.toImage();

    m_subtitleTexture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, size));
    m_subtitleTexture->create();
    rub->uploadTexture(m_subtitleTexture.get(), img);

    QRhiShaderResourceBinding bindings[2];

    bindings[0] = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                           m_subtitleUniformBuf.get());

    bindings[1] = QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                            m_subtitleTexture.get(), m_textureSampler.get());
    m_subtitleResourceBindings->setBindings(bindings, bindings + 2);
    m_subtitleResourceBindings->create();

    if (!m_subtitlePipeline) {
        m_subtitlePipeline.reset(m_rhi->newGraphicsPipeline());

        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        m_subtitlePipeline->setTargetBlends({ blend });
        setupGraphicsPipeline(m_subtitlePipeline.get(), m_subtitleResourceBindings.get(), QVideoFrameFormat::Format_RGBA8888);
    }
}

void QVideoWindowPrivate::freeTextures()
{
    for (int i = 0; i < 3; ++i) {
        if (m_frameTextures[i])
            delete m_frameTextures[i];
        m_frameTextures[i] = nullptr;
    }
}

void QVideoWindowPrivate::init()
{
    if (initialized)
        return;
    initialized = true;

    initRhi();

    if (!m_rhi)
        backingStore = new QBackingStore(q);
    else
        m_sink->setRhi(m_rhi.get());
}

void QVideoWindowPrivate::resizeSwapChain()
{
    m_hasSwapChain = m_swapChain->createOrResize();
}

void QVideoWindowPrivate::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_swapChain->destroy();
    }
}

void QVideoWindowPrivate::render()
{
    if (!initialized)
        init();

    if (!q->isExposed() || !isExposed)
        return;

    QRect rect(0, 0, q->width(), q->height());

    if (backingStore) {
        if (backingStore->size() != q->size())
            backingStore->resize(q->size());

        backingStore->beginPaint(rect);

        QPaintDevice *device = backingStore->paintDevice();
        if (!device)
            return;
        QPainter painter(device);

        m_currentFrame.paint(&painter, rect, { Qt::black, aspectRatioMode });
        painter.end();

        backingStore->endPaint();
        backingStore->flush(rect);
        return;
    }

    int frameRotationIndex = (m_currentFrame.rotationAngle() / 90) % 4;
    QSize frameSize = m_currentFrame.size();
    if (frameRotationIndex % 2)
        frameSize.transpose();
    QSize scaled = frameSize.scaled(rect.size(), aspectRatioMode);
    QRect videoRect = QRect(QPoint(0, 0), scaled);
    videoRect.moveCenter(rect.center());
    QRect subtitleRect = videoRect.intersected(rect);

    if (m_swapChain->currentPixelSize() != m_swapChain->surfacePixelSize())
        resizeSwapChain();

    if (!m_hasSwapChain)
        return;

    QRhi::FrameOpResult r = m_rhi->beginFrame(m_swapChain.get());

    // keep the video frames alive until we know that they are not needed anymore
    m_videoFrameSlots[m_rhi->currentFrameSlot()] = m_currentFrame;

    if (r == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        r = m_rhi->beginFrame(m_swapChain.get());
    }
    if (r != QRhi::FrameOpSuccess) {
        qWarning("beginFrame failed with %d, retry", r);
        q->requestUpdate();
        return;
    }

    QRhiResourceUpdateBatch *rub = m_rhi->nextResourceUpdateBatch();

    if (!m_vertexBufReady) {
        m_vertexBufReady = true;
        rub->uploadStaticBuffer(m_vertexBuf.get(), g_quad);
    }

    if (m_texturesDirty)
        updateTextures(rub);

    if (m_subtitleDirty || m_subtitleLayout.videoSize != subtitleRect.size())
        updateSubtitle(rub, subtitleRect.size());

    float mirrorFrame = m_currentFrame.mirrored() ? -1.f : 1.f;
    float xscale = mirrorFrame * float(videoRect.width())/float(rect.width());
    float yscale = -1.f * float(videoRect.height())/float(rect.height());

    QMatrix4x4 transform;
    transform.scale(xscale, yscale);

    QByteArray uniformData(64 + 64 + 4 + 4, Qt::Uninitialized);
    QVideoTextureHelper::updateUniformData(&uniformData, m_currentFrame.surfaceFormat(), m_currentFrame, transform, 1.f);
    rub->updateDynamicBuffer(m_uniformBuf.get(), 0, uniformData.size(), uniformData.constData());

    if (m_hasSubtitle) {
        QMatrix4x4 st;
        st.translate(0, -2.f * (float(m_subtitleLayout.bounds.center().y())  + float(subtitleRect.top()))/ float(rect.height()) + 1.f);
        st.scale(float(m_subtitleLayout.bounds.width())/float(rect.width()),
                -1.f * float(m_subtitleLayout.bounds.height())/float(rect.height()));

        QByteArray uniformData(64 + 64 + 4 + 4, Qt::Uninitialized);
        QVideoFrameFormat fmt(m_subtitleLayout.bounds.size().toSize(), QVideoFrameFormat::Format_ARGB8888);
        QVideoTextureHelper::updateUniformData(&uniformData, fmt, QVideoFrame(), st, 1.f);
        rub->updateDynamicBuffer(m_subtitleUniformBuf.get(), 0, uniformData.size(), uniformData.constData());
    }

    QRhiCommandBuffer *cb = m_swapChain->currentFrameCommandBuffer();
    cb->beginPass(m_swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, rub);
    cb->setGraphicsPipeline(m_graphicsPipeline.get());
    auto size = m_swapChain->currentPixelSize();
    cb->setViewport({ 0, 0, float(size.width()), float(size.height()) });
    cb->setShaderResources(m_shaderResourceBindings.get());

    quint32 vertexOffset = quint32(sizeof(float)) * 16 * frameRotationIndex;
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuf.get(), vertexOffset);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(4);

    if (m_hasSubtitle) {
        cb->setGraphicsPipeline(m_subtitlePipeline.get());
        cb->setShaderResources(m_subtitleResourceBindings.get());
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuf.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(4);
    }

    cb->endPass();

    m_rhi->endFrame(m_swapChain.get());
}

/*!
    \class QVideoWindow
    \internal
*/
QVideoWindow::QVideoWindow(QScreen *screen)
    : QWindow(screen)
    , d(new QVideoWindowPrivate(this))
{
}

QVideoWindow::QVideoWindow(QWindow *parent)
    : QWindow(parent)
    , d(new QVideoWindowPrivate(this))
{
}

QVideoWindow::~QVideoWindow() = default;

QVideoSink *QVideoWindow::videoSink() const
{
    return d->m_sink.get();
}

Qt::AspectRatioMode QVideoWindow::aspectRatioMode() const
{
    return d->aspectRatioMode;
}

void QVideoWindow::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (d->aspectRatioMode == mode)
        return;
    d->aspectRatioMode = mode;
    emit aspectRatioModeChanged(mode);
}

bool QVideoWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        d->render();
        return true;

    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            d->releaseSwapChain();
            d->isExposed = false;
        }
        break;
    case QEvent::Expose:
        d->isExposed = isExposed();
        if (d->isExposed)
            requestUpdate();
        return true;

    default:
        break;
    }

    return QWindow::event(e);
}

void QVideoWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    if (!d->backingStore)
        return;
    if (!d->initialized)
        d->init();
    d->backingStore->resize(resizeEvent->size());
}

void QVideoWindow::setVideoFrame(const QVideoFrame &frame)
{
    if (d->m_currentFrame.subtitleText() != frame.subtitleText())
        d->m_subtitleDirty = true;
    d->m_currentFrame = frame;
    d->m_texturesDirty = true;
    if (d->isExposed)
        requestUpdate();
}

QT_END_NAMESPACE
