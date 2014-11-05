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
    property bool hiding: false
    signal seek(int ms)
    signal seekForward(int ms)
    signal seekBackward(int ms)
    signal stop
    signal play
    signal togglePause
    signal showInfo
    signal showHelp
    signal openFile

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
    }
    function setPlayingState() {
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
        GradientStop { position: 0.0; color: "#aa445566" }
        GradientStop { position: 0.618; color: "#bb1a2b3a" }
        GradientStop { position: 1.0; color: "#ff000000" }
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
            preview.state = dpos.y > 0 ? "out" : "out_"
        }
        onHoverAt: {
            //if (player.playbackState == MediaPlayer.StoppedState)
              //  return
            if (playState == "stop")
                return;
            if (PlayerConfig.previewEnabled && preview.video.file) {
                preview.video.timestamp = value*duration
            }

            var v = value * progress.width
            preview.anchors.leftMargin = v - preview.width/2
            previewText.text = Utils.msec2string(value*duration)
            //console.log("hover: "+value + " duration: " + player.duration)
        }
    }
    Text {
        id: now
        text: Utils.msec2string(progress.value*duration)
        anchors {
            top: progress.bottom
            topMargin: Utils.scaled(2)
            left: progress.left
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
            top: progress.bottom
            topMargin: Utils.scaled(2)
            right: progress.right
        }
        color: "white"
        font {
            pixelSize: Utils.scaled(12)
        }
    }
    Rectangle {
        id: preview
        opacity: 0.8
        anchors.left: progress.left
        anchors.bottom: progress.top
        width: PlayerConfig.previewEnabled ? Utils.scaled(180) : previewText.contentWidth + 2*Utils.kSpacing
        height: PlayerConfig.previewEnabled ? Utils.scaled(120) : previewText.contentHeight + 2*Utils.kSpacing
        color: "black"
        state: "out"
        property alias video: video
        VideoPreview {
            id: video
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
            margins: Utils.scaled(8)
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
        Row {
            anchors.right: playBtn.left
            anchors.verticalCenter: playBtn.verticalCenter
            spacing: Utils.scaled(4)
            Button {
                id: stopBtn
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
            Button {
                id: backwardBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(35)
                height: Utils.scaled(35)
                icon: Utils.resurl("theme/default/backward.svg")
                onClicked: {
                    //player.seek(player.position-10000)
                    seekBackward(10000)
                }
            }
        }
        Row {
            anchors.left: playBtn.right
            anchors.verticalCenter: playBtn.verticalCenter
            spacing: Utils.scaled(4)
            Button {
                id: forwardBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(35)
                height: Utils.scaled(35)
                icon: Utils.resurl("theme/default/forward.svg")
                onClicked: {
                    //player.seek(player.position+10000)
                    seekForward(10000)
                }
            }
        }
        Row {
            anchors.left: parent.left
            anchors.leftMargin: Utils.scaled(50)
            anchors.verticalCenter: parent.verticalCenter
            Button {
                id: fullScreenBtn
                checkable: true
                checked: false
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(25)
                height: Utils.scaled(25)
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
            Slider { //volume
                width: Utils.scaled(80)
                height: Utils.scaled(30)
                opacity: 0.9
                value: volume/2
                onValueChanged: {
                    //player.volume = 2*value
                    volume = 2*value
                }
                Text {
                    color: "white"
                    id: voltext
                    text: Math.round(10*volume)/10
                }
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: Utils.scaled(50)
            anchors.verticalCenter: parent.verticalCenter
            Button {
                id: infoBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(25)
                height: Utils.scaled(25)
                icon: Utils.resurl("theme/default/info.svg")
                visible: true
                onClicked: showInfo()
            }
            Button {
                id: openFileBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(25)
                height: Utils.scaled(25)
                icon: Utils.resurl("theme/default/open.svg")
                onClicked: openFile()
            }
            Button {
                id: helpBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: Utils.scaled(25)
                height: Utils.scaled(25)
                icon: Utils.resurl("theme/default/help.svg")
                onClicked: showHelp()
            }
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
}
