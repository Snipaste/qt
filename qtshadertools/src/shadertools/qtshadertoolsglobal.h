/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Shader Tools module
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTSHADERTOOLSGLOBAL_H
#define QTSHADERTOOLSGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#ifndef Q_SHADERTOOLS_EXPORT
#  if !defined(QT_STATIC)
#    if defined(QT_BUILD_SHADERTOOLS_LIB)
#      define Q_SHADERTOOLS_EXPORT Q_DECL_EXPORT
#    else
#      define Q_SHADERTOOLS_EXPORT Q_DECL_IMPORT
#    endif
#  else
#    define Q_SHADERTOOLS_EXPORT
#  endif
#endif

#ifdef Q_CLANG_QDOC
#define Q_SHADERTOOLS_EXPORT
#endif

QT_END_NAMESPACE

#endif
