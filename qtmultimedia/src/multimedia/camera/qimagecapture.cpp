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
#include <qimagecapture.h>
#include <private/qplatformimagecapture_p.h>
#include <qmediametadata.h>
#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformmediaformatinfo_p.h>
#include <qmediacapturesession.h>

#include "private/qobject_p.h"
#include <qcamera.h>
#include <private/qplatformcamera_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

/*!
    \class QImageCapture
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera


    \brief The QImageCapture class is used for the recording of media content.

    The QImageCapture class is a high level images recording class.
    It's not intended to be used alone but for accessing the media
    recording functions of other media objects, like QCamera.

    \snippet multimedia-snippets/camera.cpp Camera

    \snippet multimedia-snippets/camera.cpp Camera keys

    \sa QCamera
*/

class QImageCapturePrivate
{
    Q_DECLARE_PUBLIC(QImageCapture)
public:
    QCamera *camera = nullptr;

    QMediaCaptureSession *captureSession = nullptr;
    QPlatformImageCapture *control = nullptr;

    QImageCapture::Error error = QImageCapture::NoError;
    QString errorString;
    QMediaMetaData metaData;

    void _q_error(int id, int error, const QString &errorString);

    void unsetError() { error = QImageCapture::NoError; errorString.clear(); }

    QImageCapture *q_ptr;
};

void QImageCapturePrivate::_q_error(int id, int error, const QString &errorString)
{
    Q_Q(QImageCapture);

    this->error = QImageCapture::Error(error);
    this->errorString = errorString;

    emit q->errorChanged();
    emit q->errorOccurred(id, this->error, errorString);
}

/*!
    Constructs a image capture object, from a \a parent, that can capture
    individual still images produced by a camera.

    You must connect both an image capture object and a QCamera to a capture
    session to capture images.
*/

QImageCapture::QImageCapture(QObject *parent)
    : QObject(parent), d_ptr(new QImageCapturePrivate)
{
    Q_D(QImageCapture);
    d->q_ptr = this;
    d->control = QPlatformMediaIntegration::instance()->createImageCapture(this);

    connect(d->control, SIGNAL(imageExposed(int)),
            this, SIGNAL(imageExposed(int)));
    connect(d->control, SIGNAL(imageCaptured(int,QImage)),
            this, SIGNAL(imageCaptured(int,QImage)));
    connect(d->control, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)),
            this, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)));
    connect(d->control, SIGNAL(imageAvailable(int,QVideoFrame)),
            this, SIGNAL(imageAvailable(int,QVideoFrame)));
    connect(d->control, SIGNAL(imageSaved(int,QString)),
            this, SIGNAL(imageSaved(int,QString)));
    connect(d->control, SIGNAL(readyForCaptureChanged(bool)),
            this, SIGNAL(readyForCaptureChanged(bool)));
    connect(d->control, SIGNAL(error(int,int,QString)),
            this, SLOT(_q_error(int,int,QString)));
}

/*!
    \internal
*/
void QImageCapture::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QImageCapture);
    d->captureSession = session;
}

/*!
    Destroys images capture object.
*/

QImageCapture::~QImageCapture()
{
    if (d_ptr->captureSession)
        d_ptr->captureSession->setImageCapture(nullptr);
    delete d_ptr;
}

/*!
    Returns true if the images capture service ready to use.
*/
bool QImageCapture::isAvailable() const
{
    return d_func()->captureSession && d_func()->captureSession->camera();
}

/*!
    Returns the capture session this camera is connected to, or
    a nullptr if the camera is not connected to a capture session.

    Use QMediaCaptureSession::setImageCapture() to connect the image capture to
    a session.
*/
QMediaCaptureSession *QImageCapture::captureSession() const
{
    return d_ptr->captureSession;
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QImageCapture::Error QImageCapture::error() const
{
    return d_func()->error;
}

/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QImageCapture::errorString() const
{
    return d_func()->errorString;
}

/*!
    \property QImageCapture::metaData
    \brief The meta data that will get embedded into the image.

    \note Additional fields such as a time stamp or location may get added by
     the camera back end.
*/
QMediaMetaData QImageCapture::metaData() const
{
    Q_D(const QImageCapture);
    return d->metaData;
}

/*!
    Replaces any existing meta data, to be embedded into the captured image,
    with a set of \a metaData.
*/
void QImageCapture::setMetaData(const QMediaMetaData &metaData)
{
    Q_D(QImageCapture);
    d->metaData = metaData;
    d->control->setMetaData(d->metaData);
    emit metaDataChanged();
}

/*!
    Adds additional \a metaData to any existing meta data, that is embedded
    into the captured image.
*/
void QImageCapture::addMetaData(const QMediaMetaData &metaData)
{
    Q_D(QImageCapture);
    auto data = d->metaData;
    for (auto k : metaData.keys())
        data.insert(k, metaData.value(k));
    setMetaData(data);
}

/*!
  \property QImageCapture::readyForCapture

  Holds \c true if the camera is ready to capture an image immediately.
  Calling capture() while \c readyForCapture is \c false is not
  permitted and results in an error.
*/
bool QImageCapture::isReadyForCapture() const
{
    Q_D(const QImageCapture);
    if (!d->control || !d->captureSession || !d->control->isReadyForCapture())
        return false;
    auto *camera = d->captureSession->camera();
    if (!camera || !camera->isActive())
        return false;
    return true;
}

/*!
    \fn QImageCapture::readyForCaptureChanged(bool ready)

    Signals that a camera's \a ready for capture state has changed.
*/


/*!
    Capture the image and save it to \a file.
    This operation is asynchronous in majority of cases,
    followed by signals QImageCapture::imageExposed(),
    QImageCapture::imageCaptured(), QImageCapture::imageSaved()
    or QImageCapture::error().

    If an empty \a file is passed, the camera back end chooses
    the default location and naming scheme for photos on the system,
    if only file name without full path is specified, the image will be saved to
    the default directory, with a full path reported with imageCaptured() and imageSaved() signals.

    QCamera saves all the capture parameters like exposure settings or
    image processing parameters, so changes to camera parameters after
    capture() is called do not affect previous capture requests.

    QImageCapture::capture returns the capture Id parameter, used with
    imageExposed(), imageCaptured() and imageSaved() signals.

    \sa isReadyForCapture()
*/
int QImageCapture::captureToFile(const QString &file)
{
    Q_D(QImageCapture);

    d->unsetError();

    if (!d->control) {
        d->_q_error(-1, NotSupportedFeatureError, QPlatformImageCapture::msgCameraNotReady());
        return -1;
    }

    if (!isReadyForCapture()) {
        d->_q_error(-1, NotReadyError, tr("Could not capture in stopped state"));
        return -1;
    }

    return d->control->capture(file);
}

/*!
    Capture the image and make it available as a QImage.
    This operation is asynchronous in majority of cases,
    followed by signals QImageCapture::imageExposed(),
    QImageCapture::imageCaptured()
    or QImageCapture::error().

    QImageCapture::capture returns the capture Id parameter, used with
    imageExposed(), imageCaptured() and imageSaved() signals.

    \sa isReadyForCapture()
*/
int QImageCapture::capture()
{
    Q_D(QImageCapture);

    d->unsetError();

    if (d->control)
        return d->control->captureToBuffer();

    d->error = NotSupportedFeatureError;
    d->errorString = tr("Device does not support images capture.");

    d->_q_error(-1, d->error, d->errorString);

    return -1;
}

/*!
    \enum QImageCapture::Error

    \value NoError         No Errors.
    \value NotReadyError   The service is not ready for capture yet.
    \value ResourceError   Device is not ready or not available.
    \value OutOfSpaceError No space left on device.
    \value NotSupportedFeatureError Device does not support stillimages capture.
    \value FormatError     Current format is not supported.
*/

/*!
    \fn QImageCapture::errorOccurred(int id, QImageCapture::Error error, const QString &errorString);

    Signals that the capture request \a id has failed with an \a error
    and \a errorString description.
*/

/*!
    \fn QImageCapture::imageExposed(int id)

    Signal emitted when the frame with request \a id was exposed.
*/

/*!
    \fn QImageCapture::imageCaptured(int id, const QImage &preview);

    Signal emitted when the frame with request \a id was captured, but not
    processed and saved yet. Frame \a preview can be displayed to user.
*/

/*!
    \fn QImageCapture::imageAvailable(int id, const QVideoFrame &frame)

    Signal emitted when the \a frame with request \a id is available.
*/

/*!
    \fn QImageCapture::imageSaved(int id, const QString &fileName)

    Signal emitted when QImageCapture::CaptureToFile is set and
    the frame with request \a id was saved to \a fileName.
*/

/*!
    \property QImageCapture::fileFormat
    \brief The image format.
*/

QImageCapture::FileFormat QImageCapture::fileFormat() const
{
    Q_D(const QImageCapture);
    if (!d->control)
        return UnspecifiedFormat;
    return d->control->imageSettings().format();
}

/*!
    Sets the image \a format.
*/
void QImageCapture::setFileFormat(QImageCapture::FileFormat format)
{
    Q_D(QImageCapture);
    if (!d->control)
        return;
    auto fmt = d->control->imageSettings();
    if (fmt.format() == format)
        return;
    fmt.setFormat(format);
    d->control->setImageSettings(fmt);
    emit fileFormatChanged();
}

QList<QImageCapture::FileFormat> QImageCapture::supportedFormats()
{
    return QPlatformMediaIntegration::instance()->formatInfo()->imageFormats;
}

QString QImageCapture::fileFormatName(QImageCapture::FileFormat f)
{
    const char *name = nullptr;
    switch (f) {
    case UnspecifiedFormat:
        name = "Unspecified image format";
        break;
    case JPEG:
        name = "JPEG";
        break;
    case PNG:
        name = "PNG";
        break;
    case WebP:
        name = "WebP";
        break;
    case Tiff:
        name = "Tiff";
        break;
    }
    return QString::fromUtf8(name);
}

QString QImageCapture::fileFormatDescription(QImageCapture::FileFormat f)
{
    const char *name = nullptr;
    switch (f) {
    case UnspecifiedFormat:
        name = "Unspecified image format";
        break;
    case JPEG:
        name = "JPEG";
        break;
    case PNG:
        name = "PNG";
        break;
    case WebP:
        name = "WebP";
        break;
    case Tiff:
        name = "Tiff";
        break;
    }
    return QString::fromUtf8(name);
}

/*!
    Returns the resolution of the encoded image.
*/

QSize QImageCapture::resolution() const
{
    Q_D(const QImageCapture);
    if (!d->control)
        return QSize();
    return d->control->imageSettings().resolution();
}

/*!
    Sets the \a resolution of the encoded image.

    An empty QSize indicates the encoder should make an optimal choice based on
    what is available from the image source and the limitations of the codec.
*/
void QImageCapture::setResolution(const QSize &resolution)
{
    Q_D(QImageCapture);
    if (!d->control)
        return;
    auto fmt = d->control->imageSettings();
    if (fmt.resolution() == resolution)
        return;
    fmt.setResolution(resolution);
    d->control->setImageSettings(fmt);
    emit resolutionChanged();
}

/*!
    Sets the \a width and \a height of the resolution of the encoded image.

    \overload
*/
void QImageCapture::setResolution(int width, int height)
{
    setResolution(QSize(width, height));
}

/*!
    \enum QImageCapture::Quality

    Enumerates quality encoding levels.

    \value VeryLowQuality
    \value LowQuality
    \value NormalQuality
    \value HighQuality
    \value VeryHighQuality
*/

/*!
    \property QImageCapture::quality
    \brief The image encoding quality.
*/
QImageCapture::Quality QImageCapture::quality() const
{
    Q_D(const QImageCapture);
    if (!d->control)
        return NormalQuality;
    return d->control->imageSettings().quality();
}

/*!
    Sets the image encoding \a quality.
*/
void QImageCapture::setQuality(Quality quality)
{
    Q_D(QImageCapture);
    if (!d->control)
        return;
    auto fmt = d->control->imageSettings();
    if (fmt.quality() == quality)
        return;
    fmt.setQuality(quality);
    d->control->setImageSettings(fmt);
    emit resolutionChanged();
}

/*!
    \internal
*/
QPlatformImageCapture *QImageCapture::platformImageCapture()
{
    Q_D(QImageCapture);
    return d->control;
}

QT_END_NAMESPACE

#include "moc_qimagecapture.cpp"
