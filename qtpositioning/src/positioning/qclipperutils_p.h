/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
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
#ifndef QCLIPPERUTILS_P_H
#define QCLIPPERUTILS_P_H

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

/*
 * This file is intended to be include only in source files of
 * QtPositioning/QtLocation. It is in QtPositioning to enable manipulation
 * of geo polygons
 */

#include <QtPositioning/private/qpositioningglobal_p.h>
#include <QtCore/QtGlobal>
#include <QtCore/QList>
#include <QtPositioning/private/qdoublevector2d_p.h>

QT_BEGIN_NAMESPACE

/*
 * This class provides a wrapper around the clip2tri library, so that
 * we do not need to export any of the internal types. That is needed
 * because after QtLocation and QtPositioning are moved to different
 * repos, we need to use the features of the library in QtLocation without
 * explicitly linking to it.
*/

class QClipperUtilsPrivate;
class Q_POSITIONING_PRIVATE_EXPORT QClipperUtils
{
public:
    QClipperUtils();
    QClipperUtils(const QClipperUtils &other);
    ~QClipperUtils();

    // Must be in sync with c2t::clip2tri::Operation
    enum Operation {
        Union,
        Intersection,
        Difference,
        Xor
    };

    // Must be in sync with QtClipperLib::PolyFillType
    enum PolyFillType {
        pftEvenOdd,
        pftNonZero,
        pftPositive,
        pftNegative
    };

    static double clipperScaleFactor();

    static int pointInPolygon(const QDoubleVector2D &point, const QList<QDoubleVector2D> &polygon);

    // wrap some useful non-static methods of c2t::clip2tri
    void clearClipper();
    void addSubjectPath(const QList<QDoubleVector2D> &path, bool closed);
    void addClipPolygon(const QList<QDoubleVector2D> &path);
    QList<QList<QDoubleVector2D>> execute(Operation op, PolyFillType subjFillType = pftNonZero,
                                          PolyFillType clipFillType = pftNonZero);

    // For optimization purposes. Set the polygon once and check for multiple
    // points. Without the need to convert between Qt and clip2tri types
    // every time
    void setPolygon(const QList<QDoubleVector2D> &polygon);
    int pointInPolygon(const QDoubleVector2D &point) const;

private:
    QClipperUtilsPrivate *d_ptr;
};

QT_END_NAMESPACE

#endif // QCLIPPERUTILS_P_H
