import QtQuick 2.0
import "utils.js" as Utils

Rectangle {
    id: root
    layer.enabled: true
    color: "#aa1a2b3a"
    focus: true
    property alias title: title.text
    property alias content: content
    property real titleHeight: title.height + 2*Utils.kMargin
    property real maxHeight: Utils.scaled(300)

    Text {
        id: title
        anchors.top: parent.top
        width: parent.width
        height: Utils.scaled(40)
        color: "white"
        font.pixelSize: Utils.scaled(20)
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext('2d')
            var g = ctx.createLinearGradient(0, 0, parent.width, 0)
            g.addColorStop(0, "#00ffffff")
            g.addColorStop(0.3, "#f0ffffff")
            g.addColorStop(0.7, "#f0ffffff")
            g.addColorStop(1, "#00ffffff")
            ctx.fillStyle = g
            ctx.fillRect(0, title.height, parent.width, 1)
        }
    }
    MouseArea { // avoid mouse events propagated to parents
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: false
    }
    Item {
        id: content
        anchors {
            top: title.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        anchors.margins: Utils.kMargin
    }

    // TODO: why must put here otherwise can't clicked?
    Button {
        anchors.top: parent.top
        anchors.right: parent.right
        width: Utils.scaled(20)
        height: Utils.scaled(20)
        bgColor: "transparent"
        bgColorSelected: "transparent"
        icon: Utils.resurl("theme/default/close.svg")
        onClicked: root.visible = false
    }
}
