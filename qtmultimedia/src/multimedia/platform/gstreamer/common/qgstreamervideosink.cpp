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

#include "qgstreamervideosink_p.h"
#include "private/qgstvideorenderersink_p.h"
#include "private/qgstsubtitlesink_p.h"
#include <private/qgstutils_p.h>
#include <QtGui/private/qrhi_p.h>

#if QT_CONFIG(gstreamer_gl)
#include <QtGui/private/qrhigles2_p.h>
#include <QGuiApplication>
#include <QtGui/qopenglcontext.h>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <gst/gl/gstglconfig.h>

#if GST_GL_HAVE_WINDOW_X11 && __has_include("X11/Xlib-xcb.h")
#    include <gst/gl/x11/gstgldisplay_x11.h>
#endif
#if GST_GL_HAVE_PLATFORM_EGL
#    include <gst/gl/egl/gstgldisplay_egl.h>
#    include <EGL/egl.h>
#    include <EGL/eglext.h>
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND && __has_include("wayland-client.h")
#    include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif
#endif // #if QT_CONFIG(gstreamer_gl)

#include <QtCore/qdebug.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMediaVideoSink, "qt.multimedia.videosink")

QGstreamerVideoSink::QGstreamerVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
    sinkBin = QGstBin("videoSinkBin");
    // This is a hack for some iMX platforms. Thos require the use of a special video
    // conversion element in the pipeline before the video sink, as they unfortunately
    // output some proprietary format from the decoder even though it's marked as
    // a regular supported video/x-raw format.
    //
    // To fix this, simply insert the element into the pipeline if it's available. Otherwise
    // we simply use an identity element.
    gstQueue = QGstElement("queue");
    auto imxVideoConvert = QGstElement("imxvideoconvert_g2d");
    if (!imxVideoConvert.isNull())
        gstPreprocess = imxVideoConvert;
    else
        gstPreprocess = QGstElement("identity");
    sinkBin.add(gstQueue, gstPreprocess);
    gstQueue.link(gstPreprocess);
    sinkBin.addGhostPad(gstQueue, "sink");

    gstSubtitleSink = GST_ELEMENT(QGstSubtitleSink::createSink(this));
}

QGstreamerVideoSink::~QGstreamerVideoSink()
{
    unrefGstContexts();

    setPipeline(QGstPipeline());
}

QGstElement QGstreamerVideoSink::gstSink()
{
    updateSinkElement();
    return sinkBin;
}

void QGstreamerVideoSink::setPipeline(QGstPipeline pipeline)
{
    gstPipeline = pipeline;
}

bool QGstreamerVideoSink::inStoppedState() const
{
    if (gstPipeline.isNull())
        return true;
    return gstPipeline.inStoppedState();
}

void QGstreamerVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && rhi->backend() != QRhi::OpenGLES2)
        rhi = nullptr;
    if (m_rhi == rhi)
        return;

    m_rhi = rhi;
    updateGstContexts();
    if (!gstQtSink.isNull()) {
        // force creation of a new sink with proper caps
        createQtSink();
        updateSinkElement();
    }
}

void QGstreamerVideoSink::createQtSink()
{
    gstQtSink = QGstElement(reinterpret_cast<GstElement *>(QGstVideoRendererSink::createSink(this)));
}

void QGstreamerVideoSink::updateSinkElement()
{
    QGstElement newSink;
    if (gstQtSink.isNull())
        createQtSink();
    newSink = gstQtSink;

    if (newSink == gstVideoSink)
        return;

    gstPipeline.beginConfig();

    if (!gstVideoSink.isNull()) {
        gstVideoSink.setStateSync(GST_STATE_NULL);
        sinkBin.remove(gstVideoSink);
    }

    gstVideoSink = newSink;
    sinkBin.add(gstVideoSink);
    if (!gstPreprocess.link(gstVideoSink))
        qCDebug(qLcMediaVideoSink) << "couldn't link preprocess and sink";
    gstVideoSink.setState(GST_STATE_PAUSED);

    gstPipeline.endConfig();
    gstPipeline.dumpGraph("updateVideoSink");
}

void QGstreamerVideoSink::unrefGstContexts()
{
    if (m_gstGlDisplayContext)
        gst_context_unref(m_gstGlDisplayContext);
    m_gstGlDisplayContext = nullptr;
    if (m_gstGlLocalContext)
        gst_context_unref(m_gstGlLocalContext);
    m_gstGlLocalContext = nullptr;
    m_eglDisplay = nullptr;
    m_eglImageTargetTexture2D = nullptr;
}

void QGstreamerVideoSink::updateGstContexts()
{
    unrefGstContexts();

#if QT_CONFIG(gstreamer_gl)
    if (!m_rhi || m_rhi->backend() != QRhi::OpenGLES2)
        return;

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(m_rhi->nativeHandles());
    auto glContext = nativeHandles->context;
    Q_ASSERT(glContext);

    const QString platform = QGuiApplication::platformName();
    QPlatformNativeInterface *pni = QGuiApplication::platformNativeInterface();
    m_eglDisplay = pni->nativeResourceForIntegration("egldisplay");
//    qDebug() << "platform is" << platform << m_eglDisplay;

    GstGLDisplay *gstGlDisplay = nullptr;
    const char *contextName = "eglcontext";
    GstGLPlatform glPlatform = GST_GL_PLATFORM_EGL;
    // use the egl display if we have one
    if (m_eglDisplay) {
#if GST_GL_HAVE_PLATFORM_EGL
        gstGlDisplay = (GstGLDisplay *)gst_gl_display_egl_new_with_egl_display(m_eglDisplay);
        m_eglImageTargetTexture2D = eglGetProcAddress("glEGLImageTargetTexture2DOES");
#endif
    } else {
        auto display = pni->nativeResourceForIntegration("display");

        if (display) {
#if GST_GL_HAVE_WINDOW_X11 && __has_include("X11/Xlib-xcb.h")
            if (platform == QLatin1String("xcb")) {
                contextName = "glxcontext";
                glPlatform = GST_GL_PLATFORM_GLX;

                gstGlDisplay = (GstGLDisplay *)gst_gl_display_x11_new_with_display((Display *)display);
            }
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND && __has_include("wayland-client.h")
            if (platform.startsWith(QLatin1String("wayland"))) {
                Q_ASSERT(!gstGlDisplay);
                gstGlDisplay = (GstGLDisplay *)gst_gl_display_wayland_new_with_display((struct wl_display *)display);
            }
#endif
        }
    }

    if (!gstGlDisplay) {
        qWarning() << "Could not create GstGLDisplay";
        return;
    }

    void *nativeContext = pni->nativeResourceForContext(contextName, glContext);
    if (!nativeContext)
        qWarning() << "Could not find resource for" << contextName;

    GstGLAPI glApi = QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL ? GST_GL_API_OPENGL : GST_GL_API_GLES2;
    GstGLContext *appContext = gst_gl_context_new_wrapped(gstGlDisplay, (guintptr)nativeContext, glPlatform, glApi);
    if (!appContext)
        qWarning() << "Could not create wrappped context for platform:" << glPlatform;

    GstGLContext *displayContext = nullptr;
    GError *error = nullptr;
    gst_gl_display_create_context(gstGlDisplay, appContext, &displayContext, &error);
    if (error) {
        qWarning() << "Could not create display context:" << error->message;
        g_clear_error(&error);
    }

    if (appContext)
        gst_object_unref(appContext);

    m_gstGlDisplayContext = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, false);
    gst_context_set_gl_display(m_gstGlDisplayContext, gstGlDisplay);
    gst_object_unref(gstGlDisplay);

    m_gstGlLocalContext = gst_context_new("gst.gl.local_context", false);
    GstStructure *structure = gst_context_writable_structure(m_gstGlLocalContext);
    gst_structure_set(structure, "context", GST_TYPE_GL_CONTEXT, displayContext, nullptr);
    gst_object_unref(displayContext);

    if (!gstPipeline.isNull())
        gst_element_set_context(gstPipeline.element(), m_gstGlLocalContext);
#endif // #if QT_CONFIG(gstreamer_gl)
}

QT_END_NAMESPACE
