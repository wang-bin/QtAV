import QtQuick 2.0
import "utils.js" as Utils

Page {
    title: qsTr("Preview")
    height: titleHeight + Utils.kItemHeight + detail.contentHeight

    Column {
        anchors.fill: content
        Button {
            text: qsTr("Enable")
            checkable: true
            checked: PlayerConfig.previewEnabled
            width: parent.width
            height: Utils.kItemHeight
            onCheckedChanged: PlayerConfig.previewEnabled = checked
        }
        Text {
            id: detail
            font.pixelSize: Utils.kFontSize
            color: "white"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Press on the preview item to seek")
        }
    }
}
