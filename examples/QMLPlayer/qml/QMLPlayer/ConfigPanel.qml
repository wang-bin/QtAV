import QtQuick 2.0

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
        anchors.topMargin: 40
        itemWidth: root.width - 2
        spacing: 1
        model: ListModel {
            ListElement { name: "Media info"; url: "MediaInfoPage.qml" }
            ListElement { name: "Video codec"; url: "VideoCodec.qml" }
            ListElement { name: "Subtitle"; url: "SubtitlePage.qml" }
            ListElement { name: "Audio"; url: "AudioPage.qml" }
        }
        onClicked: {
            selectedX = x
            selectedY = y
            selectedUrl = model.get(index).url
            root.clicked()
        }
    }

}
