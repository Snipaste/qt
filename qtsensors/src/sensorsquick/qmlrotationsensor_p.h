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

#ifndef QMLROTATIONSENSOR_P_H
#define QMLROTATIONSENSOR_P_H

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

QT_BEGIN_NAMESPACE

class QRotationSensor;

class Q_SENSORSQUICK_PRIVATE_EXPORT QmlRotationSensor : public QmlSensor
{
    Q_OBJECT
    Q_PROPERTY(bool hasZ READ hasZ NOTIFY hasZChanged)
    QML_NAMED_ELEMENT(RotationSensor)
    QML_ADDED_IN_VERSION(5,0)
public:
    explicit QmlRotationSensor(QObject *parent = 0);
    ~QmlRotationSensor();

    bool hasZ() const;
    QSensor *sensor() const override;

Q_SIGNALS:
    void hasZChanged(bool hasZ);

private:
    QRotationSensor *m_sensor;
    QmlSensorReading *createReading() const override;
};

class Q_SENSORSQUICK_PRIVATE_EXPORT QmlRotationSensorReading : public QmlSensorReading
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x NOTIFY xChanged BINDABLE bindableX)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged BINDABLE bindableY)
    Q_PROPERTY(qreal z READ z NOTIFY zChanged BINDABLE bindableZ)
    QML_NAMED_ELEMENT(RotationReading)
    QML_UNCREATABLE("Cannot create RotationReading")
    QML_ADDED_IN_VERSION(5,0)
public:
    explicit QmlRotationSensorReading(QRotationSensor *sensor);
    ~QmlRotationSensorReading();

    qreal x() const;
    QBindable<qreal> bindableX() const;
    qreal y() const;
    QBindable<qreal> bindableY() const;
    qreal z() const;
    QBindable<qreal> bindableZ() const;

Q_SIGNALS:
    void xChanged();
    void yChanged();
    void zChanged();

private:
    QSensorReading *reading() const override;
    void readingUpdate() override;
    QRotationSensor *m_sensor;
    Q_OBJECT_BINDABLE_PROPERTY(QmlRotationSensorReading, qreal,
                               m_x, &QmlRotationSensorReading::xChanged)
    Q_OBJECT_BINDABLE_PROPERTY(QmlRotationSensorReading, qreal,
                               m_y, &QmlRotationSensorReading::yChanged)
    Q_OBJECT_BINDABLE_PROPERTY(QmlRotationSensorReading, qreal,
                               m_z, &QmlRotationSensorReading::zChanged)
};

QT_END_NAMESPACE
#endif
