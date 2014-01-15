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
import "utils.js" as Utils


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
            var i = a.indexOf("-vd")
            if (i >= 0) {
                player.videoCodecPriority = a[i+1].split(";")
            }
            // FIXME: source is relative to this qml
            //player.source = a[a.length-1]
            //player.play()
        }
    }
    function resurl(s) { //why called twice if in qrc?
        return resprefix + s
    }

    VideoOutput {
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
            now.text = Utils.msec2string(position)
        }
        onPlaying: {
            control.aniShow()
            playBtn.checked = true
            life.text = Utils.msec2string(duration)
            control.hideIfTimedout()
        }
        onStopped: {
            control.visible = true
            control.aniShow()
            playBtn.checked = false
            progress.value = 0
            now.text = Utils.msec2string(0)
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
                    if (player.playbackState !== MediaPlayer.StoppedState)
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
                previewText.text = Utils.msec2string(value*player.duration)
                console.log("hover: "+value + " duration: " + player.duration)
            }
        }
        Text {
            id: now
            text: Utils.msec2string(0)
            anchors {
                top: progress.bottom
                topMargin: 2
                left: progress.left
            }
            color: "white"
        }
        Text {
            id: life
            text: Utils.msec2string(0)
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
                    onCheckedChanged: {
                        if (checked)
                            requestFullScreen()
                        else
                            requestNormalSize()
                    }
                }
                Slider {
                    id: vol
                    width: 80
                    height: 30
                    opacity: 0.9
                    onValueChanged: {
                        player.volume = 2*value
                        voltext.text = vol.value*2
                    }
                    Text {
                        color: "white"
                        id: voltext
                        text: vol.value*2
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
                    onClicked: {
                        help.text = player.source
                        help.visible = true
                    }
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
                    onClicked: {
                        help.text = help.helpText()
                        help.visible = true
                    }
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
                break
            case Qt.Key_Up:
                vol.value = Math.min(1, vol.value+0.05)
                break
            case Qt.Key_Down:
                vol.value = Math.max(0, vol.value-0.05)
                break
            case Qt.Key_Space:
                player.togglePause()
                break
            case Qt.Key_F:
                console.log("F pressed " + fullScreenBtn.checked)
                fullScreenBtn.checked = !fullScreenBtn.checked
                break
            case Qt.Key_Q:
                Qt.quit()
            }
        }
    }
    Rectangle {
        id: help
        property alias text: title.text
        color: "#77222222"
        anchors {
            //top: root.top
            left: root.left
            right: root.right
            bottom: control.top
        }
        height: 60
        visible: false
        Text {
            id: title
            color: "white"
            anchors.fill: parent
            anchors.bottom: parent.bottom
            //horizontalAlignment: Qt.AlignHCenter
            font {
                bold: true
                pixelSize: 16
            }
            onContentHeightChanged: {
                parent.height = contentHeight
            }
        }
        function helpText() {
            return "<h3>QMLPlayer based on QtAV  1.3.0 </h3>"
             + "<p>Distributed under the terms of LGPLv2.1 or later.</p>"
             + "<p>Copyright (C) 2012-2014 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>"
             + "<p>Shanghai University->S3 Graphics, Shanghai, China</p>"
             + "<p>Source code: <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>"
             + "<p>Downloads: <a href='https://sourceforge.net/projects/qtav'>https://sourceforge.net/projects/qtav</a></p>"
             + "\n<h3>Shortcut:</h3>"
             + "<p>M: mute</p><p>F: fullscreen</p><p>Up: volume+</p><p>Down: volume-
                </p><p>Space: pause/play</p><p>Q: quite</p>"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                console.log("click on help<<<<<<<")
                parent.visible = false
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
}
