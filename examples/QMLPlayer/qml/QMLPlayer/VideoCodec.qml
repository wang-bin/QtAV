import QtQuick 2.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Video Codec")
    height: Utils.scaled(200)

    QtObject {
        id: d
        property Item selectedItem
        property string detail: qsTr("Takes effect on the next play")
    }

    Item {
        anchors.fill: content
        Text {
            id: detail
            anchors.fill: parent
            anchors.bottomMargin: Utils.scaled(50)
            text: d.detail
            color: "white"
            font.pixelSize: Utils.kFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        ListView {
            id: listView
            anchors {
                top: detail.bottom
                bottom: parent.bottom
                topMargin: Utils.kMargin
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
            ListElement { name: "VDA"; hardware: true; description: "VDA (OSX)" }
            ListElement { name: "DXVA"; hardware: true; description: "DirectX Video Acceleration (Windows)" }
            ListElement { name: "VAAPI"; hardware: true; description: "VA-API (Linux) " }
            ListElement { name: "CUDA"; hardware: true; description: "NVIDIA CUDA (Windows, Linux)"}
            ListElement { name: "FFmpeg"; hardware: false; description: "FFmpeg/Libav" }
        }

        Component {
            id: contentDelegate
            DelegateItem {
                id: delegateItem
                text: name
                width: textContentWidth + Utils.scaled(8)
                height: textContentHeight + Utils.scaled(8)
                color: "#aa000000"
                selectedColor: "#aa0000cc"
                border.color: "white"
                onClicked: {
                    if (d.selectedItem == delegateItem)
                        return
                    if (d.selectedItem)
                        d.selectedItem.state = "baseState"
                    d.selectedItem = delegateItem
                    d.detail = description + " " + (hardware ? qsTr("hardware decoding") : qsTr("software decoding"))  + "\n" + qsTr("Takes effect on the next play")
                    PlayerConfig.decoderPriorityNames = [ name ]
                }
            }
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
