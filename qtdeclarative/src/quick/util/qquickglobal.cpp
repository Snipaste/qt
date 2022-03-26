/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 BasysKom GmbH.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include <QtQuick/private/qquickvaluetypes_p.h>
#include <QtQuick/private/qquickapplication_p.h>
#include <QtQuick/private/qquickstate_p.h>
#include <QtQuick/private/qquickpropertychanges_p.h>
#include <QtQuick/private/qquickitemsmodule_p.h>
#include <QtQuick/private/qquickaccessiblefactory_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qfontdatabase.h>
#include <QtGui/qstylehints.h>

#include <QtQml/private/qqmlbinding_p.h>
#include <QtQml/private/qqmldebugserviceinterfaces_p.h>
#include <QtQml/private/qqmldebugstatesdelegate_p.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/private/qv4engine_p.h>
#include <QtQml/private/qv4object_p.h>
#include <QtQml/private/qqmlanybinding_p.h>

#include <QtCore/qiterable.h>

#ifdef Q_CC_MSVC
// MSVC2010 warns about 'unused variable t', even if it's used in t->~T()
#  pragma warning( disable : 4189 )
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(qml_debug)

class QQmlQtQuick2DebugStatesDelegate : public QQmlDebugStatesDelegate
{
public:
    QQmlQtQuick2DebugStatesDelegate();
    ~QQmlQtQuick2DebugStatesDelegate();
    void buildStatesList(bool cleanList, const QList<QPointer<QObject> > &instances) override;
    void updateBinding(QQmlContext *context,
                       const QQmlProperty &property,
                       const QVariant &expression, bool isLiteralValue,
                       const QString &fileName, int line, int column,
                       bool *isBaseState) override;
    bool setBindingForInvalidProperty(QObject *object,
                                      const QString &propertyName,
                                      const QVariant &expression,
                                      bool isLiteralValue) override;
    void resetBindingForInvalidProperty(QObject *object,
                                        const QString &propertyName) override;

private:
    void buildStatesList(QObject *obj);

    QList<QPointer<QQuickState> > m_allStates;
};

QQmlQtQuick2DebugStatesDelegate::QQmlQtQuick2DebugStatesDelegate()
{
}

QQmlQtQuick2DebugStatesDelegate::~QQmlQtQuick2DebugStatesDelegate()
{
}

void QQmlQtQuick2DebugStatesDelegate::buildStatesList(bool cleanList,
                                                      const QList<QPointer<QObject> > &instances)
{
    if (cleanList)
        m_allStates.clear();

    //only root context has all instances
    for (int ii = 0; ii < instances.count(); ++ii) {
        buildStatesList(instances.at(ii));
    }
}

void QQmlQtQuick2DebugStatesDelegate::buildStatesList(QObject *obj)
{
    if (QQuickState *state = qobject_cast<QQuickState *>(obj)) {
        m_allStates.append(state);
    }

    QObjectList children = obj->children();
    for (int ii = 0; ii < children.count(); ++ii) {
        buildStatesList(children.at(ii));
    }
}

void QQmlQtQuick2DebugStatesDelegate::updateBinding(QQmlContext *context,
                                                            const QQmlProperty &property,
                                                            const QVariant &expression, bool isLiteralValue,
                                                            const QString &fileName, int line, int column,
                                                            bool *inBaseState)
{
    Q_UNUSED(column);
    typedef QPointer<QQuickState> QuickStatePointer;
    QObject *object = property.object();
    QString propertyName = property.name();
    for (const QuickStatePointer& statePointer : qAsConst(m_allStates)) {
        if (QQuickState *state = statePointer.data()) {
            // here we assume that the revert list on itself defines the base state
            if (state->isStateActive() && state->containsPropertyInRevertList(object, propertyName)) {
                *inBaseState = false;

                QQmlAnyBinding newBinding;
                if (!isLiteralValue) {
                    newBinding = QQmlAnyBinding::createFromCodeString(property,
                                                     expression.toString(), object,
                                                     QQmlContextData::get(context), fileName,
                                                     line);
                }
                state->changeBindingInRevertList(object, propertyName, newBinding);

                if (isLiteralValue)
                    state->changeValueInRevertList(object, propertyName, expression);
            }
        }
    }
}

bool QQmlQtQuick2DebugStatesDelegate::setBindingForInvalidProperty(QObject *object,
                                                                           const QString &propertyName,
                                                                           const QVariant &expression,
                                                                           bool isLiteralValue)
{
    if (QQuickPropertyChanges *propertyChanges = qobject_cast<QQuickPropertyChanges *>(object)) {
        if (isLiteralValue)
            propertyChanges->changeValue(propertyName, expression);
        else
            propertyChanges->changeExpression(propertyName, expression.toString());
        return true;
    } else {
        return false;
    }
}

void QQmlQtQuick2DebugStatesDelegate::resetBindingForInvalidProperty(QObject *object, const QString &propertyName)
{
    if (QQuickPropertyChanges *propertyChanges = qobject_cast<QQuickPropertyChanges *>(object)) {
        propertyChanges->removeProperty(propertyName);
    }
}

static QQmlDebugStatesDelegate *statesDelegateFactory()
{
    return new QQmlQtQuick2DebugStatesDelegate;
}

#endif // QT_CONFIG(qml_debug)

class QQuickColorProvider : public QQmlColorProvider
{
public:
    QVariant colorFromString(const QString &s, bool *ok) override
    {
        QColor c(s);
        if (c.isValid()) {
            if (ok) *ok = true;
            return QVariant(c);
        }

        if (ok) *ok = false;
        return QVariant();
    }

    unsigned rgbaFromString(const QString &s, bool *ok) override
    {
        QColor c(s);
        if (c.isValid()) {
            if (ok) *ok = true;
            return c.rgba();
        }

        if (ok) *ok = false;
        return 0;
    }

    QString stringFromRgba(unsigned rgba)
    {
        QColor c(QColor::fromRgba(rgba));
        if (c.isValid()) {
            return QVariant(c).toString();
        }

        return QString();
    }

    QVariant fromRgbF(double r, double g, double b, double a) override
    {
        return QVariant(QColor::fromRgbF(r, g, b, a));
    }

    QVariant fromHslF(double h, double s, double l, double a) override
    {
        return QVariant(QColor::fromHslF(h, s, l, a));
    }

    QVariant fromHsvF(double h, double s, double v, double a) override
    {
        return QVariant(QColor::fromHsvF(h, s, v, a));
    }

    QVariant lighter(const QVariant &var, qreal factor) override
    {
        QColor color = var.value<QColor>();
        color = color.lighter(int(qRound(factor*100.)));
        return QVariant::fromValue(color);
    }

    QVariant darker(const QVariant &var, qreal factor) override
    {
        QColor color = var.value<QColor>();
        color = color.darker(int(qRound(factor*100.)));
        return QVariant::fromValue(color);
    }

    QVariant alpha(const QVariant &var, qreal value) override
    {
        QColor color = var.value<QColor>();
        color.setAlphaF(value);
        return QVariant::fromValue(color);
    }

    QVariant tint(const QVariant &baseVar, const QVariant &tintVar) override
    {
        QColor tintColor = tintVar.value<QColor>();

        int tintAlpha = tintColor.alpha();
        if (tintAlpha == 0xFF) {
            return tintVar;
        } else if (tintAlpha == 0x00) {
            return baseVar;
        }

        // tint the base color and return the final color
        QColor baseColor = baseVar.value<QColor>();
        qreal a = tintColor.alphaF();
        qreal inv_a = 1.0 - a;

        qreal r = tintColor.redF() * a + baseColor.redF() * inv_a;
        qreal g = tintColor.greenF() * a + baseColor.greenF() * inv_a;
        qreal b = tintColor.blueF() * a + baseColor.blueF() * inv_a;

        return QVariant::fromValue(QColor::fromRgbF(r, g, b, a + inv_a * baseColor.alphaF()));
    }
};

class QQuickGuiProvider : public QQmlGuiProvider
{
public:
    QQuickApplication *application(QObject *parent) override
    {
        return new QQuickApplication(parent);
    }

#if QT_CONFIG(im)
    QInputMethod *inputMethod() override
    {
        QInputMethod *im = qGuiApp->inputMethod();
        QQmlEngine::setObjectOwnership(im, QQmlEngine::CppOwnership);
        return im;
    }
#endif

    QStyleHints *styleHints() override
    {
        QStyleHints *sh = qGuiApp->styleHints();
        QQmlEngine::setObjectOwnership(sh, QQmlEngine::CppOwnership);
        return sh;
    }

    QStringList fontFamilies() override
    {
        return QFontDatabase::families();
    }

    bool openUrlExternally(const QUrl &url) override
    {
#ifndef QT_NO_DESKTOPSERVICES
        return QDesktopServices::openUrl(url);
#else
        Q_UNUSED(url);
        return false;
#endif
    }

    QString pluginName() const override
    {
        return QGuiApplication::platformName();
    }
};

static QQuickColorProvider *getColorProvider()
{
    static QQuickColorProvider colorProvider;
    return &colorProvider;
}

static QQuickGuiProvider *getGuiProvider()
{
    static QQuickGuiProvider guiProvider;
    return &guiProvider;
}

void QQuick_initializeModule()
{
    // This is used by QQuickPath, and on macOS it fails to automatically register.
    qRegisterMetaType<QVector<QVector<QPointF>>>();

    QQml_setColorProvider(getColorProvider());
    QQml_setGuiProvider(getGuiProvider());

    QQuickItemsModule::defineModule();

#if QT_CONFIG(accessibility)
    QAccessible::installFactory(&qQuickAccessibleFactory);
#endif

#if QT_CONFIG(qml_debug)
    QQmlEngineDebugService::setStatesDelegateFactory(statesDelegateFactory);
#endif
}

Q_CONSTRUCTOR_FUNCTION(QQuick_initializeModule)

QT_END_NAMESPACE
