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

    function init(argv) {
        console.log("init>>>>>screen density logical: " + Screen.logicalPixelDensity + " pixel: " + Screen.pixelDensity);
    }

    VideoOutput {
        id: videoOut
        fillMode: VideoOutput.PreserveAspectFit
        anchors.fill: parent
        source: player
        orientation: 0
        SubtitleItem {
            id: subtitleItem
            fillMode: videoOut.fillMode
            rotation: -videoOut.orientation
            source: subtitle
            anchors.fill: parent
        }
        Text {
            id: subtitleLabel
            rotation: -videoOut.orientation
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignBottom
            font: PlayerConfig.subtitleFont
            style: PlayerConfig.subtitleOutline ? Text.Outline : Text.Normal
            styleColor: PlayerConfig.subtitleOutlineColor
            color: PlayerConfig.subtitleColor
            anchors.fill: parent
            anchors.bottomMargin: PlayerConfig.subtitleBottomMargin
        }
    }

    MediaPlayer {
        id: player
        objectName: "player"
        //loops: MediaPlayer.Infinite
        //autoLoad: true
        autoPlay: true
        videoCodecPriority: PlayerConfig.decoderPriorityNames
        onPositionChanged: control.setPlayingProgress(position/duration)
        onPlaying: {
            control.duration = duration
            control.setPlayingState()
            if (!pageLoader.item)
                return
            pageLoader.item.information = {
                source: player.source,
                hasAudio: player.hasAudio,
                hasVideo: player.hasVideo,
                metaData: player.metaData
            }
        }
        onStopped: control.setStopState()
        onPaused: control.setPauseState()
        onError: {
            if (error != MediaPlayer.NoError)
                msg.text = errorString
        }
    }

    Subtitle {
        id: subtitle
        player: player
        enabled: PlayerConfig.subtitleEnabled
        autoLoad: PlayerConfig.subtitleAutoLoad
        engines: PlayerConfig.subtitleEngines
        onContentChanged: { //already enabled
            if (!canRender || !subtitleItem.visible)
                subtitleLabel.text = text
        }
        onLoaded: {
            msg.text = qsTr("Subtitle") + ": " + path.substring(path.lastIndexOf("/") + 1)
            console.log(msg.text)
        }
        onSupportedSuffixesChanged: {
            if (!pageLoader.item)
                return
            pageLoader.item.supportedFormats = supportedSuffixes
        }
        onEngineChanged: { // assume a engine canRender is only used as a renderer
            subtitleItem.visible = canRender
            subtitleLabel.visible = !canRender
        }
        onEnableChanged: {
            subtitleItem.visible = enabled
            subtitleLabel.visible = enabled
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            control.toggleVisible()
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
        anchors.top: root.top
        width: root.width
        height: root.height / 4
        onTextChanged: {
            msg_timer.stop()
            visible = true
            msg_timer.start()
        }
        Timer {
            id: msg_timer
            interval: 2000
            onTriggered: msg.visible = false
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
        onShowInfo: pageLoader.source = "MediaInfoPage.qml"
        onShowHelp: pageLoader.source = "About.qml"
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
        //anchors.bottom: control.top
        y: Math.max(0, Math.min(configPanel.selectedY, root.height - pageLoader.height - control.height))
        width: parent.width - 2*configPanel.width
        height: Utils.scaled(200)
        Loader {
            id: pageLoader
            anchors.right: parent.right
            width: parent.width
            focus: true
            onLoaded: {
                if (!item)
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
            onVisibleChanged: {
                if (!pageLoader.item.visible)
                    pageLoader.source = ""
            }
            onChannelChanged: player.channelLayout = channel
            onSubtitleChanged: subtitle.file = file
            onMuteChanged: player.muted = value
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
        onClicked: {
            pageLoader.source = selectedUrl
            if (pageLoader.item)
                pageLoader.item.visible = true
        }
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
