/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "visualtestutils_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquickitemview_p.h>
#include <QtQuickTest/QtQuickTest>

QT_BEGIN_NAMESPACE

QQuickItem *QQuickVisualTestUtils::findVisibleChild(QQuickItem *parent, const QString &objectName)
{
    QQuickItem *item = 0;
    QList<QQuickItem*> items = parent->findChildren<QQuickItem*>(objectName);
    for (int i = 0; i < items.count(); ++i) {
        if (items.at(i)->isVisible() && !QQuickItemPrivate::get(items.at(i))->culled) {
            item = items.at(i);
            break;
        }
    }
    return item;
}

void QQuickVisualTestUtils::dumpTree(QQuickItem *parent, int depth)
{
    static QString padding = QStringLiteral("                       ");
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
        if (!item)
            continue;
        qDebug() << padding.left(depth*2) << item;
        dumpTree(item, depth+1);
    }
}

void QQuickVisualTestUtils::moveMouseAway(QQuickWindow *window)
{
#if QT_CONFIG(cursor) // Get the cursor out of the way.
    // Using "bottomRight() + QPoint(100, 100)" was causing issues on Ubuntu,
    // where the window was positioned at the bottom right corner of the window
    // (even after centering the window on the screen), so we use another position.
    QCursor::setPos(window->frameGeometry().bottomLeft() + QPoint(-10, 10));
#endif

    // make sure hover events from QQuickWindowPrivate::flushFrameSynchronousEvents()
    // do not interfere with the tests
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(window, &leave);
}

void QQuickVisualTestUtils::centerOnScreen(QQuickWindow *window)
{
    const QRect screenGeometry = window->screen()->availableGeometry();
    const QPoint offset = QPoint(window->width() / 2, window->height() / 2);
    window->setFramePosition(screenGeometry.center() - offset);
}

bool QQuickVisualTestUtils::delegateVisible(QQuickItem *item)
{
    return item->isVisible() && !QQuickItemPrivate::get(item)->culled;
}

/*!
    \internal

    Compares \a ia with \a ib, returning \c true if the images are equal.
    If they are not equal, \c false is returned and \a errorMessage is set.

    A custom compare function to avoid issues such as:
    When running on native Nvidia graphics cards on linux, the
    distance field glyph pixels have a measurable, but not visible
    pixel error. This was GT-216 with the ubuntu "nvidia-319" driver package.
    llvmpipe does not show the same issue.
*/
bool QQuickVisualTestUtils::compareImages(const QImage &ia, const QImage &ib, QString *errorMessage)
{
    if (ia.size() != ib.size()) {
        QDebug(errorMessage) << "Images are of different size:" << ia.size() << ib.size()
            << "DPR:" << ia.devicePixelRatio() << ib.devicePixelRatio();
        return false;
    }
    if (ia.format() != ib.format()) {
        QDebug(errorMessage) << "Images are of different formats:" << ia.format() << ib.format();
        return false;
    }

    int w = ia.width();
    int h = ia.height();
    const int tolerance = 5;
    for (int y=0; y<h; ++y) {
        const uint *as= (const uint *) ia.constScanLine(y);
        const uint *bs= (const uint *) ib.constScanLine(y);
        for (int x=0; x<w; ++x) {
            uint a = as[x];
            uint b = bs[x];

            // No tolerance for error in the alpha.
            if ((a & 0xff000000) != (b & 0xff000000)
                || qAbs(qRed(a) - qRed(b)) > tolerance
                || qAbs(qRed(a) - qRed(b)) > tolerance
                || qAbs(qRed(a) - qRed(b)) > tolerance) {
                QDebug(errorMessage) << "Mismatch at:" << x << y << ':'
                    << Qt::hex << Qt::showbase << a << b;
                return false;
            }
        }
    }
    return true;
}

/*!
    \internal

    Finds the delegate at \c index belonging to \c itemView, using the given \c flags.

    If the view needs to be polished, the function will wait for it to be done before continuing,
    and returns \c nullptr if the polish didn't happen.
*/
QQuickItem *QQuickVisualTestUtils::findViewDelegateItem(QQuickItemView *itemView, int index, FindViewDelegateItemFlags flags)
{
    if (QQuickTest::qIsPolishScheduled(itemView)) {
        if (!QQuickTest::qWaitForItemPolished(itemView)) {
            qWarning() << "failed to polish" << itemView;
            return nullptr;
        }
    }

    // Do this after the polish, just in case the count changes after a polish...
    if (index <= -1 || index >= itemView->count()) {
        qWarning() << "index" << index << "is out of bounds for" << itemView;
        return nullptr;
    }

    if (flags.testFlag(FindViewDelegateItemFlag::PositionViewAtIndex))
        itemView->positionViewAtIndex(index, QQuickItemView::Center);

    return itemView->itemAtIndex(index);
}

QQuickVisualTestUtils::QQuickApplicationHelper::QQuickApplicationHelper(QQmlDataTest *testCase, const QString &testFilePath, const QStringList &qmlImportPaths, const QVariantMap &initialProperties)
{
    for (const auto &path : qmlImportPaths)
        engine.addImportPath(path);

    QQmlComponent component(&engine);

    component.loadUrl(testCase->testFileUrl(testFilePath));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QObject *rootObject = component.createWithInitialProperties(initialProperties);
    cleanup.reset(rootObject);
    if (component.isError() || !rootObject) {
        errorMessage = QString::fromUtf8("Failed to create window: %1").arg(component.errorString()).toUtf8();
        return;
    }

    window = qobject_cast<QQuickWindow*>(rootObject);
    if (!window) {
        errorMessage = QString::fromUtf8("Root object %1 must be a QQuickWindow subclass").arg(QDebug::toString(window)).toUtf8();
        return;
    }

    if (window->isVisible()) {
        errorMessage = QString::fromUtf8("Expected window not to be visible, but it is").toUtf8();
        return;
    }

    ready = true;
}

QQuickVisualTestUtils::MnemonicKeySimulator::MnemonicKeySimulator(QWindow *window)
    : m_window(window), m_modifiers(Qt::NoModifier)
{
}

void QQuickVisualTestUtils::MnemonicKeySimulator::press(Qt::Key key)
{
    // QTest::keyPress() but not generating the press event for the modifier key.
    if (key == Qt::Key_Alt)
        m_modifiers |= Qt::AltModifier;
    QTest::simulateEvent(m_window, true, key, m_modifiers, QString(), false);
}

void QQuickVisualTestUtils::MnemonicKeySimulator::release(Qt::Key key)
{
    // QTest::keyRelease() but not generating the release event for the modifier key.
    if (key == Qt::Key_Alt)
        m_modifiers &= ~Qt::AltModifier;
    QTest::simulateEvent(m_window, false, key, m_modifiers, QString(), false);
}

void QQuickVisualTestUtils::MnemonicKeySimulator::click(Qt::Key key)
{
    press(key);
    release(key);
}

QT_END_NAMESPACE
