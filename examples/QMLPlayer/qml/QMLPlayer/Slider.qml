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
    color: "transparent"
    radius: 5
    property alias value: grip.value
    property color fillColor: "white"
    property color lineColor: "blue"
    property color gripColor: "white"
    property real gripSize: 10
    property real gripTolerance: 3.0
    property real increment: 0.1
    property bool enabled: true

    Rectangle {
        anchors { left: parent.left; right: parent.right
            verticalCenter: parent.verticalCenter
        }
        height: 3
        color: displayedColor(root.lineColor)

        Rectangle {
            anchors { fill: parent; margins: 1 }
            color: root.fillColor
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        onClicked: {
            if (parent.width) {
                var newValue = mouse.x / parent.width
                if (Math.abs(newValue - parent.value) > parent.increment) {
                    if (newValue > parent.value)
                        parent.value = Math.min(1.0, parent.value + parent.increment)
                    else
                        parent.value = Math.max(0.0, parent.value - parent.increment)
                }
            }
        }
    }

    Rectangle {
        id: grip
        property real value: 0.5
        x: (value * parent.width - width/2)
        anchors.verticalCenter: parent.verticalCenter
        width: root.gripTolerance * root.gripSize
        height: width
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
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: root.gripSize
            height: width
            radius: width/2
            color: root.gripColor
        }
    }

    function displayedColor(c) {
        var tint = Qt.rgba(c.r, c.g, c.b, 0.25)
        return enabled ? c : Qt.tint(c, tint)
    }
}
