import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.0
import QtAV 1.7
import com.qtav.vumeter 1.0
Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("QtAV VU Meter Filter. Press key 'O' to open a file")

    VUMeterFilter {
        id: vu
    }

    AudioFilter {
        id: afilter
        type: AudioFilter.UserFilter
        userFilter: vu
    }

    Video {
        id: video
        autoPlay: true
        anchors.fill: parent
        audioFilters: [afilter]
        Text {
            anchors.fill: parent
            color: "white"
            text: "dB left=" + vu.leftLevel + " right=" + vu.rightLevel
        }
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
