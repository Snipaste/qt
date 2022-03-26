/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QOPCUAAUTHENTICATIONINFORMATION_H
#define QOPCUAAUTHENTICATIONINFORMATION_H

#include <QtOpcUa/qopcuaglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qvariant.h>

#include <QtOpcUa/QOpcUaUserTokenPolicy>

QT_BEGIN_NAMESPACE

class QOpcUaAuthenticationInformationData;
class Q_OPCUA_EXPORT QOpcUaAuthenticationInformation
{
    Q_GADGET

public:
    QOpcUaAuthenticationInformation();
    QOpcUaAuthenticationInformation(const QOpcUaAuthenticationInformation &);
    QOpcUaAuthenticationInformation &operator=(const QOpcUaAuthenticationInformation &);
    bool operator==(const QOpcUaAuthenticationInformation &rhs) const;
    ~QOpcUaAuthenticationInformation();

    Q_INVOKABLE void setAnonymousAuthentication();
    Q_INVOKABLE void setUsernameAuthentication(const QString &username, const QString &password);
    Q_INVOKABLE void setCertificateAuthentication();

    const QVariant &authenticationData() const;
    Q_INVOKABLE QOpcUaUserTokenPolicy::TokenType authenticationType() const;

private:
    QSharedDataPointer<QOpcUaAuthenticationInformationData> data;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QOpcUaAuthenticationInformation)

#endif // QOPCUAAUTHETICATIONINFORMATION_H
