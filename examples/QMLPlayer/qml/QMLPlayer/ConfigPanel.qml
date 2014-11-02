import QtQuick 2.0
import "utils.js" as Utils

Rectangle {
    id: root
    property string selected: ""
    property string selectedUrl
    property int selectedX: 0
    property int selectedY: 0
    color: "#80101010"

    signal clicked

    Menu {
        id: menu
        anchors.fill: parent
        anchors.topMargin: Utils.scaled(40)
        itemWidth: root.width - Utils.scaled(2)
        spacing: Utils.scaled(1)
        model: ListModel {
            ListElement { name: QT_TR_NOOP("Media info"); url: "MediaInfoPage.qml" }
            ListElement { name: QT_TR_NOOP("Video codec"); url: "VideoCodec.qml" }
            ListElement { name: QT_TR_NOOP("Subtitle"); url: "SubtitlePage.qml" }
            ListElement { name: QT_TR_NOOP("Audio"); url: "AudioPage.qml" }
            ListElement { name: QT_TR_NOOP("About"); url: "About.qml" }
        }
        onClicked: {
            selectedX = x
            selectedY = y
            selectedUrl = model.get(index).url
            root.clicked()
        }
    }

}
