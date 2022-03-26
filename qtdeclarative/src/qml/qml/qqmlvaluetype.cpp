/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlvaluetype_p.h"

#include <QtCore/qmutex.h>
#include <private/qqmlglobal_p.h>
#include <QtCore/qdebug.h>
#include <private/qqmlengine_p.h>
#include <private/qmetaobjectbuilder_p.h>

QT_BEGIN_NAMESPACE

QQmlValueType::QQmlValueType(int typeId, const QMetaObject *gadgetMetaObject)
    : metaType(typeId)
{
    QMetaObjectBuilder builder(gadgetMetaObject);
    dynamicMetaObject = builder.toMetaObject();
    *static_cast<QMetaObject*>(this) = *dynamicMetaObject;
}

QQmlValueType::~QQmlValueType()
{
    ::free(dynamicMetaObject);
}

QQmlGadgetPtrWrapper *QQmlGadgetPtrWrapper::instance(QQmlEngine *engine, QMetaType type)
{
    return engine ? QQmlEnginePrivate::get(engine)->valueTypeInstance(type) : nullptr;
}

QQmlGadgetPtrWrapper::QQmlGadgetPtrWrapper(QQmlValueType *valueType, QObject *parent)
    : QObject(parent), m_gadgetPtr(valueType->create())
{
    QObjectPrivate *d = QObjectPrivate::get(this);
    Q_ASSERT(!d->metaObject);
    d->metaObject = valueType;
}

QQmlGadgetPtrWrapper::~QQmlGadgetPtrWrapper()
{
    QObjectPrivate *d = QObjectPrivate::get(this);
    static_cast<const QQmlValueType *>(d->metaObject)->destroy(m_gadgetPtr);
    d->metaObject = nullptr;
}

void QQmlGadgetPtrWrapper::read(QObject *obj, int idx)
{
    Q_ASSERT(m_gadgetPtr);
    void *a[] = { m_gadgetPtr, nullptr };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QQmlGadgetPtrWrapper::write(QObject *obj, int idx, QQmlPropertyData::WriteFlags flags)
{
    Q_ASSERT(m_gadgetPtr);
    int status = -1;
    void *a[] = { m_gadgetPtr, nullptr, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QQmlGadgetPtrWrapper::value()
{
    Q_ASSERT(m_gadgetPtr);
    return QVariant(QMetaType(metaTypeId()), m_gadgetPtr);
}

void QQmlGadgetPtrWrapper::setValue(const QVariant &value)
{
    Q_ASSERT(m_gadgetPtr);
    Q_ASSERT(metaTypeId() == value.userType());
    const QQmlValueType *type = valueType();
    type->destruct(m_gadgetPtr);
    type->construct(m_gadgetPtr, value.constData());
}

int QQmlGadgetPtrWrapper::metaCall(QMetaObject::Call type, int id, void **argv)
{
    Q_ASSERT(m_gadgetPtr);
    const QMetaObject *metaObject = valueType();
    QQmlMetaObject::resolveGadgetMethodOrPropertyIndex(type, &metaObject, &id);
    metaObject->d.static_metacall(static_cast<QObject *>(m_gadgetPtr), type, id, argv);
    return id;
}

const QQmlValueType *QQmlGadgetPtrWrapper::valueType() const
{
    const QObjectPrivate *d = QObjectPrivate::get(this);
    return static_cast<const QQmlValueType *>(d->metaObject);
}

QAbstractDynamicMetaObject *QQmlValueType::toDynamicMetaObject(QObject *)
{
    return this;
}

void QQmlValueType::objectDestroyed(QObject *)
{
}

int QQmlValueType::metaCall(QObject *object, QMetaObject::Call type, int _id, void **argv)
{
    return static_cast<QQmlGadgetPtrWrapper *>(object)->metaCall(type, _id, argv);
}

QString QQmlPointFValueType::toString() const
{
    return QString::asprintf("QPointF(%g, %g)", v.x(), v.y());
}

qreal QQmlPointFValueType::x() const
{
    return v.x();
}

qreal QQmlPointFValueType::y() const
{
    return v.y();
}

void QQmlPointFValueType::setX(qreal x)
{
    v.setX(x);
}

void QQmlPointFValueType::setY(qreal y)
{
    v.setY(y);
}


QString QQmlPointValueType::toString() const
{
    return QString::asprintf("QPoint(%d, %d)", v.x(), v.y());
}

int QQmlPointValueType::x() const
{
    return v.x();
}

int QQmlPointValueType::y() const
{
    return v.y();
}

void QQmlPointValueType::setX(int x)
{
    v.setX(x);
}

void QQmlPointValueType::setY(int y)
{
    v.setY(y);
}


QString QQmlSizeFValueType::toString() const
{
    return QString::asprintf("QSizeF(%g, %g)", v.width(), v.height());
}

qreal QQmlSizeFValueType::width() const
{
    return v.width();
}

qreal QQmlSizeFValueType::height() const
{
    return v.height();
}

void QQmlSizeFValueType::setWidth(qreal w)
{
    v.setWidth(w);
}

void QQmlSizeFValueType::setHeight(qreal h)
{
    v.setHeight(h);
}


QString QQmlSizeValueType::toString() const
{
    return QString::asprintf("QSize(%d, %d)", v.width(), v.height());
}

int QQmlSizeValueType::width() const
{
    return v.width();
}

int QQmlSizeValueType::height() const
{
    return v.height();
}

void QQmlSizeValueType::setWidth(int w)
{
    v.setWidth(w);
}

void QQmlSizeValueType::setHeight(int h)
{
    v.setHeight(h);
}

QString QQmlRectFValueType::toString() const
{
    return QString::asprintf("QRectF(%g, %g, %g, %g)", v.x(), v.y(), v.width(), v.height());
}

qreal QQmlRectFValueType::x() const
{
    return v.x();
}

qreal QQmlRectFValueType::y() const
{
    return v.y();
}

void QQmlRectFValueType::setX(qreal x)
{
    v.moveLeft(x);
}

void QQmlRectFValueType::setY(qreal y)
{
    v.moveTop(y);
}

qreal QQmlRectFValueType::width() const
{
    return v.width();
}

qreal QQmlRectFValueType::height() const
{
    return v.height();
}

void QQmlRectFValueType::setWidth(qreal w)
{
    v.setWidth(w);
}

void QQmlRectFValueType::setHeight(qreal h)
{
    v.setHeight(h);
}

qreal QQmlRectFValueType::left() const
{
    return v.left();
}

qreal QQmlRectFValueType::right() const
{
    return v.right();
}

qreal QQmlRectFValueType::top() const
{
    return v.top();
}

qreal QQmlRectFValueType::bottom() const
{
    return v.bottom();
}


QString QQmlRectValueType::toString() const
{
    return QString::asprintf("QRect(%d, %d, %d, %d)", v.x(), v.y(), v.width(), v.height());
}

int QQmlRectValueType::x() const
{
    return v.x();
}

int QQmlRectValueType::y() const
{
    return v.y();
}

void QQmlRectValueType::setX(int x)
{
    v.moveLeft(x);
}

void QQmlRectValueType::setY(int y)
{
    v.moveTop(y);
}

int QQmlRectValueType::width() const
{
    return v.width();
}

int QQmlRectValueType::height() const
{
    return v.height();
}

void QQmlRectValueType::setWidth(int w)
{
    v.setWidth(w);
}

void QQmlRectValueType::setHeight(int h)
{
    v.setHeight(h);
}

int QQmlRectValueType::left() const
{
    return v.left();
}

int QQmlRectValueType::right() const
{
    return v.right();
}

int QQmlRectValueType::top() const
{
    return v.top();
}

int QQmlRectValueType::bottom() const
{
    return v.bottom();
}

#if QT_CONFIG(easingcurve)
QQmlEasingEnums::Type QQmlEasingValueType::type() const
{
    return (QQmlEasingEnums::Type)v.type();
}

qreal QQmlEasingValueType::amplitude() const
{
    return v.amplitude();
}

qreal QQmlEasingValueType::overshoot() const
{
    return v.overshoot();
}

qreal QQmlEasingValueType::period() const
{
    return v.period();
}

void QQmlEasingValueType::setType(QQmlEasingEnums::Type type)
{
    v.setType((QEasingCurve::Type)type);
}

void QQmlEasingValueType::setAmplitude(qreal amplitude)
{
    v.setAmplitude(amplitude);
}

void QQmlEasingValueType::setOvershoot(qreal overshoot)
{
    v.setOvershoot(overshoot);
}

void QQmlEasingValueType::setPeriod(qreal period)
{
    v.setPeriod(period);
}

void QQmlEasingValueType::setBezierCurve(const QVariantList &customCurveVariant)
{
    if (customCurveVariant.isEmpty())
        return;

    if ((customCurveVariant.count() % 6) != 0)
        return;

    auto convert = [](const QVariant &v, qreal &r) {
        bool ok;
        r = v.toReal(&ok);
        return ok;
    };

    QEasingCurve newEasingCurve(QEasingCurve::BezierSpline);
    for (int i = 0, ei = customCurveVariant.size(); i < ei; i += 6) {
        qreal c1x, c1y, c2x, c2y, c3x, c3y;
        if (!convert(customCurveVariant.at(i    ), c1x)) return;
        if (!convert(customCurveVariant.at(i + 1), c1y)) return;
        if (!convert(customCurveVariant.at(i + 2), c2x)) return;
        if (!convert(customCurveVariant.at(i + 3), c2y)) return;
        if (!convert(customCurveVariant.at(i + 4), c3x)) return;
        if (!convert(customCurveVariant.at(i + 5), c3y)) return;

        const QPointF c1(c1x, c1y);
        const QPointF c2(c2x, c2y);
        const QPointF c3(c3x, c3y);

        newEasingCurve.addCubicBezierSegment(c1, c2, c3);
    }

    v = newEasingCurve;
}

QVariantList QQmlEasingValueType::bezierCurve() const
{
    QVariantList rv;
    const QVector<QPointF> points = v.toCubicSpline();
    rv.reserve(points.size() * 2);
    for (const auto &point : points)
        rv << QVariant(point.x()) << QVariant(point.y());
    return rv;
}
#endif // easingcurve


QT_END_NAMESPACE

#include "moc_qqmlvaluetype_p.cpp"
