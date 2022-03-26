/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtCore/qcoreapplication_platform.h>

#include <QtCore/private/qnativeinterface_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qjniobject.h>
#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
#include <QtConcurrent/QtConcurrent>
#include <QtCore/qpromise.h>
#include <deque>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
static const char qtNativeClassName[] = "org/qtproject/qt/android/QtNative";

typedef std::pair<std::function<QVariant()>, QSharedPointer<QPromise<QVariant>>> RunnablePair;
typedef std::deque<RunnablePair> PendingRunnables;
Q_GLOBAL_STATIC(PendingRunnables, g_pendingRunnables);
static QBasicMutex g_pendingRunnablesMutex;
#endif

/*!
    \class QNativeInterface::QAndroidApplication
    \since 6.2
    \brief Native interface to a core application on Android.

    Accessed through QCoreApplication::nativeInterface().

    \inmodule QtCore
    \inheaderfile QCoreApplication
    \ingroup native-interfaces
    \ingroup native-interfaces-qcoreapplication
*/
QT_DEFINE_NATIVE_INTERFACE(QAndroidApplication);

/*!
     \fn jobject QNativeInterface::QAndroidApplication::context()

    Returns the Android context as a \c jobject. The context is an \c Activity
    if the main activity object is valid. Otherwise, the context is a \c Service.

    \since 6.2
*/
jobject QNativeInterface::QAndroidApplication::context()
{
    return QtAndroidPrivate::context();
}

/*!
     \fn bool QNativeInterface::QAndroidApplication::isActivityContext()

    Returns \c true if QAndroidApplication::context() provides an \c Activity
    context.

    \since 6.2
*/
bool QNativeInterface::QAndroidApplication::isActivityContext()
{
    return QtAndroidPrivate::activity();
}

/*!
    \fn int QNativeInterface::QAndroidApplication::sdkVersion()

    Returns the Android SDK version. This is also known as the API level.

    \since 6.2
*/
int QNativeInterface::QAndroidApplication::sdkVersion()
{
    return QtAndroidPrivate::androidSdkVersion();
}

/*!
    \fn void QNativeInterface::QAndroidApplication::hideSplashScreen(int duration)

    Hides the splash screen by using a fade effect for the given \a duration.
    If \a duration is not provided (default is 0) the splash screen is hidden
    immedetiately after the app starts.

    \since 6.2
*/
void QNativeInterface::QAndroidApplication::hideSplashScreen(int duration)
{
    QJniObject::callStaticMethod<void>("org/qtproject/qt/android/QtNative",
                                       "hideSplashScreen", "(I)V", duration);
}

/*!
    Posts the function \a runnable to the Android thread. The function will be
    queued and executed on the Android UI thread. If the call is made on the
    Android UI thread \a runnable will be executed immediately. If the Android
    app is paused or the main Activity is null, \c runnable is added to the
    Android main thread's queue.

    This call returns a QFuture<QVariant> which allows doing both synchronous
    and asynchronous calls, and can handle any return type. However, to get
    a result back from the QFuture::result(), QVariant::value() should be used.

    If the \a runnable execution takes longer than the period of \a timeout,
    the blocking calls \l QFuture::waitForFinished() and \l QFuture::result()
    are ended once \a timeout has elapsed. However, if \a runnable has already
    started execution, it won't be cancelled.

    The following example shows how to run an asynchronous call that expects
    a return type:

    \code
    auto task = QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        QJniObject surfaceView;
        if (!surfaceView.isValid())
            qDebug() << "SurfaceView object is not valid yet";

        surfaceView = QJniObject("android/view/SurfaceView",
                                 "(Landroid/content/Context;)V",
                                 QNativeInterface::QAndroidApplication::context());

        return QVariant::fromValue(surfaceView);
    }).then([](QFuture<QVariant> future) {
        auto surfaceView = future.result().value<QJniObject>();
        if (surfaceView.isValid())
            qDebug() << "Retrieved SurfaceView object is valid";
    });
    \endcode

    The following example shows how to run a synchronous call with a void
    return type:

    \code
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
       QJniObject activity = QNativeInterface::QAndroidApplication::context();
       // Hide system ui elements or go full screen
       activity.callObjectMethod("getWindow", "()Landroid/view/Window;")
               .callObjectMethod("getDecorView", "()Landroid/view/View;")
               .callMethod<void>("setSystemUiVisibility", "(I)V", 0xffffffff);
    }).waitForFinished();
    \endcode

    \note Becareful about the type of operations you do on the Android's main
    thread, as any long operation can block the app's UI rendering and input
    handling. If the function is expected to have long execution time, it's
    also good to use a \l QDeadlineTimer in your \a runnable to manage
    the execution and make sure it doesn't block the UI thread. Usually,
    any operation longer than 5 seconds might block the app's UI. For more
    information, see \l {Android: Keeping your app responsive}{Keeping your app responsive}.

    \since 6.2
*/
#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
QFuture<QVariant> QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                                                    const std::function<QVariant()> &runnable,
                                                    const QDeadlineTimer timeout)
{
    QSharedPointer<QPromise<QVariant>> promise(new QPromise<QVariant>());
    QFuture<QVariant> future = promise->future();
    promise->start();

    (void) QtConcurrent::run([=, &future]() {
        if (!timeout.isForever()) {
            QEventLoop loop;
            QTimer::singleShot(timeout.remainingTime(), &loop, [&]() {
                future.cancel();
                promise->finish();
                loop.quit();
            });

            QFutureWatcher<QVariant> watcher;
            QObject::connect(&watcher, &QFutureWatcher<QVariant>::finished, &loop, [&]() {
                loop.quit();
            });
            QObject::connect(&watcher, &QFutureWatcher<QVariant>::canceled, &loop, [&]() {
                loop.quit();
            });
            watcher.setFuture(future);
            loop.exec();
        }
    });

    QMutexLocker locker(&g_pendingRunnablesMutex);
    g_pendingRunnables->push_back(std::pair(runnable, promise));
    locker.unlock();

    QJniObject::callStaticMethod<void>(qtNativeClassName,
                                       "runPendingCppRunnablesOnAndroidThread",
                                       "()V");
    return future;
}

// function called from Java from Android UI thread
static void runPendingCppRunnables(JNIEnv */*env*/, jobject /*obj*/)
{
    // run all posted runnables
    for (;;) {
        QMutexLocker locker(&g_pendingRunnablesMutex);
        if (g_pendingRunnables->empty())
            break;

        std::pair pair = std::move(g_pendingRunnables->front());
        g_pendingRunnables->pop_front();
        locker.unlock();

        // run the runnable outside the sync block!
        auto promise = pair.second;
        if (!promise->isCanceled())
            promise->addResult(pair.first());
        promise->finish();
    }
}
#endif

bool QtAndroidPrivate::registerNativeInterfaceNatives()
{
#if QT_CONFIG(future) && !defined(QT_NO_QOBJECT)
    JNINativeMethod methods = {"runPendingCppRunnables", "()V", (void *)runPendingCppRunnables};
    return QJniEnvironment().registerNativeMethods(qtNativeClassName, &methods, 1);
#else
    return true;
#endif
}

QT_END_NAMESPACE
