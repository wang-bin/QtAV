/******************************************************************************
    Copyright (C) 2013-2016 Wang Bin <wbsecg1@gmail.com>

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
import "utils.js" as Utils

Item {
    id: root
    property alias value: grip.value
    property color fillColor: "white"
    property color lineColor: "blue"
    property real lineWidth: Utils.scaled(6)
    property color gripColor: "white"
    property real gripSize: Utils.scaled(12)
    property real gripTolerance: Utils.scaled(3.0)
    property int orientation: Qt.Horizontal
    property bool hovered: false //mouseArea.containsMouse || gripMouseArea.containsMouse

    Rectangle {
        anchors.centerIn: parent
        height: orientation === Qt.Horizontal ? lineWidth : parent.height
        width: orientation === Qt.Horizontal ? parent.width : lineWidth
        color: lineColor
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onHoveredChanged: {
            //console.log("slider.hover mouseX: " + mouseX)
            if (mouseArea.mouseX == 2147483447) //qt5.6 touch screen release finger becomes 0x7fffffff
                return
            hovered = mouseArea.containsMouse
        }
        onClicked: {
            var newValue = mouse.x / parent.width
            if (orientation === Qt.Horizontal) {
                newValue = mouse.x / parent.width
            } else {
                newValue = mouse.y / parent.height
            }
            var increment = 1.0/width
            if (Math.abs(newValue - parent.value) > parent.increment) {
                if (newValue > parent.value)
                    parent.value = Math.min(1.0, parent.value + parent.increment)
                else
                    parent.value = Math.max(0.0, parent.value - parent.increment)
            }
        }
    }

    Item {
        id: grip
        property real value: 0.5
        x: orientation === Qt.Horizontal ? (value * parent.width - width/2) : (parent.width - width)/2
        y: orientation === Qt.Horizontal ? (parent.height - height)/2 : (value * parent.height - height/2)
        width: root.gripTolerance * root.gripSize
        height: width
        readonly property real radius: width/2

        MouseArea {
            id: gripMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onHoveredChanged: {
                //console.log("slider.grip.hover mouseX: " + mouseX)
                if (gripMouseArea.mouseX == 2147483447) //qt5.6 touch screen release finger becomes 0x7fffffff
                    return
                hovered = gripMouseArea.containsMouse
            }
            drag {
                target: grip
                axis: orientation === Qt.Horizontal ? Drag.XAxis : Drag.YAxis
                minimumX: orientation === Qt.Horizontal ? -parent.radius : 0
                maximumX: orientation === Qt.Horizontal ? root.width - parent.radius : 0
                minimumY: orientation === Qt.Horizontal ? 0 : -parent.radius
                maximumY: orientation === Qt.Horizontal ? 0 : root.height - parent.radius
            }
            onPositionChanged:  {
                if (drag.active)
                    updatePosition()
            }
            onReleased: updatePosition()
            function updatePosition() {
                if (orientation === Qt.Horizontal)
                    value = (grip.x + grip.radius) / grip.parent.width
                else
                    value = (grip.y + grip.radius) / grip.parent.height
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
}
