/******************************************************************************
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

import QtQuick 2.0

Rectangle {
    id: root
    color: "#88eeeeee"
    radius: 5
    property alias value: grip.value
    property color fillColor: "red"
    property color lineColor: "#770000ee"
    property color gripColor: "white"
    property real gripSize: 10
    property real gripTolerance: 3.0
    property real increment: 0.1
    property bool enabled: true
    property bool showGrip: true
    property bool tracking: true
    signal valueChangedByUi
    signal hoverAt(real value)
    // dx, dy: only the direction. dx>0 means enter from left or leave to left
    signal enter(point pos, point dpos)
    signal leave(point pos, point dpos)

    Rectangle {
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        radius: parent.radius
        width: grip.x + grip.radius
        height: parent.height
        color: displayedColor(root.lineColor)
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onClicked: {
            if (parent.width) {
                parent.value = mouse.x / parent.width
                valueChangedByUi(parent.value)
            }
        }
        onMouseXChanged: {
            hoverAt(mouseX/parent.width)
//time and preview
        }
        onEntered: {
            enter(Qt.point(mouseX, mouseY), Qt.point(0, mouseY > height/2 ? 1 : -1))
            hoverAt(mouseX/parent.width)
        }
        onExited: {
            leave(Qt.point(mouseX, mouseY), Qt.point(0, mouseY > height/2 ? 1 : -1))
        }
    }

    Rectangle {
        id: grip
        property real value: 0
        x: (value * parent.width - width/2)
        anchors.verticalCenter: parent.verticalCenter
        width: root.gripTolerance * root.gripSize
        height: parent.height
        radius: width/2
        color: "transparent"

        MouseArea {
            id: mouseArea
            enabled: root.enabled
            anchors.fill:  parent
            drag {
                target: grip
                axis: Drag.XAxis
                minimumX: -parent.width/2
                maximumX: root.width - parent.width/2
            }
            onPositionChanged:  {
                if (drag.active)
                    updatePosition()
            }
            onReleased: {
                updatePosition()
            }
            function updatePosition() {
                value = (grip.x + grip.width/2) / grip.parent.width
                valueChangedByUi(value)
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: root.gripSize
            height: parent.height
            radius: width/2
            color: root.showGrip ? root.gripColor : "transparent"
        }
    }

    function displayedColor(c) {
        var tint = Qt.rgba(c.r, c.g, c.b, 0.25)
        return enabled ? c : Qt.tint(c, tint)
    }
}
