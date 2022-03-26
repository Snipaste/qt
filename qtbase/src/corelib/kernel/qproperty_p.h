/***************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QPROPERTY_P_H
#define QPROPERTY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include <qproperty.h>

#include <qscopedpointer.h>
#include <qscopedvaluerollback.h>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
    Q_CORE_EXPORT bool isAnyBindingEvaluating();
}

// Keep all classes related to QProperty in one compilation unit. Performance of this code is crucial and
// we need to allow the compiler to inline where it makes sense.

// This is a helper "namespace"
struct Q_AUTOTEST_EXPORT QPropertyBindingDataPointer
{
    const QtPrivate::QPropertyBindingData *ptr = nullptr;

    QPropertyBindingPrivate *binding() const
    {
        return ptr->binding();
    }

    void setObservers(QPropertyObserver *observer)
    {
        auto &d = ptr->d_ref();
        observer->prev = reinterpret_cast<QPropertyObserver**>(&d);
        d = reinterpret_cast<quintptr>(observer);
    }
    static void fixupAfterMove(QtPrivate::QPropertyBindingData *ptr);
    void Q_ALWAYS_INLINE addObserver(QPropertyObserver *observer);
    void setFirstObserver(QPropertyObserver *observer);
    QPropertyObserverPointer firstObserver() const;

    int observerCount() const;

    template <typename T>
    static QPropertyBindingDataPointer get(QProperty<T> &property)
    {
        return QPropertyBindingDataPointer{&property.bindingData()};
    }
};

// This is a helper "namespace"
struct QPropertyObserverPointer
{
    QPropertyObserver *ptr = nullptr;

    void unlink();
    void unlink_fast();

    void setBindingToNotify(QPropertyBindingPrivate *binding);
    void setBindingToNotify_unsafe(QPropertyBindingPrivate *binding);
    void setChangeHandler(QPropertyObserver::ChangeHandler changeHandler);

    void notify(QUntypedPropertyData *propertyDataPtr);
#ifndef QT_NO_DEBUG
    void noSelfDependencies(QPropertyBindingPrivate *binding);
#else
    void noSelfDependencies(QPropertyBindingPrivate *) {}
#endif
    void evaluateBindings(QBindingStatus *status);
    void observeProperty(QPropertyBindingDataPointer property);

    explicit operator bool() const { return ptr != nullptr; }

    QPropertyObserverPointer nextObserver() const { return {ptr->next.data()}; }
};

class QPropertyBindingErrorPrivate : public QSharedData
{
public:
    QPropertyBindingError::Type type = QPropertyBindingError::NoError;
    QString description;
};

namespace QtPrivate {

struct BindingEvaluationState
{
    BindingEvaluationState(QPropertyBindingPrivate *binding, QBindingStatus *status);
    ~BindingEvaluationState()
    {
        *currentState = previousState;
    }

    QPropertyBindingPrivate *binding;
    BindingEvaluationState *previousState = nullptr;
    BindingEvaluationState **currentState = nullptr;
};

/*!
 * \internal
 * CompatPropertySafePoint needs to be constructed before the setter of
 * a QObjectCompatProperty runs. It prevents spurious binding dependencies
 * caused by reads of properties inside the compat property setter.
 * Moreover, it ensures that we don't destroy bindings when using operator=
 */
struct CompatPropertySafePoint
{
    Q_CORE_EXPORT CompatPropertySafePoint(QBindingStatus *status, QUntypedPropertyData *property);
    ~CompatPropertySafePoint()
    {
        *currentState = previousState;
        *currentlyEvaluatingBindingList = bindingState;
    }
    QUntypedPropertyData *property;
    CompatPropertySafePoint *previousState = nullptr;
    CompatPropertySafePoint **currentState = nullptr;
    QtPrivate::BindingEvaluationState **currentlyEvaluatingBindingList = nullptr;
    QtPrivate::BindingEvaluationState *bindingState = nullptr;
};

}

class Q_CORE_EXPORT QPropertyBindingPrivate : public QtPrivate::RefCounted
{
private:
    friend struct QPropertyBindingDataPointer;
    friend class QPropertyBindingPrivatePtr;

    using ObserverArray = std::array<QPropertyObserver, 4>;

private:

    // used to detect binding loops for lazy evaluated properties
    bool updating = false;
    bool hasStaticObserver = false;
    bool pendingNotify = false;
    bool hasBindingWrapper:1;
    // used to detect binding loops for eagerly evaluated properties
    bool isQQmlPropertyBinding:1;
    /* a sticky binding does not get removed in removeBinding
       this is used to support QQmlPropertyData::DontRemoveBinding
       in qtdeclarative
    */
    bool m_sticky:1;

    const QtPrivate::BindingFunctionVTable *vtable;

    union {
        QtPrivate::QPropertyObserverCallback staticObserverCallback = nullptr;
        QtPrivate::QPropertyBindingWrapper staticBindingWrapper;
    };
    ObserverArray inlineDependencyObservers; // for things we are observing

    QPropertyObserverPointer firstObserver; // list of observers observing us
    QScopedPointer<std::vector<QPropertyObserver>> heapObservers; // for things we are observing

protected:
    QUntypedPropertyData *propertyDataPtr = nullptr;

    /* For bindings set up from C++, location stores where the binding was created in the C++ source
       For QQmlPropertyBinding that information does not make sense, and the location in the QML  file
       is stored somewhere else. To make efficient use of the space, we instead provide a scratch space
       for QQmlPropertyBinding (which stores further binding information there).
       Anything stored in the union must be trivially destructible.
       (checked in qproperty.cpp)
    */
    using DeclarativeErrorCallback = void(*)(QPropertyBindingPrivate *);
    union {
        QPropertyBindingSourceLocation location;
        struct {
            std::byte declarativeExtraData[sizeof(QPropertyBindingSourceLocation) - sizeof(DeclarativeErrorCallback)];
            DeclarativeErrorCallback errorCallBack;
        };
    };
private:
    QPropertyBindingError error;

    QMetaType metaType;

public:
    static constexpr size_t getSizeEnsuringAlignment() {
        constexpr auto align = alignof (std::max_align_t) - 1;
        constexpr size_t sizeEnsuringAlignment = (sizeof(QPropertyBindingPrivate) + align) & ~align;
        static_assert (sizeEnsuringAlignment % alignof (std::max_align_t) == 0,
                   "Required for placement new'ing the function behind it.");
        return sizeEnsuringAlignment;
    }


    // public because the auto-tests access it, too.
    size_t dependencyObserverCount = 0;

    bool isUpdating() {return updating;}
    void setSticky(bool keep = true) {m_sticky = keep;}
    bool isSticky() {return m_sticky;}

    QPropertyBindingPrivate(QMetaType metaType, const QtPrivate::BindingFunctionVTable *vtable,
                            const QPropertyBindingSourceLocation &location, bool isQQmlPropertyBinding=false)
        : hasBindingWrapper(false)
        , isQQmlPropertyBinding(isQQmlPropertyBinding)
        , m_sticky(false)
        , vtable(vtable)
        , location(location)
        , metaType(metaType)
    {}
    ~QPropertyBindingPrivate();


    void setProperty(QUntypedPropertyData *propertyPtr) { propertyDataPtr = propertyPtr; }
    void setStaticObserver(QtPrivate::QPropertyObserverCallback callback, QtPrivate::QPropertyBindingWrapper bindingWrapper)
    {
        Q_ASSERT(!(callback && bindingWrapper));
        if (callback) {
            hasStaticObserver = true;
            hasBindingWrapper = false;
            staticObserverCallback = callback;
        } else if (bindingWrapper) {
            hasStaticObserver = false;
            hasBindingWrapper = true;
            staticBindingWrapper = bindingWrapper;
        } else {
            hasStaticObserver = false;
            hasBindingWrapper = false;
            staticObserverCallback = nullptr;
        }
    }
    void prependObserver(QPropertyObserverPointer observer)
    {
        observer.ptr->prev = const_cast<QPropertyObserver **>(&firstObserver.ptr);
        firstObserver = observer;
    }

    QPropertyObserverPointer takeObservers()
    {
        auto observers = firstObserver;
        firstObserver.ptr = nullptr;
        return observers;
    }

    void clearDependencyObservers() {
        for (size_t i = 0; i < qMin(dependencyObserverCount, inlineDependencyObservers.size()); ++i) {
            QPropertyObserverPointer p{&inlineDependencyObservers[i]};
            p.unlink_fast();
        }
        if (heapObservers)
            heapObservers->clear();
        dependencyObserverCount = 0;
    }

    Q_ALWAYS_INLINE QPropertyObserverPointer allocateDependencyObserver() {
        if (dependencyObserverCount < inlineDependencyObservers.size()) {
            ++dependencyObserverCount;
            return {&inlineDependencyObservers[dependencyObserverCount - 1]};
        }
        return allocateDependencyObserver_slow();
    }

    Q_NEVER_INLINE QPropertyObserverPointer allocateDependencyObserver_slow()
    {
        ++dependencyObserverCount;
        if (!heapObservers)
            heapObservers.reset(new std::vector<QPropertyObserver>());
        return {&heapObservers->emplace_back()};
    }

    QPropertyBindingSourceLocation sourceLocation() const
    {
        if (!hasCustomVTable())
            return this->location;
        QPropertyBindingSourceLocation location;
        constexpr auto msg = "Custom location";
        location.fileName = msg;
        return location;
    }
    QPropertyBindingError bindingError() const { return error; }
    QMetaType valueMetaType() const { return metaType; }

    void unlinkAndDeref();

    void evaluateRecursive(QBindingStatus *status = nullptr);
    void Q_ALWAYS_INLINE evaluateRecursive_inline(QBindingStatus *status);

    void notifyRecursive();

    static QPropertyBindingPrivate *get(const QUntypedPropertyBinding &binding)
    { return static_cast<QPropertyBindingPrivate *>(binding.d.data()); }

    void setError(QPropertyBindingError &&e)
    { error = std::move(e); }

    void detachFromProperty()
    {
        hasStaticObserver = false;
        hasBindingWrapper = false;
        propertyDataPtr = nullptr;
        clearDependencyObservers();
    }

    static QPropertyBindingPrivate *currentlyEvaluatingBinding();

    bool hasCustomVTable() const
    {
        return vtable->size == 0;
    }

    static void destroyAndFreeMemory(QPropertyBindingPrivate *priv) {
        if (priv->hasCustomVTable()) {
            // special hack for QQmlPropertyBinding which has a
            // different memory layout than normal QPropertyBindings
            priv->vtable->destroy(priv);
        } else{
            priv->~QPropertyBindingPrivate();
            delete[] reinterpret_cast<std::byte *>(priv);
        }
    }
};

inline void QPropertyBindingDataPointer::setFirstObserver(QPropertyObserver *observer)
{
    if (auto *b = binding()) {
        b->firstObserver.ptr = observer;
        return;
    }
    auto &d = ptr->d_ref();
    d = reinterpret_cast<quintptr>(observer);
}

inline void QPropertyBindingDataPointer::fixupAfterMove(QtPrivate::QPropertyBindingData *ptr)
{
    auto &d = ptr->d_ref();
    if (ptr->isNotificationDelayed()) {
        QPropertyProxyBindingData *proxyData
                = reinterpret_cast<QPropertyProxyBindingData*>(d & ~QtPrivate::QPropertyBindingData::BindingBit);
        proxyData->originalBindingData = ptr;
    }
    // If QPropertyBindingData has been moved, and it has an observer
    // we have to adjust the firstObserver's prev pointer to point to
    // the moved to QPropertyBindingData's d_ptr
    if (d & QtPrivate::QPropertyBindingData::BindingBit)
        return; // nothing to do if the observer is stored in the binding
    if (auto observer = reinterpret_cast<QPropertyObserver *>(d))
        observer->prev = reinterpret_cast<QPropertyObserver **>(&d);
}

inline QPropertyObserverPointer QPropertyBindingDataPointer::firstObserver() const
{
    if (auto *b = binding())
        return b->firstObserver;
    return { reinterpret_cast<QPropertyObserver *>(ptr->d()) };
}

namespace QtPrivate {
    Q_CORE_EXPORT bool isPropertyInBindingWrapper(const QUntypedPropertyData *property);
    void Q_CORE_EXPORT initBindingStatusThreadId();
}

template<typename Class, typename T, auto Offset, auto Setter, auto Signal=nullptr>
class QObjectCompatProperty : public QPropertyData<T>
{
    template<typename Property, typename>
    friend class QtPrivate::QBindableInterfaceForProperty;

    using ThisType = QObjectCompatProperty<Class, T, Offset, Setter, Signal>;
    using SignalTakesValue = std::is_invocable<decltype(Signal), Class, T>;
    Class *owner()
    {
        char *that = reinterpret_cast<char *>(this);
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }
    const Class *owner() const
    {
        char *that = const_cast<char *>(reinterpret_cast<const char *>(this));
        return reinterpret_cast<Class *>(that - QtPrivate::detail::getOffset(Offset));
    }

    static bool bindingWrapper(QMetaType type, QUntypedPropertyData *dataPtr, QtPrivate::QPropertyBindingFunction binding)
    {
        auto *thisData = static_cast<ThisType *>(dataPtr);
        QPropertyData<T> copy;
        binding.vtable->call(type, &copy, binding.functor);
        if constexpr (QTypeTraits::has_operator_equal_v<T>)
            if (copy.valueBypassingBindings() == thisData->valueBypassingBindings())
                return false;
        // ensure value and setValue know we're currently evaluating our binding
        QBindingStorage *storage = qGetBindingStorage(thisData->owner());
        QtPrivate::CompatPropertySafePoint guardThis(storage->bindingStatus, thisData);
        (thisData->owner()->*Setter)(copy.valueBypassingBindings());
        return true;
    }
    bool inBindingWrapper(const QBindingStorage *storage) const
    {
        return storage->bindingStatus->currentCompatProperty
            && QtPrivate::isPropertyInBindingWrapper(this);
    }

public:
    using value_type = typename QPropertyData<T>::value_type;
    using parameter_type = typename QPropertyData<T>::parameter_type;
    using arrow_operator_result = typename QPropertyData<T>::arrow_operator_result;

    QObjectCompatProperty() = default;
    explicit QObjectCompatProperty(const T &initialValue) : QPropertyData<T>(initialValue) {}
    explicit QObjectCompatProperty(T &&initialValue) : QPropertyData<T>(std::move(initialValue)) {}

    parameter_type value() const
    {
        const QBindingStorage *storage = qGetBindingStorage(owner());
        // make sure we don't register this binding as a dependency to itself
        if (storage->bindingStatus->currentlyEvaluatingBinding && !inBindingWrapper(storage))
            storage->registerDependency_helper(this);
        return this->val;
    }

    arrow_operator_result operator->() const
    {
        if constexpr (QTypeTraits::is_dereferenceable_v<T>) {
            return value();
        } else if constexpr (std::is_pointer_v<T>) {
            value();
            return this->val;
        } else {
            return;
        }
    }

    parameter_type operator*() const
    {
        return value();
    }

    operator parameter_type() const
    {
        return value();
    }

    void setValue(parameter_type t)
    {
        QBindingStorage *storage = qGetBindingStorage(owner());
        if (auto *bd = storage->bindingData(this)) {
            // make sure we don't remove the binding if called from the bindingWrapper
            if (bd->hasBinding() && !inBindingWrapper(storage))
                bd->removeBinding_helper();
        }
        this->val = t;
    }

    QObjectCompatProperty &operator=(parameter_type newValue)
    {
        setValue(newValue);
        return *this;
    }

    QPropertyBinding<T> setBinding(const QPropertyBinding<T> &newBinding)
    {
        QtPrivate::QPropertyBindingData *bd = qGetBindingStorage(owner())->bindingData(this, true);
        QUntypedPropertyBinding oldBinding(bd->setBinding(newBinding, this, nullptr, bindingWrapper));
        // notification is already handled in QPropertyBindingData::setBinding
        return static_cast<QPropertyBinding<T> &>(oldBinding);
    }

    bool setBinding(const QUntypedPropertyBinding &newBinding)
    {
        if (!newBinding.isNull() && newBinding.valueMetaType() != QMetaType::fromType<T>())
            return false;
        setBinding(static_cast<const QPropertyBinding<T> &>(newBinding));
        return true;
    }

#ifndef Q_CLANG_QDOC
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor &&f,
                                   const QPropertyBindingSourceLocation &location = QT_PROPERTY_DEFAULT_BINDING_LOCATION,
                                   std::enable_if_t<std::is_invocable_v<Functor>> * = nullptr)
    {
        return setBinding(Qt::makePropertyBinding(std::forward<Functor>(f), location));
    }
#else
    template <typename Functor>
    QPropertyBinding<T> setBinding(Functor f);
#endif

    bool hasBinding() const {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return bd && bd->binding() != nullptr;
    }

    void removeBindingUnlessInWrapper()
    {
        QBindingStorage *storage = qGetBindingStorage(owner());
        if (auto *bd = storage->bindingData(this)) {
            // make sure we don't remove the binding if called from the bindingWrapper
            if (bd->hasBinding() && !inBindingWrapper(storage))
                bd->removeBinding_helper();
        }
    }

    void notify()
    {
        QBindingStorage *storage = qGetBindingStorage(owner());
        if (auto bd = storage->bindingData(this, false)) {
            if (!inBindingWrapper(storage))
                notify(bd);
        }
        if constexpr (Signal != nullptr) {
            if constexpr (SignalTakesValue::value)
                (owner()->*Signal)(value());
            else
                (owner()->*Signal)();
        }
    }

    QPropertyBinding<T> binding() const
    {
        auto *bd = qGetBindingStorage(owner())->bindingData(this);
        return static_cast<QPropertyBinding<T> &&>(QUntypedPropertyBinding(bd ? bd->binding() : nullptr));
    }

    QPropertyBinding<T> takeBinding()
    {
        return setBinding(QPropertyBinding<T>());
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> onValueChanged(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyChangeHandler<Functor>(*this, f);
    }

    template<typename Functor>
    QPropertyChangeHandler<Functor> subscribe(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        f();
        return onValueChanged(f);
    }

    template<typename Functor>
    QPropertyNotifier addNotifier(Functor f)
    {
        static_assert(std::is_invocable_v<Functor>, "Functor callback must be callable without any parameters");
        return QPropertyNotifier(*this, f);
    }

    QtPrivate::QPropertyBindingData &bindingData() const
    {
        auto *storage = const_cast<QBindingStorage *>(qGetBindingStorage(owner()));
        return *storage->bindingData(const_cast<QObjectCompatProperty *>(this), true);
    }

private:
    void notify(const QtPrivate::QPropertyBindingData *binding)
    {
        if (binding)
            binding->notifyObservers(this, qGetBindingStorage(owner()));
    }
};

namespace QtPrivate {
template<typename Class, typename Ty, auto Offset, auto Setter, auto Signal>
class QBindableInterfaceForProperty<QObjectCompatProperty<Class, Ty, Offset, Setter, Signal>, std::void_t<Class>>
{
    using Property = QObjectCompatProperty<Class, Ty, Offset, Setter, Signal>;
    using T = typename Property::value_type;
public:
    static constexpr QBindableInterface iface = {
        [](const QUntypedPropertyData *d, void *value) -> void
        { *static_cast<T*>(value) = static_cast<const Property *>(d)->value(); },
        [](QUntypedPropertyData *d, const void *value) -> void
        {
            (static_cast<Property *>(d)->owner()->*Setter)(*static_cast<const T*>(value));
        },
        [](const QUntypedPropertyData *d) -> QUntypedPropertyBinding
        { return static_cast<const Property *>(d)->binding(); },
        [](QUntypedPropertyData *d, const QUntypedPropertyBinding &binding) -> QUntypedPropertyBinding
        { return static_cast<Property *>(d)->setBinding(static_cast<const QPropertyBinding<T> &>(binding)); },
        [](const QUntypedPropertyData *d, const QPropertyBindingSourceLocation &location) -> QUntypedPropertyBinding
        { return Qt::makePropertyBinding([d]() -> T { return static_cast<const Property *>(d)->value(); }, location); },
        [](const QUntypedPropertyData *d, QPropertyObserver *observer) -> void
        { observer->setSource(static_cast<const Property *>(d)->bindingData()); },
        []() { return QMetaType::fromType<T>(); }
    };
};
}

#define QT_OBJECT_COMPAT_PROPERTY_4(Class, Type, name,  setter) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter> name;

#define QT_OBJECT_COMPAT_PROPERTY_5(Class, Type, name,  setter, signal) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter, signal> name;

#define Q_OBJECT_COMPAT_PROPERTY(...) \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(QT_OBJECT_COMPAT_PROPERTY, __VA_ARGS__) \
    QT_WARNING_POP

#define QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS_5(Class, Type, name,  setter, value) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter> name =         \
            QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter>(       \
                    value);

#define QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS_6(Class, Type, name, setter, signal, value) \
    static constexpr size_t _qt_property_##name##_offset() { \
        QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
        return offsetof(Class, name); \
        QT_WARNING_POP \
    } \
    QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter, signal> name = \
            QObjectCompatProperty<Class, Type, Class::_qt_property_##name##_offset, setter,        \
                                  signal>(value);

#define Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(...) \
    QT_WARNING_PUSH QT_WARNING_DISABLE_INVALID_OFFSETOF \
    QT_OVERLOADED_MACRO(QT_OBJECT_COMPAT_PROPERTY_WITH_ARGS, __VA_ARGS__) \
    QT_WARNING_POP


namespace QtPrivate {
Q_CORE_EXPORT BindingEvaluationState *suspendCurrentBindingStatus();
Q_CORE_EXPORT void restoreBindingStatus(BindingEvaluationState *status);
}

struct QUntypedBindablePrivate
{
    static QtPrivate::QBindableInterface const *getInterface(const QUntypedBindable &bindable)
    {
        return bindable.iface;
    }

    static QUntypedPropertyData *getPropertyData(const QUntypedBindable &bindable)
    {
        return bindable.data;
    }
};

inline void QPropertyBindingPrivate::evaluateRecursive_inline(QBindingStatus *status)
{
    if (updating) {
        error = QPropertyBindingError(QPropertyBindingError::BindingLoop);
        if (isQQmlPropertyBinding)
            errorCallBack(this);
        return;
    }

    /*
     * Evaluating the binding might lead to the binding being broken. This can
     * cause ref to reach zero at the end of the function.  However, the
     * updateGuard's destructor will then still trigger, trying to set the
     * updating bool to its old value
     * To prevent this, we create a QPropertyBindingPrivatePtr which ensures
     * that the object is still alive when updateGuard's dtor runs.
     */
    QPropertyBindingPrivatePtr keepAlive {this};

    QScopedValueRollback<bool> updateGuard(updating, true);

    QtPrivate::BindingEvaluationState evaluationFrame(this, status);

    auto bindingFunctor =  reinterpret_cast<std::byte *>(this) +
            QPropertyBindingPrivate::getSizeEnsuringAlignment();
    bool changed = false;
    if (hasBindingWrapper) {
        changed = staticBindingWrapper(metaType, propertyDataPtr,
                                       {vtable, bindingFunctor});
    } else {
        changed = vtable->call(metaType, propertyDataPtr, bindingFunctor);
    }
    // If there was a change, we must set pendingNotify.
    // If there was not, we must not clear it, as that only should happen in notifyRecursive
    pendingNotify = pendingNotify || changed;
    if (!changed || !firstObserver)
        return;

    firstObserver.noSelfDependencies(this);
    firstObserver.evaluateBindings(status);
}

QT_END_NAMESPACE

#endif // QPROPERTY_P_H
