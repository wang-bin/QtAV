import QtQuick 2.0
import "utils.js" as Utils

Page {
    title: qsTr("About")
    height: titleHeight + about.contentHeight + donateHeight
    property int donateHeight: Utils.scaled(50)
    Item {
        anchors.fill: content
        Text {
            id: about
            color: "white"
            wrapMode: Text.Wrap
            anchors.fill: parent
            anchors.margins: Utils.kMargin
            anchors.bottomMargin: donateHeight
            //horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Utils.scaled(15)
            onContentHeightChanged: parent.height = contentHeight + 2*anchors.margins
            onLinkActivated: Qt.openUrlExternally(link)
            text: "<h3>QMLPlayer for " + Qt.platform.os + " " + qsTr("based on") + " QtAV 1.9.0</h3>"
                  + "<p>" + qsTr("QtAV is a cross platform, high performance multimedia framework") + "</p>"
                  + "<p>Distributed under the terms of LGPLv2.1 or later.</p>"
                  + "<p>Copyright (C) 2012-2015 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>"
                  + "<p>Home page: <a href='http://qtav.org'>http://qtav.org</a></p>"
                  + "\n<p>Double click: show/hide control bar</p>"
                  + "<p>Click right area: show config panel</p>"
                  + (Qt.platform.os === "android" || Qt.platform.os === "ios" || Qt.platform.os === "winphone" || Qt.platform.os === "blackberry" ?
                      " " :
                    "\n<h3>" + qsTr("Shortcut") + ":</h3>"
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
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Utils.kMargin
            width: Utils.scaled(80)
            height: donateHeight
            onClicked: Qt.openUrlExternally("http://www.qtav.org/donate.html")
        }
    }
}
