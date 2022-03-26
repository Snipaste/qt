/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
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

#ifndef QANDROIDCAMERASESSION_H
#define QANDROIDCAMERASESSION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcamera.h>
#include <QImageCapture>
#include <QSet>
#include <QMutex>
#include <private/qplatformimagecapture_p.h>
#include "androidcamera_p.h"

QT_BEGIN_NAMESPACE

class QAndroidVideoOutput;
class QAndroidTextureVideoOutput ;
class QVideoSink;

class QAndroidCameraSession : public QObject
{
    Q_OBJECT
public:
    explicit QAndroidCameraSession(QObject *parent = 0);
    ~QAndroidCameraSession();

    static const QList<QCameraDevice> &availableCameras();

    void setSelectedCameraId(int cameraId) { m_selectedCamera = cameraId; }
    int getSelectedCameraId() { return m_selectedCamera; }
    AndroidCamera *camera() const { return m_camera; }

    bool isActive() const { return m_active; }
    void setActive(bool active);

    void applyResolution(const QSize &captureSize = QSize(), bool restartPreview = true);

    QAndroidVideoOutput *videoOutput() const { return m_videoOutput; }
    void setVideoOutput(QAndroidVideoOutput *output);

    void setCameraFormat(const QCameraFormat &format);

    QList<QSize> getSupportedPreviewSizes() const;
    QList<QVideoFrameFormat::PixelFormat> getSupportedPixelFormats() const;
    QList<AndroidCamera::FpsRange> getSupportedPreviewFpsRange() const;

    QImageEncoderSettings imageSettings() const { return m_actualImageSettings; }
    void setImageSettings(const QImageEncoderSettings &settings);

    bool isReadyForCapture() const;
    void setReadyForCapture(bool ready);
    int capture(const QString &fileName);
    int captureToBuffer();

    int currentCameraRotation() const;

    void setPreviewFormat(AndroidCamera::ImageFormat format);

    struct PreviewCallback
    {
        virtual void onFrameAvailable(const QVideoFrame &frame) = 0;
    };
    void setPreviewCallback(PreviewCallback *callback);

    void setVideoSink(QVideoSink *surface);

    void disableRotation();
    void enableRotation();

    void setKeepAlive(bool keepAlive);

Q_SIGNALS:
    void activeChanged(bool);
    void error(int error, const QString &errorString);
    void opened();

    void readyForCaptureChanged(bool);
    void imageExposed(int id);
    void imageCaptured(int id, const QImage &preview);
    void imageMetadataAvailable(int id, const QMediaMetaData &key);
    void imageAvailable(int id, const QVideoFrame &buffer);
    void imageSaved(int id, const QString &fileName);
    void imageCaptureError(int id, int error, const QString &errorString);

private Q_SLOTS:
    void onVideoOutputReady(bool ready);
    void updateOrientation();

    void onApplicationStateChanged();

    void onCameraTakePictureFailed();
    void onCameraPictureExposed();
    void onCameraPictureCaptured(const QVideoFrame &frame);
    void onLastPreviewFrameFetched(const QVideoFrame &frame);
    void onNewPreviewFrame(const QVideoFrame &frame);
    void onCameraPreviewStarted();
    void onCameraPreviewFailedToStart();
    void onCameraPreviewStopped();

private:
    static void updateAvailableCameras();

    bool open();
    void close();

    bool startPreview();
    void stopPreview();

    void applyImageSettings();

    void processPreviewImage(int id, const QVideoFrame &frame, int rotation);
    void processCapturedImage(int id, const QVideoFrame &frame, bool captureToBuffer,
                              const QString &fileName);

    void setActiveHelper(bool active);

    int captureImage();

    QSize getDefaultResolution() const;

    int m_selectedCamera;
    AndroidCamera *m_camera;
    QAndroidVideoOutput *m_videoOutput;

    bool m_active = false;
    bool m_isStateSaved = false;
    bool m_savedState = false;
    bool m_previewStarted;

    bool m_rotationEnabled = false;

    QVideoSink *m_sink = nullptr;
    QAndroidTextureVideoOutput *m_textureOutput = nullptr;

    QImageEncoderSettings m_requestedImageSettings;
    QImageEncoderSettings m_actualImageSettings;
    AndroidCamera::FpsRange m_requestedFpsRange;
    AndroidCamera::ImageFormat m_requestedPixelFromat;

    bool m_readyForCapture;
    int m_currentImageCaptureId;
    QString m_currentImageCaptureFileName;
    bool m_imageCaptureToBuffer;

    QMutex m_videoFrameCallbackMutex;
    PreviewCallback *m_previewCallback;
    bool m_keepActive;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERASESSION_H
