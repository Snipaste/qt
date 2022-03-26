/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include <QtNetwork/private/qnetworkinformation_p.h>

#include <QtNetwork/private/qnetconmonitor_p.h>

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoSCR)
Q_LOGGING_CATEGORY(lcNetInfoSCR, "qt.network.info.scnetworkreachability");


static QString backendName = QStringLiteral("scnetworkreachability");

class QSCNetworkReachabilityNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QSCNetworkReachabilityNetworkInformationBackend();
    ~QSCNetworkReachabilityNetworkInformationBackend();

    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return QNetworkInformation::Features(QNetworkInformation::Feature::Reachability);
    }

private Q_SLOTS:
    void reachabilityChanged(bool isOnline);

private:
    Q_DISABLE_COPY_MOVE(QSCNetworkReachabilityNetworkInformationBackend);

    QNetworkConnectionMonitor ipv4Probe;
    QNetworkConnectionMonitor ipv6Probe;
};

class QSCNetworkReachabilityNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QSCNetworkReachabilityNetworkInformationBackendFactory() = default;
    ~QSCNetworkReachabilityNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return QSCNetworkReachabilityNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        return new QSCNetworkReachabilityNetworkInformationBackend();
    }

private:
    Q_DISABLE_COPY_MOVE(QSCNetworkReachabilityNetworkInformationBackendFactory);
};

QSCNetworkReachabilityNetworkInformationBackend::QSCNetworkReachabilityNetworkInformationBackend()
{
    bool isOnline = false;
    if (ipv4Probe.setTargets(QHostAddress::AnyIPv4, {})) {
        // We manage to create SCNetworkReachabilityRef for IPv4, let's
        // read the last known state then!
        isOnline |= ipv4Probe.isReachable();
        ipv4Probe.startMonitoring();
    }

    if (ipv6Probe.setTargets(QHostAddress::AnyIPv6, {})) {
        // We manage to create SCNetworkReachability ref for IPv6, let's
        // read the last known state then!
        isOnline |= ipv6Probe.isReachable();
        ipv6Probe.startMonitoring();
    }
    reachabilityChanged(isOnline);

    connect(&ipv4Probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);
    connect(&ipv6Probe, &QNetworkConnectionMonitor::reachabilityChanged, this,
            &QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged,
            Qt::QueuedConnection);
}

QSCNetworkReachabilityNetworkInformationBackend::~QSCNetworkReachabilityNetworkInformationBackend()
{
}

void QSCNetworkReachabilityNetworkInformationBackend::reachabilityChanged(bool isOnline)
{
    setReachability(isOnline ? QNetworkInformation::Reachability::Online
                             : QNetworkInformation::Reachability::Disconnected);
}

QT_END_NAMESPACE

#include "qscnetworkreachabilitynetworkinformationbackend.moc"
