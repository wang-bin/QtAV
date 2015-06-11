import QtQuick 2.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Video Codec")
    height: titleHeight + detail.height + listView.height + copyMode.height + Utils.kSpacing*4
    signal zeroCopyChanged(bool value)

    QtObject {
        id: d
        property Item selectedItem
        property string detail: qsTr("Takes effect on the next play")
    }

    Column {
        anchors.fill: content
        spacing: Utils.kSpacing
        Text {
            id: detail
            text: d.detail
            color: "white"
            height: contentHeight + 1.6*Utils.kItemHeight
            width: parent.width
            font.pixelSize: Utils.kFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        ListView {
            id: listView
            contentWidth: parent.width - Utils.scaled(20)
            height: Utils.kItemHeight
            anchors {
                //topMargin: Utils.kMargin
                horizontalCenter: parent.horizontalCenter
            }
            onContentWidthChanged: {
                anchors.leftMargin = Math.max(10, (parent.width - contentWidth)/2)
                anchors.rightMargin = anchors.leftMargin
                width = parent.width - 2*anchors.leftMargin
            }
            orientation: ListView.Horizontal
            spacing: Utils.scaled(6)
            focus: true

            delegate: contentDelegate
            model: contentModel
        }

        ListModel {
            id: contentModel
            ListElement { name: "VDA"; hardware: true; zcopy: true; description: "VDA (OSX)" }
            ListElement { name: "DXVA"; hardware: true; zcopy: true; description: "DirectX Video Acceleration (Windows)\nUse ANGLE D3D to support 0-copy" }
            ListElement { name: "VAAPI"; hardware: true; zcopy: true; description: "VA-API (Linux) " }
            ListElement { name: "CUDA"; hardware: true; zcopy: false; description: "NVIDIA CUDA (Windows, Linux)"}
            ListElement { name: "FFmpeg"; hardware: false; zcopy: false; description: "FFmpeg/Libav" }
        }

        Component {
            id: contentDelegate
            DelegateItem {
                id: delegateItem
                text: name
                width: Utils.kItemWidth
                height: Utils.kItemHeight
                color: "#aa000000"
                selectedColor: "#aa0000cc"
                border.color: "white"
                onClicked: {
                    if (d.selectedItem == delegateItem)
                        return
                    if (d.selectedItem)
                        d.selectedItem.state = "baseState"
                    d.selectedItem = delegateItem
                }
                onStateChanged: {
                    if (state != "selected")
                        return
                    d.detail = description + " " + (hardware ? qsTr("hardware decoding") : qsTr("software decoding"))  + "\n"
                            + qsTr("Zero Copy support") + ":" + zcopy + "\n\n"
                            + qsTr("Takes effect on the next play")
                    PlayerConfig.decoderPriorityNames = [ name ]
                }
            }
        }
        Button {
            id: copyMode
            text: qsTr("Zero copy")
            checked: false
            checkable: true
            width: parent.width
            height: Utils.kItemHeight
            onCheckedChanged: root.zeroCopyChanged(checked)
        }
    }
    Component.onCompleted: {
        for (var i = 0; i < contentModel.count; ++i) {
            if (contentModel.get(i).name === PlayerConfig.decoderPriorityNames[0]) {
                listView.currentIndex = i;
                d.selectedItem = listView.currentItem
                listView.currentItem.state = "selected"
                break
            }
        }
    }
}
