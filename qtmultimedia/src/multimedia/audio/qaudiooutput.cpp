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

#include <qaudiooutput.h>
#include <qaudiodevice.h>
#include <qmediadevices.h>
#include <private/qplatformaudiooutput_p.h>
#include <private/qplatformmediaintegration_p.h>

/*!
    \qmltype AudioOutput
    \instantiates QAudioOutput
    \brief An audio output to be used for playback or monitoring of a capture session.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml

    \qml
    MediaPlayer {
        id: playMusic
        source: "music.wav"
        audioOutput: AudioOutput {
            volume: slider.value
        }
    }
    Slider {
        id: slider
        from: 0.
        to: 1.
    }
    \endqml

    You can use AudioOutput together with a QtMultiMedia::MediaPlayer to play audio content, or you can use it
    in conjunction with a MultiMedia::CaptureSession to monitor the audio processed by the capture session.

    \sa VideoOutput AudioInput
*/

/*!
    \class QAudioOutput
    \brief Represents an output channel for audio.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio
    \since 6.0

    This class represents an output channel that can be used together with
    QMediaPlayer or QMediaCaptureSession. It enables the selection of the
    physical output device to be used, muting the channel, and changing the
    channel's volume.
*/
QAudioOutput::QAudioOutput(QObject *parent)
    : QAudioOutput(QMediaDevices::defaultAudioOutput(), parent)
{}

QAudioOutput::QAudioOutput(const QAudioDevice &device, QObject *parent)
    : QObject(parent),
    d(QPlatformMediaIntegration::instance()->createAudioOutput(this))
{
    d->device = device.mode() == QAudioDevice::Output ? device : QMediaDevices::defaultAudioOutput();
    d->setAudioDevice(d->device);
}

QAudioOutput::~QAudioOutput()
{
    setDisconnectFunction({});
    delete d;
}


/*!
    \qmlproperty real QtMultimedia::AudioOutput::volume

    This property holds the volume of the audio output.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume).
    Values outside this range will be clamped: a value lower than 0.0 is set to
    0.0, a value higher than 1.0 will set to 1.0.

    The default volume is \c{1.0}.

    UI \l{volume controls} should usually be scaled non-linearly. For example,
    using a logarithmic scale will produce linear changes in perceived \l{loudness},
    which is what a user would normally expect from a volume control.

    See \l {QAudio::convertVolume()}{QtMultimedia.convertVolume()}
    for more details.
*/

/*!
    \property QAudioOutput::volume
    \brief The current volume.

    The volume is scaled linearly, ranging from \c 0 (silence) to \c 1
    (full volume).
    \note values outside this range will be clamped.

    By default the volume is \c 1.

    UI volume controls should usually be scaled non-linearly. For example,
    using a logarithmic scale will produce linear changes in perceived loudness,
    which is what a user would normally expect from a volume control.

    \sa QAudio::convertVolume()
*/
float QAudioOutput::volume() const
{
    return d->volume;
}

void QAudioOutput::setVolume(float volume)
{
    volume = qBound(0., volume, 1.);
    if (d->volume == volume)
        return;
    d->volume = volume;
    d->setVolume(volume);
    emit volumeChanged(volume);
}

/*!
    \qmlproperty bool QtMultimedia::AudioOutput::muted

    This property holds whether the audio output is muted.

    Defaults to \c{false}.
*/

/*!
    \property QAudioOutput::muted
    \brief The muted state of the current media.

    The value will be \c true if the output is muted; otherwise \c{false}.
*/
bool QAudioOutput::isMuted() const
{
    return d->muted;
}

void QAudioOutput::setMuted(bool muted)
{
    if (d->muted == muted)
        return;
    d->muted = muted;
    d->setMuted(muted);
    emit mutedChanged(muted);
}

/*!
    \qmlproperty AudioDevice QtMultimedia::AudioOutput::device

    This property describes the audio device connected to this output.

    The device property represents the audio device this output is connected to.
    This property can be used to select an output device from the
    QtMultimedia::MediaDevices::audioOutputs() list.
*/

/*!
    \property QAudioOutput::device
    \brief The audio device connected to this output.

    The device property represents the audio device this output is connected to.
    This property can be used to select an output device from the
    QMediaDevices::audioOutputs() list.
    You can select the system default audio output by setting this property to
    a default constructed QAudioDevice object.
*/
QAudioDevice QAudioOutput::device() const
{
    return d->device;
}

void QAudioOutput::setDevice(const QAudioDevice &device)
{
    auto dev = device;
    if (dev.isNull())
        dev = QMediaDevices::defaultAudioOutput();
    if (dev.mode() != QAudioDevice::Output)
        return;
    if (d->device == dev)
        return;
    d->device = dev;
    d->setAudioDevice(dev);
    emit deviceChanged();
}

/*!
    \internal
*/
void QAudioOutput::setDisconnectFunction(std::function<void()> disconnectFunction)
{
    if (d->disconnectFunction) {
        auto df = d->disconnectFunction;
        d->disconnectFunction = {};
        df();
    }
    d->disconnectFunction = std::move(disconnectFunction);
}


#include "moc_qaudiooutput.cpp"
