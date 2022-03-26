/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

Image {
    id: corkPanel
    source: "content/cork.jpg"
    width: ListView.view.width
    height: ListView.view.height
    fillMode: Image.PreserveAspectCrop

    TapHandler {
        objectName: name
        onTapped: corkPanel.Window.activeFocusItem.focus = false
    }

    Text {
        text: name
        x: 15
        y: 8
        height: 40
        width: 370
        font.pixelSize: 18
        font.bold: true
        color: "white"
        style: Text.Outline
        styleColor: "black"
        wrapMode: Text.Wrap
    }

    Repeater {
        model: notes
        Item {
            id: fulcrum

            x: 100 + Math.random() * (corkPanel.width - 0.5 * paper.width)
            y: 50 + Math.random() * (corkPanel.height - 0.5 * paper.height)

            Item {
                id: note
                scale: 0.7

                Image {
                    id: paper
                    x: 8 + -width * 0.6 / 2
                    y: -20
                    source: "note-yellow.png"
                    scale: 0.6
                    transformOrigin: Item.TopLeft
                    antialiasing: true

                    DragHandler {
                        target: fulcrum
                        xAxis.minimum: 100
                        xAxis.maximum: corkPanel.width - 80
                        yAxis.minimum: 0
                        yAxis.maximum: corkPanel.height - 80
                    }
                }

                TextEdit {
                    id: text
                    x: -104
                    y: 36
                    width: 215
                    height: 24
                    font.pixelSize: 24
                    readOnly: false
                    selectByMouse: activeFocus
                    rotation: -8
                    text: noteText
                    wrapMode: Text.Wrap
                }

                rotation: -flickable.horizontalVelocity / 100
                Behavior on rotation {
                    SpringAnimation { spring: 2.0; damping: 0.15 }
                }
            }

            Image {
                x: -width / 2
                y: -height * 0.5 / 2
                source: "tack.png"
                scale: 0.7
                transformOrigin: Item.TopLeft
            }

            states: State {
                name: "pressed"
                when: text.activeFocus
                PropertyChanges { target: note; rotation: 8; scale: 1 }
                PropertyChanges { target: fulcrum; z: 8 }
            }

            transitions: Transition {
                NumberAnimation { properties: "rotation,scale"; duration: 200 }
            }
        }
    }
}
