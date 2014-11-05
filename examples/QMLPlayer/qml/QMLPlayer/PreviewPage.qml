import QtQuick 2.0
import "utils.js" as Utils

Page {
    title: qsTr("Preview")
    height: titleHeight + Utils.kItemHeight

    Column {
        anchors.fill: content
        Text {
            font.pointSize: Utils.kFontSize
            color: "white"
        }
        Button {
            text: qsTr("Enable")
            checkable: true
            checked: PlayerConfig.previewEnabled
            width: parent.width
            height: Utils.kItemHeight
            onCheckedChanged: PlayerConfig.previewEnabled = checked
        }
    }
}
