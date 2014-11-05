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
            id: contentModel
            // ListElement property value can not be script before Qt5.4. translate here(QT_TR_NOOP) and used in other qml fails
        }
        onClicked: {
            selectedX = x
            selectedY = y
            selectedUrl = model.get(index).url
            root.clicked()
        }
    }
    Component.onCompleted: {
        contentModel.append({name: qsTr("Media info"),  url: "MediaInfoPage.qml" })
        contentModel.append({name: qsTr("Video codec"), url: "VideoCodec.qml" })
        contentModel.append({name: qsTr("Subtitle"), url: "SubtitlePage.qml" })
        contentModel.append({name: qsTr("Audio"), url: "AudioPage.qml" })
        contentModel.append({name: qsTr("Preview"), url: "PreviewPage.qml" })
        contentModel.append({name: qsTr("About"), url: "About.qml" })
    }
}
