import QtQuick 2.0
import QtAV 1.4

Page {
    id: root
    title: qsTr("Audio")
    signal channelChanged(int channel)
    Column {
        anchors.fill: content
        spacing: 4
        Text {
            id: channelLabel
            color: "white"
            text: qsTr("Channel layout")
            font.pixelSize: 16
            width: parent.width
        }
        Menu {
            width: parent.width
            itemWidth: parent.width
            model: ListModel {
                ListElement { name: "Stero"; value: MediaPlayer.Stero }
                ListElement { name: "Mono"; value: MediaPlayer.Mono }
                ListElement { name: "Left"; value: MediaPlayer.Left }
                ListElement { name: "Right"; value: MediaPlayer.Right }
            }
            onClicked: {
                root.channelChanged(model.get(index).value)
            }
        }
    }
}
