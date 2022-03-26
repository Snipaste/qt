/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
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

#include <QtCore/QFileSelector>
#include <QtQml/QQmlAbstractUrlInterceptor>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qqmlapplicationengine_p.h>
#include <qobjectdefs.h>
#include "qqmlfileselector.h"
#include "qqmlfileselector_p.h"
#include "qqmlengine_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
   \class QQmlFileSelector
   \since 5.2
   \inmodule QtQml
   \brief A class for applying a QFileSelector to QML file loading.

  QQmlFileSelector will automatically apply a QFileSelector to
  qml file and asset paths.

  It is used as follows:

  \code
  QQmlEngine engine;
  QQmlFileSelector* selector = new QQmlFileSelector(&engine);
  \endcode

  Then you can swap out files like so:
  \code
  main.qml
  Component.qml
  asset.png
  +unix/Component.qml
  +mac/asset.png
  \endcode

  In this example, main.qml will normally use Component.qml for the Component type. However on a
  unix platform, the unix selector will be present and the +unix/Component.qml version will be
  used instead. Note that this acts like swapping out Component.qml with +unix/Component.qml, so
  when using Component.qml you should not need to alter any paths based on which version was
  selected.

  For example, to pass the "asset.png" file path around you would refer to it just as "asset.png" in
  all of main.qml, Component.qml, and +linux/Component.qml. It will be replaced with +mac/asset.png
  on Mac platforms in all cases.

  For a list of available selectors, see \c QFileSelector.

  Your platform may also provide additional selectors for you to use. As specified by QFileSelector,
  directories used for selection must start with a '+' character, so you will not accidentally
  trigger this feature unless you have directories with such names inside your project.

  If a new QQmlFileSelector is set on the engine, the old one will be replaced.
 */

/*!
  Creates a new QQmlFileSelector with parent object \a parent, which includes
  its own QFileSelector. \a engine is the QQmlEngine you wish to apply file
  selectors to. It will also take ownership of the QQmlFileSelector.
*/

QQmlFileSelector::QQmlFileSelector(QQmlEngine* engine, QObject* parent)
    : QObject(*(new QQmlFileSelectorPrivate), parent)
{
    Q_D(QQmlFileSelector);
    d->engine = engine;
    d->engine->addUrlInterceptor(d->myInstance.data());
}

/*!
   Destroys the QQmlFileSelector object.
*/
QQmlFileSelector::~QQmlFileSelector()
{
    Q_D(QQmlFileSelector);
    if (d->engine) {
        d->engine->removeUrlInterceptor(d->myInstance.data());
        d->engine = nullptr;
    }
}

/*!
  \since 5.7
  Returns the QFileSelector instance used by the QQmlFileSelector.
*/
QFileSelector *QQmlFileSelector::selector() const noexcept
{
    Q_D(const QQmlFileSelector);
    return d->selector;
}

QQmlFileSelectorPrivate::QQmlFileSelectorPrivate()
{
    Q_Q(QQmlFileSelector);
    ownSelector = true;
    selector = new QFileSelector(q);
    myInstance.reset(new QQmlFileSelectorInterceptor(this));
}

QQmlFileSelectorPrivate::~QQmlFileSelectorPrivate()
{
    if (ownSelector)
        delete selector;
}

/*!
  Sets the QFileSelector instance for use by the QQmlFileSelector to \a selector.
  QQmlFileSelector does not take ownership of the new QFileSelector. To reset QQmlFileSelector
  to use its internal QFileSelector instance, call setSelector(\nullptr).
*/

void QQmlFileSelector::setSelector(QFileSelector *selector)
{
    Q_D(QQmlFileSelector);
    if (selector) {
        if (d->ownSelector) {
            delete d->selector;
            d->ownSelector = false;
        }
        d->selector = selector;
    } else {
        if (!d->ownSelector) {
            d->ownSelector = true;
            d->selector = new QFileSelector(this);
        } // Do nothing if already using internal selector
    }
}

/*!
  Adds extra selectors contained in \a strings to the current QFileSelector being used.
  Use this when extra selectors are all you need to avoid having to create your own
  QFileSelector instance.
*/
void QQmlFileSelector::setExtraSelectors(const QStringList &strings)
{
    Q_D(QQmlFileSelector);
    d->selector->setExtraSelectors(strings);
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
  \deprecated [6.0] The file selector should not be accessed after it
  is set. It may be in use. See below for further details.

  Gets the QQmlFileSelector currently active on the target \a engine.

  This method is deprecated. You should not retrieve the files selector from an
  engine after setting it. It may be in use.

  If the \a engine passed here is a QQmlApplicationEngine that hasn't loaded any
  QML files, yet, it will be initialized. Any later calls to
  QQmlApplicationEngine::setExtraFileSelectors() will have no effect.

  \sa QQmlApplicationEngine
*/
QQmlFileSelector* QQmlFileSelector::get(QQmlEngine* engine)
{
    QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);

    if (qobject_cast<QQmlApplicationEngine *>(engine)) {
        auto *appEnginePrivate = static_cast<QQmlApplicationEnginePrivate *>(enginePrivate);
        if (!appEnginePrivate->isInitialized) {
            appEnginePrivate->init();
            appEnginePrivate->isInitialized = true;
        }
    }

    const QUrl nonEmptyInvalid(QLatin1String(":"));
    for (QQmlAbstractUrlInterceptor *interceptor : enginePrivate->urlInterceptors) {
        const QUrl result = interceptor->intercept(
                    nonEmptyInvalid, QQmlAbstractUrlInterceptor::UrlString);
        if (result.scheme() == QLatin1String("type")
                && result.path() == QLatin1String("fileselector")) {
            return static_cast<QQmlFileSelectorInterceptor *>(interceptor)->d->q_func();
        }
    }
    return nullptr;
}
#endif

/*!
  \internal
*/
QQmlFileSelectorInterceptor::QQmlFileSelectorInterceptor(QQmlFileSelectorPrivate* pd)
    : d(pd)
{
}

/*!
  \internal
*/
QUrl QQmlFileSelectorInterceptor::intercept(const QUrl &path, DataType type)
{
    if (!path.isEmpty() && !path.isValid())
        return QUrl(QLatin1String("type:fileselector"));

    return type == QQmlAbstractUrlInterceptor::QmldirFile
            ? path // Don't intercept qmldir files, to prevent double interception
            : d->selector->select(path);
}

QT_END_NAMESPACE

#include "moc_qqmlfileselector.cpp"
