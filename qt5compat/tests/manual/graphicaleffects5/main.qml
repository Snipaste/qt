/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Graphical Effects module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick

Window {
    width: 640
    height: 600
    visible: true
    title: qsTr("Graphical Effects")

    Image {
        id: bug
        source: "images/bug.jpg"
        sourceSize: Qt.size(parent.width / grid.columns, parent.width / grid.columns)
        smooth: true
        visible: false
    }

    Image {
        id: butterfly
        source: "images/butterfly.png"
        sourceSize: Qt.size(parent.width / grid.columns, parent.width / grid.columns)
        smooth: true
        visible: false
    }

    Grid {
        id: grid
        anchors.fill: parent
        columns: 5

        BrightnessContrastEffect {
            width: parent.width / parent.columns
            height: width
        }

        ColorOverlayEffect {
            width: parent.width / parent.columns
            height: width
        }

        ColorizeEffect {
            width: parent.width / parent.columns
            height: width
        }

        DesaturateEffect {
            width: parent.width / parent.columns
            height: width
        }

        GammaAdjustEffect {
            width: parent.width / parent.columns
            height: width
        }

        HueSaturationEffect {
            width: parent.width / parent.columns
            height: width
        }

        LevelAdjustEffect {
            width: parent.width / parent.columns
            height: width
        }

        ConicalGradientEffect {
            width: parent.width / parent.columns
            height: width
        }

        LinearGradientEffect {
            width: parent.width / parent.columns
            height: width
        }

        RadialGradientEffect {
            width: parent.width / parent.columns
            height: width
        }

        DisplaceEffect {
            width: parent.width / parent.columns
            height: width
        }

        DropShadowEffect {
            width: parent.width / parent.columns
            height: width
        }

        FastBlurEffect {
            width: parent.width / parent.columns
            height: width
        }

        GlowEffect {
            width: parent.width / parent.columns
            height: width
        }

        RectangularGlowEffect {
            width: parent.width / parent.columns
            height: width
        }

        OpacityMaskEffect {
            width: parent.width / parent.columns
            height: width
        }

        ThresholdMaskEffect {
            width: parent.width / parent.columns
            height: width
        }
    }
}
