import QtQuick 2.0
import QtQuick.Dialogs 1.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Subtitle")
    signal subtitleChanged(string file)
    signal engineChanged(string engine)
    signal autoLoadChanged(bool autoLoad)
    signal enabledChanged(bool value)
    property var supportedFormats: ["ass" , "ssa"]
    height: titleHeight + 5*Utils.kItemHeight + engine.contentHeight
    Column {
        anchors.fill: content
        spacing: Utils.kSpacing
        Row {
            width: parent.width
            height: Utils.kItemHeight
            Button {
                text: qsTr("Enabled")
                checkable: true
                checked: true
                width: parent.width/2
                height: Utils.kItemHeight
                onCheckedChanged: root.enabledChanged(checked)
            }
            Button {
                id: autoLoad
                text: qsTr("Auto load")
                checkable: true
                checked: true
                width: parent.width/2
                height: Utils.scaled(30)
                onCheckedChanged: root.autoLoadChanged(checked)
            }
        }
        Row {
            width: parent.width
            height: Utils.kItemHeight
            TextEdit {
                id: file
                color: "orange"
                font.pixelSize: Utils.kFontSize
                width: parent.width - open.width
                height: parent.height
            }
            FileDialog {
                id: dialog
                title: qsTr("Open a subtitle file")
                onAccepted: {
                    file.text = fileUrl
                    root.subtitleChanged(fileUrl.toString())
                }
            }
            Button {
                id: open
                text: qsTr("Open")
                width: Utils.scaled(60)
                height: Utils.kItemHeight
                onClicked: dialog.open()
            }
        }

        Text {
            color: "white"
            text: qsTr("Engine")
            font.pixelSize: Utils.kFontSize
        }
        Menu {
            id: engine
            width: parent.width
            itemWidth: parent.width
            model: ListModel {
                ListElement { name: "FFmpeg" }
                ListElement { name: "LibASS" }
            }
            onClicked: {
                root.engineChanged(model.get(index).name)
            }
        }
        Text {
            color: "white"
            text: qsTr("Supported formats") + ": "
            font.pixelSize: Utils.kFontSize
        }
        Text {
            color: "blue"
            font.pixelSize: Utils.kFontSize
            text: supportedFormats.join(",")
            wrapMode: Text.Wrap
        }
    }
}
