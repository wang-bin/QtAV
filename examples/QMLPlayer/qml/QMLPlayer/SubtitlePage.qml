import QtQuick 2.0
import QtQuick.Dialogs 1.0
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Subtitle")
    signal subtitleChanged(string file)
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
                checked: PlayerConfig.subtitleEnabled
                width: parent.width/2
                height: Utils.kItemHeight
                onCheckedChanged: PlayerConfig.subtitleEnabled = checked
            }
            Button {
                id: autoLoad
                text: qsTr("Auto load")
                checkable: true
                checked: PlayerConfig.subtitleAutoLoad
                width: parent.width/2
                height: Utils.scaled(30)
                onCheckedChanged: PlayerConfig.subtitleAutoLoad = checked
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
            currentIndex: PlayerConfig.subtitleEngines[0] === "FFmpeg" ? 0 : 1
            model: ListModel {
                ListElement { name: "FFmpeg" }
                ListElement { name: "LibASS" }
            }
            onClicked: {
                PlayerConfig.subtitleEngines = [ model.get(index).name ]
            }
        }
        Text {
            color: "white"
            text: qsTr("Supported formats") + ": "
            font.pixelSize: Utils.kFontSize
        }
        Text {
            color: "orange"
            font.pixelSize: Utils.kFontSize
            text: supportedFormats.join(",")
            wrapMode: Text.Wrap
        }
    }
}
