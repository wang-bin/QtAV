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
        //autoLoad: true
        onPositionChanged: {
            control.setPlayingProgress(position/duration)
        }
        onPlaying: {
            control.duration = duration
            control.setPlayingState()
        }
        onStopped: {
            control.setStopState()
        }
        onPaused: {
            control.setPauseState()
        }
    }

    ControlPanel {
        id: control
        anchors {
            left: parent.left
            bottom: parent.bottom
            right: parent.right
            margins: 12
        }

        mediaSource: player.source
        duration: player.duration

        onSeek: player.seek(ms)
        onSeekForward: player.seek(player.position + ms)
        onSeekBackward: player.seek(player.position - ms)
        onPlay: player.play()
        onStop: player.stop()
        onTogglePause: {
            if (player.playbackState == MediaPlayer.PlayingState) {
                player.pause()
            } else {
                player.play()
            }
        }
        onVolumeChanged: player.volume = volume
        onOpenFile: fileDialog.open()
        onShowInfo: {
            help.text = player.source
            help.visible = true
        }
        onShowHelp: {
            help.text = help.helpText()
            help.visible = true
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
                player.seek(player.position + 10000)
                break
            case Qt.Key_Left:
                player.seek(player.position - 10000)
                break
            case Qt.Key_Up:
                control.volume = Math.min(2, control.volume+0.05)
                break
            case Qt.Key_Down:
                control.volume = Math.max(0, control.volume-0.05)
                break
            case Qt.Key_Space:
                if (player.playbackState == MediaPlayer.PlayingState) {
                    player.pause()
                } else if (player.playbackState == MediaPlayer.PausedState){
                    player.play()
                }
                break
            case Qt.Key_F:
                control.toggleFullScreen()
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
                pixelSize: 16
            }
            onContentHeightChanged: {
                parent.height = contentHeight
            }
        }
        function helpText() {
            return "<h3>QMLPlayer based on QtAV  1.3.1 </h3>"
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
            player.stop() //remove this if autoLoad works
            player.play()
        }
        //Component.onCompleted: visible = true
    }
}
