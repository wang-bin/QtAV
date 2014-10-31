import QtQuick 2.0

Page {
    id: root
    title: qsTr("Video Codec")
    property string codecName: "FFmpeg"
    signal decoderChanged(string value)
    QtObject {
        id: d
        property Item selectedItem
        property string detail: ""
    }

    Item {
        anchors.fill: content
        Text {
            id: detail
            anchors.fill: parent
            anchors.bottomMargin: 60
            text: d.detail
            color: "white"
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        ListView {
            anchors {
                top: detail.bottom
                bottom: parent.bottom
                topMargin: 10
                horizontalCenter: parent.horizontalCenter
            }
            onContentWidthChanged: {
                anchors.leftMargin = Math.max(10, (parent.width - contentWidth)/2)
                anchors.rightMargin = anchors.leftMargin
                width = parent.width - 2*anchors.leftMargin
            }
            orientation: ListView.Horizontal
            spacing: 6
            focus: true

            delegate: contentDelegate
            model: contentModel
        }

        ListModel {
            id: contentModel
            ListElement { name: "VDA"; description: "VDA (OSX) hardware decoding" }
            ListElement { name: "DXVA"; description: "DXVA (Windows) hardware decoding" }
            ListElement { name: "VAAPI"; description: "VA-API (Linux) hardware decoding" }
            ListElement { name: "CUDA"; description: "NVIDIA CUDA (Windows, Linux) hardware decoding" }
            ListElement { name: "FFmpeg"; description: "FFmpeg software decoding" }
        }

        Component {
            id: contentDelegate
            DelegateItem {
                id: delegateItem
                text: name
                width: textContentWidth + 8
                height: textContentHeight + 8
                color: "#aa000000"
                selectedColor: "#aa0000cc"
                border.color: "white"
                onClicked: {
                    if (d.selectedItem == delegateItem)
                        return
                    if (d.selectedItem)
                        d.selectedItem.state = "baseState"
                    d.selectedItem = delegateItem
                    d.detail = description + "\nTakes effect on the next play"
                    root.decoderChanged(name)
                }
            }
        }
    }
}
