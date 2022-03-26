/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtScxml module of the Qt Toolkit.
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

#include "statemachineloader_p.h"

#include <QtScxml/qscxmlstatemachine.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qqmlfile.h>
#include <qbuffer.h>

/*!
    \qmltype StateMachineLoader
    \instantiates QScxmlStateMachineLoader
    \inqmlmodule QtScxml

    \brief Dynamically loads an SCXML document and instantiates the state machine.

    \since QtScxml 5.7
 */

QScxmlStateMachineLoader::QScxmlStateMachineLoader(QObject *parent)
    : QObject(parent)
    , m_implicitDataModel(nullptr)
{
}

/*!
    \qmlproperty ScxmlStateMachine StateMachineLoader::stateMachine

    The state machine instance.
 */
QT_PREPEND_NAMESPACE(QScxmlStateMachine) *QScxmlStateMachineLoader::stateMachine() const
{
    return m_stateMachine;
}

void QScxmlStateMachineLoader::setStateMachine(QScxmlStateMachine* stateMachine)
{
    if (m_stateMachine.value() != stateMachine) {
        delete m_stateMachine.value();
        m_stateMachine = stateMachine;
    }
}

QBindable<QScxmlStateMachine*> QScxmlStateMachineLoader::bindableStateMachine()
{
    return &m_stateMachine;
}

/*!
    \qmlproperty url StateMachineLoader::source

    The URL of the SCXML document to load. Only synchronously accessible URLs
    are supported.
 */
QUrl QScxmlStateMachineLoader::source()
{
    return m_source;
}

void QScxmlStateMachineLoader::setSource(const QUrl &source)
{
    if (!source.isValid())
        return;

    const QUrl oldSource = m_source;
    setStateMachine(nullptr);
    m_implicitDataModel = nullptr;

    if (parse(source))
        m_source = source;
    else
        m_source = QUrl();

    if (oldSource != m_source)
        m_source.notify();
}

QBindable<QUrl> QScxmlStateMachineLoader::bindableSource()
{
    return &m_source;
}

QVariantMap QScxmlStateMachineLoader::initialValues() const
{
    return m_initialValues;
}

void QScxmlStateMachineLoader::setInitialValues(const QVariantMap &initialValues)
{
    if (initialValues == m_initialValues.value()) {
        m_initialValues.removeBindingUnlessInWrapper();
        return;
    }
    m_initialValues = initialValues;
    if (m_stateMachine.value())
        m_stateMachine.value()->setInitialValues(initialValues);
    m_initialValues.notify();
}

QBindable<QVariantMap> QScxmlStateMachineLoader::bindableInitialValues()
{
    return &m_initialValues;
}

QScxmlDataModel *QScxmlStateMachineLoader::dataModel() const
{
    return m_dataModel;
}

void QScxmlStateMachineLoader::setDataModel(QScxmlDataModel *dataModel)
{
    if (dataModel == m_dataModel.value()) {
        m_dataModel.removeBindingUnlessInWrapper();
        return;
    }
    m_dataModel = dataModel;
    if (m_stateMachine.value()) {
        if (dataModel)
            m_stateMachine.value()->setDataModel(dataModel);
        else
            m_stateMachine.value()->setDataModel(m_implicitDataModel);
    }
    m_dataModel.notify();
}

QBindable<QScxmlDataModel*> QScxmlStateMachineLoader::bindableDataModel()
{
    return &m_dataModel;
}

bool QScxmlStateMachineLoader::parse(const QUrl &source)
{
    if (!QQmlFile::isSynchronous(source)) {
        qmlWarning(this) << QStringLiteral("Cannot open '%1' for reading: only synchronous access is supported.")
                         .arg(source.url());
        return false;
    }
    QQmlFile scxmlFile(QQmlEngine::contextForObject(this)->engine(), source);
    if (scxmlFile.isError()) {
        // the synchronous case can only fail when the file is not found (or not readable).
        qmlWarning(this) << QStringLiteral("Cannot open '%1' for reading.").arg(source.url());
        return false;
    }

    QByteArray data(scxmlFile.dataByteArray());
    QBuffer buf(&data);
    if (!buf.open(QIODevice::ReadOnly)) {
        qmlWarning(this) << QStringLiteral("Cannot open input buffer for reading");
        return false;
    }

    QString fileName;
    if (source.isLocalFile()) {
        fileName = source.toLocalFile();
    } else if (source.scheme() == QStringLiteral("qrc")) {
        fileName = QStringLiteral(":") + source.path();
    } else {
        qmlWarning(this) << QStringLiteral("%1 is neither a local nor a resource URL.")
                            .arg(source.url())
                         << QStringLiteral("Invoking services by relative path will not work.");
    }

    auto stateMachine = QScxmlStateMachine::fromData(&buf, fileName);
    stateMachine->setParent(this);
    m_implicitDataModel = stateMachine->dataModel();

    if (stateMachine->parseErrors().isEmpty()) {
        if (m_dataModel.value())
            stateMachine->setDataModel(m_dataModel.value());
        stateMachine->setInitialValues(m_initialValues.value());
        setStateMachine(stateMachine);
        // as this is deferred any pending property updates to m_dataModel and m_initialValues
        // should still occur before start().
        QMetaObject::invokeMethod(m_stateMachine.value(), "start", Qt::QueuedConnection);
        return true;
    } else {
        qmlWarning(this) << QStringLiteral("Something went wrong while parsing '%1':")
                         .arg(source.url())
                      << Qt::endl;
        const auto errors = stateMachine->parseErrors();
        for (const QScxmlError &error : errors) {
            qmlWarning(this) << error.toString();
        }
        return false;
    }
}
