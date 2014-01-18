import QtQuick 2.0
import "utils.js" as Utils

// TODO: Control.qml
Rectangle {
    id: control
    color: "black"
    opacity: 0.9
    radius: 10
    height: 80

    property string playState: "stop"
    property string mediaSource
    property int duration: 0
    property real volume: 1

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
            //var m = mapToItem(control, mouse.x, mouse.y)
            // TODO: why control.contains(m) always true?
            if (containsMouse) {
                if (timer.running) //timer may ran a few seconds(<3) ago
                    timer.stop();
                control.aniShow()
            } else {
                //if (player.playbackState !== MediaPlayer.StoppedState)
                if (playState !== "stop")
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
            topMargin: 2
            left: progress.left
        }
        color: "white"
    }
    Text {
        id: life
        text: Utils.msec2string(duration)
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
            spacing: 4
            Button {
                id: stopBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: 35
                height: 35
                icon: resurl("theme/default/stop.svg")
                onClicked: {
                    //player.stop()
                    stop()
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
                    //player.seek(player.position-10000)
                    seekBackward(10000)
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
                    //player.seek(player.position+10000)
                    seekForward(10000)
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
            Slider { //volume
                width: 80
                height: 30
                opacity: 0.9
                value: volume/2
                onValueChanged: {
                    //player.volume = 2*value
                    volume = 2*value
                }
                Text {
                    color: "white"
                    id: voltext
                    text: volume
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
                onClicked: showInfo()
            }
            Button {
                id: openFileBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: 25
                height: 25
                icon: resurl("theme/default/open.svg")
                onClicked: openFile()
            }
            Button {
                id: helpBtn
                bgColor: "transparent"
                bgColorSelected: "transparent"
                width: 25
                height: 25
                icon: resurl("theme/default/help.svg")
                onClicked: showHelp()
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
