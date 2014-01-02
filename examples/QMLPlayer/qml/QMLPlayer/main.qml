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
    id: root
    objectName: "root"
    width: 240*3
    height: 240
    color: "black"

    function init(argv) {
        var a = JSON.parse(argv)
        if (a.length > 1) {
            player.source = a[a.length-1]
            player.play()
        }
    }

    VideoOutput {
        id: vo
        fillMode: VideoOutput.PreserveAspectFit
        anchors.fill: parent
        source: player
    }
    MediaPlayer {
        id: player
        objectName: "player"
        onPositionChanged: {
            console.log("pos change")
            progress.value = position/duration
            now.text = msec2string(position)
        }
        onPlaying: {
            playBtn.checked = true
            life.text = msec2string(duration)
            control.hideIfTimedout()
            help.title = source
        }
        onStopped: {
            control.visible = true
            control.aniShow()
            playBtn.checked = false
        }
    }

    Rectangle {
        id: control
        color: "black"
        opacity: 0.9
        radius: 10
        height: 80
        anchors {
            left: parent.left
            bottom: parent.bottom
            right: parent.right
            margins: 12
        }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#aa445566" }
            GradientStop { position: 0.618; color: "#bb1a2b3a" }
            GradientStop { position: 1.0; color: "#ff000000" }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            // onEntered, onExited
            onHoveredChanged: {
                //var m = mapToItem(control, mouse.x, mouse.y)
                // TODO: why control.contains(m) always true?
                if (containsMouse) {
                    if (timer.running) //timer may ran a few seconds(<3) ago
                        timer.stop();
                    control.aniShow()
                } else {
                    if (player.playbackState != MediaPlayer.StoppedState)
                        timer.start()
                }
            }
        }
        ProgressBar {
            id: progress
            anchors {
                top: parent.top
                topMargin: 8
                left: parent.left
                leftMargin: 20
                right: parent.right
                rightMargin: 20
            }
            height: 10
            onValueChangedByUi: {
                if (player.playbackState != MediaPlayer.StoppedState) {
                    player.seek(player.duration * value)
                }
            }
        }
        Text {
            id: now
            text: msec2string(0)
            anchors {
                top: progress.bottom
                topMargin: 2
                left: progress.left
            }
            color: "white"
        }
        Text {
            id: life
            text: msec2string(0)
            anchors {
                top: progress.bottom
                topMargin: 2
                right: progress.right
            }
            color: "white"
        }

        Item {
            anchors {
                top: progress.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: 8
            }
            Button {
                id: playBtn
                anchors.centerIn: parent
                checkable: true
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: 50
                height: 50
                icon: "play.svg"
                iconChecked: "pause.svg"
                onClicked: {
                    if (player.playbackState == MediaPlayer.StoppedState) {
                        player.play()
                    } else {
                        player.togglePause()
                    }
                }
            }
            Row {
                anchors.right: playBtn.left
                anchors.verticalCenter: playBtn.verticalCenter
                spacing: 4
                Button {
                    id: stopBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 35
                    height: 35
                    icon: "stop.svg"
                    onClicked: {
                        player.stop()
                    }
                }
                Button {
                    id: backwardBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 35
                    height: 35
                    icon: "backward.svg"
                    onClicked: {
                        player.seek(player.position-10000)
                    }
                }
            }
            Row {
                anchors.left: playBtn.right
                anchors.verticalCenter: playBtn.verticalCenter
                spacing: 4
                Button {
                    id: forwardBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 35
                    height: 35
                    icon: "forward.svg"
                    onClicked: {
                        player.seek(player.position+10000)
                    }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 50
                anchors.verticalCenter: parent.verticalCenter
                Button {
                    id: infoBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 25
                    height: 25
                    icon: "info.svg"
                    visible: true
                    onClicked: help.visible = !help.visible
                }
                Button {
                    id: openFileBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 25
                    height: 25
                    icon: "open.svg"
                    onClicked: fileDialog.open()
                }
                Button {
                    id: helpBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 25
                    height: 25
                    icon: "help.svg"
                    onClicked: help.visible = !help.visible
                }
            }

        }
        Timer {
            id: timer
            interval: 3000
            onTriggered: {
                control.aniHide()
                //control.visible = false //no mouse event
            }
        }
        function hideIfTimedout() {
            timer.start()
        }
        PropertyAnimation {
            id: anim
            target: control
            properties: "opacity"
            function reverse() {
                duration = 1500
                to = 0
                from = control.opacity
            }
            function reset() {
                duration = 200
                from = control.opacity
                to = 0.9
            }
        }
        function aniShow() {
            anim.stop()
            anim.reset()
            anim.start()
        }
        function aniHide() {
            anim.stop()
            anim.reverse()
            anim.start()
        }
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
    Rectangle {
        id: help
        property alias title: title.text
        color: "#77222222"
        anchors {
            top: root.top
            left: root.left
            right: root.right
        }
        height: 40
        opacity: 0.9
        visible: false
        Text {
            id: title
            color: "white"
            anchors.fill: parent
            //horizontalAlignment: Qt.AlignHCenter
            font {
                bold: true
                pixelSize: 16
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
        //Component.onCompleted: visible = true
    }
    function msec2string(t) {
        t = Math.floor(t/1000)
        var s = t%60
        t /= 60
        var m = Math.floor(t%60)
        t /= 60
        var h = Math.floor(t/24)
        if (s < 10)
            s = "0" + s
        if (m < 10)
            m = "0" + m
        if (h < 10)
            h = "0" + h
        return h + ":" + m +":" + s
    }
}
