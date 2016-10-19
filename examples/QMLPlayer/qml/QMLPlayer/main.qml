/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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
//if qt<5.3, remove lines: sed '/\/\/IF_QT53/,/\/\/ENDIF_QT53/d'
import QtQuick 2.0
//IF_QT53
import QtQuick.Dialogs 1.2
/*
//ENDIF_QT53
import QtQuick.Dialogs 1.1 /*
*/
//import QtMultimedia 5.0
import QtAV 1.7
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

    VideoFilter {
        id: negate
        type: VideoFilter.GLSLFilter
        shader: Shader {
            postProcess: "gl_FragColor.rgb = vec3(1.0-gl_FragColor.r, 1.0-gl_FragColor.g, 1.0-gl_FragColor.b);"
        }
    }
    VideoFilter {
        id: hflip
        type: VideoFilter.GLSLFilter
        shader: Shader {
            sample: "vec4 sample2d(sampler2D tex, vec2 pos, int p) { return texture(tex, vec2(1.0-pos.x, pos.y));}"
        }
    }

    VideoOutput2 {
        id: videoOut
        opengl: true
        fillMode: VideoOutput.PreserveAspectFit
        anchors.fill: parent
        source: player
        orientation: 0
        property real zoom: 1
        //filters: [negate, hflip]
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
            videoOut.zoom = 1
            videoOut.regionOfInterest = Qt.rect(0, 0, 0, 0)
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
                player.fastSeek = true
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
            cursorShape: control.opacity > 0 || cursor_timer.running ? Qt.ArrowCursor : Qt.BlankCursor
            onWheel: {
                var deg = wheel.angleDelta.y/8
                var dp = wheel.pixelDelta
                var p = Qt.point(mouseX, mouseY) //root.mapToItem(videoOut, Qt.point(mouseX, mouseY))
                var fp = videoOut.mapPointToSource(p)
                if (fp.x < 0)
                    fp.x = 0;
                if (fp.y < 0)
                    fp.y = 0;
                if (fp.x > videoOut.videoFrameSize.width)
                    fp.x = videoOut.videoFrameSize.width
                if (fp.y > videoOut.videoFrameSize.height)
                    fp.y = videoOut.videoFrameSize.height
                videoOut.zoom *= (1.0 + deg*3.14/180.0);
                if (videoOut.zoom < 1.0)
                    videoOut.zoom = 1.0
                var x0 = fp.x - fp.x/videoOut.zoom;
                var y0 = fp.y - fp.y/videoOut.zoom;
                // in fact, it must insected with video frame rect. opengl save us
                videoOut.regionOfInterest.x = x0
                videoOut.regionOfInterest.y = y0
                videoOut.regionOfInterest.width = videoOut.videoFrameSize.width/videoOut.zoom
                videoOut.regionOfInterest.height = videoOut.videoFrameSize.height/videoOut.zoom
            }

            onDoubleClicked: {
                control.toggleVisible()
            }
            onClicked: {
                if (playList.state === "show")
                    return
                if (playList.state === "hide")
                    playList.state = "ready"
                else
                    playList.state = "hide"

            }

            onMouseXChanged: {
                cursor_timer.start()
                if (mouseX > root.width || mouseX < 0
                        || mouseY > root.height || mouseY < 0)
                    return;
                if (mouseX < Utils.scaled(20)) {
                    if (playList.state === "hide") {
                        playList.state = "ready"
                    }
                }

                if (root.width - mouseX < configPanel.width) { //qt5.6 mouseX is very large if mouse released
                    //console.log("configPanel show: root width: " + root.width + " mouseX: " + mouseX + "panel width: " + configPanel.width)
                    configPanel.state = "show"
                } else {
                    configPanel.state = "hide"
                    //console.log("configPanel hide: root width: " + root.width + " mouseX: " + mouseX + "panel width: " + configPanel.width)
                }
                if (player.playbackState == MediaPlayer.StoppedState || !player.hasVideo)
                    return;
                if (mouseY < control.y - control.previewHeight) {
                    control.hidePreview() // TODO: check previw hovered too
                } else {
                    if (pressed) {
                        control.showPreview(mouseX/parent.width)
                    }
                }
            }
            Timer {
                id: cursor_timer
                interval: 2000
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
        anchors.rightMargin: -configPanel.anchors.rightMargin*Utils.scaled(20)/configPanel.width
        //anchors.bottom: control.top
        y: Math.max(0, Math.min(configPanel.selectedY, root.height - pageLoader.height - control.height))
        width: parent.width < 4*configPanel.width ? parent.width - configPanel.width : parent.width/2 + configPanel.width -16
       // height: maxHeight
        readonly property real maxHeight: control.y //- Math.max(0, configPanel.selectedY)
        Loader {
            id: pageLoader
            anchors.right: parent.right
            width: parent.width
            focus: true
            onLoaded: {
                if (!item)
                    return
                item.maxHeight = configPage.maxHeight
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
            onExternalAudioChanged: player.externalAudio = file
            onAudioTrackChanged: player.audioTrack = track
            onSubtitleTrackChanged: player.internalSubtitleTrack = track
            onBrightnessChanged: videoOut.brightness = target.brightness
            onContrastChanged: videoOut.contrast = target.contrast
            onHueChanged: videoOut.hue = target.hue
            onSaturationChanged: {
                console.log("saturation: " + target.saturation)
                videoOut.saturation = target.saturation
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
        width: Utils.scaled(100)
        onClicked: {
            pageLoader.source = selectedUrl
            if (pageLoader.item)
                pageLoader.item.visible = true
        }
        onSelectedUrlChanged: pageLoader.source = selectedUrl
    }

    PlayListPanel {
        id: playList
        visible: Qt.platform.os !== "winrt"
        anchors {
            top: parent.top
            left: parent.left
            bottom: control.top
        }
        width: Math.min(parent.width, Utils.scaled(480)) - Utils.scaled(20)
        Connections {
            target: player
            // onStatusChanged: too late to call status is wrong value
            onDurationChanged: {
                if (player.duration <= 0)
                    return
                var url = player.source.toString()
                if (url.startsWith("winrt:@")) {
                    url = url.substring(url.indexOf(":", 7) + 1);
                }
                console.log("duration changed: " + url)
                playList.addHistory(url, player.duration)
            }
        }
        onPlay: {
            player.source = source
            if (start > 0)
                player.seek(start)
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
        //IF_QT53
        onOpenUrl: urlDialog.open()
        //ENDIF_QT53
        onShowInfo: pageLoader.source = "MediaInfoPage.qml"
        onShowHelp: pageLoader.source = "About.qml"
    }
//IF_QT53
    Dialog {
        id: urlDialog
        standardButtons: StandardButton.Open | StandardButton.Cancel
        title: qsTr("Open a URL")
        Rectangle {
            color: "black"
            anchors.top: parent.top
            height: Utils.kItemHeight
            width: parent.width
            TextInput {
                id: urlEdit
                color: "orange"
                font.pixelSize: Utils.kFontSize
                anchors.fill: parent
            }
        }
        onAccepted: player.source = urlEdit.displayText
    }
//ENDIF_QT53
    FileDialog {
        id: fileDialog
        title: "Please choose a media file"
        selectMultiple: true
        folder: PlayerConfig.lastFile
        onAccepted: {
            var sub, av
            for (var i = 0; i < fileUrls.length; ++i) {
                var s = fileUrls[i].toString()
                if (s.endsWith(".srt")
                        || s.endsWith(".ass")
                        || s.endsWith(".ssa")
                        || s.endsWith(".sub")
                        || s.endsWith(".idx") //vob
                        || s.endsWith(".mpl2")
                        || s.endsWith(".smi")
                        || s.endsWith(".sami")
                        || s.endsWith(".sup")
                        || s.endsWith(".txt"))
                    sub = fileUrls[i]
                else
                    av = fileUrls[i]
            }
            if (sub) {
                subtitle.autoLoad = false
                subtitle.file = sub
            } else {
                subtitle.autoLoad = PlayerConfig.subtitleAutoLoad
                subtitle.file = ""
            }
            if (av) {
                player.source = av
                PlayerConfig.lastFile = av
            }
        }
    }
    Connections {
        target: Qt.application
        onStateChanged: { //since 5.1
            if (Qt.platform.os === "winrt" || Qt.platform.os === "winphone") //paused by system
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
    Connections {
        target: PlayerConfig
        onZeroCopyChanged: {
            var opt = player.videoCodecOptions
            if (PlayerConfig.zeroCopy) {
                opt["copyMode"] = "ZeroCopy"
            } else {
                opt["copyMode"] = "OptimizedCopy" //FIXME: CUDA
            }
            player.videoCodecOptions = opt
        }
    }
}
