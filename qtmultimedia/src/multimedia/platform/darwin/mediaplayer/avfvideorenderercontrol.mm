/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfvideorenderercontrol_p.h"
#include "avfdisplaylink_p.h"
#include <private/avfvideobuffer_p.h>

#include <private/qabstractvideobuffer_p.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <private/avfvideosink_p.h>
#include <QtGui/private/qrhi_p.h>

#include <QtCore/qdebug.h>

#import <AVFoundation/AVFoundation.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>

QT_USE_NAMESPACE

@interface SubtitleDelegate : NSObject <AVPlayerItemLegibleOutputPushDelegate>
{
    AVFVideoRendererControl *m_renderer;
}

- (void)legibleOutput:(AVPlayerItemLegibleOutput *)output
        didOutputAttributedStrings:(NSArray<NSAttributedString *> *)strings
        nativeSampleBuffers:(NSArray *)nativeSamples
        forItemTime:(CMTime)itemTime;

@end

@implementation SubtitleDelegate

-(id)initWithRenderer: (AVFVideoRendererControl *)renderer
{
    if (!(self = [super init]))
        return nil;

    m_renderer = renderer;

    return self;
}

- (void)legibleOutput:(AVPlayerItemLegibleOutput *)output
        didOutputAttributedStrings:(NSArray<NSAttributedString *> *)strings
        nativeSampleBuffers:(NSArray *)nativeSamples
        forItemTime:(CMTime)itemTime
{
    QString text;
    for (NSAttributedString *s : strings) {
        if (!text.isEmpty())
            text += QChar::LineSeparator;
        text += QString::fromNSString(s.string);
    }
    m_renderer->setSubtitleText(text);
}

@end


AVFVideoRendererControl::AVFVideoRendererControl(QObject *parent)
    : QObject(parent)
{
    m_displayLink = new AVFDisplayLink(this);
    connect(m_displayLink, SIGNAL(tick(CVTimeStamp)), SLOT(updateVideoFrame(CVTimeStamp)));
}

AVFVideoRendererControl::~AVFVideoRendererControl()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    m_displayLink->stop();
    if (m_videoOutput)
        [m_videoOutput release];
    if (m_subtitleOutput)
        [m_subtitleOutput release];
    if (m_subtitleDelegate)
        [m_subtitleDelegate release];
}

void AVFVideoRendererControl::reconfigure()
{
#ifdef QT_DEBUG_AVF
    qDebug() << "reconfigure";
#endif
    if (!m_layer) {
        m_displayLink->stop();
        return;
    }

    QMutexLocker locker(&m_mutex);

    m_displayLink->start();

    nativeSizeChanged();
}

void AVFVideoRendererControl::setLayer(CALayer *layer)
{
    if (m_layer == layer)
        return;

    AVPlayerLayer *plLayer = playerLayer();
    if (plLayer) {
        if (m_videoOutput)
            [[[plLayer player] currentItem] removeOutput:m_videoOutput];

        if (m_subtitleOutput)
            [[[plLayer player] currentItem] removeOutput:m_subtitleOutput];
    }

    if (!layer && m_sink)
        m_sink->setVideoFrame(QVideoFrame());

    AVFVideoSinkInterface::setLayer(layer);
}

void AVFVideoRendererControl::setVideoRotation(QVideoFrame::RotationAngle rotation)
{
    m_rotation = rotation;
}

void AVFVideoRendererControl::setVideoMirrored(bool mirrored)
{
    m_mirrored = mirrored;
}

void AVFVideoRendererControl::updateVideoFrame(const CVTimeStamp &ts)
{
    Q_UNUSED(ts);

    if (!m_sink)
        return;

    if (!m_layer)
        return;

    auto *layer = playerLayer();
    if (!layer.readyForDisplay)
        return;
    nativeSizeChanged();

    QVideoFrame frame;
    size_t width, height;
    CVPixelBufferRef pixelBuffer = copyPixelBufferFromLayer(width, height);
    if (!pixelBuffer)
        return;
    AVFVideoBuffer *buffer = new AVFVideoBuffer(this, pixelBuffer);
    auto fmt = buffer->fromCVVideoPixelFormat(CVPixelBufferGetPixelFormatType(pixelBuffer));
//    qDebug() << "Got pixelbuffer with format" << fmt << Qt::hex << CVPixelBufferGetPixelFormatType(pixelBuffer);
    CVPixelBufferRelease(pixelBuffer);

    QVideoFrameFormat format(QSize(width, height), fmt);

    frame = QVideoFrame(buffer, format);
    frame.setRotationAngle(m_rotation);
    frame.setMirrored(m_mirrored);
    m_sink->setVideoFrame(frame);
}

static NSDictionary* const AVF_OUTPUT_SETTINGS = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey: @[
            @(kCVPixelFormatType_32BGRA),
            @(kCVPixelFormatType_32RGBA),
            @(kCVPixelFormatType_422YpCbCr8),
            @(kCVPixelFormatType_422YpCbCr8_yuvs),
            @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
            @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange),
            @(kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange),
            @(kCVPixelFormatType_420YpCbCr10BiPlanarFullRange),
            @(kCVPixelFormatType_OneComponent8),
            @(q_kCVPixelFormatType_OneComponent16),
            @(kCVPixelFormatType_420YpCbCr8Planar),
            @(kCVPixelFormatType_420YpCbCr8PlanarFullRange)
        ],
        (NSString *)kCVPixelBufferMetalCompatibilityKey: @true
};

CVPixelBufferRef AVFVideoRendererControl::copyPixelBufferFromLayer(size_t& width, size_t& height)
{
    AVPlayerLayer *layer = playerLayer();
    //Is layer valid
    if (!layer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferFromLayer: invalid layer");
#endif
        return nullptr;
    }

    AVPlayerItem * item = [[layer player] currentItem];

    if (!m_videoOutput) {
        if (!m_outputSettings)
            m_outputSettings = AVF_OUTPUT_SETTINGS;
        m_videoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:m_outputSettings];
        [m_videoOutput setDelegate:nil queue:nil];
    }
    if (!m_subtitleOutput) {
        m_subtitleOutput = [[AVPlayerItemLegibleOutput alloc] init];
        m_subtitleDelegate = [[SubtitleDelegate alloc] initWithRenderer:this];
        [m_subtitleOutput setDelegate:m_subtitleDelegate queue:dispatch_get_main_queue()];
    }
    if (![item.outputs containsObject:m_videoOutput])
        [item addOutput:m_videoOutput];
    if (![item.outputs containsObject:m_subtitleOutput])
        [item addOutput:m_subtitleOutput];

    CFTimeInterval currentCAFrameTime =  CACurrentMediaTime();
    CMTime currentCMFrameTime =  [m_videoOutput itemTimeForHostTime:currentCAFrameTime];
    // happens when buffering / loading
    if (CMTimeCompare(currentCMFrameTime, kCMTimeZero) < 0) {
        return nullptr;
    }

    if (![m_videoOutput hasNewPixelBufferForItemTime:currentCMFrameTime])
        return nullptr;

    CVPixelBufferRef pixelBuffer = [m_videoOutput copyPixelBufferForItemTime:currentCMFrameTime
                                                   itemTimeForDisplay:nil];
    if (!pixelBuffer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferForItemTime returned nil");
        CMTimeShow(currentCMFrameTime);
#endif
        return nullptr;
    }

    width = CVPixelBufferGetWidth(pixelBuffer);
    height = CVPixelBufferGetHeight(pixelBuffer);
//    auto f = CVPixelBufferGetPixelFormatType(pixelBuffer);
//    char fmt[5];
//    memcpy(fmt, &f, 4);
//    fmt[4] = 0;
//    qDebug() << "copyPixelBuffer" << f << fmt << width << height;
    return pixelBuffer;
}

#include "moc_avfvideorenderercontrol_p.cpp"
