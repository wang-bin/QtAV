import QtQuick 2.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Video Codec")
    height: titleHeight + detail.height + listView.height + copyMode.height + Utils.kSpacing*4
    signal zeroCopyChanged(bool value)
    property var defaultDecoders: []

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
            model: codecMode
        }

        ListModel {
            id: codecMode
            ListElement { name: "Auto"; hardware: true; zcopy: true; description: qsTr("Try hardware decoders first") }
            ListElement { name: "FFmpeg"; hardware: false; zcopy: false; description: "FFmpeg/Libav" }
        }

        Component {
            id: contentDelegate
            DelegateItem {
                id: delegateItem
                text: name
                //width: Utils.kItemWidth
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
                    if (name === "Auto") {
                        PlayerConfig.decoderPriorityNames = defaultDecoders
                        d.detail = description
                        return
                    }
                    d.detail = description + " " + (hardware ? qsTr("hardware decoding") : qsTr("software decoding"))
                    if (name === "FFmpeg") {
                        copyMode.visible = false
                    } else {
                        copyMode.visible = zcopy
                        d.detail += "\n" + qsTr("Zero Copy support") + ":" + zcopy
                    }
                    PlayerConfig.decoderPriorityNames = [ name ]
                }
            }
        }
        Button {
            id: copyMode
            text: qsTr("Zero copy")
            checked: PlayerConfig.zeroCopy
            checkable: true
            width: parent.width
            height: Utils.kItemHeight
            onCheckedChanged: PlayerConfig.zeroCopy = checked
        }
    }
    Component.onCompleted: {
        if (Qt.platform.os == "windows") {
            defaultDecoders.push("DXVA")
            defaultDecoders.push("D3D11")
            defaultDecoders.push("CUDA")
            codecMode.append({ name: "DXVA", hardware: true, zcopy: true, description: "DirectX Video Acceleration (Windows)\nUse OpenGLES(ANGLE) + D3D to support 0-copy" })
            codecMode.append({ name: "D3D11", hardware: true, zcopy: true, description: "D3D11 Video Acceleration\n0-copy is supported under OpenGLES(ANGLE)" })
            codecMode.append({ name: "CUDA", hardware: true, zcopy: true, description: "NVIDIA CUDA (Windows, Linux).\nH264 10bit support."})
        } else if (Qt.platform.os == "winrt" || Qt.platform.os == "winphone") {
            defaultDecoders.push("D3D11")
            codecMode.append({ name: "D3D11", hardware: true, zcopy: true, description: "D3D11 Video Acceleration" })
        } else if (Qt.platform.os == "osx") {
            defaultDecoders.push("VideoToolbox")
            defaultDecoders.push("VDA")
            codecMode.append({ name: "VideoToolbox", hardware: true, zcopy: true, description: "VideoToolbox (OSX)" })
            codecMode.append({ name: "VDA", hardware: true, zcopy: true, description: "VDA (OSX)" })
        } else if (Qt.platform.os == "ios") {
            defaultDecoders.append("VideoToolbox")
            codecMode.append({ name: "VideoToolbox", hardware: true, zcopy: true, description: "VideoToolbox (iOS)" })
        } else if(Qt.platform.os == "android") {
            defaultDecoders.push("MediaCodec")
            codecMode.append({ name: "MediaCodec", hardware: true, zcopy: false, description: "Android(>=4.1) MediaCodec (H.264)" })
        } else if (Qt.platform.os == "linux") {
            defaultDecoders.push("VAAPI")
            defaultDecoders.push("CUDA")
            codecMode.append({ name: "VAAPI", hardware: true, zcopy: true, description: "VA-API (Linux) " })
            codecMode.append({ name: "CUDA", hardware: true, zcopy: true, description: "NVIDIA CUDA (Windows, Linux)"})
        }
        defaultDecoders.push("FFmpeg")
        if (PlayerConfig.decoderPriorityNames.length > 1) {
            listView.currentIndex = 0;
            d.selectedItem = listView.currentItem
            listView.currentItem.state = "selected"
            return
        }
        for (var i = 1; i < codecMode.count; ++i) {
            if (codecMode.get(i).name === PlayerConfig.decoderPriorityNames[0]) {
                listView.currentIndex = i;
                d.selectedItem = listView.currentItem
                listView.currentItem.state = "selected"
                break
            }
        }
    }
}
