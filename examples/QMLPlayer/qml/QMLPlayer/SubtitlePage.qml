import QtQuick 2.0
import QtQuick.Dialogs 1.0

Page {
    id: root
    title: qsTr("Subtitle")
    signal subtitleChanged(string file)
    signal engineChanged(string engine)
    signal autoLoadChanged(bool autoLoad)
    signal enabledChanged(bool value)
    Column {
        anchors.fill: content
        spacing: 4
        Row {
            width: parent.width
            height: 30
            Button {
                text: qsTr("Enabled")
                checkable: true
                checked: true
                width: parent.width/2
                height: 30
                onCheckedChanged: root.enabledChanged(checked)
            }
            Button {
                id: autoLoad
                text: qsTr("Auto load")
                checkable: true
                checked: true
                width: parent.width/2
                height: 30
                onCheckedChanged: root.autoLoadChanged(checked)
            }
        }
        Row {
            width: parent.width
            height: 30
            TextEdit {
                id: file
                color: "orange"
                font.pixelSize: 16
                width: parent.width - open.width
                height: 30
            }
            FileDialog {
                id: dialog
                title: qsTr("get a subtitle file")
                onAccepted: {
                    file.text = fileUrl
                    root.subtitleChanged(fileUrl.toString())
                }
            }
            Button {
                id: open
                text: qsTr("Open")
                width:60
                height: 30
                onClicked: dialog.open()
            }
        }

        Text {
            id: engineLable
            color: "white"
            text: qsTr("Engine")
            font.pixelSize: 16
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
    }

}
