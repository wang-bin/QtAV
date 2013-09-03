import QtQuick 2.0
import QtAV 1.0

Rectangle {
    width: 640
    height: 360

    QQuickItemRenderer {
        id: vo
        anchors.fill: parent
        objectName: "quickItemRenderer"
    }
    AVPlayer {
        id: player
        source: "~/share/tbbt.rmvb"
        videoOut: vo
    }
    MouseArea {
        anchors.fill: parent
        onClicked: player.play()
    }

}
