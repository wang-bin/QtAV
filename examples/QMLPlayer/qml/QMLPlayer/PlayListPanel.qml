import QtQuick 2.0
import "utils.js" as Utils

Rectangle {
    id: root
    color: "#aa204010"
    layer.enabled: true
    readonly property int kMaxItems: 128
    border {
        color: "white"
        width: 1
    }
    readonly property int kMargin: Utils.scaled(20)
    signal play(url source, int start)
    function addHistory(url, duration) {
        console.log("add history: " + url)
        var info = {url:url, file: Utils.fileName(url), start: 0, duration: duration}
        historyModel.insert(0, info)
        historyView.currentIndex = 0
        for (var i = historyModel.count - 1; i > 0; --i) {
            if (historyModel.get(i).url === url)
                historyModel.remove(i, 1)
        }
        if (historyModel.count > kMaxItems) {
            historyModel.remove(kMaxItems, 1)
        }
        PlayerConfig.removeHistory(url)
        PlayerConfig.addHistory(info)
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: !isTouchScreen
        onEntered: {
            if (mouseY < closeBtn.height)
                return
            if (root.state !== "show")
                root.state = "show"
        }
        onExited: {
            if (isTouchScreen)
                return
            if (root.state === "show")
                root.state = "ready"
        }
    }
    Column {
        anchors.fill: parent
        anchors.margins: Utils.scaled(6)
        anchors.rightMargin: kMargin
        spacing: Utils.kSpacing
        Text {
            id: title
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: Utils.kFontSize
            color: "orange"
            font.bold: true
            text: qsTr("History")
            horizontalAlignment: Text.AlignHCenter
        }
        ListView {
            id: historyView
            highlightMoveDuration: 100
            highlight: Rectangle { color: "#88ddaadd"; radius: 1 }
            flickableDirection: Flickable.VerticalFlick
            width: parent.width
            height: parent.height - title.height
            focus: true
            model: ListModel { id: historyModel}
            delegate: Component {
                Item {
                    width: parent.width
                    height: info.height
                    Column {
                        id: info
                        width: parent.width
                        Text {
                            width: parent.width
                            elide: Text.ElideMiddle
                            font.pixelSize: Utils.kFontSize
                            color: "white"
                            text: file
                        }
                        Text {
                            font.pixelSize: Utils.kFontSize*2/3
                            color: "white"
                            text: Utils.msec2string(start) + "/" + Utils.msec2string(duration)
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked:                         historyView.currentIndex = index
                        onDoubleClicked: {
                            console.log("click: " + url + " file: " + file)
                            root.play(Qt.resolvedUrl(url), start)
                        }
                    }
                }
            }
            Component.onCompleted: {
                for (var i = 0; i < PlayerConfig.history.length; ++i) {
                    var info = PlayerConfig.history[i];
                    historyModel.insert(0, {url:info.url, file: Utils.fileName(info.url), start: info.start, duration: info.duration})
                }
                historyView.currentIndex = 0
            }
        }
    }
    Button {
        id: closeBtn
        anchors.top: parent.top
        anchors.right: parent.right
        width: kMargin
        height: kMargin
        bgColor: "transparent"
        bgColorSelected: "transparent"
        icon: Utils.resurl("theme/default/close.svg")
        onClicked: {
            if (root.state === "ready")
                root.state = "hide"
            else
                root.state = "ready"
        }
    }
    states: [
        State {
            name: "show"
            PropertyChanges {
                target: root
                opacity: 0.9
                anchors.leftMargin: 0
            }
        },
        State {
            name: "hide"
            PropertyChanges {
                target: root
                opacity: 0.6
                anchors.leftMargin: -root.width
            }
        },
        State {
            name: "ready"
            PropertyChanges {
                target: root
                opacity: 0.9
                anchors.leftMargin: -root.width + kMargin
            }
        }
    ]
    transitions: [
        Transition {
            from: "*"; to: "*"
            PropertyAnimation {
                properties: "opacity,anchors.leftMargin"
                easing.type: Easing.OutQuart
                duration: 500
            }
        }
    ]
}
