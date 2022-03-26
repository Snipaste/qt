/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "private/qnativesocketengine_p.h"
#include "private/qnetconmonitor_p.h"

#include "private/qobject_p.h"

#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreFoundation/CoreFoundation.h>

#include <netinet/in.h>

#include <cstring>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetMon, "qt.network.monitor");

namespace {

class ReachabilityDispatchQueue
{
public:
    ReachabilityDispatchQueue()
    {
        queue = dispatch_queue_create("qt-network-reachability-queue", nullptr);
        if (!queue)
            qCWarning(lcNetMon, "Failed to create a dispatch queue for reachability probes");
    }

    ~ReachabilityDispatchQueue()
    {
        if (queue)
            dispatch_release(queue);
    }

    dispatch_queue_t data() const
    {
        return queue;
    }

private:
    dispatch_queue_t queue = nullptr;

    Q_DISABLE_COPY_MOVE(ReachabilityDispatchQueue)
};

dispatch_queue_t qt_reachability_queue()
{
    static const ReachabilityDispatchQueue reachabilityQueue;
    return reachabilityQueue.data();
}

qt_sockaddr qt_hostaddress_to_sockaddr(const QHostAddress &src)
{
    if (src.isNull())
        return {};

    qt_sockaddr dst;
    if (src.protocol() == QAbstractSocket::IPv4Protocol) {
        dst.a4 = sockaddr_in{};
        dst.a4.sin_family = AF_INET;
        dst.a4.sin_addr.s_addr = htonl(src.toIPv4Address());
        dst.a4.sin_len = sizeof(sockaddr_in);
    } else if (src.protocol() == QAbstractSocket::IPv6Protocol) {
        dst.a6 = sockaddr_in6{};
        dst.a6.sin6_family = AF_INET6;
        dst.a6.sin6_len = sizeof(sockaddr_in6);
        const Q_IPV6ADDR ipv6 = src.toIPv6Address();
        std::memcpy(&dst.a6.sin6_addr, &ipv6, sizeof ipv6);
    } else {
        Q_UNREACHABLE();
    }

    return dst;
}

} // unnamed namespace

class QNetworkConnectionMonitorPrivate : public QObjectPrivate
{
public:
    SCNetworkReachabilityRef probe = nullptr;
    SCNetworkReachabilityFlags state = kSCNetworkReachabilityFlagsIsLocalAddress;
    bool scheduled = false;

    void updateState(SCNetworkReachabilityFlags newState);
    void reset();
    bool isReachable() const;

    static void probeCallback(SCNetworkReachabilityRef probe, SCNetworkReachabilityFlags flags, void *info);

    Q_DECLARE_PUBLIC(QNetworkConnectionMonitor)
};

void QNetworkConnectionMonitorPrivate::updateState(SCNetworkReachabilityFlags newState)
{
    // To be executed only on the reachability queue.
    Q_Q(QNetworkConnectionMonitor);

    // NETMONTODO: for now, 'online' for us means kSCNetworkReachabilityFlagsReachable
    // is set. There are more possible flags that require more tests/some special
    // setup. So in future this part and related can change/be extended.
    const bool wasReachable = isReachable();
    state = newState;
    if (wasReachable != isReachable())
        emit q->reachabilityChanged(isReachable());
}

void QNetworkConnectionMonitorPrivate::reset()
{
    if (probe) {
        CFRelease(probe);
        probe = nullptr;
    }

    state = kSCNetworkReachabilityFlagsIsLocalAddress;
    scheduled = false;
}

bool QNetworkConnectionMonitorPrivate::isReachable() const
{
    return !!(state & kSCNetworkReachabilityFlagsReachable);
}

void QNetworkConnectionMonitorPrivate::probeCallback(SCNetworkReachabilityRef probe, SCNetworkReachabilityFlags flags, void *info)
{
    // To be executed only on the reachability queue.
    Q_UNUSED(probe);

    auto monitorPrivate = static_cast<QNetworkConnectionMonitorPrivate *>(info);
    Q_ASSERT(monitorPrivate);
    monitorPrivate->updateState(flags);
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor()
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor(const QHostAddress &local, const QHostAddress &remote)
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
    setTargets(local, remote);
}

QNetworkConnectionMonitor::~QNetworkConnectionMonitor()
{
    Q_D(QNetworkConnectionMonitor);

    stopMonitoring();
    d->reset();
}

bool QNetworkConnectionMonitor::setTargets(const QHostAddress &local, const QHostAddress &remote)
{
    Q_D(QNetworkConnectionMonitor);

    if (isMonitoring()) {
        qCWarning(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }

    if (local.isNull()) {
        qCWarning(lcNetMon, "Invalid (null) local address, cannot create a reachability target");
        return false;
    }

    // Clear the old target if needed:
    d->reset();

    qt_sockaddr client = qt_hostaddress_to_sockaddr(local);
    if (remote.isNull()) {
        // That's a special case our QNetworkInformation backend is using (AnyIpv4/6 address to check an overall status).
        d->probe = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, reinterpret_cast<sockaddr *>(&client));
    } else {
        qt_sockaddr target = qt_hostaddress_to_sockaddr(remote);
        d->probe = SCNetworkReachabilityCreateWithAddressPair(kCFAllocatorDefault,
                                                              reinterpret_cast<sockaddr *>(&client),
                                                              reinterpret_cast<sockaddr *>(&target));
    }

    if (d->probe) {
        // Let's read the initial state so that callback coming later can
        // see a difference. Ignore errors though.
        SCNetworkReachabilityGetFlags(d->probe, &d->state);
    }else {
        qCWarning(lcNetMon, "Failed to create network reachability probe");
        return false;
    }

    return true;
}

bool QNetworkConnectionMonitor::startMonitoring()
{
    Q_D(QNetworkConnectionMonitor);

    if (isMonitoring()) {
        qCWarning(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }

    if (!d->probe) {
        qCWarning(lcNetMon, "Can not start monitoring, set targets first");
        return false;
    }

    auto queue = qt_reachability_queue();
    if (!queue) {
        qWarning(lcNetMon, "Failed to create a dispatch queue to schedule a probe on");
        return false;
    }

    SCNetworkReachabilityContext context = {};
    context.info = d;
    if (!SCNetworkReachabilitySetCallback(d->probe, QNetworkConnectionMonitorPrivate::probeCallback, &context)) {
        qWarning(lcNetMon, "Failed to set a reachability callback");
        return false;
    }


    if (!SCNetworkReachabilitySetDispatchQueue(d->probe, queue)) {
        qWarning(lcNetMon, "Failed to schedule a reachability callback on a queue");
        return false;
    }

    return d->scheduled = true;
}

bool QNetworkConnectionMonitor::isMonitoring() const
{
    Q_D(const QNetworkConnectionMonitor);

    return d->scheduled;
}

void QNetworkConnectionMonitor::stopMonitoring()
{
    Q_D(QNetworkConnectionMonitor);

    if (d->scheduled) {
        Q_ASSERT(d->probe);
        SCNetworkReachabilitySetDispatchQueue(d->probe, nullptr);
        SCNetworkReachabilitySetCallback(d->probe, nullptr, nullptr);
        d->scheduled = false;
    }
}

bool QNetworkConnectionMonitor::isReachable()
{
    Q_D(QNetworkConnectionMonitor);

    if (isMonitoring()) {
        qCWarning(lcNetMon, "Calling isReachable() is unsafe after the monitoring started");
        return false;
    }

    if (!d->probe) {
        qCWarning(lcNetMon, "Reachability is unknown, set the target first");
        return false;
    }

    return d->isReachable();
}

bool QNetworkConnectionMonitor::isEnabled()
{
    return true;
}

QT_END_NAMESPACE
