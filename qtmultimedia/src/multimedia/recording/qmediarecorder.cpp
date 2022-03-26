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

#include "qmediarecorder_p.h"

#include <private/qplatformmediarecorder_p.h>
#include <qaudiodevice.h>
#include <qcamera.h>
#include <qmediacapturesession.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformmediacapture_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>

#include <qaudioformat.h>

QT_BEGIN_NAMESPACE

/*!
    \class QMediaRecorder
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_recording
    \ingroup multimedia_video
    \ingroup multimedia_audio

    \brief The QMediaRecorder class is used for encoding and recording a capture session.

    The QMediaRecorder class is a class for encoding and recording media generated in a
    QMediaCaptureSession.

    \snippet multimedia-snippets/media.cpp Media recorder
*/
/*!
    \qmltype MediaRecorder
    \instantiates QMediaRecorder
    \brief For encoding and recording media generated in a CaptureSession.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml

    The MediaRecorder element can be used within a CaptureSession to record and encode audio and
    video captured from a microphone and camera

    \since 6.2
    The code below shows a simple capture session containing a MediaRecorder using the default
    camera and default audio input.

\qml
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
        }
        audioInput: AudioInput {}
        recorder: MediaRecorder {
            id: recorder
        }
    }
\endqml

    The code below shows how the recording can be started and stopped.
\qml
    CameraButton {
        text: "Record"
        visible: recorder.status !== MediaRecorder.RecordingStatus
        onClicked: recorder.record()
    }

    CameraButton {
        id: stopButton
        text: "Stop"
        visible: recorder.status === MediaRecorder.RecordingStatus
        onClicked: recorder.stop()
    }
\endqml

    \sa CaptureSession, Camera, AudioInput, ImageCapture
*/
QMediaRecorderPrivate::QMediaRecorderPrivate()
{
    // Force an early initialization of the mime database
    // to avoid a delay when recording for the first time.
    encoderSettings.mimeType();
}

QString QMediaRecorderPrivate::msgFailedStartRecording()
{
    return QMediaRecorder::tr("Failed to start recording");
}

/*!
    Constructs a media recorder which records the media produced by a microphone and camera.
    The media recorder is a child of \a{parent}.
*/

QMediaRecorder::QMediaRecorder(QObject *parent)
    : QObject(parent),
      d_ptr(new QMediaRecorderPrivate)
{
    Q_D(QMediaRecorder);
    d->q_ptr = this;
    d->control = QPlatformMediaIntegration::instance()->createRecorder(this);
}

/*!
    Destroys a media recorder object.
*/

QMediaRecorder::~QMediaRecorder()
{
    if (d_ptr->captureSession)
        d_ptr->captureSession->setRecorder(nullptr);
    delete d_ptr->control;
    delete d_ptr;
}

/*!
    \internal
*/
QPlatformMediaRecorder *QMediaRecorder::platformRecoder() const
{
    return d_ptr->control;
}

/*!
    \internal
*/
void QMediaRecorder::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QMediaRecorder);
    d->captureSession = session;
}
/*!
    \qmlproperty QUrl QtMultimedia::MediaRecorder::outputLocation
    \brief The destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the operation
    fails an errorOccured() signal is emitted.

    The location can be relative or empty. If empty the recorder uses the
    system specific place and file naming scheme.

   \sa errorOccurred()
*/

/*!
    \property QMediaRecorder::outputLocation
    \brief the destination location of media content.

    Setting the location can fail, for example when the service supports only
    local file system locations but a network URL was passed. If the operation
    fails an errorOccured() signal is emitted.

    The output location can be relative or empty; in the latter case the recorder
    uses the system specific place and file naming scheme.
*/

/*!
    \qmlproperty QUrl QtMultimedia::MediaRecorder::actualLocation
    \brief The actual location of the last media content.

    The actual location is usually available after recording starts,
    and reset when new location is set or new recording starts.
*/

/*!
    \property QMediaRecorder::actualLocation
    \brief The actual location of the last media content.

    The actual location is usually available after recording starts,
    and reset when new location is set or new recording starts.
*/

/*!
    \qmlproperty bool QtMultimedia::MediaRecorder::isAvailable
    \brief This property holds whether the recorder service is ready to use.

    Returns \c true if media recorder service ready to use.
*/
/*!
    Returns \c true if media recorder service ready to use.
*/
bool QMediaRecorder::isAvailable() const
{
    return d_func()->control != nullptr && d_func()->captureSession;
}

QUrl QMediaRecorder::outputLocation() const
{
    return d_func()->control ? d_func()->control->outputLocation() : QUrl();
}

void QMediaRecorder::setOutputLocation(const QUrl &location)
{
    Q_D(QMediaRecorder);
    if (!d->control) {
        emit errorOccurred(QMediaRecorder::ResourceError, tr("Not available"));
        return;
    }
    d->control->setOutputLocation(location);
    d->control->clearActualLocation();
    if (!location.isEmpty() && !d->control->isLocationWritable(location))
            emit errorOccurred(QMediaRecorder::LocationNotWritable, tr("Output location not writable"));
}

QUrl QMediaRecorder::actualLocation() const
{
    Q_D(const QMediaRecorder);
    return d->control ? d->control->actualLocation() : QUrl();
}

/*!
    Returns the current media recorder state.

    \sa QMediaRecorder::RecorderState
*/

QMediaRecorder::RecorderState QMediaRecorder::recorderState() const
{
    return d_func()->control ? QMediaRecorder::RecorderState(d_func()->control->state()) : StoppedState;
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QMediaRecorder::Error QMediaRecorder::error() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->error() : QMediaRecorder::ResourceError;
}
/*!
    \qmlproperty string QtMultimedia::MediaRecorder::errorString
    \brief This property holds a string describing the current error state.

    \sa error
*/
/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QMediaRecorder::errorString() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->errorString() : tr("QMediaRecorder not supported on this platform");
}
/*!
    \qmlproperty qint64 QtMultimedia::MediaRecorder::duration

    \brief This property holds the recorded media duration in milliseconds.
*/

/*!
    \property QMediaRecorder::duration

    \brief the recorded media duration in milliseconds.
*/

qint64 QMediaRecorder::duration() const
{
    return d_func()->control ? d_func()->control->duration() : 0;
}
/*!
    \qmlmethod QtMultimedia::MediaRecorder::record()
    \brief Starts recording.

    While the recorder state is changed immediately to
    \c MediaRecorder.RecordingState, recording may start asynchronously.

    If recording fails, the error() signal is emitted with recorder state being
    reset back to \c{QMediaRecorder.StoppedState}.

    \note On mobile devices, recording will happen in the orientation the
    device had when calling record and is locked for the duration of the recording.
    To avoid artifacts on the user interface, we recommend to keep the user interface
    locked to the same orientation as long as the recording is ongoing using
    the contentOrientation property of the Window and unlock it again once the recording
    is finished.
*/
/*!
    Start recording.

    While the recorder state is changed immediately to
    c\{QMediaRecorder::RecordingState}, recording may start asynchronously.

    If recording fails error() signal is emitted with recorder state being
    reset back to \c{QMediaRecorder::StoppedState}.

    \note On mobile devices, recording will happen in the orientation the
    device had when calling record and is locked for the duration of the recording.
    To avoid artifacts on the user interface, we recommend to keep the user interface
    locked to the same orientation as long as the recording is ongoing using
    the contentOrientation property of QWindow and unlock it again once the recording
    is finished.
*/

void QMediaRecorder::record()
{
    Q_D(QMediaRecorder);

    if (!d->control || ! d->captureSession)
        return;

    if (d->control->state() == QMediaRecorder::PausedState) {
        d->control->resume();
    } else {
        auto oldMediaFormat = d->encoderSettings.mediaFormat();
        auto camera = d->captureSession->camera();
        auto flags = camera && camera->isActive() ? QMediaFormat::RequiresVideo
                                                  : QMediaFormat::NoFlags;
        d->encoderSettings.resolveFormat(flags);
        d->control->clearActualLocation();
        d->control->clearError();

        auto settings = d->encoderSettings;
        d->control->record(d->encoderSettings);

        if (settings != d->encoderSettings)
            emit encoderSettingsChanged();

        if (oldMediaFormat != d->encoderSettings.mediaFormat())
            emit mediaFormatChanged();
    }
}
/*!
    \qmlmethod QtMultimedia::MediaRecorder::pause()
    \brief Pauses recording.

    The recorder state is changed to QMediaRecorder.PausedState.

    Depending on the platform, pausing recording may be not supported.
    In this case the recorder state is unchanged.
*/
/*!
    Pauses recording.

    The recorder state is changed to QMediaRecorder::PausedState.

    Depending on the platform, pausing recording may be not supported.
    In this case the recorder state is unchanged.
*/

void QMediaRecorder::pause()
{
    Q_D(QMediaRecorder);
    if (d->control && d->captureSession)
        d->control->pause();
}
/*!
    \qmlmethod QtMultimedia::MediaRecorder::stop()
    \brief Stops recording.

    The recorder state is changed to \c{QMediaRecorder.StoppedState}.
*/

/*!
    Stops recording.

    The recorder state is changed to QMediaRecorder::StoppedState.
*/

void QMediaRecorder::stop()
{
    Q_D(QMediaRecorder);
    if (d->control && d->captureSession)
        d->control->stop();
}
/*!
    \qmlproperty enumeration QtMultimedia::MediaRecorder::recorderState
    \brief This property holds the current media recorder state.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    RecorderSstate may also change asynchronously when recording fails.

    \value MediaRecorder.StoppedState The recorder is not active.
    \value MediaRecorder.RecordingState The recording is requested.
    \value MediaRecorder.PausedState The recorder is pause.
*/
/*!
    \enum QMediaRecorder::RecorderState

    \value StoppedState    The recorder is not active.
    \value RecordingState  The recording is requested.
    \value PausedState     The recorder is paused.
*/
/*!
    \qmlproperty enumeration QtMultimedia::MediaRecorder::error
    \brief This property holds the current media recorder error state.

    \value MediaRecorder.NoError Not in an error state.
    \value MediaRecorder.ResourceError Not enough system resources
    \value MediaRecorder.FormatError the current format is not supported.
    \value MediaRecorder.OutOfSpaceError No space left on device.
    \value MediaRecorder.LocationNotWriteable The output location is not writable.
*/
/*!
    \enum QMediaRecorder::Error

    \value NoError         No Errors.
    \value ResourceError   Device is not ready or not available.
    \value FormatError     Current format is not supported.
    \value OutOfSpaceError No space left on device.
    \value LocationNotWritable The output location is not writable.
*/

/*!
    \property QMediaRecorder::recorderState
    \brief The current state of the media recorder.

    The state property represents the user request and is changed synchronously
    during record(), pause() or stop() calls.
    Recorder state may also change asynchronously when recording fails.
*/

/*!
    \qmlsignal QtMultimedia::MediaRecorder::recorderStateChanged(RecorderState state)
    \brief Signals that a media recorder's \a state has changed.
*/

/*!
    \fn QMediaRecorder::recorderStateChanged(QMediaRecorder::RecorderState state)

    Signals that a media recorder's \a state has changed.
*/

/*!
    \qmlsignal QtMultimedia::MediaRecorder::durationChanged(qint64 duration)
    \brief Signals that the \a duration of the recorded media has changed.
*/

/*!
    \fn QMediaRecorder::durationChanged(qint64 duration)

    Signals that the \a duration of the recorded media has changed.
*/
/*!
    \qmlsignal QtMultimedia::MediaRecorder::actualLocationChanged(const QUrl &location)
    \brief Signals that the actual \a location of the recorded media has changed.

    This signal is usually emitted when recording starts.
*/
/*!
    \fn QMediaRecorder::actualLocationChanged(const QUrl &location)

    Signals that the actual \a location of the recorded media has changed.
    This signal is usually emitted when recording starts.
*/
/*!
    \qmlsignal QtMultimedia::MediaRecorder::errorOccurred(Error error, const QString &errorString)
    \brief Signals that an \a error has occurred.

    The \a errorString contains a description of the error.
*/
/*!
    \fn QMediaRecorder::errorOccurred(QMediaRecorder::Error error, const QString &errorString)

    Signals that an \a error has occurred, with \a errorString containing
    a description of the error.
*/

/*!
    \qmlproperty mediaMetaData QtMultimedia::MediaRecorder::metaData

    \brief This property holds meta data associated with the recording.

    When a recording is started, any meta-data assigned will be attached to  that
    recording.

    \note Ensure that meta-data is assigned correctly by assigning it before
    starting the recording.

    \sa mediaMetaData
*/

/*!
    Returns the metaData associated with the recording.
*/
QMediaMetaData QMediaRecorder::metaData() const
{
    Q_D(const QMediaRecorder);

    return d->control ? d->control->metaData() : QMediaMetaData{};
}

/*!
    Sets the meta data to \a metaData.

    \note To ensure that meta-data is set correctly, it should be set before starting the recording.
    Once the recording is started, any meta-data set will be attached to the next recording.
*/
void QMediaRecorder::setMetaData(const QMediaMetaData &metaData)
{
    Q_D(QMediaRecorder);

    if (d->control && d->captureSession)
        d->control->setMetaData(metaData);
}

void QMediaRecorder::addMetaData(const QMediaMetaData &metaData)
{
    auto data = this->metaData();
    // merge data
    for (const auto &k : metaData.keys())
        data.insert(k, metaData.value(k));
    setMetaData(data);
}
/*!
    \qmlsignal QtMultimedia::MediaRecorder::metaDataChanged()

    \brief Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed metaDataChanged() is emitted
    once.
*/
/*!
    \fn QMediaRecorder::metaDataChanged()

    Signals that a media object's meta-data has changed.

    If multiple meta-data elements are changed metaDataChanged() is emitted
    once.
*/

QMediaCaptureSession *QMediaRecorder::captureSession() const
{
    Q_D(const QMediaRecorder);
    return d->captureSession;
}
/*!
    \qmlproperty enumeration QtMultimedia::MediaRecorder::quality

    Enumerates quality encoding levels.

    \value MediaRecorder.VeryLowQuality
    \value MediaRecorder.LowQuality
    \value MediaRecorder.NormalQuality
    \value MediaRecorder.HighQuality
    \value MediaRecorder.VeryHighQuality
*/
/*!
    \enum QMediaRecorder::Quality

    Enumerates quality encoding levels.

    \value VeryLowQuality
    \value LowQuality
    \value NormalQuality
    \value HighQuality
    \value VeryHighQuality
*/

/*!
    \enum QMediaRecorder::EncodingMode

    Enumerates encoding modes.

    \value ConstantQualityEncoding Encoding will aim to have a constant quality, adjusting bitrate to fit.
    \value ConstantBitRateEncoding Encoding will use a constant bit rate, adjust quality to fit.
    \value AverageBitRateEncoding Encoding will try to keep an average bitrate setting, but will use
           more or less as needed.
    \value TwoPassEncoding The media will first be processed to determine the characteristics,
           and then processed a second time allocating more bits to the areas
           that need it.
*/

/*!

    \qmlproperty MediaFormat QtMultimedia::MediaRecorder::mediaFormat

    \brief This property holds the current MediaFormat of the recorder.
*/

QMediaFormat QMediaRecorder::mediaFormat() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.mediaFormat();
}

void QMediaRecorder::setMediaFormat(const QMediaFormat &format)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.mediaFormat() == format)
        return;
    d->encoderSettings.setMediaFormat(format);
    emit mediaFormatChanged();
}

/*!
    Returns the encoding mode.

    \sa EncodingMode
*/
QMediaRecorder::EncodingMode QMediaRecorder::encodingMode() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.encodingMode();
}

/*!
    Sets the encoding \a mode setting.

    If ConstantQualityEncoding is set, the quality
    encoding parameter is used and bit rates are ignored,
    otherwise the bitrates are used.

    \sa encodingMode(), EncodingMode
*/
void QMediaRecorder::setEncodingMode(EncodingMode mode)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.encodingMode() == mode)
        return;
    d->encoderSettings.setEncodingMode(mode);
    emit encodingModeChanged();
}

QMediaRecorder::Quality QMediaRecorder::quality() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.quality();
}

void QMediaRecorder::setQuality(Quality quality)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.quality() == quality)
        return;
    d->encoderSettings.setQuality(quality);
    emit qualityChanged();
}


/*!
    Returns the resolution of the encoded video.
*/
QSize QMediaRecorder::videoResolution() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoResolution();
}

/*!
    Sets the resolution of the encoded video to \a{size}.

    Pass an empty QSize to make the recorder choose an optimal resolution based
    on what is available from the video source and the limitations of the codec.
*/
void QMediaRecorder::setVideoResolution(const QSize &size)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoResolution() == size)
        return;
    d->encoderSettings.setVideoResolution(size);
    emit videoResolutionChanged();
}

/*! \fn void QMediaRecorder::setVideoResolution(int width, int height)

    Sets the \a width and \a height of the resolution of the encoded video.

    \overload
*/

/*!
    Returns the video frame rate.
*/
qreal QMediaRecorder::videoFrameRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoFrameRate();
}

/*!
    Sets the video \a frameRate.

    A value of 0 indicates the recorder should make an optimal choice based on what is available
    from the video source and the limitations of the codec.
*/
void QMediaRecorder::setVideoFrameRate(qreal frameRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoFrameRate() == frameRate)
        return;
    d->encoderSettings.setVideoFrameRate(frameRate);
    emit videoFrameRateChanged();
}

/*!
    Returns the bit rate of the compressed video stream in bits per second.
*/
int QMediaRecorder::videoBitRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.videoBitRate();
}

/*!
    Sets the video \a bitRate in bits per second.
*/
void QMediaRecorder::setVideoBitRate(int bitRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.videoBitRate() == bitRate)
        return;
    d->encoderSettings.setVideoBitRate(bitRate);
    emit videoBitRateChanged();
}

/*!
    Returns the bit rate of the compressed audio stream in bits per second.
*/
int QMediaRecorder::audioBitRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioBitRate();
}

/*!
    Sets the audio \a bitRate in bits per second.
*/
void QMediaRecorder::setAudioBitRate(int bitRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioBitRate() == bitRate)
        return;
    d->encoderSettings.setAudioBitRate(bitRate);
    emit audioBitRateChanged();
}

/*!
    Returns the number of audio channels.
*/
int QMediaRecorder::audioChannelCount() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioChannelCount();
}

/*!
    Sets the number of audio \a channels.

    A value of -1 indicates the recorder should make an optimal choice based on
    what is available from the audio source and the limitations of the codec.
*/
void QMediaRecorder::setAudioChannelCount(int channels)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioChannelCount() == channels)
        return;
    d->encoderSettings.setAudioChannelCount(channels);
    emit audioChannelCountChanged();
}

/*!
    Returns the audio sample rate in Hz.
*/
int QMediaRecorder::audioSampleRate() const
{
    Q_D(const QMediaRecorder);
    return d->encoderSettings.audioSampleRate();
}

/*!
    Sets the audio \a sampleRate in Hz.

    A value of \c -1 indicates the recorder should make an optimal choice based
    on what is available from the audio source, and the limitations of the codec.
*/
void QMediaRecorder::setAudioSampleRate(int sampleRate)
{
    Q_D(QMediaRecorder);
    if (d->encoderSettings.audioSampleRate() == sampleRate)
        return;
    d->encoderSettings.setAudioSampleRate(sampleRate);
    emit audioSampleRateChanged();
}

QT_END_NAMESPACE

#include "moc_qmediarecorder.cpp"
