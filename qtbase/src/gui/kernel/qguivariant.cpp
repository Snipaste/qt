/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

// Gui types
#include "qbitmap.h"
#include "qbrush.h"
#include "qcolor.h"
#include "qcolorspace.h"
#include "qcursor.h"
#include "qfont.h"
#include "qimage.h"
#if QT_CONFIG(shortcut)
#  include "qkeysequence.h"
#endif
#include "qtransform.h"
#include "qpalette.h"
#include "qpen.h"
#include "qpixmap.h"
#include "qpolygon.h"
#include "qregion.h"
#include "qtextformat.h"
#include "qmatrix4x4.h"
#include "qvector2d.h"
#include "qvector3d.h"
#include "qvector4d.h"
#include "qquaternion.h"
#include "qicon.h"

// Core types
#include "qvariant.h"
#include "qbitarray.h"
#include "qbytearray.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qmap.h"
#include "qdatetime.h"
#include "qlist.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qurl.h"
#include "qlocale.h"
#include "quuid.h"

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#include "qline.h"
#endif

#include <float.h>

#include "private/qvariant_p.h"
#include <private/qmetatype_p.h>

QT_BEGIN_NAMESPACE

namespace {
struct GuiTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QModulesPrivate::QTypeModuleInfo<T>::IsGui && QtMetaTypePrivate::TypeDefinition<T>::IsAvailable;
    };
};

static const struct : QMetaTypeModuleHelper
{
#define QT_IMPL_METATYPEINTERFACE_GUI_TYPES(MetaTypeName, MetaTypeId, RealName) \
    QT_METATYPE_INTERFACE_INIT(RealName),

    const QtPrivate::QMetaTypeInterface *interfaceForType(int type) const override {
        switch (type) {
            QT_FOR_EACH_STATIC_GUI_CLASS(QT_METATYPE_CONVERT_ID_TO_TYPE)
            default: return nullptr;
        }
    }
#undef QT_IMPL_METATYPEINTERFACE_GUI_TYPES

    bool convert(const void *from, int fromTypeId, void *to, int toTypeId) const override
    {
        Q_ASSERT(fromTypeId != toTypeId);

        bool onlyCheck = (from == nullptr && to == nullptr);
        // either two nullptrs from canConvert, or two valid pointers
        Q_ASSERT(onlyCheck || (bool(from) && bool(to)));

        using Int = int;
        switch (makePair(toTypeId, fromTypeId)) {
        QMETATYPE_CONVERTER(QByteArray, QColor,
            result = source.name(source.alpha() != 255 ?
                                 QColor::HexArgb : QColor::HexRgb).toLatin1();
            return true;
        );
        QMETATYPE_CONVERTER(QColor, QByteArray,
            result.setNamedColor(QLatin1String(source));
            return result.isValid();
        );
        QMETATYPE_CONVERTER(QString, QColor,
            result = source.name(source.alpha() != 255 ?
                                 QColor::HexArgb : QColor::HexRgb);
            return true;
        );
        QMETATYPE_CONVERTER(QColor, QString,
            result.setNamedColor(source);
            return result.isValid();
        );
#if QT_CONFIG(shortcut)
        QMETATYPE_CONVERTER(QString, QKeySequence,
            result = source.toString(QKeySequence::NativeText);
            return true;
        );
        QMETATYPE_CONVERTER(QKeySequence, QString, result = source; return true;);
        QMETATYPE_CONVERTER(Int, QKeySequence,
            result = source.isEmpty() ? 0 : source[0].toCombined();
            return true;
        );
        QMETATYPE_CONVERTER(QKeySequence, Int, result = source; return true;);
#endif
        QMETATYPE_CONVERTER(QString, QFont, result = source.toString(); return true;);
        QMETATYPE_CONVERTER(QFont, QString, return result.fromString(source););
        QMETATYPE_CONVERTER(QPixmap, QImage, result = QPixmap::fromImage(source); return true;);
        QMETATYPE_CONVERTER(QImage, QPixmap, result = source.toImage(); return true;);
        QMETATYPE_CONVERTER(QPixmap, QBitmap, result = source; return true;);
        QMETATYPE_CONVERTER(QBitmap, QPixmap, result = QBitmap::fromPixmap(source); return true;);
        QMETATYPE_CONVERTER(QImage, QBitmap, result = source.toImage(); return true;);
        QMETATYPE_CONVERTER(QBitmap, QImage, result = QBitmap::fromImage(source); return true;);
        QMETATYPE_CONVERTER(QPixmap, QBrush, result = source.texture(); return true;);
        QMETATYPE_CONVERTER(QBrush, QPixmap, result = source; return true;);
        QMETATYPE_CONVERTER(QColor, QBrush,
            if (source.style() == Qt::SolidPattern) {
                result = source.color();
                return true;
            }
            return false;
        );
        QMETATYPE_CONVERTER(QBrush, QColor, result = source; return true;);
        default:
            break;
        }
        return false;
    }
} qVariantGuiHelper;

} // namespace used to hide QVariant handler

void qRegisterGuiVariant()
{
    qMetaTypeGuiHelper = &qVariantGuiHelper;
}
Q_CONSTRUCTOR_FUNCTION(qRegisterGuiVariant)

QT_END_NAMESPACE
