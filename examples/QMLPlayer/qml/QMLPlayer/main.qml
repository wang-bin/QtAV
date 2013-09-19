/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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

import QtQuick 2.1
import QtQuick.Dialogs 1.0
//import QtMultimedia 5.0
import QtAV 1.3

Rectangle {
    width: 640
    height: 360
    color: "black"

    VideoOutput {
        id: vo
        x: parent.x
        y: parent.y
        width: parent.width/2
        height: parent.height
        source: player
    }
    VideoOutput {
        id: vo2
        x: parent.width/2
        y: parent.y
        width: parent.width/2
        height: parent.height
        source: player
    }
    MediaPlayer {
        id: player
    }
    MouseArea {
        anchors.fill: parent
        onDoubleClicked: {
            fileDialog.open()
        }
    }
    Text {
        anchors.fill: parent
        text: "Double click to select a file\n" +
        "Left/Right: seek forward/backward\n" +
        "Space: pause/resume\n" +
        "Q: quit"
        font.pointSize: 28
        color: "white"
    }
    Item {

        anchors.fill: parent
        focus: true
        Keys.onPressed: {

            switch (event.key) {
            case Qt.Key_M:
                player.muted = !player.muted
                break
            case Qt.Key_Right:
                player.seekForward()
                break
            case Qt.Key_Left:
                player.seekBackward()
            case Qt.Key_Space:
                player.togglePause()
                break
            case Qt.Key_Q:
                Qt.quit()
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Please choose a media file"
        onAccepted: {
            player.source = fileDialog.fileUrl
            player.play()
        }
        Component.onCompleted: visible = true
    }
}
