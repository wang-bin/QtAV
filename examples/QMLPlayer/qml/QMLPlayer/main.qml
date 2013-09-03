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
    }
    AVPlayer {
        id: player
        source: "g:/av/t.mov"
        videoOut: vo
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
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
