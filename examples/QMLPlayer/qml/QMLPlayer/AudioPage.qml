import QtQuick 2.0
import QtQuick.Dialogs 1.0
import QtAV 1.6
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Audio")
    signal channelChanged(int channel)
    signal externalAudioChanged(string file)
    signal audioTrackChanged(int track)
    property var internalAudioTracks : "unkown"
    property var externalAudioTracks : "unkown"
    property alias isExternal: externalCheck.checked
    height: Math.min(maxHeight, scroll.contentHeight)
    Flickable {
        id: scroll
        anchors.fill: content
        contentHeight: titleHeight + channelLabel.height + (channels.visible ? channels.contentHeight : 0)
                       + Utils.kItemHeight + trackLabel.height + tracksMenu.contentHeight + Utils.kSpacing*5
    Column {
        anchors.fill: parent
        spacing: Utils.kSpacing
        Text {
            id: channelLabel
            color: "white"
            text: qsTr("Channel layout")
            font.pixelSize: Utils.kFontSize
            width: parent.width
            visible: Qt.platform.os !== "winphone" && Qt.platform.os !== "winrt"
        }
        Menu {
            visible: channelLabel.visible
            id: channels
            width: parent.width
            itemWidth: parent.width
            model: ListModel {
                id: channelModel
            }
            onClicked: {
                root.channelChanged(model.get(index).value)
            }
        }
        Text {
            id: trackLabel
            color: "white"
            text: qsTr("Audio track")
            font.pixelSize: Utils.kFontSize
            width: parent.width
        }
        Menu {
            id: tracksMenu
            width: parent.width
            itemWidth: parent.width
            model: ListModel {
                id: tracksModel
            }
            onClicked: {
                console.log("audio track clikced: " + tracksModel.get(index).value)
                root.audioTrackChanged(tracksModel.get(index).value)
            }
        }
        Row {
            width: parent.width
            height: Utils.kItemHeight
            Button {
                id: externalCheck
                checked: false
                checkable: true
                text: qsTr("External")
                width: Utils.scaled(60)
                height: Utils.kItemHeight
                onCheckedChanged: {
                    if (checked)
                        root.externalAudioChanged(file.text)
                    else
                        root.externalAudioChanged("")
                    updateTracksMenu()
                }
            }
            Text {
                id: file
                color: "orange"
                font.pixelSize: Utils.kFontSize
                width: parent.width - externalCheck.width
                height: parent.height
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Click here to select a file")
                MouseArea {
                    anchors.fill: parent
                    onClicked: fileDialog.open()
                }
            }
        }
    }
    } //Flickable

    Component.onCompleted: {
        channelModel.append({name: qsTr("Stereo"), value: MediaPlayer.Stereo })
        channelModel.append({name: qsTr("Mono"), value: MediaPlayer.Mono })
        channelModel.append({name: qsTr("Left"), value: MediaPlayer.Left })
        channelModel.append({name: qsTr("Right"), value: MediaPlayer.Right })
    }
    onInternalAudioTracksChanged: updateTracksMenu()
    onExternalAudioTracksChanged: updateTracksMenu()
    function updateTracksMenu() {
        tracksModel.clear()
        var c = internalAudioTracks
        if (isExternal) {
            c = externalAudioTracks
        }
        for (var i = 0; i < c.length; ++i) {
            var t = c[i]
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
        id: fileDialog
        title: qsTr("Open an external audio track")
        onAccepted: {
            file.text = fileUrl
            if (externalCheck.checked)
                root.externalAudioChanged(fileUrl.toString())
        }
    }
}
