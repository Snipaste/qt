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

#include "qwindowsformatinfo_p.h"

#include <mfapi.h>
#include <mftransform.h>
#include <private/qwindowsiupointer_p.h>
#include <private/qwindowsmultimediautils_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtGui/qimagewriter.h>

QT_BEGIN_NAMESPACE

template<typename T>
static T codecForFormat(GUID format) = delete;

template<>
QMediaFormat::AudioCodec codecForFormat(GUID format)
{
    return QWindowsMultimediaUtils::codecForAudioFormat(format);
}

template<>
QMediaFormat::VideoCodec codecForFormat(GUID format)
{
    return QWindowsMultimediaUtils::codecForVideoFormat(format);
}

template<typename T>
static QSet<T> getCodecSet(GUID category)
{
    QSet<T> codecSet;
    IMFActivate **activateArray = nullptr;
    UINT32 num = 0;

    HRESULT hr = MFTEnumEx(category, MFT_ENUM_FLAG_ALL, nullptr, nullptr, &activateArray, &num);

    if (SUCCEEDED(hr)) {
        for (UINT32 i = 0; i < num; ++i) {
            QWindowsIUPointer<IMFTransform> transform;
            UINT32 typeIndex = 0;

            hr = activateArray[i]->ActivateObject(IID_PPV_ARGS(transform.address()));

            while (SUCCEEDED(hr)) {
                QWindowsIUPointer<IMFMediaType> mediaType;

                if (category == MFT_CATEGORY_AUDIO_ENCODER || category == MFT_CATEGORY_VIDEO_ENCODER)
                    hr = transform->GetOutputAvailableType(0, typeIndex++, mediaType.address());
                else
                    hr = transform->GetInputAvailableType(0, typeIndex++, mediaType.address());

                if (SUCCEEDED(hr)) {
                    GUID subtype = GUID_NULL;
                    hr = mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
                    if (SUCCEEDED(hr))
                        codecSet.insert(codecForFormat<T>(subtype));
                }
            }
        }

        for (UINT32 i = 0; i < num; ++i)
            activateArray[i]->Release();

        CoTaskMemFree(activateArray);
    }

    return codecSet;
}

static QList<QImageCapture::FileFormat> getImageFormatList()
{
    QList<QImageCapture::FileFormat> list;
    auto formats = QImageWriter::supportedImageFormats();

    for (const auto &f : formats) {
        auto format = QString::fromUtf8(f);
        if (format.compare(QLatin1String("jpg"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::JPEG);
        else if (format.compare(QLatin1String("png"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::PNG);
        else if (format.compare(QLatin1String("webp"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::WebP);
        else if (format.compare(QLatin1String("tiff"), Qt::CaseInsensitive) == 0)
            list.append(QImageCapture::FileFormat::Tiff);
    }

    return list;
}

QWindowsFormatInfo::QWindowsFormatInfo()
{
    const QList<CodecMap> containerTable = {
        { QMediaFormat::MPEG4,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3 },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::Matroska,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3, QMediaFormat::AudioCodec::FLAC, QMediaFormat::AudioCodec::Vorbis, QMediaFormat::AudioCodec::Opus },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::VP8, QMediaFormat::VideoCodec::VP9, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::WebM,
          { QMediaFormat::AudioCodec::Vorbis, QMediaFormat::AudioCodec::Opus },
          { QMediaFormat::VideoCodec::VP8, QMediaFormat::VideoCodec::VP9 } },
        { QMediaFormat::QuickTime,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3 },
          { QMediaFormat::VideoCodec::H264, QMediaFormat::VideoCodec::H265, QMediaFormat::VideoCodec::MotionJPEG } },
        { QMediaFormat::AAC,
          { QMediaFormat::AudioCodec::AAC },
          {} },
        { QMediaFormat::MP3,
          { QMediaFormat::AudioCodec::MP3 },
          {} },
        { QMediaFormat::FLAC,
          { QMediaFormat::AudioCodec::FLAC },
          {} },
        { QMediaFormat::Mpeg4Audio,
          { QMediaFormat::AudioCodec::AAC, QMediaFormat::AudioCodec::MP3, QMediaFormat::AudioCodec::ALAC, QMediaFormat::AudioCodec::AC3, QMediaFormat::AudioCodec::EAC3 },
          {} },
        { QMediaFormat::WMA,
          { QMediaFormat::AudioCodec::WMA },
          {} },
        { QMediaFormat::WMV,
          { QMediaFormat::AudioCodec::WMA },
          { QMediaFormat::VideoCodec::WMV } }
    };

    const QSet<QMediaFormat::FileFormat> decoderFormats = {
        QMediaFormat::MPEG4,
        QMediaFormat::Matroska,
        QMediaFormat::WebM,
        QMediaFormat::QuickTime,
        QMediaFormat::AAC,
        QMediaFormat::MP3,
        QMediaFormat::FLAC,
        QMediaFormat::Mpeg4Audio,
        QMediaFormat::WMA,
        QMediaFormat::WMV,
    };

    const QSet<QMediaFormat::FileFormat> encoderFormats = {
        QMediaFormat::MPEG4,
        QMediaFormat::AAC,
        QMediaFormat::MP3,
        QMediaFormat::FLAC,
        QMediaFormat::Mpeg4Audio,
        QMediaFormat::WMA,
        QMediaFormat::WMV,
    };

    const auto audioDecoders = getCodecSet<QMediaFormat::AudioCodec>(MFT_CATEGORY_AUDIO_DECODER);
    const auto audioEncoders = getCodecSet<QMediaFormat::AudioCodec>(MFT_CATEGORY_AUDIO_ENCODER);
    const auto videoDecoders = getCodecSet<QMediaFormat::VideoCodec>(MFT_CATEGORY_VIDEO_DECODER);
    const auto videoEncoders = getCodecSet<QMediaFormat::VideoCodec>(MFT_CATEGORY_VIDEO_ENCODER);

    for (const auto &codecMap : containerTable) {

        const QSet<QMediaFormat::AudioCodec> mapAudioSet(codecMap.audio.cbegin(), codecMap.audio.cend());
        const QSet<QMediaFormat::VideoCodec> mapVideoSet(codecMap.video.cbegin(), codecMap.video.cend());

        if (decoderFormats.contains(codecMap.format)) {
            CodecMap m;
            m.format = codecMap.format;
            m.audio = (audioDecoders & mapAudioSet).values();
            m.video = (videoDecoders & mapVideoSet).values();
            if (!m.video.empty() || !m.audio.empty())
                decoders.append(m);
        }

        if (encoderFormats.contains(codecMap.format)) {
            CodecMap m;
            m.format = codecMap.format;
            m.audio = (audioEncoders & mapAudioSet).values();
            m.video = (videoEncoders & mapVideoSet).values();
            if (!m.video.empty() || !m.audio.empty())
                encoders.append(m);
        }
    }

    imageFormats = getImageFormatList();
}

QWindowsFormatInfo::~QWindowsFormatInfo()
{
}

QT_END_NAMESPACE
