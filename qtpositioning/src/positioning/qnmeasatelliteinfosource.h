/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
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
#ifndef QNMEASATELLITEINFOSOURCE_H
#define QNMEASATELLITEINFOSOURCE_H

#include <QtPositioning/QGeoSatelliteInfoSource>

QT_BEGIN_NAMESPACE

class QIODevice;

class QNmeaSatelliteInfoSourcePrivate;
class Q_POSITIONING_EXPORT QNmeaSatelliteInfoSource : public QGeoSatelliteInfoSource
{
    Q_OBJECT
public:
    enum class UpdateMode {
        RealTimeMode = 1,
        SimulationMode
    };

    static QString SimulationUpdateInterval;

    explicit QNmeaSatelliteInfoSource(UpdateMode mode, QObject *parent = nullptr);
    ~QNmeaSatelliteInfoSource() override;

    UpdateMode updateMode() const;

    void setDevice(QIODevice *source);
    QIODevice *device() const;

    void setUpdateInterval(int msec) override;
    int minimumUpdateInterval() const override;
    Error error() const override;

    bool setBackendProperty(const QString &name, const QVariant &value) override;
    QVariant backendProperty(const QString &name) const override;

public Q_SLOTS:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

protected:
    virtual QGeoSatelliteInfo::SatelliteSystem
    parseSatellitesInUseFromNmea(const char *data, int size, QList<int> &pnrsInUse);
    enum SatelliteInfoParseStatus {
        NotParsed = 0,
        PartiallyParsed,
        FullyParsed
    };
    virtual SatelliteInfoParseStatus parseSatelliteInfoFromNmea(const char *data, int size,
                                                                QList<QGeoSatelliteInfo> &infos,
                                                                QGeoSatelliteInfo::SatelliteSystem &system);

    QNmeaSatelliteInfoSourcePrivate *d;
    void setError(QGeoSatelliteInfoSource::Error satelliteError);

    friend class QNmeaSatelliteInfoSourcePrivate;
    Q_DISABLE_COPY(QNmeaSatelliteInfoSource)

    // for using the SatelliteInfoParseStatus enum
    friend class QLocationUtils;
};

QT_END_NAMESPACE

#endif // QNMEASATELLITEINFOSOURCE_H
