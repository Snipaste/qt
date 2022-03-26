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

#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#include "avfcameraservice_p.h"
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfimagecapture_p.h"
#include "avfcamerarenderer_p.h"
#include "avfimagecapture_p.h"
#include "avfmediaencoder_p.h"
#include <qmediadevices.h>
#include <private/qplatformaudioinput_p.h>
#include <private/qplatformaudiooutput_p.h>
#include <qaudioinput.h>
#include <qaudiooutput.h>

QT_USE_NAMESPACE

AVFCameraService::AVFCameraService()
{
    m_session = new AVFCameraSession(this);
}

AVFCameraService::~AVFCameraService()
{
    if (m_session)
        delete m_session;
}

QPlatformCamera *AVFCameraService::camera()
{
    return m_cameraControl;
}

void AVFCameraService::setCamera(QPlatformCamera *camera)
{
    AVFCamera *control = static_cast<AVFCamera *>(camera);
    if (m_cameraControl == control)
        return;

    if (m_cameraControl)
        m_cameraControl->setCaptureSession(nullptr);

    m_cameraControl = control;

    if (m_cameraControl)
        m_cameraControl->setCaptureSession(this);

    emit cameraChanged();
}

QPlatformImageCapture *AVFCameraService::imageCapture()
{
    return m_imageCaptureControl;
}

void AVFCameraService::setImageCapture(QPlatformImageCapture *imageCapture)
{
    AVFImageCapture *control = static_cast<AVFImageCapture *>(imageCapture);
    if (m_imageCaptureControl == control)
        return;

    if (m_imageCaptureControl)
        m_imageCaptureControl->setCaptureSession(nullptr);

    m_imageCaptureControl = control;
    if (m_imageCaptureControl)
        m_imageCaptureControl->setCaptureSession(this);
}

QPlatformMediaRecorder *AVFCameraService::mediaRecorder()
{
    return m_encoder;
}

void AVFCameraService::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    AVFMediaEncoder *control = static_cast<AVFMediaEncoder *>(recorder);
    if (m_encoder == control)
        return;

    if (m_encoder)
        m_encoder->setCaptureSession(nullptr);

    m_encoder = control;
    if (m_encoder)
        m_encoder->setCaptureSession(this);

    emit encoderChanged();
}

void AVFCameraService::setAudioInput(QPlatformAudioInput *input)
{
    if (m_audioInput == input)
        return;
    if (m_audioInput)
        m_audioInput->q->disconnect(this);

    m_audioInput = input;

    if (input) {
        connect(m_audioInput->q, &QAudioInput::destroyed, this, &AVFCameraService::audioInputDestroyed);
        connect(m_audioInput->q, &QAudioInput::deviceChanged, this, &AVFCameraService::audioInputChanged);
        connect(m_audioInput->q, &QAudioInput::mutedChanged, this, &AVFCameraService::setAudioInputMuted);
        connect(m_audioInput->q, &QAudioInput::volumeChanged, this, &AVFCameraService::setAudioInputVolume);
    }
    audioInputChanged();
}

void AVFCameraService::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    if (m_audioOutput)
        m_audioOutput->q->disconnect(this);

    m_audioOutput = output;

    if (m_audioOutput) {
        connect(m_audioOutput->q, &QAudioOutput::destroyed, this, &AVFCameraService::audioOutputDestroyed);
        connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this, &AVFCameraService::audioOutputChanged);
        connect(m_audioOutput->q, &QAudioOutput::mutedChanged, this, &AVFCameraService::setAudioOutputMuted);
        connect(m_audioOutput->q, &QAudioOutput::volumeChanged, this, &AVFCameraService::setAudioOutputVolume);
    }
    audioOutputChanged();
}

void AVFCameraService::audioInputChanged()
{
    m_session->updateAudioInput();
}

void AVFCameraService::audioOutputChanged()
{
    m_session->updateAudioOutput();
}

void AVFCameraService::setAudioInputMuted(bool muted)
{
    m_session->setAudioInputMuted(muted);
}

void AVFCameraService::setAudioInputVolume(float volume)
{
    m_session->setAudioInputVolume(volume);
}

void AVFCameraService::setAudioOutputMuted(bool muted)
{
    m_session->setAudioOutputMuted(muted);
}

void AVFCameraService::setAudioOutputVolume(float volume)
{
    m_session->setAudioOutputVolume(volume);
}

void AVFCameraService::setVideoPreview(QVideoSink *sink)
{
    m_session->setVideoSink(sink);
}

#include "moc_avfcameraservice_p.cpp"
