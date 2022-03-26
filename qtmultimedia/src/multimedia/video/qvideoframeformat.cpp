/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qvideoframeformat.h"
#include "qvideotexturehelper_p.h"

#include <qdebug.h>
#include <qlist.h>
#include <qmetatype.h>
#include <qpair.h>
#include <qvariant.h>
#include <qmatrix4x4.h>

static void initResource() {
    Q_INIT_RESOURCE(shaders);
}

QT_BEGIN_NAMESPACE

class QVideoFrameFormatPrivate : public QSharedData
{
public:
    QVideoFrameFormatPrivate() = default;

    QVideoFrameFormatPrivate(
            const QSize &size,
            QVideoFrameFormat::PixelFormat format)
        : pixelFormat(format)
        , frameSize(size)
        , viewport(QPoint(0, 0), size)
    {
    }

    bool operator ==(const QVideoFrameFormatPrivate &other) const
    {
        if (pixelFormat == other.pixelFormat
            && scanLineDirection == other.scanLineDirection
            && frameSize == other.frameSize
            && viewport == other.viewport
            && frameRatesEqual(frameRate, other.frameRate)
            && ycbcrColorSpace == other.ycbcrColorSpace
            && mirrored == other.mirrored)
            return true;

        return false;
    }

    inline static bool frameRatesEqual(qreal r1, qreal r2)
    {
        return qAbs(r1 - r2) <= 0.00001 * qMin(qAbs(r1), qAbs(r2));
    }

    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
    QVideoFrameFormat::Direction scanLineDirection = QVideoFrameFormat::TopToBottom;
    QSize frameSize;
    QVideoFrameFormat::YCbCrColorSpace ycbcrColorSpace = QVideoFrameFormat::YCbCr_Undefined;
    QRect viewport;
    qreal frameRate = 0.0;
    bool mirrored = false;
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QVideoFrameFormatPrivate);

/*!
    \class QVideoFrameFormat
    \brief The QVideoFrameFormat class specifies the stream format of a video presentation
    surface.
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_video

    A video sink presents a stream of video frames.  QVideoFrameFormat describes the type of
    the frames and determines how they should be presented.

    The core properties of a video stream required to setup a video sink are the pixel format
    given by pixelFormat(), and the frame dimensions given by frameSize().

    The region of a frame that is actually displayed on a video surface is given by the viewport().
    A stream may have a viewport less than the entire region of a frame to allow for videos smaller
    than the nearest optimal size of a video frame.  For example the width of a frame may be
    extended so that the start of each scan line is eight byte aligned.

    Other common properties are the scanLineDirection(), frameRate() and the yCrCbColorSpace().
*/

/*!
    \enum QVideoFrameFormat::PixelFormat

    Enumerates video data types.

    \value Format_Invalid
    The frame is invalid.

    \value Format_ARGB8888
    The frame is stored using a ARGB format with 8 bits per component.

    \value Format_ARGB8888_Premultiplied
    The frame stored using a premultiplied ARGB format with 8 bits per component.

    \value Format_XRGB8888
    The frame stored using a 32 bits per pixel RGB format (0xff, R, G, B).

    \value Format_BGRA8888
    The frame is stored using a 32-bit BGRA format (0xBBGGRRAA).

    \value Format_BGRA8888_Premultiplied
    The frame is stored using a premultiplied 32bit BGRA format.

    \value Format_ABGR8888
    The frame is stored using a 32-bit ABGR format (0xAABBGGRR).

    \value Format_XBGR8888
    The frame is stored using a 32-bit BGR format (0xffBBGGRR).

    \value Format_RGBA8888
    The frame is stored in memory as the bytes R, G, B, A/X, with R at the lowest address and A/X at the highest address.

    \value Format_BGRX8888
    The frame is stored in format 32-bit BGRx format, [31:0] B:G:R:x 8:8:8:8 little endian

    \value Format_RGBX8888
    The frame is stored in memory as the bytes R, G, B, A/X, with R at the lowest address and A/X at the highest address.

    \value Format_AYUV
    The frame is stored using a packed 32-bit AYUV format (0xAAYYUUVV).

    \value Format_AYUV_Premultiplied
    The frame is stored using a packed premultiplied 32-bit AYUV format (0xAAYYUUVV).

    \value Format_YUV420P
    The frame is stored using an 8-bit per component planar YUV format with the U and V planes
    horizontally and vertically sub-sampled, i.e. the height and width of the U and V planes are
    half that of the Y plane.

    \value Format_YUV422P
    The frame is stored using an 8-bit per component planar YUV format with the U and V planes
    horizontally sub-sampled, i.e. the width of the U and V planes are
    half that of the Y plane, and height of U and V planes is the same as Y.

    \value Format_YV12
    The frame is stored using an 8-bit per component planar YVU format with the V and U planes
    horizontally and vertically sub-sampled, i.e. the height and width of the V and U planes are
    half that of the Y plane.

    \value Format_UYVY
    The frame is stored using an 8-bit per component packed YUV format with the U and V planes
    horizontally sub-sampled (U-Y-V-Y), i.e. two horizontally adjacent pixels are stored as a 32-bit
    macropixel which has a Y value for each pixel and common U and V values.

    \value Format_YUYV
    The frame is stored using an 8-bit per component packed YUV format with the U and V planes
    horizontally sub-sampled (Y-U-Y-V), i.e. two horizontally adjacent pixels are stored as a 32-bit
    macropixel which has a Y value for each pixel and common U and V values.

    \value Format_NV12
    The frame is stored using an 8-bit per component semi-planar YUV format with a Y plane (Y)
    followed by a horizontally and vertically sub-sampled, packed UV plane (U-V).

    \value Format_NV21
    The frame is stored using an 8-bit per component semi-planar YUV format with a Y plane (Y)
    followed by a horizontally and vertically sub-sampled, packed VU plane (V-U).

    \value Format_IMC1
    The frame is stored using an 8-bit per component planar YUV format with the U and V planes
    horizontally and vertically sub-sampled.  This is similar to the Format_YUV420P type, except
    that the bytes per line of the U and V planes are padded out to the same stride as the Y plane.

    \value Format_IMC2
    The frame is stored using an 8-bit per component planar YUV format with the U and V planes
    horizontally and vertically sub-sampled.  This is similar to the Format_YUV420P type, except
    that the lines of the U and V planes are interleaved, i.e. each line of U data is followed by a
    line of V data creating a single line of the same stride as the Y data.

    \value Format_IMC3
    The frame is stored using an 8-bit per component planar YVU format with the V and U planes
    horizontally and vertically sub-sampled.  This is similar to the Format_YV12 type, except that
    the bytes per line of the V and U planes are padded out to the same stride as the Y plane.

    \value Format_IMC4
    The frame is stored using an 8-bit per component planar YVU format with the V and U planes
    horizontally and vertically sub-sampled.  This is similar to the Format_YV12 type, except that
    the lines of the V and U planes are interleaved, i.e. each line of V data is followed by a line
    of U data creating a single line of the same stride as the Y data.

    \value Format_P010
    The frame is stored using a 16bit per component semi-planar YUV format with a Y plane (Y)
    followed by a horizontally and vertically sub-sampled, packed UV plane (U-V). Only the 10 most
    significant bits of each component are being used.

    \value Format_P016
    The frame is stored using a 16bit per component semi-planar YUV format with a Y plane (Y)
    followed by a horizontally and vertically sub-sampled, packed UV plane (U-V).

    \value Format_Y8
    The frame is stored using an 8-bit greyscale format.

    \value Format_Y16
    The frame is stored using a 16-bit linear greyscale format.  Little endian.

    \value Format_Jpeg
    The frame is stored in compressed Jpeg format.

    \value Format_SamplerExternalOES
    The frame is stored in external OES texture format. This is currently only being used on Android.

    \value Format_SamplerRect
    The frame is stored in rectangle texture format (GL_TEXTURE_RECTANGLE). This is only being used on
    macOS with an OpenGL based Rendering Hardware interface. The underlying pixel format stored in the
    texture is Format_BRGA8888.
*/

/*!
    \enum QVideoFrameFormat::Direction

    Enumerates the layout direction of video scan lines.

    \value TopToBottom Scan lines are arranged from the top of the frame to the bottom.
    \value BottomToTop Scan lines are arranged from the bottom of the frame to the top.
*/

/*!
    \enum QVideoFrameFormat::YCbCrColorSpace

    Enumerates the Y'CbCr color space of video frames.

    \value YCbCr_Undefined
    No color space is specified.

    \value YCbCr_BT601
    A Y'CbCr color space defined by ITU-R recommendation BT.601
    with Y value range from 16 to 235, and Cb/Cr range from 16 to 240.
    Used in standard definition video.

    \value YCbCr_BT709
    A Y'CbCr color space defined by ITU-R BT.709 with the same values range as YCbCr_BT601.  Used
    for HDTV.

    \value YCbCr_xvYCC601
    The BT.601 color space with the value range extended to 0 to 255.
    It is backward compatible with BT.601 and uses values outside BT.601 range to represent a
    wider range of colors.

    \value YCbCr_xvYCC709
    The BT.709 color space with the value range extended to 0 to 255.

    \value YCbCr_JPEG
    The full range Y'CbCr color space used in JPEG files.

    \value YCbCr_BT2020
    The color space defined by ITU-R BT.2020. Used mainly for HDR videos.
*/

/*!
    Constructs a null video stream format.
*/
QVideoFrameFormat::QVideoFrameFormat()
    : d(new QVideoFrameFormatPrivate)
{
    initResource();
}

/*!
    Constructs a video stream with the given frame \a size and pixel \a format.
*/
QVideoFrameFormat::QVideoFrameFormat(
        const QSize& size, QVideoFrameFormat::PixelFormat format)
    : d(new QVideoFrameFormatPrivate(size, format))
{
}

/*!
    Constructs a copy of \a other.
*/
QVideoFrameFormat::QVideoFrameFormat(const QVideoFrameFormat &other) = default;

/*!
    \fn QVideoFrameFormat::QVideoFrameFormat(QVideoFrameFormat &&other)

    Constructs a QVideoFrameFormat by moving from \a other.
*/

/*!
    Assigns the values of \a other to this object.
*/
QVideoFrameFormat &QVideoFrameFormat::operator =(const QVideoFrameFormat &other) = default;

/*!
    \fn QVideoFrameFormat &QVideoFrameFormat::operator =(QVideoFrameFormat &&other)

    Moves \a other into this QVideoFrameFormat.
*/

/*!
    Destroys a video stream description.
*/
QVideoFrameFormat::~QVideoFrameFormat() = default;

/*!
    Identifies if a video surface format has a valid pixel format and frame size.

    Returns true if the format is valid, and false otherwise.
*/
bool QVideoFrameFormat::isValid() const
{
    return d->pixelFormat != Format_Invalid && d->frameSize.isValid();
}

/*!
    Returns true if \a other is the same as this video format, and false if they are different.
*/
bool QVideoFrameFormat::operator ==(const QVideoFrameFormat &other) const
{
    return d == other.d || *d == *other.d;
}

/*!
    Returns true if \a other is different to this video format, and false if they are the same.
*/
bool QVideoFrameFormat::operator !=(const QVideoFrameFormat &other) const
{
    return d != other.d && !(*d == *other.d);
}

/*!
    \internal
*/
void QVideoFrameFormat::detach()
{
    d.detach();
}

/*!
    Returns the pixel format of frames in a video stream.
*/
QVideoFrameFormat::PixelFormat QVideoFrameFormat::pixelFormat() const
{
    return d->pixelFormat;
}

/*!
    Returns the dimensions of frames in a video stream.

    \sa frameWidth(), frameHeight()
*/
QSize QVideoFrameFormat::frameSize() const
{
    return d->frameSize;
}

/*!
    Returns the width of frames in a video stream.

    \sa frameSize(), frameHeight()
*/
int QVideoFrameFormat::frameWidth() const
{
    return d->frameSize.width();
}

/*!
    Returns the height of frame in a video stream.
*/
int QVideoFrameFormat::frameHeight() const
{
    return d->frameSize.height();
}

/*!
    Returns the number of planes used.
    This number is depending on the pixel format and is
    1 for RGB based formats, and a number between 1 and 3 for
    YUV based formats.
*/
int QVideoFrameFormat::planeCount() const
{
    return QVideoTextureHelper::textureDescription(d->pixelFormat)->nplanes;
}

/*!
    Sets the size of frames in a video stream to \a size.

    This will reset the viewport() to fill the entire frame.
*/
void QVideoFrameFormat::setFrameSize(const QSize &size)
{
    detach();
    d->frameSize = size;
    d->viewport = QRect(QPoint(0, 0), size);
}

/*!
    \overload

    Sets the \a width and \a height of frames in a video stream.

    This will reset the viewport() to fill the entire frame.
*/
void QVideoFrameFormat::setFrameSize(int width, int height)
{
    detach();
    d->frameSize = QSize(width, height);
    d->viewport = QRect(0, 0, width, height);
}

/*!
    Returns the viewport of a video stream.

    The viewport is the region of a video frame that is actually displayed.

    By default the viewport covers an entire frame.
*/
QRect QVideoFrameFormat::viewport() const
{
    return d->viewport;
}

/*!
    Sets the viewport of a video stream to \a viewport.
*/
void QVideoFrameFormat::setViewport(const QRect &viewport)
{
    detach();
    d->viewport = viewport;
}

/*!
    Returns the direction of scan lines.
*/
QVideoFrameFormat::Direction QVideoFrameFormat::scanLineDirection() const
{
    return d->scanLineDirection;
}

/*!
    Sets the \a direction of scan lines.
*/
void QVideoFrameFormat::setScanLineDirection(Direction direction)
{
    detach();
    d->scanLineDirection = direction;
}

/*!
    Returns the frame rate of a video stream in frames per second.
*/
qreal QVideoFrameFormat::frameRate() const
{
    return d->frameRate;
}

/*!
    Sets the frame \a rate of a video stream in frames per second.
*/
void QVideoFrameFormat::setFrameRate(qreal rate)
{
    detach();
    d->frameRate = rate;
}

/*!
    Returns the Y'CbCr color space of a video stream.
*/
QVideoFrameFormat::YCbCrColorSpace QVideoFrameFormat::yCbCrColorSpace() const
{
    return d->ycbcrColorSpace;
}

/*!
    Sets the Y'CbCr color \a space of a video stream.
    It is only used with raw YUV frame types.
*/
void QVideoFrameFormat::setYCbCrColorSpace(QVideoFrameFormat::YCbCrColorSpace space)
{
    detach();
    d->ycbcrColorSpace = space;
}

/*!
    Returns \c true if the surface is mirrored around its vertical axis.
    This is typically needed for video frames coming from a front camera of a mobile device.

    \note The mirroring here differs from QImage::mirrored, as a vertically mirrored QImage
    will be mirrored around its x-axis.

    \since 5.11
 */
bool QVideoFrameFormat::isMirrored() const
{
    return d->mirrored;
}

/*!
    Sets if the surface is \a mirrored around its vertical axis.
    This is typically needed for video frames coming from a front camera of a mobile device.
    Default value is false.

    \note The mirroring here differs from QImage::mirrored, as a vertically mirrored QImage
    will be mirrored around its x-axis.

    \since 5.11
 */
void QVideoFrameFormat::setMirrored(bool mirrored)
{
    detach();
    d->mirrored = mirrored;
}

/*!
    \internal
*/
QString QVideoFrameFormat::vertexShaderFileName() const
{
    return QVideoTextureHelper::vertexShaderFileName(d->pixelFormat);
}

/*!
    \internal
*/
QString QVideoFrameFormat::fragmentShaderFileName() const
{
    return QVideoTextureHelper::fragmentShaderFileName(d->pixelFormat);
}

/*!
    \internal
*/
void QVideoFrameFormat::updateUniformData(QByteArray *dst, const QVideoFrame &frame, const QMatrix4x4 &transform, float opacity) const
{
    QVideoTextureHelper::updateUniformData(dst, *this, frame, transform, opacity);
}


/*!
    Returns a video pixel format equivalent to an image \a format.  If there is no equivalent
    format QVideoFrameFormat::Format_Invalid is returned instead.

    \note In general \l QImage does not handle YUV formats.

*/
QVideoFrameFormat::PixelFormat QVideoFrameFormat::pixelFormatFromImageFormat(QImage::Format format)
{
    switch (format) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    case QImage::Format_RGB32:
        return QVideoFrameFormat::Format_BGRX8888;
    case QImage::Format_ARGB32:
        return QVideoFrameFormat::Format_BGRA8888;
    case QImage::Format_ARGB32_Premultiplied:
        return QVideoFrameFormat::Format_BGRA8888_Premultiplied;
#else
    case QImage::Format_RGB32:
        return QVideoFrameFormat::Format_XRGB8888;
    case QImage::Format_ARGB32:
        return QVideoFrameFormat::Format_ARGB8888;
    case QImage::Format_ARGB32_Premultiplied:
        return QVideoFrameFormat::Format_ARGB8888_Premultiplied;
#endif
    case QImage::Format_RGBA8888:
        return QVideoFrameFormat::Format_RGBA8888;
    case QImage::Format_RGBA8888_Premultiplied:
        return QVideoFrameFormat::Format_ARGB8888_Premultiplied;
    case QImage::Format_RGBX8888:
        return QVideoFrameFormat::Format_RGBX8888;
    case QImage::Format_Grayscale8:
        return QVideoFrameFormat::Format_Y8;
    case QImage::Format_Grayscale16:
        return QVideoFrameFormat::Format_Y16;
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

/*!
    Returns an image format equivalent to a video frame pixel \a format.  If there is no equivalent
    format QImage::Format_Invalid is returned instead.

    \note In general \l QImage does not handle YUV formats.

*/
QImage::Format QVideoFrameFormat::imageFormatFromPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    case QVideoFrameFormat::Format_BGRA8888:
        return QImage::Format_ARGB32;
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
        return QImage::Format_ARGB32_Premultiplied;
    case QVideoFrameFormat::Format_BGRX8888:
        return QImage::Format_RGB32;
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
        return QImage::Format_Invalid;
#else
    case QVideoFrameFormat::Format_ARGB8888:
        return QImage::Format_ARGB32;
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
        return QImage::Format_ARGB32_Premultiplied;
    case QVideoFrameFormat::Format_XRGB8888:
        return QImage::Format_RGB32;
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        return QImage::Format_Invalid;
#endif
    case QVideoFrameFormat::Format_RGBA8888:
        return QImage::Format_RGBA8888;
    case QVideoFrameFormat::Format_RGBX8888:
        return QImage::Format_RGBX8888;
    case QVideoFrameFormat::Format_Y8:
        return QImage::Format_Grayscale8;
    case QVideoFrameFormat::Format_Y16:
        return QImage::Format_Grayscale16;
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV:
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
    case QVideoFrameFormat::Format_Jpeg:
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_SamplerExternalOES:
    case QVideoFrameFormat::Format_SamplerRect:
        return QImage::Format_Invalid;
    }
    return QImage::Format_Invalid;
}

/*!
    Returns a string representation of the given \a pixelFormat.
*/
QString QVideoFrameFormat::pixelFormatToString(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_Invalid:
        return QStringLiteral("Invalid");
    case QVideoFrameFormat::Format_ARGB8888:
        return QStringLiteral("ARGB8888");
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
        return QStringLiteral("ARGB8888 Premultiplied");
    case QVideoFrameFormat::Format_XRGB8888:
        return QStringLiteral("XRGB8888");
    case QVideoFrameFormat::Format_BGRA8888:
        return QStringLiteral("BGRA8888");
    case QVideoFrameFormat::Format_BGRX8888:
        return QStringLiteral("BGRX8888");
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
        return QStringLiteral("BGRA8888 Premultiplied");
    case QVideoFrameFormat::Format_RGBA8888:
        return QStringLiteral("RGBA8888");
    case QVideoFrameFormat::Format_RGBX8888:
        return QStringLiteral("RGBX8888");
    case QVideoFrameFormat::Format_ABGR8888:
        return QStringLiteral("ABGR8888");
    case QVideoFrameFormat::Format_XBGR8888:
        return QStringLiteral("XBGR8888");
    case QVideoFrameFormat::Format_AYUV:
        return QStringLiteral("AYUV");
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
        return QStringLiteral("AYUV Premultiplied");
    case QVideoFrameFormat::Format_YUV420P:
        return QStringLiteral("YUV420P");
    case QVideoFrameFormat::Format_YUV422P:
        return QStringLiteral("YUV422P");
    case QVideoFrameFormat::Format_YV12:
        return QStringLiteral("YV12");
    case QVideoFrameFormat::Format_UYVY:
        return QStringLiteral("UYVY");
    case QVideoFrameFormat::Format_YUYV:
        return QStringLiteral("YUYV");
    case QVideoFrameFormat::Format_NV12:
        return QStringLiteral("NV12");
    case QVideoFrameFormat::Format_NV21:
        return QStringLiteral("NV21");
    case QVideoFrameFormat::Format_IMC1:
        return QStringLiteral("IMC1");
    case QVideoFrameFormat::Format_IMC2:
        return QStringLiteral("IMC2");
    case QVideoFrameFormat::Format_IMC3:
        return QStringLiteral("IMC3");
    case QVideoFrameFormat::Format_IMC4:
        return QStringLiteral("IMC4");
    case QVideoFrameFormat::Format_Y8:
        return QStringLiteral("Y8");
    case QVideoFrameFormat::Format_Y16:
        return QStringLiteral("Y16");
    case QVideoFrameFormat::Format_P010:
        return QStringLiteral("P010");
    case QVideoFrameFormat::Format_P016:
        return QStringLiteral("P016");
    case QVideoFrameFormat::Format_SamplerExternalOES:
        return QStringLiteral("SamplerExternalOES");
    case QVideoFrameFormat::Format_Jpeg:
        return QStringLiteral("Jpeg");
    case QVideoFrameFormat::Format_SamplerRect:
        return QStringLiteral("SamplerRect");
    }

    return QStringLiteral("");
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QVideoFrameFormat::YCbCrColorSpace cs)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (cs) {
        case QVideoFrameFormat::YCbCr_BT601:
            dbg << "YCbCr_BT601";
            break;
        case QVideoFrameFormat::YCbCr_BT709:
            dbg << "YCbCr_BT709";
            break;
        case QVideoFrameFormat::YCbCr_JPEG:
            dbg << "YCbCr_JPEG";
            break;
        case QVideoFrameFormat::YCbCr_xvYCC601:
            dbg << "YCbCr_xvYCC601";
            break;
        case QVideoFrameFormat::YCbCr_xvYCC709:
            dbg << "YCbCr_xvYCC709";
            break;
        case QVideoFrameFormat::YCbCr_BT2020:
            dbg << "YCbCr_BT2020";
            break;
        default:
            dbg << "YCbCr_Undefined";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, QVideoFrameFormat::Direction dir)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (dir) {
        case QVideoFrameFormat::BottomToTop:
            dbg << "BottomToTop";
            break;
        case QVideoFrameFormat::TopToBottom:
            dbg << "TopToBottom";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, const QVideoFrameFormat &f)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "QVideoFrameFormat(" << f.pixelFormat() << ", " << f.frameSize()
        << ", viewport=" << f.viewport()
        <<  ", yCbCrColorSpace=" << f.yCbCrColorSpace()
        << ')'
        << "\n    pixel format=" << f.pixelFormat()
        << "\n    frame size=" << f.frameSize()
        << "\n    viewport=" << f.viewport()
        << "\n    yCbCrColorSpace=" << f.yCbCrColorSpace()
        << "\n    frameRate=" << f.frameRate()
        << "\n    mirrored=" << f.isMirrored();

    return dbg;
}

QDebug operator<<(QDebug dbg, QVideoFrameFormat::PixelFormat pf)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    auto format = QVideoFrameFormat::pixelFormatToString(pf);
    if (format.isEmpty())
        return dbg;

    dbg.noquote() << QStringLiteral("Format_") << format;
    return dbg;
}
#endif

QT_END_NAMESPACE
