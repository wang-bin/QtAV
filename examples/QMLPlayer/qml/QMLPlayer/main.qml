/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2016 Wang Bin <wbsecg1@gmail.com>

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

import QtQuick 2.0
import QtQuick.Dialogs 1.0
//import QtMultimedia 5.0
import QtAV 1.6
import QtQuick.Window 2.1
import "utils.js" as Utils

Rectangle {
    id: root
    layer.enabled: false //VideoOutput2 can not update correctly if layer.enable is true. default is false
    objectName: "root"
    width: Utils.scaled(800)
    height: Utils.scaled(450)
    color: "black"
    signal requestFullScreen
    signal requestNormalSize

    function init(argv) {
        console.log("init>>>>>screen density logical: " + Screen.logicalPixelDensity + " pixel: " + Screen.pixelDensity);
    }

    VideoOutput2 {
        id: videoOut
        opengl: true
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
        videoCapture {
            autoSave: true
            onSaved: {
                msg.info("capture saved at: " + path)
            }
        }
        onSourceChanged: {
            msg.info("url: " + source)
        }

        onDurationChanged: control.duration = duration
        onPlaying: {
            control.mediaSource = player.source
            control.setPlayingState()
            if (!pageLoader.item)
                return
            if (pageLoader.item.information) {
                pageLoader.item.information = {
                    source: player.source,
                    hasAudio: player.hasAudio,
                    hasVideo: player.hasVideo,
                    metaData: player.metaData
                }
            }
        }
        onSeekFinished: {
            console.log("seek finished " + Utils.msec2string(position))
        }

        onInternalAudioTracksChanged: {
            if (typeof(pageLoader.item.internalAudioTracks) != "undefined")
                pageLoader.item.internalAudioTracks = player.internalAudioTracks
        }
        onExternalAudioTracksChanged: {
            if (typeof(pageLoader.item.externalAudioTracks) != "undefined")
                pageLoader.item.externalAudioTracks = player.externalAudioTracks
        }
        onInternalSubtitleTracksChanged: {
            if (typeof(pageLoader.item.internalSubtitleTracks) != "undefined")
                pageLoader.item.internalSubtitleTracks = player.internalSubtitleTracks
        }

        onStopped: control.setStopState()
        onPaused: control.setPauseState()
        onError: {
            if (error != MediaPlayer.NoError) {
                msg.error(errorString)
            }
        }
        muted: control.mute // TODO: control from system
        volume: control.volume
        onVolumeChanged: { //why need this? control.volume = player.volume is not enough?
            if (Math.abs(control.volume - volume) >= 0.01) {
                control.volume = volume
            }
        }
        onStatusChanged: {
            if (status == MediaPlayer.Loading)
                msg.info("Loading " + source)
            else if (status == MediaPlayer.Buffering)
                msg.info("Buffering")
            else if (status == MediaPlayer.Buffered)
                msg.info("Buffered")
            else if (status == MediaPlayer.EndOfMedia)
                msg.info("End")
            else if (status == MediaPlayer.InvalidMedia)
                msg.info("Invalid")
        }
        onBufferProgressChanged: {
            msg.info("Buffering " + Math.floor(bufferProgress*100) + "%...")
        }
       // onSeekFinished: msg.info("Seek finished: " + Utils.msec2string(position))
    }
    Subtitle {
        id: subtitle
        player: player
        enabled: PlayerConfig.subtitleEnabled
        autoLoad: PlayerConfig.subtitleAutoLoad
        engines: PlayerConfig.subtitleEngines
        delay: PlayerConfig.subtitleDelay
        fontFile: PlayerConfig.assFontFile
        fontFileForced: PlayerConfig.assFontFileForced
        fontsDir: PlayerConfig.assFontsDir

        onContentChanged: { //already enabled
            if (!canRender || !subtitleItem.visible)
                subtitleLabel.text = text
        }
        onLoaded: {
            msg.info(qsTr("Subtitle") + ": " + path.substring(path.lastIndexOf("/") + 1))
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
        onEnabledChanged: {
            subtitleItem.visible = enabled
            subtitleLabel.visible = enabled
        }
    }

    MultiPointTouchArea {
        //mouseEnabled: true //not available on qt5.2(ubuntu14.04)
        anchors.fill: parent
        onGestureStarted: {
            if (player.playbackState == MediaPlayer.StoppedState)
                return
            var p = gesture.touchPoints[0]
            var dx = p.x - p.previousX
            var dy = p.y - p.previousY
            var t = dy/dx
            var ml = Math.abs(dx) + Math.abs(dy)
            var ML = Math.abs(p.x - p.startX) + Math.abs(p.y - p.startY)
            //console.log("dx: " + dx + " dy: " + dy + " ml: " + ml + " ML: " + ML)
            if (ml < 2.0 || 5*ml < ML)
                return
            if (t > -1 && t < 1) {
                if (dx > 0) {
                    player.seekForward()
                } else {
                    player.seekBackward()
                }
            }
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onDoubleClicked: {
                control.toggleVisible()
            }
            onMouseXChanged: {
                if (root.width - mouseX < configPanel.width) {
                    configPanel.state = "show"
                } else {
                    configPanel.state = "hide"
                }
                if (player.playbackState == MediaPlayer.StoppedState || !player.hasVideo)
                    return;
                if (mouseY < control.y - control.previewHeight)
                    control.hidePreview() // TODO: check previw hovered too
                else
                    control.showPreview(mouseX/parent.width)
            }
        }
    }
    Text {
        id: msg
        objectName: "msg"
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
        function error(txt) {
            styleColor = "red"
            text = txt
        }
        function info(txt) {
            styleColor = "green"
            text = txt
        }
    }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_M:
                control.mute = !control.mute
                break
            case Qt.Key_Right:
                player.fastSeek = event.isAutoRepeat
                player.seek(player.position + 10000)
                break
            case Qt.Key_Left:
                player.fastSeek = event.isAutoRepeat
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
            case Qt.Key_C:
                player.videoCapture.capture()
                break
            case Qt.Key_A:
                if (videoOut.fillMode === VideoOutput.Stretch) {
                    videoOut.fillMode = VideoOutput.PreserveAspectFit
                } else if (videoOut.fillMode === VideoOutput.PreserveAspectFit) {
                    videoOut.fillMode = VideoOutput.PreserveAspectCrop
                } else {
                    videoOut.fillMode = VideoOutput.Stretch
                }
                break
            case Qt.Key_O:
                fileDialog.open()
                break;
            case Qt.Key_N:
                player.stepForward()
                break
            case Qt.Key_B:
                player.stepBackward()
                break;
            //case Qt.Key_Back:
            case Qt.Key_Q:
                Qt.quit()
                break
            }
        }
    }
    DropArea {
        anchors.fill: root
        onEntered: {
            if (!drag.hasUrls)
                return;
            console.log(drag.urls)
            player.source = drag.urls[0]
        }
    }

    Item {
        id: configPage
        anchors.right: configPanel.left
        //anchors.bottom: control.top
        y: Math.max(0, Math.min(configPanel.selectedY, root.height - pageLoader.height - control.height))
        width: parent.width < 4*configPanel.width ? parent.width - configPanel.width : parent.width/2 + configPanel.width
        height: Utils.scaled(200)
        Loader {
            id: pageLoader
            anchors.right: parent.right
            width: parent.width
            focus: true
            onLoaded: {
                if (!item)
                    return
                if (item.information) {
                    item.information = {
                        source: player.source,
                        hasAudio: player.hasAudio,
                        hasVideo: player.hasVideo,
                        metaData: player.metaData
                    }
                }
                if (item.hasOwnProperty("internalAudioTracks"))
                    item.internalAudioTracks = player.internalAudioTracks
                if (typeof(item.externalAudioTracks) != "undefined")
                    item.externalAudioTracks = player.externalAudioTracks
                if ("internalSubtitleTracks" in item)
                    item.internalSubtitleTracks = player.internalSubtitleTracks
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
            onExternalAudioChanged: player.externalAudio = file
            onAudioTrackChanged: player.audioTrack = track
            onSubtitleTrackChanged: player.internalSubtitleTrack = track
            onZeroCopyChanged: {
                var opt = player.videoCodecOptions
                if (value) {
                    opt["copyMode"] = "ZeroCopy"
                } else {
                    if (Qt.platform.os == "osx")
                        opt["copyMode"] = "LazyCopy"
                    else
                        opt["copyMode"] = "OptimizedCopy"
                }
                player.videoCodecOptions = opt
            }
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

        onSeek: {
            player.fastSeek = false
            player.seek(ms)
        }
        onSeekForward: {
            player.fastSeek = false
            player.seek(player.position + ms)
        }
        onSeekBackward: {
            player.fastSeek = false
            player.seek(player.position - ms)
        }
        onPlay: player.play()
        onStop: player.stop()
        onTogglePause: {
            if (player.playbackState == MediaPlayer.PlayingState) {
                player.pause()
            } else {
                player.play()
            }
        }
        volume: player.volume
        onOpenFile: fileDialog.open()
        onShowInfo: pageLoader.source = "MediaInfoPage.qml"
        onShowHelp: pageLoader.source = "About.qml"
    }
    FileDialog {
        id: fileDialog
        title: "Please choose a media file"
        folder: PlayerConfig.lastFile
        onAccepted: {
            PlayerConfig.lastFile = fileUrl.toString()
            player.source = fileDialog.fileUrl
            //player.stop() //remove this if autoLoad works
            //player.play()
        }
    }
    Connections {
        target: Qt.application
        onStateChanged: { //since 5.1
            if (Qt.platform.os !== "android")
                return
            // winrt is handled by system
            switch (Qt.application.state) {
            case Qt.ApplicationSuspended:
            case Qt.ApplicationHidden:
                player.pause()
                break
            default:
                break
            }
        }
    }
}
