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

#ifndef QMLORIENTATIONSENSOR_P_H
#define QMLORIENTATIONSENSOR_P_H

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
#include <QtSensors/QOrientationSensor>

QT_BEGIN_NAMESPACE

class QOrientationSensor;

class Q_SENSORSQUICK_PRIVATE_EXPORT QmlOrientationSensor : public QmlSensor
{
    Q_OBJECT
    QML_NAMED_ELEMENT(OrientationSensor)
    QML_ADDED_IN_VERSION(5,0)
public:
    explicit QmlOrientationSensor(QObject *parent = 0);
    ~QmlOrientationSensor();

    QSensor *sensor() const override;

private:
    QOrientationSensor *m_sensor;
    QmlSensorReading *createReading() const override;
};

class Q_SENSORSQUICK_PRIVATE_EXPORT QmlOrientationSensorReading : public QmlSensorReading
{
    Q_OBJECT
    Q_PROPERTY(QOrientationReading::Orientation orientation READ orientation
               NOTIFY orientationChanged BINDABLE bindableOrientation)
    QML_NAMED_ELEMENT(OrientationReading)
    QML_UNCREATABLE("Cannot create OrientationReading")
    QML_ADDED_IN_VERSION(5,0)
public:

    explicit QmlOrientationSensorReading(QOrientationSensor *sensor);
    ~QmlOrientationSensorReading();

    QOrientationReading::Orientation orientation() const;
    QBindable<QOrientationReading::Orientation> bindableOrientation() const;

Q_SIGNALS:
    void orientationChanged();

private:
    QSensorReading *reading() const override;
    void readingUpdate() override;
    QOrientationSensor *m_sensor;
    Q_OBJECT_BINDABLE_PROPERTY(QmlOrientationSensorReading, QOrientationReading::Orientation,
                               m_orientation, &QmlOrientationSensorReading::orientationChanged)
};

QT_END_NAMESPACE
#endif
