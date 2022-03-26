/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

/*!
    \class QPropertyAnimation
    \inmodule QtCore
    \brief The QPropertyAnimation class animates Qt properties.
    \since 4.6

    \ingroup animation

    QPropertyAnimation interpolates over \l{Qt's Property System}{Qt
    properties}. As property values are stored in \l{QVariant}s, the
    class inherits QVariantAnimation, and supports animation of the
    same \l{QMetaType::Type}{meta types} as its super class.

    A class declaring properties must be a QObject. To make it
    possible to animate a property, it must provide a setter (so that
    QPropertyAnimation can set the property's value). Note that this
    makes it possible to animate many of Qt's widgets. Let's look at
    an example:

    \snippet code/src_corelib_animation_qpropertyanimation.cpp 0

    The property name and the QObject instance of which property
    should be animated are passed to the constructor. You can then
    specify the start and end value of the property. The procedure is
    equal for properties in classes you have implemented
    yourself--just check with QVariantAnimation that your QVariant
    type is supported.

    The QVariantAnimation class description explains how to set up the
    animation in detail. Note, however, that if a start value is not
    set, the property will start at the value it had when the
    QPropertyAnimation instance was created.

    QPropertyAnimation works like a charm on its own. For complex
    animations that, for instance, contain several objects,
    QAnimationGroup is provided. An animation group is an animation
    that can contain other animations, and that can manage when its
    animations are played. Look at QParallelAnimationGroup for an
    example.

    \sa QVariantAnimation, QAnimationGroup, {The Animation Framework}
*/

#include "qpropertyanimation.h"
#include "qanimationgroup.h"
#include "qpropertyanimation_p.h"

#include <QtCore/QMutex>
#include <QtCore/private/qlocking_p.h>

QT_BEGIN_NAMESPACE

void QPropertyAnimationPrivate::updateMetaProperty()
{
    if (!targetObject || propertyName.value().isEmpty()) {
        propertyType = QMetaType::UnknownType;
        propertyIndex = -1;
        return;
    }

    //propertyType will be set to a valid type only if there is a Q_PROPERTY
    //otherwise it will be set to QVariant::Invalid at the end of this function
    propertyType = targetObject->property(propertyName.value()).userType();
    propertyIndex = targetObject->metaObject()->indexOfProperty(propertyName.value());

    if (propertyType != QMetaType::UnknownType)
        convertValues(propertyType);
    if (propertyIndex == -1) {
        //there is no Q_PROPERTY on the object
        propertyType = QMetaType::UnknownType;
        if (!targetObject->dynamicPropertyNames().contains(propertyName))
            qWarning("QPropertyAnimation: you're trying to animate a non-existing property %s of "
                     "your QObject",
                     propertyName.value().constData());
    } else if (!targetObject->metaObject()->property(propertyIndex).isWritable()) {
        qWarning("QPropertyAnimation: you're trying to animate the non-writable property %s of "
                 "your QObject",
                 propertyName.value().constData());
    }
}

void QPropertyAnimationPrivate::updateProperty(const QVariant &newValue)
{
    if (state == QAbstractAnimation::Stopped)
        return;

    if (!targetObject)
        return;

    if (newValue.userType() == propertyType) {
        //no conversion is needed, we directly call the QMetaObject::metacall
        //check QMetaProperty::write for an explanation of these
        int status = -1;
        int flags = 0;
        void *argv[] = { const_cast<void *>(newValue.constData()), const_cast<QVariant *>(&newValue), &status, &flags };
        QMetaObject::metacall(targetObject, QMetaObject::WriteProperty, propertyIndex, argv);
    } else {
        targetObject->setProperty(propertyName.value().constData(), newValue);
    }
}

/*!
    Construct a QPropertyAnimation object. \a parent is passed to QObject's
    constructor.
*/
QPropertyAnimation::QPropertyAnimation(QObject *parent)
    : QVariantAnimation(*new QPropertyAnimationPrivate, parent)
{
}

/*!
    Construct a QPropertyAnimation object. \a parent is passed to QObject's
    constructor. The animation changes the property \a propertyName on \a
    target. The default duration is 250ms.

    \sa targetObject, propertyName
*/
QPropertyAnimation::QPropertyAnimation(QObject *target, const QByteArray &propertyName, QObject *parent)
    : QVariantAnimation(*new QPropertyAnimationPrivate, parent)
{
    setTargetObject(target);
    setPropertyName(propertyName);
}

/*!
    Destroys the QPropertyAnimation instance.
 */
QPropertyAnimation::~QPropertyAnimation()
{
    stop();
}

/*!
    \property QPropertyAnimation::targetObject
    \brief the target QObject for this animation.

    This property defines the target QObject for this animation.
 */
QObject *QPropertyAnimation::targetObject() const
{
    return d_func()->targetObject;
}

QBindable<QObject *> QPropertyAnimation::bindableTargetObject()
{
    return &d_func()->targetObject;
}

void QPropertyAnimation::setTargetObject(QObject *target)
{
    Q_D(QPropertyAnimation);
    if (d->state != QAbstractAnimation::Stopped) {
        qWarning("QPropertyAnimation::setTargetObject: you can't change the target of a running animation");
        return;
    }

    d->targetObject.removeBindingUnlessInWrapper();
    if (d->targetObject == target)
        return;

    if (d->targetObject != nullptr)
        QObject::disconnect(d->targetObject, &QObject::destroyed, this, nullptr);
    d->targetObject.setValueBypassingBindings(target);

    if (d->targetObject != nullptr) {
        QObject::connect(d->targetObject, &QObject::destroyed, this,
                         [d] { d->targetObjectDestroyed(); });
    }
    d->updateMetaProperty();
    d->targetObject.notify();
}

/*!
    \property QPropertyAnimation::propertyName
    \brief the target property name for this animation

    This property defines the target property name for this animation. The
    property name is required for the animation to operate.
 */
QByteArray QPropertyAnimation::propertyName() const
{
    Q_D(const QPropertyAnimation);
    return d->propertyName;
}

void QPropertyAnimation::setPropertyName(const QByteArray &propertyName)
{
    Q_D(QPropertyAnimation);
    if (d->state != QAbstractAnimation::Stopped) {
        qWarning("QPropertyAnimation::setPropertyName: you can't change the property name of a running animation");
        return;
    }

    d->propertyName.removeBindingUnlessInWrapper();

    if (d->propertyName == propertyName)
        return;

    d->propertyName.setValueBypassingBindings(propertyName);
    d->updateMetaProperty();
    d->propertyName.notify();
}

QBindable<QByteArray> QPropertyAnimation::bindablePropertyName()
{
    return &d_func()->propertyName;
}

/*!
    \reimp
 */
bool QPropertyAnimation::event(QEvent *event)
{
    return QVariantAnimation::event(event);
}

/*!
    This virtual function is called by QVariantAnimation whenever the current value
    changes. \a value is the new, updated value. It updates the current value
    of the property on the target object.

    \sa currentValue, currentTime
 */
void QPropertyAnimation::updateCurrentValue(const QVariant &value)
{
    Q_D(QPropertyAnimation);
    d->updateProperty(value);
}

/*!
    \reimp

    If the startValue is not defined when the state of the animation changes from Stopped to Running,
    the current property value is used as the initial value for the animation.
*/
void QPropertyAnimation::updateState(QAbstractAnimation::State newState,
                                     QAbstractAnimation::State oldState)
{
    Q_D(QPropertyAnimation);

    if (!d->targetObject && oldState == Stopped) {
        qWarning("QPropertyAnimation::updateState (%s): Changing state of an animation without "
                 "target",
                 d->propertyName.value().constData());
        return;
    }

    QVariantAnimation::updateState(newState, oldState);

    QPropertyAnimation *animToStop = nullptr;
    {
        static QBasicMutex mutex;
        auto locker = qt_unique_lock(mutex);
        typedef QPair<QObject *, QByteArray> QPropertyAnimationPair;
        typedef QHash<QPropertyAnimationPair, QPropertyAnimation*> QPropertyAnimationHash;
        static QPropertyAnimationHash hash;

        // in case the targetObject gets deleted, the following happens:
        // 1. targetObject's destroyed signal calls our targetObjectDestroyed.
        // 2. targetObjectDestroyed calls stop()
        // 3. QAbstractAnimation::stop() calls setState(Stopped)
        // 4. setState(Stopped) calls updateState(newState, oldState)
        // 5. we arrive here. d->targetObject is not yet set to nullptr, we can safely use it.
        Q_ASSERT(d->targetObject);

        QPropertyAnimationPair key(d->targetObject, d->propertyName);
        if (newState == Running) {
            d->updateMetaProperty();
            animToStop = hash.value(key, nullptr);
            hash.insert(key, this);
            locker.unlock();
            // update the default start value
            if (oldState == Stopped) {
                d->setDefaultStartEndValue(
                        d->targetObject->property(d->propertyName.value().constData()));
                //let's check if we have a start value and an end value
                const char *what = nullptr;
                if (!startValue().isValid() && (d->direction == Backward || !d->defaultStartEndValue.isValid())) {
                    what = "start";
                }
                if (!endValue().isValid() && (d->direction == Forward || !d->defaultStartEndValue.isValid())) {
                    if (what)
                        what = "start and end";
                    else
                        what = "end";
                }
                if (Q_UNLIKELY(what)) {
                    qWarning("QPropertyAnimation::updateState (%s, %s, %ls): starting an animation "
                             "without %s value",
                             d->propertyName.value().constData(),
                             d->targetObject->metaObject()->className(),
                             qUtf16Printable(d->targetObject->objectName()), what);
                }
            }
        } else if (hash.value(key) == this) {
            hash.remove(key);
        }
    }

    //we need to do that after the mutex was unlocked
    if (animToStop) {
        // try to stop the top level group
        QAbstractAnimation *current = animToStop;
        while (current->group() && current->state() != Stopped)
            current = current->group();
        current->stop();
    }
}

QT_END_NAMESPACE

#include "moc_qpropertyanimation.cpp"
