/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
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

#ifndef QT3DANIMATION_QANIMATIONCLIPLOADER_H
#define QT3DANIMATION_QANIMATIONCLIPLOADER_H

#include <Qt3DAnimation/qt3danimation_global.h>
#include <Qt3DAnimation/qabstractanimationclip.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

namespace Qt3DAnimation {

class QAnimationClipLoaderPrivate;

class Q_3DANIMATIONSHARED_EXPORT QAnimationClipLoader : public QAbstractAnimationClip
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    explicit QAnimationClipLoader(Qt3DCore::QNode *parent = nullptr);
    explicit QAnimationClipLoader(const QUrl &source,
                                  Qt3DCore::QNode *parent = nullptr);
    ~QAnimationClipLoader();

    enum Status {
        NotReady = 0,
        Ready,
        Error
    };
    Q_ENUM(Status) // LCOV_EXCL_LINE

    QUrl source() const;
    Status status() const;

public Q_SLOTS:
    void setSource(const QUrl &source);

Q_SIGNALS:
    void sourceChanged(const QUrl &source);
    void statusChanged(Status status);

protected:
    explicit QAnimationClipLoader(QAnimationClipLoaderPrivate &dd, Qt3DCore::QNode *parent = nullptr);

private:
    Q_DECLARE_PRIVATE(QAnimationClipLoader)
};

} // namespace Qt3DAnimation

QT_END_NAMESPACE

#endif // QT3DANIMATION_QANIMATIONCLIPLOADER_H
