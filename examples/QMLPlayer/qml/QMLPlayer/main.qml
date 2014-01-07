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
    width: 800
    height: 450
    color: "black"
    signal requestFullScreen
    signal requestNormalSize

    // "/xxx" will be resolved as qrc:///xxx. while "xxx" is "qrc:///QMLDIR/xxx
    property string resprefix: Qt.resolvedUrl(" ").substring(0, 4) == "qrc:" ? "/" : ""
    function init(argv) {
        var a = JSON.parse(argv)
        if (a.length > 1) {
            // FIXME: source is relative to this qml
            //player.source = a[a.length-1]
            //player.play()
        }
    }
    function resurl(s) { //why called twice if in qrc?
        return resprefix + s
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
        //loops: MediaPlayer.Infinite
        onPositionChanged: {
            console.log("pos change")
            progress.value = position/duration
            now.text = msec2string(position)
        }
        onPlaying: {
            control.aniShow()
            playBtn.checked = true
            life.text = msec2string(duration)
            control.hideIfTimedout()
            help.title = source
        }
        onStopped: {
            control.visible = true
            control.aniShow()
            playBtn.checked = false
            progress.value = 0
            now.text = msec2string(0)
        }
        onPaused: {
            control.aniShow()
            control.visible = true
            playBtn.checked = false
        }
    }

    // TODO: Control.qml
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
            onEnter: {
                if (player.playbackState == MediaPlayer.StoppedState)
                    return
                preview.visible = true
                preview.state = dpos.y > 0 ? "out" : "out_"
                preview.state = "in"
                preview.anchors.leftMargin = pos.x - preview.width/2
            }
            onLeave: {
                if (player.playbackState == MediaPlayer.StoppedState)
                    return
                preview.state = dpos.y > 0 ? "out" : "out_"
            }
            onHoverAt: {
                if (player.playbackState == MediaPlayer.StoppedState)
                    return
                var v = value * progress.width
                preview.anchors.leftMargin = v - preview.width/2
                previewText.text = msec2string(value*player.duration)
                console.log("hover: "+value + " duration: " + player.duration)
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
        Rectangle {
            id: preview
            opacity: 0.9
            anchors.left: progress.left
            anchors.bottom: progress.top
            width: 60
            height: 16
            color: "black"
            state: "out"
            Text {
                id: previewText
                anchors.fill: parent
                text: ""
                color: "white"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
            states: [
                State {
                    name: "in"
                    PropertyChanges {
                        target: preview
                        anchors.bottomMargin: 2
                        opacity: 0.9
                    }
                },
                State {
                    name: "out"
                    PropertyChanges {
                        target: preview
                        anchors.bottomMargin: preview.height
                        opacity: 0
                    }
                },
                State {
                    name: "out_"
                    PropertyChanges {
                        target: preview
                        anchors.bottomMargin: -preview.height
                        opacity: 0
                    }
                }
            ]
            transitions: [
                Transition {
                    to: "in"
                    NumberAnimation {
                        properties: "opacity,anchors.bottomMargin"
                        duration: 200
                        easing.type: Easing.OutCubic
                    }
                },
                Transition {
                    to: "out"
                    NumberAnimation {
                        properties: "opacity,anchors.bottomMargin"
                        duration: 200
                        easing.type: Easing.InCubic
                    }
                },
                Transition {
                    to: "out_"
                    NumberAnimation {
                        properties: "opacity,anchors.bottomMargin"
                        duration: 200
                        easing.type: Easing.InCubic
                    }
                }//,
                //Transition { from: "out"; to: "out_" },
                //Transition { from: "out_"; to: "out" }
            ]
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
                icon: resurl("theme/default/play.svg")
                iconChecked: resurl("theme/default/pause.svg")

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
                    icon: resurl("theme/default/stop.svg")
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
                    icon: resurl("theme/default/backward.svg")
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
                    icon: resurl("theme/default/forward.svg")
                    onClicked: {
                        player.seek(player.position+10000)
                    }
                }
            }
            Row {
                anchors.left: parent.left
                anchors.leftMargin: 50
                anchors.verticalCenter: parent.verticalCenter
                Button {
                    id: fullScreenBtn
                    checkable: true
                    checked: false
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 25
                    height: 25
                    icon: resurl("theme/default/fullscreen.svg")
                    iconChecked: resurl("theme/default/fullscreen.svg")
                    visible: true
                    onClicked: {
                        if (checked)
                            requestFullScreen()
                        else
                            requestNormalSize()
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
                    icon: resurl("theme/default/info.svg")
                    visible: true
                    onClicked: help.visible = !help.visible
                }
                Button {
                    id: openFileBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 25
                    height: 25
                    icon: resurl("theme/default/open.svg")
                    onClicked: fileDialog.open()
                }
                Button {
                    id: helpBtn
                    bgColor: "transparent"
                    bgColorSelected: "transparent"
                    width: 25
                    height: 25
                    icon: resurl("theme/default/help.svg")
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
        var ss = t%60
        t = (t-ss)/60
        var mm = t%60
        var hh = (t-mm)/60
        if (ss < 10)
            ss = "0" + ss
        if (mm < 10)
            mm = "0" + mm
        if (hh < 10)
            hh = "0" + hh
        return hh + ":" + mm +":" + ss
    }
}
