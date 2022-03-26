/****************************************************************************
**
** Copyright (C) 2018 basysKom GmbH, opensource@basyskom.com
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtOpcUa module.
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

import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.3
import QtOpcUa 5.13 as QtOpcUa
import "qrc:/machine"

RowLayout {
    property Machine machine
    property QtOpcUa.Connection connection

    opacity: connection.connected ? 1.0 : 0.25
    Tank1Unit {
        startButtonEnabled: connection.connected && machine.state === Machine.MachineState.Idle &&
                            machine.tank1.percentFilled > 0 && machine.tank2.percentFilled < machine.tank2.targetPercent
        stopButtonEnabled: connection.connected && machine.state === Machine.MachineState.Pumping
        percentFilled: machine.tank1.percentFilled
        startButtonText: machine.startMethod.displayName.text
        stopButtonText: machine.stopMethod.displayName.text

        id: tank1unit
        Layout.fillHeight: true
        Layout.fillWidth: true

        Component.onCompleted: {
            tank1unit.startPump.connect(machine.startMethod.callMethod)
            tank1unit.stopPump.connect(machine.stopMethod.callMethod)
        }
    }
    Pump {
        machineIsPumping: machine.state === Machine.MachineState.Pumping

        Component.onCompleted: {
            machine.tank2.onPercentFilledChanged.connect(rotatePump)
        }
    }
    Tank2Unit {
        flushButtonEnabled: connection.connected && machine.state === Machine.MachineState.Idle && machine.tank2.percentFilled > machine.tank2.targetPercent
        percentFilled: machine.tank2.percentFilled
        valveState: machine.tank2valveState
        flushButtonText: machine.flushMethod.displayName.text

        Layout.fillHeight: true
        Layout.fillWidth: true

        id: tank2unit

        Component.onCompleted: {
            tank2unit.flushTank.connect(machine.flushMethod.callMethod)
        }
    }
    Slider {
        id: setpointSlider
        Layout.fillHeight: false
        Layout.preferredHeight: tank1unit.tankHeight
        Layout.alignment: Qt.AlignBottom
        enabled: connection.connected && machine.state === Machine.MachineState.Idle
        from: 0
        to: 100
        value: machine.tank2.targetPercent
        live: false
        stepSize: 1.0
        orientation: Qt.Vertical
        onValueChanged: {
            machine.tank2.targetPercent = value;
        }
    }
    ValueDisplay {
        designation: machine.designation
        percentFilledTank1: machine.tank1.percentFilled
        percentFilledTank2: machine.tank2.percentFilled
        targetPercentTank2: machine.tank2.targetPercent
        machineState: machine.state === Machine.MachineState.Idle ?
                          "Idle" : (machine.state === Machine.MachineState.Pumping ? "Pumping" : "Flushing")
        valveState: machine.tank2valveState
    }
}
