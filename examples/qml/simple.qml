import QtQuick 2.1
import QtAV 1.4
import QtQuick.Dialogs 1.0

Rectangle {
    width: 800
    height: 500
    color: "black"
    Video {
        id: video
        autoPlay: true
        anchors.fill: parent
        subtitle.engines: ["FFmpeg"]
    }
    Text {
        color: "white"
        text: "Press key 'O' to open a file"
    }
    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_O:
                fileDialog.open()
                break
            }
        }
    }
    FileDialog {
        id: fileDialog
        onAccepted: video.source = fileUrl
    }
}
