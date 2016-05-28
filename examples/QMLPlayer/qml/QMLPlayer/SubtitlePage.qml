import QtQuick 2.0
import QtQuick.Dialogs 1.1
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Subtitle")
    signal subtitleTrackChanged(int track)
    property var supportedFormats: ["ass" , "ssa"]
    property var internalSubtitleTracks : "unkown"
    height: Math.min(maxHeight, scroll.contentHeight)
    Flickable {
        id: scroll
        anchors.fill: content
        contentHeight: titleHeight + tracksMenu.height + 5*Utils.kItemHeight + engine.contentHeight + Utils.kSpacing*4
    Column {
        anchors.fill: parent
        spacing: Utils.kSpacing
        Row {
            width: parent.width
            height: Utils.kItemHeight
            spacing: Utils.kSpacing
            Text {
                id: delayLabel
                color: "white"
                font.pixelSize: Utils.kFontSize
                text: qsTr("Delay") + " (" + qsTr("ms") + ")"
                width: contentWidth
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

        Row {
            width: parent.width
            height: 2*Utils.kItemHeight // engine.contentHeight
            spacing: Utils.kSpacing
            Text {
                color: "white"
                text: qsTr("Engine")
                font.pixelSize: Utils.kFontSize
                width: contentWidth
                height: parent.height
            }
            Menu {
                id: engine
                width: parent.width - Utils.kItemWidth
                height: parent.height
                itemWidth: width
                currentIndex: PlayerConfig.subtitleEngines[0] === "FFmpeg" ? 0 : 1
                model: ListModel {
                    ListElement { name: "FFmpeg" }
                    ListElement { name: "LibASS" }
                }
                onClicked: {
                    PlayerConfig.subtitleEngines = [ model.get(index).name, model.get((index+1)%model.count).name ]
                    var e = model.get(index).name
                    if (e === "FFmpeg")
                        engineDetail.sourceComponent = ffmpeg
                    else
                        engineDetail.sourceComponent = libass
                }
            }
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
        Item {
            width: parent.width
            height: 2*Utils.kItemHeight
            Loader {
                id: engineDetail
                anchors.fill: parent
            }
            Component {
                id: ffmpeg
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
                        id: stylePreview
                        text: "Q"
                        font: PlayerConfig.subtitleFont
                        style: PlayerConfig.subtitleOutline ? Text.Outline : Text.Normal
                        styleColor: PlayerConfig.subtitleOutlineColor
                        color: PlayerConfig.subtitleColor
                    }
                }
            }
            Component {
                id: libass
                Column {
                    Item {
                        width: parent.width
                        height: Utils.kItemHeight
                        Text {
                            id: fontFile
                            color: "orange"
                            elide: Text.ElideMiddle
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
                        visible: Qt.platform.os !== "winphone" && Qt.platform.os !== "winrt"
                        width: parent.width
                        height: Utils.kItemHeight
                        Text {
                            id: fontsDir
                            color: "orange"
                            elide: Text.ElideMiddle
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
                }
            }
        }


        Item {
            width: parent.width
            height: tracksMenu.height
            Text {
                id: intSubLabel
                color: "white"
                text: qsTr("Embedded Subtitles") + ": " + internalSubtitleTracks.length
                font.pixelSize: Utils.kFontSize
                font.bold: true
                wrapMode: Text.WrapAnywhere
                width: Utils.scaled(110)
                height: parent.height
                visible: height > 0
            }
            Menu {
                id: tracksMenu
                anchors.left: intSubLabel.right
                anchors.right: parent.right
                itemWidth: width
                model: ListModel {
                    id: tracksModel
                }
                onClicked: {
                    console.log("subtitle track clikced: " + tracksModel.get(index).value)
                    root.subtitleTrackChanged(tracksModel.get(index).value)
                }
            }
        }

        Row {
            width: parent.width
            height: 2*Utils.kItemHeight
            spacing: Utils.kSpacing
            Text {
                id: extSubLabel
                color: "white"
                text: qsTr("External Subtitle")
                wrapMode: Text.WrapAnywhere
                font.pixelSize: Utils.kFontSize
                font.bold: true
                width: Utils.scaled(110)
            }
            Column {
                width:parent.width - extSubLabel.width
                height: parent.height
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
                        checked: enabled && PlayerConfig.subtitleAutoLoad
                        visible: Qt.platform.os !== "winrt"
                        width: parent.width/2
                        height: Utils.kItemHeight
                        onCheckedChanged: PlayerConfig.subtitleAutoLoad = checked
                    }
                }
            }
        }
    }
    } //Flickable

    onInternalSubtitleTracksChanged: updateTracksMenu()
    function updateTracksMenu() {
        tracksModel.clear()
        tracksMenu.height = Math.min(internalSubtitleTracks.length, 3)*tracksMenu.itemHeight
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
        folder: PlayerConfig.assFontFile
        onAccepted: PlayerConfig.assFontFile = fileUrl
    }

    FileDialog {
        id: fontsDirDialog
        title: qsTr("Choose a fonts dir")
        selectFolder: true
        folder: PlayerConfig.assFontsDir
        onAccepted: PlayerConfig.assFontsDir = folder
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
    Component.onCompleted: {
        if (PlayerConfig.subtitleEngines[0] === "FFmpeg")
            engineDetail.sourceComponent = ffmpeg
        else
            engineDetail.sourceComponent = libass
    }
}
