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
            //horizontalAlignment: Qt.AlignHCenter
            font.pixelSize: Utils.scaled(14)
            onContentHeightChanged: parent.height = contentHeight + 2*anchors.margins
            onLinkActivated: Qt.openUrlExternally(link)
            text: "<h3>QMLPlayer based on QtAV  1.4.1 </h3>"
                  + "<p>Distributed under the terms of LGPLv2.1 or later.</p>"
                  + "<p>Copyright (C) 2012-2014 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>"
                  + "<p>Shanghai University->S3 Graphics->Deepin, Shanghai, China</p>"
                  + "<p>Source code: <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>"
                  + "<p>Home page: <a href='http://www.qtav.org'>http://www.qtav.org</a></p>"
                  + "\n<h3>" + qsTr("Shortcut") + ":</h3>"
                  + "<table><tr><td>M: " + qsTr("Mute") + " . </td><td>F: " + qsTr("Fullscreen") + " . </td></tr><tr><td>" + qsTr("Up/Down") + ": " + qsTr("Volume")
                  + "+/- . </td><td> " + qsTr("Left/Right") +": " + qsTr("Seek backward/forward") + " . </td></tr><tr><td>"
                  + qsTr("Space") + ": " + qsTr("Pause/Resum") + " . </td><td>Q: " + qsTr("Quit") + " . </td></tr><tr><td>"
                  + qsTr("Rotate") + "</td><td>A: " + qsTr("Aspect ratio")  + "</td></tr></table>"
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
            onClicked: Qt.openUrlExternally("http://www.qtav.org#donate")
        }
    }
}
