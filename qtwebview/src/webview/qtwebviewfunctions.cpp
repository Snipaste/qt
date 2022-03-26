/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebView module of the Qt Toolkit.
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

#include "qtwebviewfunctions.h"

#include "qwebviewfactory_p.h"
#include "qwebviewplugin_p.h"

#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

/*!
    \namespace QtWebView
    \inmodule QtWebView
    \brief The QtWebView namespace provides functions that makes it easier to set-up and use the WebView.
    \inheaderfile QtWebView
*/

// This is a separate function so we can be sure that in non-static cases it can be registered
// as a pre hook for QCoreApplication, ensuring this is called after the plugin paths have
// been set to their defaults. For static builds then this will be called explicitly when
// QtWebView::initialize() is called by the application

static void initializeImpl()
{
    if (QWebViewFactory::requiresExtraInitializationSteps()) {
        // There might be plugins available, but their dependencies might not be met,
        // so make sure we have a valid plugin before using it.
        // Note: A warning will be printed later if we're unable to load the plugin.
        QWebViewPlugin *plugin = QWebViewFactory::getPlugin();
        if (plugin)
            plugin->prepare();
    }
}

#ifndef QT_STATIC
Q_COREAPP_STARTUP_FUNCTION(initializeImpl);
#endif

/*!
    \fn void QtWebView::initialize()
    \keyword qtwebview-initialize

    This function initializes resources or sets options that are required by the different back-ends.

    \note The \c initialize() function needs to be called immediately before the QGuiApplication
    instance is created.
 */

void QtWebView::initialize()
{
#ifdef QT_STATIC
    initializeImpl();
#endif
}

QT_END_NAMESPACE
