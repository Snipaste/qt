/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtCore/qlocale.h>
#include <qaudiodevice.h>

#include <QStringList>
#include <QList>
#include <QMediaDevices>

//TESTED_COMPONENT=src/multimedia

class tst_QAudioDevice : public QObject
{
    Q_OBJECT
public:
    tst_QAudioDevice(QObject* parent=nullptr) : QObject(parent) {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void checkAvailableDefaultInput();
    void checkAvailableDefaultOutput();
    void channels();
    void sampleFormat();
    void sampleRates();
    void isFormatSupported();
    void preferred();
    void assignOperator();
    void id();
    void defaultConstructor();
    void equalityOperator();

private:
    QAudioDevice* device;
};

void tst_QAudioDevice::initTestCase()
{
    // Only perform tests if audio output device exists!
    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    if (devices.size() == 0) {
        QSKIP("NOTE: no audio output device found, no tests will be performed");
    } else {
        device = new QAudioDevice(devices.at(0));
    }
}

void tst_QAudioDevice::cleanupTestCase()
{
    delete device;
}

void tst_QAudioDevice::checkAvailableDefaultInput()
{
    // Only perform tests if audio input device exists!
    QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    if (devices.size() > 0) {
        QVERIFY(!QMediaDevices::defaultAudioInput().isNull());
    }
}

void tst_QAudioDevice::checkAvailableDefaultOutput()
{
    // Only perform tests if audio input device exists!
    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    if (devices.size() > 0) {
        QVERIFY(!QMediaDevices::defaultAudioOutput().isNull());
    }
}

void tst_QAudioDevice::channels()
{
    QVERIFY(device->minimumChannelCount() > 0);
    QVERIFY(device->maximumChannelCount() >= device->minimumChannelCount());
}

void tst_QAudioDevice::sampleFormat()
{
    QList<QAudioFormat::SampleFormat> avail = device->supportedSampleFormats();
    QVERIFY(avail.size() > 0);
}

void tst_QAudioDevice::sampleRates()
{
    QVERIFY(device->minimumSampleRate() > 0);
    QVERIFY(device->maximumSampleRate() >= device->minimumSampleRate());
}

void tst_QAudioDevice::isFormatSupported()
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    // Should always be true for these format
    QVERIFY(device->isFormatSupported(format));
}

void tst_QAudioDevice::preferred()
{
    QAudioFormat format = device->preferredFormat();
    QVERIFY(format.isValid());
    QVERIFY(device->isFormatSupported(format));
}

// QAudioDevice's assignOperator method
void tst_QAudioDevice::assignOperator()
{
    QAudioDevice dev;
    QVERIFY(dev.id().isNull());
    QVERIFY(dev.isNull() == true);

    QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
    QVERIFY(devices.size() > 0);
    QAudioDevice dev1(devices.at(0));
    dev = dev1;
    QVERIFY(dev.isNull() == false);
    QVERIFY(dev.id() == dev1.id());
}

void tst_QAudioDevice::id()
{
    QVERIFY(!device->id().isNull());
    QVERIFY(device->id() == QMediaDevices::audioOutputs().at(0).id());
}

// QAudioDevice's defaultConstructor method
void tst_QAudioDevice::defaultConstructor()
{
    QAudioDevice dev;
    QVERIFY(dev.isNull() == true);
    QVERIFY(dev.id().isNull());
}

void tst_QAudioDevice::equalityOperator()
{
    // Get some default device infos
    QAudioDevice dev1;
    QAudioDevice dev2;

    QVERIFY(dev1 == dev2);
    QVERIFY(!(dev1 != dev2));

    // Make sure each available device is not equal to null
    const auto infos = QMediaDevices::audioOutputs();
    for (const QAudioDevice &info : infos) {
        QVERIFY(dev1 != info);
        QVERIFY(!(dev1 == info));

        dev2 = info;

        QVERIFY(dev2 == info);
        QVERIFY(!(dev2 != info));

        QVERIFY(dev1 != dev2);
        QVERIFY(!(dev1 == dev2));
    }

    // XXX Perhaps each available device should not be equal to any other
}

QTEST_MAIN(tst_QAudioDevice)

#include "tst_qaudiodevice.moc"
