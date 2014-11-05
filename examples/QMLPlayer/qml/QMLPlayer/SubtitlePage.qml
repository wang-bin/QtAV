import QtQuick 2.0
import QtQuick.Dialogs 1.1
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Subtitle")
    signal subtitleChanged(string file)
    property var supportedFormats: ["ass" , "ssa"]
    height: titleHeight + 6*Utils.kItemHeight + engine.contentHeight
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
            Button {
                id: open
                text: qsTr("Open")
                width: Utils.scaled(60)
                height: Utils.kItemHeight
                onClicked: fileDialog.open()
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
            onClicked: PlayerConfig.subtitleEngines = [ model.get(index).name ]
        }
        Row {
            Text {
                color: "white"
                text: qsTr("Supported formats") + ": "
                font.pixelSize: Utils.kFontSize
            }
            Text {
                id: formats
                color: "orange"
                font.pixelSize: Utils.kFontSize
                text: supportedFormats.join(",")
                wrapMode: Text.Wrap
            }
        }
        Text {
            color: "white"
            text: qsTr("Style") + " (" + qsTr("Only for FFmpeg engine") + ")"
            font.pixelSize: Utils.kFontSize
        }
        Row {
            Text {
                color: "white"
                text: qsTr("Bottom margin")
                font.pixelSize: Utils.kFontSize
            }
            TextInput {
                color: "orange"
                font.pixelSize: Utils.kFontSize
                width: Utils.scaled(40)
                height: parent.height
                validator: IntValidator{bottom: 0;}
                text: PlayerConfig.subtitleBottomMargin
                onTextChanged: PlayerConfig.subtitleBottomMargin = parseInt(text)
            }
            Button {
                text: qsTr("Font")
                width: Utils.scaled(60)
                height: Utils.kItemHeight
                onClicked: fontDialog.open()
            }
            Rectangle {
                color: PlayerConfig.subtitleColor
                width: Utils.kItemHeight
                height: Utils.kItemHeight
                MouseArea {
                    anchors.fill: parent
                    onPressed: colorDialog.open()
                }
            }
            Button {
                text: qsTr("Outline")
                checkable: true
                checked:  PlayerConfig.subtitleOutline
                width: Utils.scaled(60)
                height: Utils.kItemHeight
                onCheckedChanged: PlayerConfig.subtitleOutline = checked
            }
            Rectangle {
                color: PlayerConfig.subtitleOutlineColor
                width: Utils.kItemHeight
                height: Utils.kItemHeight
                MouseArea {
                    anchors.fill: parent
                    onPressed: outlineColorDialog.open()
                }
            }
            Text {
                text: qsTr("Preview") + ":"
                color: "white"
                font.pointSize: Utils.kFontSize
            }
            TextInput {
                color: "orange"
                font.pointSize: Utils.kFontSize
                width: Utils.scaled(50)
                height: parent.height
                text: "QtAV"
                onTextChanged: stylePreview.text = text
            }
            Text {
                id: stylePreview
                text: "QtAV"
                font: PlayerConfig.subtitleFont
                style: PlayerConfig.subtitleOutline ? Text.Outline : Text.Normal
                styleColor: PlayerConfig.subtitleOutlineColor
                color: PlayerConfig.subtitleColor
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Open a subtitle file")
        onAccepted: {
            file.text = fileUrl
            root.subtitleChanged(fileUrl.toString())
        }
    }
    FontDialog {
        id: fontDialog
        font: PlayerConfig.subtitleFont
        onAccepted: PlayerConfig.subtitleFont = font
    }
    ColorDialog {
        id: colorDialog
        color: PlayerConfig.subtitleColor
        onAccepted: PlayerConfig.subtitleColor = color
    }
    ColorDialog {
        id: outlineColorDialog
        color: PlayerConfig.subtitleOutlineColor
        onAccepted: PlayerConfig.subtitleOutlineColor = color
    }
}
