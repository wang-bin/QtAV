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

Item {
    id: root
    //color: textColor
    //radius: 0.25 * height
    opacity: 0.7

    property string text
    property url icon
    property alias iconChecked: iconChecked.source
    property bool checkable: false
    property bool checked: false
    property color bgColor: "#00000000"
    property color bgColorSelected: "#2000ff00"
    property color textColor: "white"
    property alias enabled: mouseArea.enabled

    signal clicked

    Rectangle {
        anchors { fill: parent; margins: 1 }
        color: mouseArea.pressed ? bgColorSelected : bgColor
        border.color: Qt.lighter(color)
        //radius: 0.25 * height

        Text {
            id: text
            anchors.centerIn: parent
            text: root.text
            font.pixelSize: 0.5 * parent.height
            color: textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image {
            source: icon
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            visible: !checked
        }
        Image {
            id: iconChecked
            width: parent.width
            height: parent.height
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
            onEntered: {
                anim.stop()
                anim.reset()
                anim.start()
            }
            onExited: {
                anim.stop()
                anim.reverse()
                anim.start()
            }
        }
        PropertyAnimation {
            id: anim
            target: root
            properties: "opacity"
            function reverse() {
                to = 0.7
                from = root.opacity
            }
            function reset() {
                from = root.opacity
                to = 1.0
            }
        }
        states: State {
            name: "brighter"
            when: mouseArea.pressed
            PropertyChanges { target: root; opacity: 0.7 }
        }
        transitions: Transition {
            reversible: true
            to: "brighter"
            ParallelAnimation {
                NumberAnimation { properties: "opacity" }
            }
        }
    }
}
