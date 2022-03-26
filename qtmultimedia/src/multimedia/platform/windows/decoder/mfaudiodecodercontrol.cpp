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

#include <system_error>
#include <mferror.h>
#include <qglobal.h>
#include "Wmcodecdsp.h"
#include "mfaudiodecodercontrol_p.h"
#include <private/qwindowsaudioutils_p.h>

MFAudioDecoderControl::MFAudioDecoderControl(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
    , m_sourceResolver(new SourceResolver)
{
    connect(m_sourceResolver, SIGNAL(mediaSourceReady()), this, SLOT(handleMediaSourceReady()));
    connect(m_sourceResolver, SIGNAL(error(long)), this, SLOT(handleMediaSourceError(long)));
}

MFAudioDecoderControl::~MFAudioDecoderControl()
{
    m_sourceResolver->shutdown();
    m_sourceResolver->Release();
}

void MFAudioDecoderControl::setSource(const QUrl &fileName)
{
    if (!m_device && m_source == fileName)
        return;
    stop();
    m_sourceResolver->cancel();
    m_sourceResolver->shutdown();
    m_device = nullptr;
    m_source = fileName;
    sourceChanged();

    if (!m_source.isEmpty()) {
        m_sourceResolver->load(m_source, 0);
        m_loadingSource = true;
    }
}

void MFAudioDecoderControl::setSourceDevice(QIODevice *device)
{
    if (m_device == device && m_source.isEmpty())
        return;
    stop();
    m_sourceResolver->cancel();
    m_sourceResolver->shutdown();
    m_source.clear();
    m_device = device;
    sourceChanged();

    if (m_device) {
        if (m_device->isOpen() && m_device->isReadable()) {
            m_sourceResolver->load(QUrl(), m_device);
            m_loadingSource = true;
        }
    }
}

void MFAudioDecoderControl::handleMediaSourceReady()
{
    m_loadingSource = false;
    if (m_deferredStart) {
        m_deferredStart = false;
        startReadingSource(m_sourceResolver->mediaSource());
    }
}

void MFAudioDecoderControl::handleMediaSourceError(long hr)
{
    m_loadingSource = false;
    m_deferredStart = false;
    if (hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE) {
        error(QAudioDecoder::FormatError, tr("Unsupported media type"));
    } else if (hr == ERROR_FILE_NOT_FOUND) {
        error(QAudioDecoder::ResourceError, tr("Media not found"));
    } else {
        error(QAudioDecoder::ResourceError, tr("Unable to load specified URL")
                      + QString::fromStdString(std::system_category().message(hr)));
    }
}

void MFAudioDecoderControl::startReadingSource(IMFMediaSource *source)
{
    Q_ASSERT(source);

    m_decoderSourceReader.reset(new MFDecoderSourceReader());
    if (!m_decoderSourceReader) {
        error(QAudioDecoder::ResourceError, tr("Could not instantiate MFDecoderSourceReader"));
        return;
    }

    auto mediaType = m_decoderSourceReader->setSource(source, m_outputFormat.sampleFormat());
    QAudioFormat mediaFormat = QWindowsAudioUtils::mediaTypeToFormat(mediaType.get());
    if (!mediaFormat.isValid()) {
        error(QAudioDecoder::FormatError, tr("Invalid media format"));
        m_decoderSourceReader.reset();
        return;
    }

    QWindowsIUPointer<IMFPresentationDescriptor> pd;
    if (SUCCEEDED(source->CreatePresentationDescriptor(pd.address()))) {
        UINT64 duration = 0;
        pd->GetUINT64(MF_PD_DURATION, &duration);
        duration /= 10000;
        m_duration = qint64(duration);
        durationChanged(m_duration);
    }

    if (!m_resampler.setup(mediaFormat, m_outputFormat.isValid() ? m_outputFormat : mediaFormat)) {
        qWarning() << "Failed to setup resampler";
        return;
    }

    connect(m_decoderSourceReader.get(), SIGNAL(finished()), this, SLOT(handleSourceFinished()));
    connect(m_decoderSourceReader.get(), SIGNAL(newSample(QWindowsIUPointer<IMFSample>)), this, SLOT(handleNewSample(QWindowsIUPointer<IMFSample>)));

    setIsDecoding(true);

    m_decoderSourceReader->readNextSample();
}

void MFAudioDecoderControl::start()
{
    if (isDecoding())
        return;

    if (m_loadingSource) {
        m_deferredStart = true;
    } else {
        IMFMediaSource *source = m_sourceResolver->mediaSource();
        if (!source) {
            if (m_device)
                error(QAudioDecoder::ResourceError, tr("Unable to read from specified device"));
            else if (m_source.isValid())
                error(QAudioDecoder::ResourceError, tr("Unable to load specified URL"));
            else
                error(QAudioDecoder::ResourceError, tr("No media source specified"));
            return;
        } else {
            startReadingSource(source);
        }
    }
}

void MFAudioDecoderControl::stop()
{
    m_deferredStart = false;
    if (!isDecoding())
        return;

    disconnect(m_decoderSourceReader.get());
    m_decoderSourceReader->clearSource();
    m_decoderSourceReader.reset();

    if (bufferAvailable()) {
        QAudioBuffer buffer;
        m_audioBuffer.swap(buffer);
        emit bufferAvailableChanged(false);
    }
    setIsDecoding(false);

    if (m_position != -1) {
        m_position = -1;
        positionChanged(m_position);
    }
    if (m_duration != -1) {
        m_duration = -1;
        durationChanged(m_duration);
    }
}

void MFAudioDecoderControl::handleNewSample(QWindowsIUPointer<IMFSample> sample)
{
    Q_ASSERT(sample);

    qint64 sampleStartTimeUs = m_resampler.outputFormat().durationForBytes(m_resampler.totalOutputBytes());
    QByteArray out = m_resampler.resample(sample.get());

    if (out.isEmpty()) {
        error(QAudioDecoder::Error::ResourceError, tr("Failed processing a sample"));

    } else {
        m_audioBuffer = QAudioBuffer(out, m_resampler.outputFormat(), sampleStartTimeUs);

        emit bufferAvailableChanged(true);
        emit bufferReady();
    }
}

void MFAudioDecoderControl::handleSourceFinished()
{
    stop();
    emit finished();
}

void MFAudioDecoderControl::setAudioFormat(const QAudioFormat &format)
{
    if (m_outputFormat == format)
        return;
    m_outputFormat = format;
    emit formatChanged(m_outputFormat);
}

QAudioBuffer MFAudioDecoderControl::read()
{
    QAudioBuffer buffer;

    if (bufferAvailable()) {
        buffer.swap(m_audioBuffer);
        m_position = buffer.startTime() / 1000;
        emit positionChanged(m_position);
        emit bufferAvailableChanged(false);
        m_decoderSourceReader->readNextSample();
    }

    return buffer;
}
