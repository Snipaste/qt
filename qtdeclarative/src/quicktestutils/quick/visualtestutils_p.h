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

#ifndef QQUICKVISUALTESTUTILS_P_H
#define QQUICKVISUALTESTUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/qqmlexpression.h>
#include <QtQuick/private/qquickitem_p.h>

#include <private/qmlutils_p.h>

QT_BEGIN_NAMESPACE

class QQuickItemView;
class QQuickWindow;

namespace QQuickVisualTestUtils
{
    QQuickItem *findVisibleChild(QQuickItem *parent, const QString &objectName);

    void dumpTree(QQuickItem *parent, int depth = 0);

    bool delegateVisible(QQuickItem *item);

    void moveMouseAway(QQuickWindow *window);
    void centerOnScreen(QQuickWindow *window);

    bool delegateVisible(QQuickItem *item);

    /*
       Find an item with the specified objectName.  If index is supplied then the
       item must also evaluate the {index} expression equal to index
    */
    template<typename T>
    T *findItem(QQuickItem *parent, const QString &objectName, int index = -1)
    {
        const QMetaObject &mo = T::staticMetaObject;
        for (int i = 0; i < parent->childItems().count(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
            if (!item)
                continue;
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
                if (index != -1) {
                    QQmlExpression e(qmlContext(item), item, "index");
                    if (e.evaluate().toInt() == index)
                        return static_cast<T*>(item);
                } else {
                    return static_cast<T*>(item);
                }
            }
            item = findItem<T>(item, objectName, index);
            if (item)
                return static_cast<T*>(item);
        }

        return 0;
    }

    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName, bool visibleOnly = true)
    {
        QList<T*> items;
        const QMetaObject &mo = T::staticMetaObject;
        for (int i = 0; i < parent->childItems().count(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
            if (!item || (visibleOnly && (!item->isVisible() || QQuickItemPrivate::get(item)->culled)))
                continue;
            if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
                items.append(static_cast<T*>(item));
            items += findItems<T>(item, objectName);
        }

        return items;
    }

    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName, const QList<int> &indexes)
    {
        QList<T*> items;
        for (int i=0; i<indexes.count(); i++)
            items << qobject_cast<QQuickItem*>(findItem<T>(parent, objectName, indexes[i]));
        return items;
    }

    bool compareImages(const QImage &ia, const QImage &ib, QString *errorMessage);

    struct SignalMultiSpy : public QObject
    {
        Q_OBJECT
    public:
        QList<QObject *> senders;
        QList<QByteArray> signalNames;

        template <typename Func1>
        QMetaObject::Connection connectToSignal(const typename QtPrivate::FunctionPointer<Func1>::Object *obj, Func1 signal,
                                                              Qt::ConnectionType type = Qt::AutoConnection)
        {
            return connect(obj, signal, this, &SignalMultiSpy::receive, type);
        }

        void clear() {
            senders.clear();
            signalNames.clear();
        }

    public slots:
        void receive() {
            QMetaMethod m = sender()->metaObject()->method(senderSignalIndex());
            senders << sender();
            signalNames << m.name();
        }
    };

    enum class FindViewDelegateItemFlag {
        None = 0x0,
        PositionViewAtIndex = 0x01
    };
    Q_DECLARE_FLAGS(FindViewDelegateItemFlags, FindViewDelegateItemFlag)

    QQuickItem* findViewDelegateItem(QQuickItemView *itemView, int index,
        FindViewDelegateItemFlags flags = FindViewDelegateItemFlag::PositionViewAtIndex);

    /*!
        \internal

        Same as above except allows use in QTRY_* functions without having to call it again
        afterwards to assign the delegate.
    */
    template<typename T>
    bool findViewDelegateItem(QQuickItemView *itemView, int index, T &delegateItem,
        FindViewDelegateItemFlags flags = FindViewDelegateItemFlag::PositionViewAtIndex)
    {
        delegateItem = qobject_cast<T>(findViewDelegateItem(itemView, index, flags));
        return delegateItem != nullptr;
    }

    class QQuickApplicationHelper
    {
    public:
        QQuickApplicationHelper(QQmlDataTest *testCase, const QString &testFilePath,
                const QStringList &qmlImportPaths = {},
                const QVariantMap &initialProperties = {});

        // Return a C-style string instead of QString because that's what QTest uses for error messages,
        // so it saves code at the calling site.
        inline const char *failureMessage() const
        {
            return errorMessage.constData();
        }

        QQmlEngine engine;
        QScopedPointer<QObject> cleanup;
        QQuickWindow *window = nullptr;

        bool ready = false;
        // Store as a byte array so that we can return its raw data safely;
        // using qPrintable() in failureMessage() will construct a throwaway QByteArray
        // that is destroyed before the function returns.
        QByteArray errorMessage;
    };

    class MnemonicKeySimulator
    {
        Q_DISABLE_COPY(MnemonicKeySimulator)
    public:
        explicit MnemonicKeySimulator(QWindow *window);

        void press(Qt::Key key);
        void release(Qt::Key key);
        void click(Qt::Key key);

    private:
        QPointer<QWindow> m_window;
        Qt::KeyboardModifiers m_modifiers;
    };
}

#define QQUICK_VERIFY_POLISH(item) \
    QTRY_COMPARE(QQuickItemPrivate::get(item)->polishScheduled, false)

QT_END_NAMESPACE

#endif // QQUICKVISUALTESTUTILS_P_H
