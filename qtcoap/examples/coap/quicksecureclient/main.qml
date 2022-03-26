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

import QtQuick 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13
import CoapSecureClient 1.0
import qtcoap.example.namespace 1.0

Window {
    visible: true
    width: 480
    height: 640
    title: qsTr("Qt Quick Secure CoAP Client")

    CoapSecureClient {
        id: client
        onFinished: {
            outputView.text = result;
            statusLabel.text = "";
            disconnectButton.enabled = true;
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10
        columns: 2

        Label {
            text: qsTr("Host:")
        }
        ComboBox {
            id: hostComboBox
            editable: true
            model: hostsModel
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Port:")
        }
        TextField {
            id: portField
            text: "5684"
            placeholderText: qsTr("<Port>")
            inputMethodHints: Qt.ImhDigitsOnly
            Layout.preferredWidth: 80
        }

        Label {
            text: qsTr("Resource:")
        }
        TextField {
            id: resourceField
            placeholderText: qsTr("<Resource Path>")
            inputMethodHints: Qt.ImhUrlCharactersOnly
            selectByMouse: true
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Security Mode:")
        }
        ButtonGroup {
            id: securityModeGroup
            onClicked: {
                if (securityModeGroup.checkedButton === preSharedMode)
                    client.setSecurityMode(QtCoap.SecurityMode.PreSharedKey);
                else
                    client.setSecurityMode(QtCoap.SecurityMode.Certificate);
            }
        }
        RowLayout {
            RadioButton {
                id: preSharedMode
                text: qsTr("Pre-shared Key")
                ButtonGroup.group: securityModeGroup
            }
            RadioButton {
                id: certificateMode
                text: qsTr("X.509 Certificate")
                ButtonGroup.group: securityModeGroup
            }
        }

        RowLayout {
            enabled: securityModeGroup.checkedButton === preSharedMode
            Layout.columnSpan: 2

            Label {
                text: qsTr("Key")
            }
            TextField {
                id: pskField
                placeholderText: qsTr("<Pre-shared Key>")
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Identity")
            }
            TextField {
                id: identityField
                placeholderText: qsTr("<Identity>")
                Layout.fillWidth: true
            }
        }

        FilePicker {
            id: localCertificatePicker
            dialogText: "Local Certificate"
            enabled: securityModeGroup.checkedButton === certificateMode
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }

        FilePicker {
            id: caCertificatePicker
            dialogText: "CA Certificate"
            enabled: securityModeGroup.checkedButton === certificateMode
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }

        FilePicker {
            id: privateKeyPicker
            dialogText: "Private Key"
            enabled: securityModeGroup.checkedButton === certificateMode
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }

        Button {
            id: requestButton
            text: qsTr("Send Request")
            enabled: securityModeGroup.checkState !== Qt.Unchecked

            onClicked: {
                outputView.text = "";
                if (securityModeGroup.checkedButton === preSharedMode)
                    client.setSecurityConfiguration(pskField.text, identityField.text);
                else
                    client.setSecurityConfiguration(localCertificatePicker.selectedFile,
                                                    caCertificatePicker.selectedFile,
                                                    privateKeyPicker.selectedFile);

                client.sendGetRequest(hostComboBox.editText, resourceField.text,
                                      parseInt(portField.text));

                statusLabel.text = qsTr("Sending request to %1%2...").arg(hostComboBox.editText)
                                                                     .arg(resourceField.text);
            }
        }

        Button {
            id: disconnectButton
            text: qsTr("Disconnect")
            enabled: false

            onClicked: {
                client.disconnect();
                statusLabel.text = qsTr("Disconnected.");
                outputView.text = "";
                disconnectButton.enabled = false;
            }
        }

        TextArea {
            id: outputView
            placeholderText: qsTr("<Client Output>")
            background: Rectangle {
                border.color: "gray"
            }
            Layout.columnSpan: 2
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
        Label {
            id: statusLabel
            Layout.columnSpan: 2
            Layout.fillWidth: true
        }
    }
}
