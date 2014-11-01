import QtQuick 2.0
import "utils.js" as Utils

Rectangle {
    id: root
    width: Utils.kItemWidth
    height: Utils.kItemHeight
    property color selectedColor: "#66ddaadd"
    property alias text: itemText.text
    property alias textContentWidth: itemText.contentWidth
    property alias textContentHeight: itemText.contentHeight
    color: "#99000000"
    signal clicked
    Text {
        id: itemText
        color: "white"
        font.pixelSize: Utils.kFontSize
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.state = "selected"
            root.clicked()
        }
    }
    states: [
        State {
            name: "selected"
            PropertyChanges {
                target: delegateItem
                color: selectedColor
            }
        }
    ]
    transitions: [
        Transition {
            from: "*"; to: "*"
            ColorAnimation {
                properties: "color"
                easing.type: Easing.OutQuart
                duration: 500
            }
        }
    ]
}
