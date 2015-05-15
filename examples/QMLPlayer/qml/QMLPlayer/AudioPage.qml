import QtQuick 2.0
import QtQuick.Dialogs 1.0
import QtAV 1.6
import "utils.js" as Utils

Page {
    id: root
    title: qsTr("Audio")
    signal channelChanged(int channel)
    signal muteChanged(bool value)
    signal externalAudioChanged(string file)
    signal audioTrackChanged(int track)
    property var internalTracks : "unkown"
    property var externalTracks : "unkown"
    property alias isExternal: externalCheck.checked
    height: titleHeight + channelLabel.height + channels.contentHeight
            + Utils.kItemHeight*2 + trackLabel.height + tracksMenu.contentHeight + Utils.kSpacing*6
    Column {
        anchors.fill: content
        spacing: Utils.kSpacing
        Text {
            id: channelLabel
            color: "white"
            text: qsTr("Channel layout")
            font.pixelSize: Utils.kFontSize
            width: parent.width
        }
        Menu {
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
        Button {
            text: qsTr("Mute")
            checked: false
            checkable: true
            width: parent.width
            height: Utils.kItemHeight
            onCheckedChanged: root.muteChanged(checked)
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
                MouseArea {
                    anchors.fill: parent
                    onClicked: fileDialog.open()
                }
            }
        }
    }
    Component.onCompleted: {
        channelModel.append({name: qsTr("Stero"), value: MediaPlayer.Stero })
        channelModel.append({name: qsTr("Mono"), value: MediaPlayer.Mono })
        channelModel.append({name: qsTr("Left"), value: MediaPlayer.Left })
        channelModel.append({name: qsTr("Right"), value: MediaPlayer.Right })
    }
    onInternalTracksChanged: updateTracksMenu()
    onExternalTracksChanged: updateTracksMenu()
    function updateTracksMenu() {
        tracksModel.clear()
        var c = internalTracks
        if (isExternal) {
            c = externalTracks
        }
        for (var i = 0; i < c.length; ++i) {
            var label = "#" + c[i].id
            if (c[i].language)
                label += " (" + c[i].language + ")"
            if (c[i].title)
                label += ": " + c[i].title
            tracksModel.append({name: label, value: c[i].id})
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
