/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "audiooutput.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QDebug>
#include <QVBoxLayout>
#include <qmath.h>
#include <qendian.h>

Generator::Generator(const QAudioFormat &format, qint64 durationUs, int sampleRate)
{
    if (format.isValid())
        generateData(format, durationUs, sampleRate);
}

void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    m_pos = 0;
    close();
}

void Generator::generateData(const QAudioFormat &format, qint64 durationUs, int sampleRate)
{
    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.channelCount() * channelBytes;
    qint64 length = format.bytesForDuration(durationUs);
    Q_ASSERT(length % sampleBytes == 0);
    Q_UNUSED(sampleBytes); // suppress warning in release builds

    m_buffer.resize(length);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data());
    int sampleIndex = 0;

    while (length) {
        // Produces value (-1..1)
        const qreal x = qSin(2 * M_PI * sampleRate * qreal(sampleIndex++ % format.sampleRate()) / format.sampleRate());
        for (int i=0; i<format.channelCount(); ++i) {
            switch(format.sampleFormat()) {
            case QAudioFormat::UInt8:
                *reinterpret_cast<quint8 *>(ptr) = static_cast<quint8>((1.0 + x) / 2 * 255);
                break;
            case QAudioFormat::Int16:
                *reinterpret_cast<qint16 *>(ptr) = static_cast<qint16>(x * 32767);
                break;
            case QAudioFormat::Int32:
                *reinterpret_cast<qint32 *>(ptr) = static_cast<qint32>(x * std::numeric_limits<qint32>::max());
                break;
            case QAudioFormat::Float:
                *reinterpret_cast<float *>(ptr) = x;
                break;
            default:
                break;
            }

            ptr += channelBytes;
            length -= channelBytes;
        }
    }
}

qint64 Generator::readData(char *data, qint64 len)
{
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (len - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            total += chunk;
        }
    }
    return total;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

AudioTest::AudioTest()
    : m_devices(new QMediaDevices(this)),
      m_pushTimer(new QTimer(this))
{
    initializeWindow();
    initializeAudio(m_devices->defaultAudioOutput());
}

AudioTest::~AudioTest()
{
    m_pushTimer->stop();
}

void AudioTest::initializeWindow()
{
    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;

    m_deviceBox = new QComboBox(this);
    const QAudioDevice &defaultDeviceInfo = m_devices->defaultAudioOutput();
    m_deviceBox->addItem(defaultDeviceInfo.description(), QVariant::fromValue(defaultDeviceInfo));
    for (auto &deviceInfo: m_devices->audioOutputs()) {
        if (deviceInfo != defaultDeviceInfo)
            m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    }
    connect(m_deviceBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::deviceChanged);
    connect(m_devices, &QMediaDevices::audioOutputsChanged, this, &AudioTest::updateAudioDevices);
    layout->addWidget(m_deviceBox);

    m_modeButton = new QPushButton(this);
    connect(m_modeButton, &QPushButton::clicked, this, &AudioTest::toggleMode);
    layout->addWidget(m_modeButton);

    m_suspendResumeButton = new QPushButton(this);
    connect(m_suspendResumeButton, &QPushButton::clicked, this, &AudioTest::toggleSuspendResume);
    layout->addWidget(m_suspendResumeButton);

    QHBoxLayout *volumeBox = new QHBoxLayout;
    m_volumeLabel = new QLabel;
    m_volumeLabel->setText(tr("Volume:"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(100);
    m_volumeSlider->setSingleStep(10);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioTest::volumeChanged);
    volumeBox->addWidget(m_volumeLabel);
    volumeBox->addWidget(m_volumeSlider);
    layout->addLayout(volumeBox);

    window->setLayout(layout);

    setCentralWidget(window);
    window->show();
}

void AudioTest::initializeAudio(const QAudioDevice &deviceInfo)
{
    QAudioFormat format = deviceInfo.preferredFormat();

    const int durationSeconds = 1;
    const int toneSampleRateHz = 600;
    m_generator.reset(new Generator(format, durationSeconds * 1000000, toneSampleRateHz));
    m_audioOutput.reset(new QAudioSink(deviceInfo, format));
    m_generator->start();

    qreal initialVolume = QAudio::convertVolume(m_audioOutput->volume(),
                                                QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * 100));
    toggleMode();
}

void AudioTest::deviceChanged(int index)
{
    m_generator->stop();
    m_audioOutput->stop();
    m_audioOutput->disconnect(this);
    initializeAudio(m_deviceBox->itemData(index).value<QAudioDevice>());
}

void AudioTest::volumeChanged(int value)
{
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    m_audioOutput->setVolume(linearVolume);
}

void AudioTest::updateAudioDevices()
{
    m_deviceBox->clear();
    const QList<QAudioDevice> devices = m_devices->audioOutputs();
    for (const QAudioDevice &deviceInfo: devices)
        m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
}

void AudioTest::toggleMode()
{
    m_pushTimer->stop();
    m_audioOutput->stop();
    toggleSuspendResume();

    if (m_pullMode) {
        //switch to pull mode (QAudioSink pulls from Generator as needed)
        m_modeButton->setText(tr("Enable push mode"));
        m_audioOutput->start(m_generator.data());
    } else {
        //switch to push mode (periodically push to QAudioSink using a timer)
        m_modeButton->setText(tr("Enable pull mode"));
        auto io = m_audioOutput->start();
        m_pushTimer->disconnect();

        connect(m_pushTimer, &QTimer::timeout, [this, io]() {
            if (m_audioOutput->state() == QAudio::StoppedState)
                return;

            int len = m_audioOutput->bytesFree();
            QByteArray buffer(len, 0);
            len = m_generator->read(buffer.data(), len);
            if (len)
               io->write(buffer.data(), len);
        });

        m_pushTimer->start(10);
    }

    m_pullMode = !m_pullMode;
}

void AudioTest::toggleSuspendResume()
{
    if (m_audioOutput->state() == QAudio::SuspendedState || m_audioOutput->state() == QAudio::StoppedState) {
        m_audioOutput->resume();
        m_suspendResumeButton->setText(tr("Suspend playback"));
    } else if (m_audioOutput->state() == QAudio::ActiveState) {
        m_audioOutput->suspend();
        m_suspendResumeButton->setText(tr("Resume playback"));
    } else if (m_audioOutput->state() == QAudio::IdleState) {
        // no-op
    }
}

