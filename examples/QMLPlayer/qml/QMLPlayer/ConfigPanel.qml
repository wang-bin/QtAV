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
            ListElement { name: qsTr("Media info"); url: "MediaInfoPage.qml" }
            ListElement { name: qsTr("Video codec"); url: "VideoCodec.qml" }
            ListElement { name: qsTr("Subtitle"); url: "SubtitlePage.qml" }
            ListElement { name: qsTr("Audio"); url: "AudioPage.qml" }
            ListElement { name: qsTr("About"); url: "About.qml" }
        }
        onClicked: {
            selectedX = x
            selectedY = y
            selectedUrl = model.get(index).url
            root.clicked()
        }
    }

}
