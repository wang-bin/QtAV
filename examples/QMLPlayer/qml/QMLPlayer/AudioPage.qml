import QtQuick 2.0
import QtAV 1.4
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Audio")
    signal channelChanged(int channel)
    signal muteChanged(bool value)
    height: titleHeight + channelLabel.height + channels.contentHeight
            + Utils.kItemHeight + Utils.kSpacing*3
    Column {
        anchors.fill: content
        spacing: Utils.kSpacing
        Text {
            id: channelLabel
            color: "white"
            text: qsTr("Channel layout")
            font.pixelSize: Utils.kFontSize
            width: parent.width
        }
        Menu {
            id: channels
            width: parent.width
            itemWidth: parent.width
            model: ListModel {
                ListElement { name: QT_TR_NOOP("Stero"); value: MediaPlayer.Stero }
                ListElement { name: QT_TR_NOOP("Mono"); value: MediaPlayer.Mono }
                ListElement { name: QT_TR_NOOP("Left"); value: MediaPlayer.Left }
                ListElement { name: QT_TR_NOOP("Right"); value: MediaPlayer.Right }
            }
            onClicked: {
                root.channelChanged(model.get(index).value)
            }
        }
        Button {
            text: qsTr("Mute")
            checked: false
            checkable: true
            width: parent.width
            height: Utils.kItemHeight
            onCheckedChanged: root.muteChanged(checked)
        }
    }
}
