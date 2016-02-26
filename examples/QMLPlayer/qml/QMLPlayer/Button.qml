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

Rectangle {
    id: root
    property string text
    property url icon
    property alias iconChecked: iconChecked.source
    property bool checkable: false
    property bool checked: false
    property color bgColor: "#555555"
    property color bgColorSelected: "#ee6666dd"
    property color textColor: "white"
    property bool hovered: false //mouseArea.containsMouse
    readonly property alias pressed: mouseArea.pressed
    signal clicked()
    signal pressAndHold()

    opacity: 0.7
    color: checked ? bgColorSelected : mouseArea.pressed ? Qt.darker(bgColor) : bgColor
    border.color: Qt.lighter(color)

    Text {
        id: text
        anchors.fill: parent
        text: root.text
        font.pixelSize: 0.5 * parent.height
        color: textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    Image {
        source: icon
        anchors.fill: parent
        visible: !checked
    }
    Image {
        id: iconChecked
        anchors.fill: parent
        visible: checked
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (root.checkable)
                root.checked = !root.checked
            root.clicked()
        }
        onHoveredChanged: {
            //console.log("button.hover mouseX: " + mouseX)
            if (mouseX > 65535) //qt5.6 touch screen release finger becomes very large e.g. 0x7fffffff
                return
            hovered = mouseArea.containsMouse
        }
        onPressAndHold: root.pressAndHold()
    }
    states: [
        State {
            name: "brighter"
            when: hovered // only the first true State is applied, so put scale and opacity together
            PropertyChanges { target: root; opacity: 1.0; scale: mouseArea.pressed ? 1.06 : 1.0 }
        }
    ]
    transitions: [
        Transition {
            from: "*"; to: "*"
            PropertyAnimation {
                properties: "opacity,scale"
                easing.type: Easing.OutQuart
                duration: 300
            }
        }
    ]
}
