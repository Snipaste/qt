/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSensors module of the Qt Toolkit.
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

#ifndef QMLTAPSENSOR_P_H
#define QMLTAPSENSOR_P_H

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

#include "qmlsensor_p.h"
#include <QtSensors/QTapSensor>

QT_BEGIN_NAMESPACE

class QTapSensor;

class Q_SENSORSQUICK_PRIVATE_EXPORT QmlTapSensor : public QmlSensor
{
    Q_OBJECT
    Q_PROPERTY(bool returnDoubleTapEvents READ returnDoubleTapEvents WRITE setReturnDoubleTapEvents NOTIFY returnDoubleTapEventsChanged)
    QML_NAMED_ELEMENT(TapSensor)
    QML_ADDED_IN_VERSION(5,0)
public:
    explicit QmlTapSensor(QObject *parent = 0);
    ~QmlTapSensor();

    bool returnDoubleTapEvents() const;
    void setReturnDoubleTapEvents(bool ret);

    QSensor *sensor() const override;

Q_SIGNALS:
    void returnDoubleTapEventsChanged(bool returnDoubleTapEvents);

private:
    QTapSensor *m_sensor;
    QmlSensorReading *createReading() const override;
};

class Q_SENSORSQUICK_PRIVATE_EXPORT QmlTapSensorReading : public QmlSensorReading
{
    Q_OBJECT
    Q_PROPERTY(QTapReading::TapDirection tapDirection READ tapDirection
               NOTIFY tapDirectionChanged BINDABLE bindableTapDirection)
    Q_PROPERTY(bool doubleTap READ isDoubleTap
               NOTIFY isDoubleTapChanged BINDABLE bindableDoubleTap)
    QML_NAMED_ELEMENT(TapReading)
    QML_UNCREATABLE("Cannot create TapReading")
    QML_ADDED_IN_VERSION(5,0)
public:

    explicit QmlTapSensorReading(QTapSensor *sensor);
    ~QmlTapSensorReading();

    QTapReading::TapDirection tapDirection() const;
    QBindable<QTapReading::TapDirection> bindableTapDirection() const;
    bool isDoubleTap() const;
    QBindable<bool> bindableDoubleTap() const;

Q_SIGNALS:
    void tapDirectionChanged();
    void isDoubleTapChanged();

private:
    QSensorReading *reading() const override;
    void readingUpdate() override;
    QTapSensor *m_sensor;
    Q_OBJECT_BINDABLE_PROPERTY(QmlTapSensorReading, QTapReading::TapDirection,
                               m_tapDirection, &QmlTapSensorReading::tapDirectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY(QmlTapSensorReading, bool,
                               m_isDoubleTap, &QmlTapSensorReading::isDoubleTapChanged)
};

QT_END_NAMESPACE
#endif
