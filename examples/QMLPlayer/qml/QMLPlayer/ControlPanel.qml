import QtQuick 2.0
import "utils.js" as Utils
import QtQuick.Window 2.1
import QtAV 1.4
// TODO: Control.qml
Rectangle {
    id: root
    function scaled(x) {
        console.log("Screen " + screenPixelDensity + "; r: " + Screen.pixelDensity/Screen.logicalPixelDensity + "; size: " + Screen.width + "x" + Screen.height);
        console.log("screen density logical: " + Screen.logicalPixelDensity + " pixel: " + Screen.pixelDensity + "; " + x + ">>>" +x*Screen.pixelDensity/Screen.logicalPixelDensity);
        return x*Screen.pixelDensity/Screen.logicalPixelDensity;
    }
    color: "black"
    opacity: 0.9
    radius: Utils.scaled(10)
    height: Utils.scaled(80)

    property string playState: "stop"
    property string mediaSource
    property int duration: 0
    property real volume: 1
    property bool mute: false
    property bool hiding: false
    property int previewHeight: preview.height
    signal seek(int ms)
    signal seekForward(int ms)
    signal seekBackward(int ms)
    signal stop
    signal play
    signal togglePause
    signal showInfo
    signal showHelp
    signal openFile
    signal openUrl

    function setPlayingProgress(value) {
        playState = "play"
        progress.value = value
        //now.text = Utils.msec2string(value*duraion)
    }
    function setStopState() {
        playState = "stop"
        aniShow()
        playBtn.checked = false
        progress.value = 0
        duration = 0
        //now.text = Utils.msec2string(0)
        video.file = ""
    }
    function setPlayingState() {
        video.file = mediaSource // why need this?
        playState = "pause"
        playBtn.checked = true
        //life.text = Utils.msec2string(duraion)
        aniShow() //for pause change
        hideIfTimedout()
    }
    function setPauseState() {
        aniShow()
        playBtn.checked = false
    }
    function toggleFullScreen() {
        fullScreenBtn.checked = !fullScreenBtn.checked
    }

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#88445566" }
        GradientStop { position: 0.618; color: "#cc1a2b3a" }
        GradientStop { position: 1.0; color: "#ee000000" }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        // onEntered, onExited
        onHoveredChanged: {
            //var m = mapToItem(root, mouse.x, mouse.y)
            // TODO: why root.contains(m) always true?
            if (containsMouse) {
                if (timer.running) //timer may ran a few seconds(<3) ago
                    timer.stop();
                root.aniShow()
            } else {
                //if (player.playbackState !== MediaPlayer.StoppedState)
                //if (playState !== "stop")
                   // timer.start()
            }
        }
        onPressed: {
            if (timer.running) //timer may ran a few seconds(<3) ago
                timer.stop();
            root.aniShow()
        }
    }
    ProgressBar {
        id: progress
        anchors {
            top: parent.top
            topMargin: Utils.scaled(8)
            left: parent.left
            leftMargin: Utils.scaled(20)
            right: parent.right
            rightMargin: Utils.scaled(20)
        }
        height: Utils.scaled(8)
        onValueChangedByUi: {
            /*if (player.playbackState != MediaPlayer.StoppedState) {
                player.seek(player.duration * value)
            }*/
            if (playState != "stop") {
                seek(duration * value)
            }
        }
        onEnter: {
            //if (player.playbackState == MediaPlayer.StoppedState)
              //  return
            if (playState == "stop")
                return
            preview.visible = true
            preview.state = dpos.y > 0 ? "out" : "out_"
            preview.state = "in"
            preview.anchors.leftMargin = pos.x - preview.width/2
        }
        onLeave: {
            //if (player.playbackState == MediaPlayer.StoppedState)
              //  return
            if (playState == "stop")
                return
            if (pos.y > -previewText.height && pos.y <= height/2)
                return
            preview.state = dpos.y > 0 ? "out" : "out_"
        }
        onHoverAt: {
            //if (player.playbackState == MediaPlayer.StoppedState)
              //  return
            if (playState == "stop")
                return;
            showPreview(value)
        }
    }
    Rectangle {
        id: preview
        layer.enabled: true
        //opacity: 0.8
        anchors.left: progress.left
        anchors.bottom: progress.top
        width: PlayerConfig.previewEnabled ? Utils.scaled(180) : previewText.contentWidth + 2*Utils.kSpacing
        height: PlayerConfig.previewEnabled ? Utils.scaled(120) : previewText.contentHeight + 2*Utils.kSpacing
        color: "black"
        state: "out"
        property real timestamp
        property alias video: video
        VideoPreview {
            id: video
            opengl: true
            visible: PlayerConfig.previewEnabled
            fillMode: VideoOutput.Stretch
            anchors.top: parent.top
            width: parent.width
            height: parent.height * 4/5
            file: mediaSource
        }
        Text {
            id: previewText
            width: parent.width
            anchors.bottom: parent.bottom
            anchors.top: PlayerConfig.previewEnabled ? video.bottom : parent.top
            text: ""
            color: "white"
            font.pixelSize: Utils.scaled(12)
            font.bold: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }
        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            property int gPos: 0
            property int moved: 0
            onClicked: {
                if (moved) //why press+move+release is click?
                    return
                if (preview.opacity === 0 || root.opacity === 0) {
                    mouse.accepted = false
                    return
                }
                mouse.accepted = true
                seek(preview.timestamp)
            }
            onPressed: {
                gPos = mapToItem(progress, mouseX, 0).x
                moved = 0
            }
            onMouseXChanged: {
                if (!pressed) {
                    mouse.accepted = false
                    return
                }
                mouse.accepted = true
                var x1 = mapToItem(progress, mouseX, 0).x
                var dx = x1 - gPos
                gPos = x1
                moved += dx
                showPreview(preview.timestamp/duration+dx/progress.width)
            }
        }

        states: [
            State {
                name: "in"
                PropertyChanges {
                    target: preview
                    anchors.bottomMargin: 2
                    opacity: 1.0//0.9
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
                    anchors.bottomMargin: -(preview.height + progress.height)
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
        layer.enabled: true
        property int volBarPos: volBtn.x + volBtn.width/2 + x
        anchors {
            top: progress.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            margins: Utils.scaled(8)
        }

        Text {
            id: now
            text: Utils.msec2string(progress.value*duration)
            anchors {
                top: parent.top
                topMargin: Utils.scaled(2)
                left: parent.left
            }
            color: "white"
            font {
                pixelSize: Utils.scaled(12) //or point size?
            }
        }
        Text {
            id: life
            text: Utils.msec2string(duration)
            anchors {
                top: parent.top
                topMargin: Utils.scaled(2)
                right: parent.right
            }
            color: "white"
            font {
                pixelSize: Utils.scaled(12)
            }
        }
        Button {
            id: playBtn
            anchors.centerIn: parent
            checkable: true
            bgColor: "transparent"
            bgColorSelected: "transparent"
            width: Utils.scaled(50)
            height: Utils.scaled(50)
            icon: Utils.resurl("theme/default/play.svg")
            iconChecked: Utils.resurl("theme/default/pause.svg")

            onClicked: {
                if (mediaSource == "")
                    return
                if (playState == "stop") {
                    play()
                } else {
                    togglePause()
                }
/*
                if (player.playbackState == MediaPlayer.StoppedState) {
                    player.play()
                } else {
                    player.togglePause()
                }
                */
            }
        }
        /*
        Button {
            id: backwardBtn
            anchors.right: stopBtn.left
            anchors.verticalCenter: playBtn.verticalCenter
            bgColor: "transparent"
            bgColorSelected: "transparent"
            width: Utils.scaled(35)
            height: Utils.scaled(35)
            icon: Utils.resurl("theme/default/backward.svg")
            onClicked: {
                //player.seek(player.position-10000)
                seekBackward(10000)
            }
        }*/
        Button {
            id: stopBtn
            anchors.verticalCenter: playBtn.verticalCenter
            anchors.right: playBtn.left
            bgColor: "transparent"
            bgColorSelected: "transparent"
            width: Utils.scaled(35)
            height: Utils.scaled(35)
            icon: Utils.resurl("theme/default/stop.svg")
            onClicked: {
                //player.stop()
                stop()
            }
        }
        /*
        Button {
            id: forwardBtn
            anchors.left: playBtn.right
            anchors.verticalCenter: playBtn.verticalCenter
            bgColor: "transparent"
            bgColorSelected: "transparent"
            width: Utils.scaled(35)
            height: Utils.scaled(35)
            icon: Utils.resurl("theme/default/forward.svg")
            onClicked: {
                //player.seek(player.position+10000)
                seekForward(10000)
            }
        }*/
        Button {
            id: fullScreenBtn
            anchors.left: parent.left
            anchors.leftMargin: Utils.scaled(50)
            anchors.verticalCenter: parent.verticalCenter
            checkable: true
            checked: false
            bgColor: "transparent"
            bgColorSelected: "transparent"
            width: Utils.scaled(30)
            height: Utils.scaled(30)
            icon: Utils.resurl("theme/default/fullscreen.svg")
            iconChecked: Utils.resurl("theme/default/fullscreen.svg")
            visible: true
            onCheckedChanged: {
                if (checked)
                    requestFullScreen()
                else
                    requestNormalSize()
            }
        }
        Button {
            id: volBtn
            anchors.left: fullScreenBtn.right
            anchors.verticalCenter: parent.verticalCenter
            checkable: true
            checked: false
            bgColor: "transparent"
            bgColorSelected: "transparent"
            width: Utils.scaled(30)
            height: Utils.scaled(30)
            icon: Utils.resurl("theme/default/volume.svg")
            iconChecked: Utils.resurl("theme/default/mute.svg")
            onClicked: {
                if (!isTouchScreen) {
                    root.mute = checked
                    return
                }
                volBar.visible = !volBar.visible
                checked = root.mute
            }
            onPressAndHold: { //for qt5.6 mobile
                if (!isTouchScreen)
                    return
                root.mute = !root.mute
                checked = root.mute
            }
            onHoveredChanged: {
                volBar.anchors.bottom = parent.top
                if (isTouchScreen)
                    volBar.anchors.bottomMargin = 0 //ensure grip does not cover volBtn
                else
                    volBar.anchors.bottomMargin = -(y + 2)//height/2)
                volBar.x = parent.volBarPos - volBar.width/2
            }
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: Utils.scaled(50)
            anchors.verticalCenter: parent.verticalCenter
            /*
            Button {
                id: infoBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(30)
                height: Utils.scaled(30)
                icon: Utils.resurl("theme/default/info.svg")
                visible: true
                onClicked: showInfo()
            }*/
            Button {
                id: openFileBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(30)
                height: Utils.scaled(30)
                icon: Utils.resurl("theme/default/open.svg")
                onClicked: openFile()
                onPressAndHold: openUrl()
            } /*
            Button {
                id: helpBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(30)
                height: Utils.scaled(30)
                icon: Utils.resurl("theme/default/help.svg")
                onClicked: showHelp()
            }*/
        }
    }

    Slider { //volume
        id: volBar
        width:Utils.scaled(60)
        height: Utils.scaled(140)
        visible: hovered || volBtn.hovered
        opacity: 0.9
        value: volume > 1 ? 0.5 - (volume - 1)/4 : 1 - volume/2
        orientation: Qt.Vertical
        onValueChanged: {
            if (value < 0.5)
                volume = 1 + 4*(0.5-value)
            else
                volume = 2*(1-value)
        }
        Text {
            color: "white"
            id: voltext
            // Math.floor(10*volume)/10 //why display 1.100000001?
            text: Math.floor(volume) + "." + Math.floor((volume - Math.floor(volume))*10)
        }
    }
    Timer {
        id: timer
        interval: 3000
        onTriggered: {
            root.aniHide()
            //root.visible = false //no mouse event
        }
    }
    function hideIfTimedout() {
        timer.start()
    }
    PropertyAnimation {
        id: anim
        target: root
        properties: "opacity"
        function reverse() {
            duration = 1500
            to = 0
            from = root.opacity
        }
        function reset() {
            duration = 200
            from = root.opacity
            to = 0.9
        }
    }
    function aniShow() {
        hiding = false
        anim.stop()
        anim.reset()
        anim.start()
    }
    function aniHide() {
        hiding = true
        anim.stop()
        anim.reverse()
        anim.start()
    }
    function toggleVisible() {
        if (hiding)
            aniShow()
        else
            aniHide()
    }
    function showPreview(value) {
        if (value < 0)
            return
        preview.visible = true
        preview.state = "in"
        if (PlayerConfig.previewEnabled && preview.video.file) {
            preview.video.timestamp = value*duration
        }
        var v = value * progress.width
        preview.anchors.leftMargin = v - preview.width/2
        preview.timestamp = value*duration
        previewText.text = Utils.msec2string(preview.timestamp)
        //console.log("hover: "+value + " duration: " + player.duration)
    }
    function hidePreview() {
        preview.state = "out_"
    }
}
