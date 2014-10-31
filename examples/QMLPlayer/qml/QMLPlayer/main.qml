/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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
import QtAV 1.4
import QtQuick.Window 2.1
import "utils.js" as Utils

Rectangle {
    id: root
    objectName: "root"
    width: Utils.scaled(800)
    height: Utils.scaled(450)
    color: "black"
    signal requestFullScreen
    signal requestNormalSize

    // "/xxx" will be resolved as qrc:///xxx. while "xxx" is "qrc:///QMLDIR/xxx
    property string resprefix: Qt.resolvedUrl(" ").substring(0, 4) == "qrc:" ? "/" : ""
    function init(argv) {
        console.log("init>>>>>screen density logical: " + Screen.logicalPixelDensity + " pixel: " + Screen.pixelDensity);
        var a = JSON.parse(argv)
        if (a.length > 1) {
            var i = a.indexOf("-vd")
            if (i >= 0) {
                player.videoCodecPriority = a[i+1].split(";")
            } else {
                player.videoCodecPriority = ["VAAPI", "DXVA", "CUDA", "FFmpeg"];
            }

            // FIXME: source is relative to this qml
            //player.source = a[a.length-1]
            //player.play()
        } else {
            player.videoCodecPriority = ["VAAPI", "DXVA", "CUDA", "FFmpeg"];
        }
    }
    function resurl(s) { //why called twice if in qrc?
        return resprefix + s
    }

    VideoOutput {
        id: videoOut
        fillMode: VideoOutput.PreserveAspectFit
        anchors.fill: parent
        source: player
        orientation: 0
        SubtitleItem {
            id: subtitleItem
            visible: subtitle.enabled
            fillMode: videoOut.fillMode
            rotation: -videoOut.orientation
            source: subtitle
            anchors.fill: parent
        }
        Text {
            id: subtitleLabel
            visible: subtitle.enabled
            rotation: -videoOut.orientation
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignBottom
            font {
                pixelSize: Utils.scaled(20)
                bold: true
            }
            style: Text.Outline
            styleColor: "blue"
            color: "white"
            anchors.fill: parent
        }
    }

    MediaPlayer {
        id: player
        objectName: "player"
        //loops: MediaPlayer.Infinite
        //autoLoad: true
        autoPlay: true
        onPositionChanged: {
            control.setPlayingProgress(position/duration)
        }
        onPlaying: {
            control.duration = duration
            control.setPlayingState()
            if (!pageLoader.item)
                return
            if (pageLoader.item.information === "undefined")
                return
            pageLoader.item.information = {
                source: player.source,
                hasAudio: player.hasAudio,
                hasVideo: player.hasVideo,
                metaData: player.metaData
            }
        }
        onStopped: {
            control.setStopState()
        }
        onPaused: {
            control.setPauseState()
        }
        onError: {
            if (error != MediaPlayer.NoError)
                msg.text = errorString
        }
    }

    Subtitle {
        id: subtitle
        player: player
        //enabled: false
        onContentChanged: {
            if (!canRender || !enabled || !subtitleItem.visible)
                subtitleLabel.text = text
        }
        onLoaded: {
            msg.text = qsTr("Subtitle") + ": " + path.substring(path.lastIndexOf("/") + 1)
        }
    }
    MouseArea {
        anchors.fill: parent
        onPressed: {
            control.toggleVisible()
            console.log("mouse: " + mouseX + " mouse.x: " + mouse.x + " w: " + root.width)
            if (root.width - mouseX < Utils.scaled(60)) {
                configPanel.state = "show"
            } else {
                configPanel.state = "hide"
            }
        }
    }
    Text {
        id: msg
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Utils.scaled(20)
        style: Text.Outline
        styleColor: "green"
        color: "white"
        anchors {
            top: root.top
            left: root.left
            right: root.right
        }
        height: root.height / 4
        onTextChanged: {
            msg_timer.stop()
            visible = true
            msg_timer.start()
        }
        Timer {
            id: msg_timer
            interval: 2000
            onTriggered: {
                msg.visible = false
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
        height: Utils.scaled(60)
        visible: false
        Text {
            id: title
            color: "white"
            anchors.fill: parent
            anchors.bottom: parent.bottom
            anchors.margins: Utils.scaled(8)
            //horizontalAlignment: Qt.AlignHCenter
            font {
                pixelSize: Utils.scaled(12)
            }
            onContentHeightChanged: {
                parent.height = contentHeight + 2*anchors.margins
            }
            onLinkActivated: Qt.openUrlExternally(link)
        }
        function helpText() {
            return "<h3>QMLPlayer based on QtAV  1.4.1 </h3>"
             + "<p>Distributed under the terms of LGPLv2.1 or later.</p>"
             + "<p>Copyright (C) 2012-2014 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>"
             + "<p>Shanghai University->S3 Graphics->Deepin, Shanghai, China</p>"
             + "<p>Source code: <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>"
             + "<p>Web Site: <a href='http://wang-bin.github.io/QtAV'>http://wang-bin.github.io/QtAV</a></p>"
             + "\n<h3>Shortcut:</h3>"
             + "<p>M: mute</p><p>F: fullscreen</p><p>Up/Down: volume +/-</p><p>Left/Right: Seek backward/forward
                </p><p>Space: pause/play</p><p>Q: quite</p>"
             + "<p>R: rotate</p><p>A: aspect ratio</p>"
        }
        Button {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Utils.scaled(0)
            width: Utils.scaled(20)
            height: Utils.scaled(20)
            icon: resurl("theme/default/close.svg")
            onClicked: parent.visible = false
        }
        Button {
            id: donateBtn
            text: qsTr("Donate")
            bgColor: "#990000ff"
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Utils.scaled(8)
            width: Utils.scaled(80)
            height: Utils.scaled(40)
            onClicked: Qt.openUrlExternally("http://wang-bin.github.io/QtAV#donate")
        }
    }

    ControlPanel {
        id: control
        anchors {
            left: parent.left
            bottom: parent.bottom
            right: parent.right
            margins: Utils.scaled(12)
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
            pageLoader.source = "MediaInfoPage.qml"
        }
        onShowHelp: {
            help.text = help.helpText()
            donateBtn.visible = true
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
            case Qt.Key_Plus:
                player.playbackRate += 0.1;
                console.log("player.playbackRate: " + player.playbackRate);
                break;
            case Qt.Key_Minus:
                player.playbackRate = Math.max(0.1, player.playbackRate - 0.1);
                break;
            case Qt.Key_F:
                control.toggleFullScreen()
                break
            case Qt.Key_R:
                videoOut.orientation += 90
                break;
            case Qt.Key_T:
                videoOut.orientation -= 90
                break;
            case Qt.Key_A:
                if (videoOut.fillMode === VideoOutput.Stretch) {
                    videoOut.fillMode = VideoOutput.PreserveAspectFit
                } else if (videoOut.fillMode === VideoOutput.PreserveAspectFit) {
                    videoOut.fillMode = VideoOutput.PreserveAspectCrop
                } else {
                    videoOut.fillMode = VideoOutput.Stretch
                }
                break
            case Qt.Key_Q:
                Qt.quit()
            }
        }
    }
    Item {
        id: configPage
        anchors.right: configPanel.left
        y: configPanel.selectedY
        width: parent.width - 2*configPanel.width
        height: Utils.scaled(200)
        Loader {
            id: pageLoader
            anchors.fill: parent
            focus: true
            onLoaded: {
                if (!item || item.information === "undefined")
                    return
                item.information = {
                    source: player.source,
                    hasAudio: player.hasAudio,
                    hasVideo: player.hasVideo,
                    metaData: player.metaData
                }
            }
        }
        Connections {
            target: pageLoader.item
            onClose: pageLoader.source = ""
            onChannelChanged: player.channelLayout = channel
            onSubtitleChanged: subtitle.file = file
            onEngineChanged: subtitle.engines = [ engine ]
            onEnabledChanged: subtitle.enabled = value
            onAutoLoadChanged: subtitle.autoLoad = autoLoad
            onDecoderChanged: player.videoCodecPriority = [ value ]
        }
    }
    ConfigPanel {
        id: configPanel
        anchors {
            top: parent.top
            right: parent.right
            bottom: control.top
        }
        width: Utils.scaled(140)
        onSelectedUrlChanged: pageLoader.source = selectedUrl
        states: [
            State {
                name: "show"
                PropertyChanges {
                    target: configPanel
                    opacity: 0.9
                    anchors.rightMargin: 0
                }
            },
            State {
                name: "hide"
                PropertyChanges {
                    target: configPanel
                    opacity: 0
                    anchors.rightMargin: -configPanel.width
                }
            }
        ]
        transitions: [
            Transition {
                from: "*"; to: "*"
                PropertyAnimation {
                    properties: "opacity,anchors.rightMargin"
                    easing.type: Easing.OutQuart
                    duration: 500
                }
            }
        ]
    }
    FileDialog {
        id: fileDialog
        title: "Please choose a media file"
        onAccepted: {
            player.source = fileDialog.fileUrl
            //player.stop() //remove this if autoLoad works
            //player.play()
        }
    }
}
