import QtQuick 2.0
import "utils.js" as Utils

Page {
    title: qsTr("About")
    height: Math.min(maxHeight, scroll.contentHeight)
    Flickable {
        id: scroll
        contentHeight: titleHeight + about.contentHeight
        anchors.fill: content
        Text {
            id: about
            color: "white"
            linkColor: "orange"
            wrapMode: Text.Wrap
            anchors.fill: parent
            anchors.margins: Utils.kMargin
            //horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Utils.scaled(15)
            onContentHeightChanged: parent.contentHeight = contentHeight + 2*anchors.margins
            onLinkActivated: Qt.openUrlExternally(link)
            text: "<img src='qrc:/QtAV.svg'><h3>QMLPlayer for " + Qt.platform.os + " " + qsTr("based on") + " QtAV 1.12.0</h3>"
                  + "<p>" + qsTr("QtAV is a cross platform, high performance multimedia framework") + "</p>"
                  + "<p>Distributed under the terms of LGPLv2.1 or later.</p>"
                  + "<p>Copyright (C) 2012-2017 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p><p>"
                  + qsTr("Home page") + ": <a href='http://qtav.org'>http://qtav.org</a></p>"
            + "<p><a href='http://qtav.org/install.html'>"+qsTr("Install QtAV for Windows desktop/store, macOS, Linux and Android")+"</a></p>"
                  + "\n<p>" + qsTr("Double click") + ": " + qsTr("show/hide control bar") + "</p><p>"
                  + qsTr("Click right area") + ": " + qsTr("show config panel") + "</p><p>"
                  + qsTr("Open") + " " + qsTr("a subtitle") + ": " + qsTr("press open button to select a subtitle, or a video + a subtitle") + "</p>"
                  + qsTr("Open") + " " + qsTr("a URL") + ": " + qsTr("press and hold open button") + "</p>"
                  + (Qt.platform.os === "android" || Qt.platform.os === "ios" || Qt.platform.os === "winphone" || Qt.platform.os === "blackberry" ?
                      " " :
                    "<h3>" + qsTr("Shortcut") + ":</h3>"
                  + "<table><tr><td>M: " + qsTr("Mute") + " . </td><td>F: " + qsTr("Fullscreen") + " . </td></tr><tr><td>" + qsTr("Up/Down") + ": " + qsTr("Volume")
                  + "+/- . </td><td> " + qsTr("Left/Right") + "/" + qsTr("Swipe") +": " + qsTr("Seek backward/forward") + " . </td></tr><tr><td>"
                  + qsTr("Space") + ": " + qsTr("Pause/Resume") + " . </td><td>Q: " + qsTr("Quit") + " . </td></tr><tr><td>"
                  + qsTr("Rotate") + "</td><td>A: " + qsTr("Aspect ratio")  + "</td></tr></table>")
        }
        Button {
            id: donateBtn
            text: qsTr("Donate")
            bgColor: "#990000ff"
            anchors.top: about.bottom
            anchors.topMargin: -Utils.scaled(50)
            anchors.right: parent.right
            anchors.rightMargin: Utils.kMargin
            width: Utils.scaled(60)
            height: Utils.scaled(40)
            onClicked: Qt.openUrlExternally("http://www.qtav.org/donate.html")
        }
    }
}
