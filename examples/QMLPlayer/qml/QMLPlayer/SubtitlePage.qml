import QtQuick 2.0
import QtQuick.Dialogs 1.1
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Subtitle")
    signal subtitleChanged(string file)
    signal subtitleTrackChanged(int track)
    property var supportedFormats: ["ass" , "ssa"]
    height: titleHeight + tracksMenu.height + 11*Utils.kItemHeight + engine.contentHeight + Utils.kSpacing*7
    property var internalSubtitleTracks : "unkown"
    Column {
        anchors.fill: content
        spacing: Utils.kSpacing
        Row {
            width: parent.width
            height: Utils.kItemHeight
            Text {
                id: delayLabel
                color: "white"
                font.pixelSize: Utils.kFontSize
                text: qsTr("Delay")
                width: Utils.scaled(60)
                height: Utils.kItemHeight
            }
            TextInput {
                color: "orange"
                font.pixelSize: Utils.kFontSize
                height: Utils.kItemHeight
                text: PlayerConfig.subtitleDelay
                onTextChanged: {
                    var v = parseFloat(text)
                    if (isNaN(v))
                        return
                    PlayerConfig.subtitleDelay = Math.round(v*1000)/1000
                }
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
                font.pixelSize: Utils.kFontSize
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

        Text {
            color: "white"
            text: qsTr("Style") + " (" + qsTr("Only for LibAss engine") + ")"
            font.pixelSize: Utils.kFontSize
        }
        Item {
            width: parent.width
            height: Utils.kItemHeight
            Text {
                id: fontFile
                color: "orange"
                font.pixelSize: Utils.kFontSize
                anchors.left: parent.left
                anchors.right: openFontFile.left
                height: parent.height
                text: PlayerConfig.assFontFile
            }
            Button {
                id: openFontFile
                text: qsTr("Font file")
                width: Utils.scaled(80)
                anchors.right: forceFontFile.left
                height: parent.height
                onClicked: fontFileDialog.open()
            }
            Button {
                id: forceFontFile
                text: qsTr("Force")
                width: Utils.scaled(50)
                anchors.right: clearFontFile.left
                height: parent.height
                checkable: true
                checked: PlayerConfig.assFontFileForced
                onCheckedChanged: PlayerConfig.assFontFileForced = checked
            }
            Button {
                id: clearFontFile
                text: qsTr("Clear")
                width: Utils.scaled(50)
                anchors.right: parent.right
                height: parent.height
                onClicked: PlayerConfig.assFontFile = ""
            }
        }
        Item {
            width: parent.width
            height: Utils.kItemHeight
            Text {
                id: fontsDir
                color: "orange"
                font.pixelSize: Utils.kFontSize
                text: PlayerConfig.assFontsDir
                anchors.left: parent.left
                anchors.right: openFontsDir.left
                height: parent.height
            }
            Button {
                id: openFontsDir
                text: qsTr("Fonts dir")
                width: Utils.scaled(80)
                anchors.right: clearFontsDir.left
                height: parent.height
                onClicked: fontsDirDialog.open()
            }
            Button {
                id: clearFontsDir
                text: qsTr("Clear")
                width: Utils.scaled(50)
                anchors.right: parent.right
                height: parent.height
                onClicked: PlayerConfig.assFontsDir = ""
            }
        }

        Text {
            color: "white"
            text: qsTr("Embedded Subtitles") + ": " + internalSubtitleTracks.length
            font.pixelSize: Utils.kFontSize
            font.bold: true
        }
        Menu {
            id: tracksMenu
            width: parent.width
            itemWidth: parent.width
            model: ListModel {
                id: tracksModel
            }
            onClicked: {
                console.log("subtitle track clikced: " + tracksModel.get(index).value)
                root.subtitleTrackChanged(tracksModel.get(index).value)
            }
        }
        Text {
            color: "white"
            text: qsTr("External Subtitle")
            font.pixelSize: Utils.kFontSize
            font.bold: true
        }
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
    }

    onInternalSubtitleTracksChanged: updateTracksMenu()
    function updateTracksMenu() {
        tracksModel.clear()
        tracksMenu.height = Math.min(internalSubtitleTracks.length, 2)*tracksMenu.itemHeight
        for (var i = 0; i < internalSubtitleTracks.length; ++i) {
            var t = internalSubtitleTracks[i]
            var label = "#" + t.id
            if (t.codec)
                label += " '" + t.codec + "'"
            if (t.language)
                label += " (" + t.language + ")"
            if (t.title)
                label += ": " + t.title
            tracksModel.append({name: label, value: t.id})
        }
    }
    FileDialog {
        id: fontFileDialog
        title: qsTr("Choose a font file")
        onAccepted: {
            fontFile.text = fileUrl
            PlayerConfig.assFontFile = fileUrl
        }
    }

    FileDialog {
        id: fontsDirDialog
        title: qsTr("Choose a fonts dir")
        selectFolder: true
        onAccepted: {
            fontsDir.text = folder
            PlayerConfig.assFontsDir = folder
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
