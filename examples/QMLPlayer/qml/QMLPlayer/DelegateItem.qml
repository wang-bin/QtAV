import QtQuick 2.0

Rectangle {
    id: root
    width: 100
    height: 30
    property color selectedColor: "#20ffffff"
    property alias text: itemText.text
    property alias textContentWidth: itemText.contentWidth
    property alias textContentHeight: itemText.contentHeight
    color: "#99000000"
    signal clicked
    Text {
        id: itemText
        color: "white"
        font.pixelSize: 16
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
