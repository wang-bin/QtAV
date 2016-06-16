import QtQuick 2.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Effect")
    property alias brightness : brightnessControl.value
    property alias contrast : contrastControl.value
    property alias hue: hueControl.value
    property alias saturation: saturationControl.value
    height: Math.min(maxHeight, scroll.contentHeight)
    Flickable {
        id: scroll
        anchors.fill: content
        contentHeight: titleHeight + brightnessControl.height + brightnessControl.height
        + hueControl.height + saturationControl.height
        + Utils.kItemHeight
                       +  Utils.kSpacing*6
    Column {
        anchors.fill: parent
        spacing: Utils.kSpacing
        Item {
            width: parent.width
            height: Utils.scaled(40)
            Text {
                color: "white"
                text: qsTr("Brightness")
                font.pixelSize: Utils.kFontSize
                anchors.left: parent.left
                height: parent.height
                verticalAlignment: Text.AlignVCenter
            }
            Slider {
                id: brightnessControl
                lineColor: "gray"
                anchors.right: parent.right
                width: parent.width*3/4
                height: parent.height
                value: 0
                min: -1
                max: 1
                orientation: Qt.Horizontal
            }
        }
        Item {
            width: parent.width
            height: Utils.scaled(40)
            Text {
                color: "white"
                text: qsTr("Contrast")
                font.pixelSize: Utils.kFontSize
                anchors.left: parent.left
                height: parent.height
                verticalAlignment: Text.AlignVCenter
            }
            Slider {
                id: contrastControl
                lineColor: "#ccddeeff"
                anchors.right: parent.right
                width: parent.width*3/4
                height: parent.height
                value: 0
                min: -1
                max: 1
                orientation: Qt.Horizontal
            }
        }
        Item {
            width: parent.width
            height: Utils.scaled(40)
            Text {
                color: "white"
                text: qsTr("Hue")
                font.pixelSize: Utils.kFontSize
                anchors.left: parent.left
                height: parent.height
                verticalAlignment: Text.AlignVCenter
            }
            Slider {
                id: hueControl
                lineColor: "#eebb8899"
                anchors.right: parent.right
                width: parent.width*3/4
                height: parent.height
                value: 0
                min: -1
                max: 1
                orientation: Qt.Horizontal
            }
        }
        Item {
            width: parent.width
            height: Utils.scaled(40)
            Text {
                color: "white"
                text: qsTr("Saturation")
                font.pixelSize: Utils.kFontSize
                anchors.left: parent.left
                height: parent.height
                verticalAlignment: Text.AlignVCenter
            }
            Slider {
                id: saturationControl
                lineColor: "#ee88ccaa"
                anchors.right: parent.right
                width: parent.width*3/4
                height: parent.height
                value: 0
                min: -1
                max: 1
                orientation: Qt.Horizontal
            }
        }

        Button {
            text: qsTr("Reset")
            bgColor: "#990000ff"
            width: parent.width/4
            height: Utils.kItemHeight
            onClicked: {
                brightness = 0
                contrast = 0
                hue = 0
                saturation = 0
            }
        }
    }
    }
}
