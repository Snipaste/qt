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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmath.h>
#include <private/qaudiohelpers_p.h>

#include "qgstreameraudiosource_p.h"
#include "qgstreameraudiodevice_p.h"
#include <sys/types.h>
#include <unistd.h>

#include <gst/gst.h>
Q_DECLARE_OPAQUE_POINTER(GstSample *);
Q_DECLARE_METATYPE(GstSample *);

QT_BEGIN_NAMESPACE


QGStreamerAudioSource::QGStreamerAudioSource(const QAudioDevice &device)
    : m_info(device),
      m_device(device.id())
{
    qRegisterMetaType<GstSample *>();
}

QGStreamerAudioSource::~QGStreamerAudioSource()
{
    close();
}

void QGStreamerAudioSource::setError(QAudio::Error error)
{
    if (m_errorState == error)
        return;

    m_errorState = error;
    emit errorChanged(error);
}

QAudio::Error QGStreamerAudioSource::error() const
{
    return m_errorState;
}

void QGStreamerAudioSource::setState(QAudio::State state)
{
    if (m_deviceState == state)
        return;

    m_deviceState = state;
    emit stateChanged(state);
}

QAudio::State QGStreamerAudioSource::state() const
{
    return m_deviceState;
}

void QGStreamerAudioSource::setFormat(const QAudioFormat &format)
{
    if (m_deviceState == QAudio::StoppedState)
        m_format = format;
}

QAudioFormat QGStreamerAudioSource::format() const
{
    return m_format;
}

void QGStreamerAudioSource::start(QIODevice *device)
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    if (!open())
        return;

    m_pullMode = true;
    m_audioSink = device;

    setState(QAudio::ActiveState);
}

QIODevice *QGStreamerAudioSource::start()
{
    setState(QAudio::StoppedState);
    setError(QAudio::NoError);

    close();

    if (!open())
        return nullptr;

    m_pullMode = false;
    m_audioSink = new GStreamerInputPrivate(this);
    m_audioSink->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    setState(QAudio::IdleState);

    return m_audioSink;
}

void QGStreamerAudioSource::stop()
{
    if (m_deviceState == QAudio::StoppedState)
        return;

    close();

    setError(QAudio::NoError);
    setState(QAudio::StoppedState);
}

bool QGStreamerAudioSource::open()
{
    if (m_opened)
        return true;

    const auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_info.handle());
    if (!deviceInfo->gstDevice) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }

    gstInput = QGstElement(gst_device_create_element(deviceInfo->gstDevice, nullptr));
    if (gstInput.isNull()) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }

    auto gstCaps = QGstUtils::capsForAudioFormat(m_format);

    if (gstCaps.isNull()) {
        setError(QAudio::OpenError);
        setState(QAudio::StoppedState);
        return false;
    }


#ifdef DEBUG_AUDIO
    qDebug() << "Opening input" << QTime::currentTime();
    qDebug() << "Caps: " << gst_caps_to_string(gstCaps);
#endif

    gstPipeline = QGstPipeline("pipeline");

    auto *gstBus = gst_pipeline_get_bus(gstPipeline.pipeline());
    gst_bus_add_watch(gstBus, &QGStreamerAudioSource::busMessage, this);
    gst_object_unref (gstBus);

    gstAppSink = createAppSink();
    gstAppSink.set("caps", gstCaps);

    QGstElement conv("audioconvert", "conv");
    gstVolume = QGstElement("volume", "volume");
    if (m_volume != 1.)
        gstVolume.set("volume", m_volume);

    gstPipeline.add(gstInput, gstVolume, conv, gstAppSink);
    gstInput.link(gstVolume, conv, gstAppSink);

    gstPipeline.setState(GST_STATE_PLAYING);

    m_opened = true;

    m_timeStamp.restart();
    m_elapsedTimeOffset = 0;
    m_bytesWritten = 0;

    return true;
}

void QGStreamerAudioSource::close()
{
    if (!m_opened)
        return;

    gstPipeline.setState(GST_STATE_NULL);
    gstPipeline = {};
    gstVolume = {};
    gstAppSink = {};
    gstInput = {};

    if (!m_pullMode && m_audioSink) {
        delete m_audioSink;
    }
    m_audioSink = nullptr;
    m_opened = false;
}

gboolean QGStreamerAudioSource::busMessage(GstBus *, GstMessage *msg, gpointer user_data)
{
    QGStreamerAudioSource *input = static_cast<QGStreamerAudioSource *>(user_data);
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
        input->stop();
        break;
    case GST_MESSAGE_ERROR: {
        input->setError(QAudio::IOError);
        gchar  *debug;
        GError *error;

        gst_message_parse_error (msg, &error, &debug);
        g_free (debug);

        qDebug("Error: %s\n", error->message);
        g_error_free (error);

        break;
    }
    default:
        break;
    }
    return false;
}

qsizetype QGStreamerAudioSource::bytesReady() const
{
    return m_buffer.size();
}

void QGStreamerAudioSource::resume()
{
    if (m_deviceState == QAudio::SuspendedState || m_deviceState == QAudio::IdleState) {
        gstPipeline.setState(GST_STATE_PLAYING);
        setState(QAudio::ActiveState);
        setError(QAudio::NoError);
    }
}

void QGStreamerAudioSource::setVolume(qreal vol)
{
    if (m_volume == vol)
        return;

    m_volume = vol;
    if (!gstVolume.isNull())
        gstVolume.set("volume", vol);
}

qreal QGStreamerAudioSource::volume() const
{
    return m_volume;
}

void QGStreamerAudioSource::setBufferSize(qsizetype value)
{
    m_bufferSize = value;
}

qsizetype QGStreamerAudioSource::bufferSize() const
{
    return m_bufferSize;
}

qint64 QGStreamerAudioSource::processedUSecs() const
{
    return m_format.durationForBytes(m_bytesWritten);
}

void QGStreamerAudioSource::suspend()
{
    if (m_deviceState == QAudio::ActiveState) {
        setError(QAudio::NoError);
        setState(QAudio::SuspendedState);

        gstPipeline.setState(GST_STATE_PAUSED);
    }
}

void QGStreamerAudioSource::reset()
{
    stop();
    m_buffer.clear();
}

//#define MAX_BUFFERS_IN_QUEUE 4

QGstElement QGStreamerAudioSource::createAppSink()
{
    QGstElement sink("appsink", "appsink");
    GstAppSink *appSink = reinterpret_cast<GstAppSink *>(sink.element());

    GstAppSinkCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.eos = &eos;
    callbacks.new_sample = &new_sample;
    gst_app_sink_set_callbacks(appSink, &callbacks, this, nullptr);
//    gst_app_sink_set_max_buffers(appSink, MAX_BUFFERS_IN_QUEUE);
    gst_base_sink_set_sync(GST_BASE_SINK(appSink), FALSE);

    return sink;
}

void QGStreamerAudioSource::newDataAvailable(GstSample *sample)
{
    if (m_audioSink) {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo mapInfo;
        gst_buffer_map(buffer, &mapInfo, GST_MAP_READ);
        const char *bufferData = (const char*)mapInfo.data;
        gsize bufferSize = mapInfo.size;

        if (!m_pullMode) {
                // need to store that data in the QBuffer
            m_buffer.append(bufferData, bufferSize);
            m_audioSink->readyRead();
        } else {
            m_bytesWritten += bufferSize;
            m_audioSink->write(bufferData, bufferSize);
        }

        gst_buffer_unmap(buffer, &mapInfo);
    }

    gst_sample_unref(sample);
}

GstFlowReturn QGStreamerAudioSource::new_sample(GstAppSink *sink, gpointer user_data)
{
    // "Note that the preroll buffer will also be returned as the first buffer when calling gst_app_sink_pull_buffer()."
    QGStreamerAudioSource *control = static_cast<QGStreamerAudioSource*>(user_data);

    GstSample *sample = gst_app_sink_pull_sample(sink);
    QMetaObject::invokeMethod(control, "newDataAvailable", Qt::AutoConnection, Q_ARG(GstSample *, sample));

    return GST_FLOW_OK;
}

void QGStreamerAudioSource::eos(GstAppSink *, gpointer user_data)
{
    QGStreamerAudioSource *control = static_cast<QGStreamerAudioSource*>(user_data);
    control->setState(QAudio::StoppedState);
}

GStreamerInputPrivate::GStreamerInputPrivate(QGStreamerAudioSource *audio)
{
    m_audioDevice = qobject_cast<QGStreamerAudioSource*>(audio);
}

qint64 GStreamerInputPrivate::readData(char *data, qint64 len)
{
    if (m_audioDevice->state() == QAudio::IdleState)
        m_audioDevice->setState(QAudio::ActiveState);
    qint64 bytes = m_audioDevice->m_buffer.read(data, len);
    m_audioDevice->m_bytesWritten += bytes;
    return bytes;
}

qint64 GStreamerInputPrivate::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 GStreamerInputPrivate::bytesAvailable() const
{
    return m_audioDevice->m_buffer.size();
}


QT_END_NAMESPACE

#include "moc_qgstreameraudiosource_p.cpp"
