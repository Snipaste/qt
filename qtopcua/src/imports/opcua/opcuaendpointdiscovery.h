/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt OPC UA module.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef OPCUAENDPOINTDISCOVERY_H
#define OPCUAENDPOINTDISCOVERY_H

#include "opcuaconnection.h"
#include "opcuastatus.h"

#include <QtOpcUa/qopcuatype.h>
#include <QOpcUaEndpointDescription>

#include <QQmlParserStatus>
#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE

class OpcUaEndpointDiscovery : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(OpcUaStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(OpcUaConnection* connection READ connection WRITE setConnection NOTIFY connectionChanged)
    Q_INTERFACES(QQmlParserStatus)

public:
    OpcUaEndpointDiscovery(QObject *parent = nullptr);
    ~OpcUaEndpointDiscovery();

    const QString &serverUrl() const;
    void setServerUrl(const QString &serverUrl);
    int count() const;
    Q_INVOKABLE QOpcUaEndpointDescription at(int row) const;
    const OpcUaStatus &status() const;
    void setConnection(OpcUaConnection *);
    OpcUaConnection *connection();

signals:
    void serverUrlChanged(const QString &serverUrl);
    void endpointsChanged();
    void countChanged();
    void statusChanged();
    void connectionChanged(OpcUaConnection *);

private slots:
    void connectSignals();
    void handleEndpoints(const QList<QOpcUaEndpointDescription> &endpoints, QOpcUa::UaStatusCode statusCode, const QUrl &requestUrl);
    void startRequestEndpoints();

private:
    // Callbacks from QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    QString m_serverUrl;
    OpcUaConnection *m_connection = nullptr;
    QList<QOpcUaEndpointDescription> m_endpoints;
    OpcUaStatus m_status;
    bool m_componentCompleted = false;
};


QT_END_NAMESPACE

#endif // OPCUAENDPOINTDISCOVERY_H
