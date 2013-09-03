import QtQuick 2.1
import QtQuick.Dialogs 1.0
import QtAV 1.3

Rectangle {
    width: 640
    height: 360
    color: "black"



    VideoOut {
        id: vo
        anchors.fill: parent
        Text {
            text: "Double click to select a file\n" +
            "Left/Right: seek forward/backward\n" +
            "Space: pause/resume\n" +
            "Q: quit"
            font.pointSize: 28
            color: "white"
        }
    }
    AVPlayer {
        id: player
        videoOut: vo
    }
    MouseArea {
        anchors.fill: parent
        onDoubleClicked: {
            fileDialog.open()
        }
    }
    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: {

            switch (event.key) {
            case Qt.Key_M:
                player.setMute(!player.mute)
                break
            case Qt.Key_Right:
                player.seekForward()
                break
            case Qt.Key_Left:
                player.seekBackward()
            case Qt.Key_Space:
                player.togglePause()
                break
            case Qt.Key_M:
                player.setMute(!player.mute)
                break
            case Qt.Key_Q:
                Qt.quit()
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
        Component.onCompleted: visible = true
    }
}
