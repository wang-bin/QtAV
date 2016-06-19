import QtQuick 2.1
import QtAV 1.7
import QtQuick.Dialogs 1.0

Rectangle {
    width: 800
    height: 500
    color: "black"
    VideoFilter {
        id: negate
        type: VideoFilter.AVFilter
        avfilter: "negate"
    }
    Video {
        id: video
        autoPlay: true
        anchors.fill: parent
        subtitle.engines: ["FFmpeg"]
        videoFilters: [negate]
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
