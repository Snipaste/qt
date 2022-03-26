/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "qscene_p.h"

#include <Qt3DCore/qnode.h>
#include <QtCore/QHash>
#include <QtCore/QReadLocker>

#include <Qt3DCore/private/qnode_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

class QScenePrivate
{
public:
    QScenePrivate(QAspectEngine *engine)
        : m_engine(engine)
        , m_arbiter(nullptr)
        , m_postConstructorInit(new NodePostConstructorInit)
        , m_rootNode(nullptr)
    {
    }

    QAspectEngine *m_engine;
    QHash<QNodeId, QNode *> m_nodeLookupTable;
    QMultiHash<QNodeId, QNodeId> m_componentToEntities;
    QChangeArbiter *m_arbiter;
    QScopedPointer<NodePostConstructorInit> m_postConstructorInit;
    mutable QReadWriteLock m_lock;
    mutable QReadWriteLock m_nodePropertyTrackModeLock;
    QNode *m_rootNode;
    QScene::DirtyNodeSet m_dirtyBits;
};


QScene::QScene(QAspectEngine *engine)
    : d_ptr(new QScenePrivate(engine))
{
}

QScene::~QScene()
{
}

QAspectEngine *QScene::engine() const
{
    Q_D(const QScene);
    return d->m_engine;
}

// Called by main thread only
void QScene::addObservable(QNode *observable)
{
    Q_D(QScene);
    if (observable != nullptr) {
        QWriteLocker lock(&d->m_lock);
        d->m_nodeLookupTable.insert(observable->id(), observable);
        if (d->m_arbiter != nullptr)
            observable->d_func()->setArbiter(d->m_arbiter);
    }
}

// Called by main thread
void QScene::removeObservable(QNode *observable)
{
    Q_D(QScene);
    if (observable != nullptr) {
        QWriteLocker lock(&d->m_lock);
        const QNodeId nodeUuid = observable->id();
        d->m_nodeLookupTable.remove(nodeUuid);
        observable->d_func()->setArbiter(nullptr);
    }
}

// Called by any thread
QNode *QScene::lookupNode(QNodeId id) const
{
    Q_D(const QScene);
    QReadLocker lock(&d->m_lock);
    return d->m_nodeLookupTable.value(id);
}

QList<QNode *> QScene::lookupNodes(const QList<QNodeId> &ids) const
{
    Q_D(const QScene);
    QReadLocker lock(&d->m_lock);
    QList<QNode *> nodes;
    nodes.reserve(ids.size());
    for (QNodeId id : ids)
        nodes.push_back(d->m_nodeLookupTable.value(id));
    return nodes;
}

QNode *QScene::rootNode() const
{
    Q_D(const QScene);
    return d->m_rootNode;
}

void QScene::setArbiter(QChangeArbiter *arbiter)
{
    Q_D(QScene);
    d->m_arbiter = arbiter;
}

QChangeArbiter *QScene::arbiter() const
{
    Q_D(const QScene);
    return d->m_arbiter;
}

QList<QNodeId> QScene::entitiesForComponent(QNodeId id) const
{
    Q_D(const QScene);
    QReadLocker lock(&d->m_lock);
    QList<QNodeId> result;
    const auto p = d->m_componentToEntities.equal_range(id);
    for (auto it = p.first; it != p.second; ++it)
        result.push_back(*it);
    return result;
}

void QScene::addEntityForComponent(QNodeId componentUuid, QNodeId entityUuid)
{
    Q_D(QScene);
    QWriteLocker lock(&d->m_lock);
    d->m_componentToEntities.insert(componentUuid, entityUuid);
}

void QScene::removeEntityForComponent(QNodeId componentUuid, QNodeId entityUuid)
{
    Q_D(QScene);
    QWriteLocker lock(&d->m_lock);
    d->m_componentToEntities.remove(componentUuid, entityUuid);
}

bool QScene::hasEntityForComponent(QNodeId componentUuid, QNodeId entityUuid)
{
    Q_D(QScene);
    QReadLocker lock(&d->m_lock);
    const auto range = d->m_componentToEntities.equal_range(componentUuid);
    return std::find(range.first, range.second, entityUuid) != range.second;
}

NodePostConstructorInit *QScene::postConstructorInit() const
{
    Q_D(const QScene);
    return d->m_postConstructorInit.get();
}

QScene::DirtyNodeSet QScene::dirtyBits()
{
    Q_D(QScene);
    return d->m_dirtyBits;
}

void QScene::clearDirtyBits()
{
    Q_D(QScene);
    d->m_dirtyBits = {};
}

void QScene::markDirty(QScene::DirtyNodeSet changes)
{
    Q_D(QScene);
    d->m_dirtyBits |= changes;
}

void QScene::setRootNode(QNode *root)
{
    Q_D(QScene);
    d->m_rootNode = root;
}

} // Qt3D

QT_END_NAMESPACE
