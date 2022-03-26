/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtCoap module.
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

import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import CoapMulticastClient 1.0
import qtcoap.example.namespace 1.0

Window {
    visible: true
    width: 480
    height: 480
    title: qsTr("Qt Quick CoAP Multicast Discovery")

    function addResource(resource) {
        resourceModel.insert(0, {"host" : resource.host,
                                 "path" : resource.path,
                                 "title" : resource.title})
    }

    CoapMulticastClient {
        id: client
        onDiscovered: addResource(resource)

        onFinished: {
            statusLabel.text = (error === QtCoap.Error.Ok)
                    ? qsTr("Finished resource discovery.")
                    : qsTr("Resource discovery failed with error code: %1").arg(error)
        }

        onError:
            statusLabel.text = qsTr("Resource discovery failed with error code: %1").arg(error)
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10
        columns: 2

        Label {
            text: qsTr("Host:")
        }

        RowLayout {
            spacing: 5

            ComboBox {
                id: groupComboBox
                textRole: "group"
                Layout.preferredWidth: 210
                model: ListModel {
                    id: cbItems
                    ListElement {
                        group: qsTr("IPv4 CoAP Nodes")
                        address: "224.0.1.187"
                        value: QtCoap.MulticastGroup.AllCoapNodesIPv4
                    }
                    ListElement {
                        group: qsTr("IPv6 Link Local CoAP Nodes")
                        address: "ff02::fd"
                        value: QtCoap.MulticastGroup.AllCoapNodesIPv6LinkLocal
                    }
                    ListElement {
                        group: qsTr("IPv6 Site Local CoAP Nodes")
                        address: "ff05::fd"
                        value: QtCoap.MulticastGroup.AllCoapNodesIPv6SiteLocal
                    }
                    ListElement {
                        group: qsTr("Other")
                        address: ""
                        value: -1
                    }
                }

                delegate: ItemDelegate {
                    width: groupComboBox.width
                    contentItem: Column {
                        Text { text: group }
                        Text { text: address }
                    }
                    highlighted: groupComboBox.highlightedIndex === index
                }
            }

            TextField {
                id: customGroupField
                placeholderText: qsTr("<Custom Group>")
                enabled: groupComboBox.model.get(groupComboBox.currentIndex).value === -1
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Port:")
        }

        TextField {
            id: portField
            text: "5683"
            placeholderText: qsTr("<Port>")
            inputMethodHints: Qt.ImhDigitsOnly
            Layout.preferredWidth: 80
        }

        Label {
            text: qsTr("Discovery Path:")
        }

        TextField {
            id: discoveryPathField
            text: "/.well-known/core"
            placeholderText: qsTr("<Resource Discovery Path>")
            inputMethodHints: Qt.ImhUrlCharactersOnly
            Layout.fillWidth: true
        }

        Button {
            id: discoverButton
            text: qsTr("Discover")
            Layout.columnSpan: 2

            onClicked: {
                var currentGroup = groupComboBox.model.get(groupComboBox.currentIndex).value;

                var path = "";
                if (currentGroup !== - 1) {
                    client.discover(currentGroup, parseInt(portField.text),
                                    discoveryPathField.text);
                    path = groupComboBox.currentText;
                } else {
                    client.discover(customGroupField.text, parseInt(portField.text),
                                    discoveryPathField.text);
                    path = customGroupField.text + discoveryPathField.text;
                }
                statusLabel.text = qsTr("Discovering resources at %1...").arg(path);
            }
        }

        ListModel {
            id: resourceModel
        }

        ListView {
            id: resourceView
            model: resourceModel
            clip: true
            Layout.columnSpan: 2
            Layout.fillHeight: true
            Layout.fillWidth: true

            delegate: Rectangle {
                width: resourceView.width
                height: 60
                color: "lightgray"
                border.color: "darkgray"
                radius: 5

                Column {
                    topPadding: 5
                    leftPadding: 5
                    Text { text: qsTr('<b>Host:</b> %1').arg(host) }
                    Text { text: qsTr('<b>Resource:</b> %1').arg(path) }
                    Text { text: qsTr('<b>Title:</b> %1').arg(title) }
                }
            }
        }
        Label {
            id: statusLabel
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }
    }
}
