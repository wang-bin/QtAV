import QtQuick 2.0
import "utils.js" as Utils

Page {
    title: qsTr("Misc")
    height: Math.min(maxHeight, scroll.contentHeight)
    Flickable {
        id: scroll
        anchors.fill: content
        contentHeight: titleHeight + (7+glSet.visible*4)*Utils.kItemHeight + detail.contentHeight + 5*Utils.kSpacing
    Column {
        anchors.fill: parent
        spacing: Utils.kSpacing
        Button {
            text: qsTr("Reset all")
            bgColor: "#ff0000"
            width: parent.width
            height: Utils.kItemHeight
            onClicked: PlayerConfig.reset()
        }
        Row {
            width: parent.width
            height: Utils.kItemHeight
            Button {
                text: qsTr("Preview")
                checkable: true
                checked: PlayerConfig.previewEnabled
                width: parent.width/3
                height: Utils.kItemHeight
                onCheckedChanged: {
                    console.log("check changed " + checked )
                    PlayerConfig.previewEnabled = checked
                }
            }
            Text {
                id: detail
                font.pixelSize: Utils.kFontSize
                color: "white"
                width: parent.width*2/3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Press on the preview item to seek")
            }
        }
        Row {
            width: parent.width
            height: Utils.kItemHeight
            visible: Qt.platform.os === "linux"
            Button {
                text: "EGL"
                checkable: true
                checked: PlayerConfig.EGL
                width: parent.width/3
                height: Utils.kItemHeight
                onCheckedChanged: PlayerConfig.EGL = checked
            }
            Text {
                font.pixelSize: Utils.kFontSize
                color: "white"
                width: parent.width*2/3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Requirement") + ": Qt>=5.5+XCB"
            }
        }
        Row {
            id: glSet
            width: parent.width
            height: 5*Utils.kItemHeight
            spacing: Utils.kSpacing
            visible: Qt.platform.os === "windows" || Qt.platform.os === "wince"
            Text {
                width: parent.width/3
                height: parent.height
                color: "white"
                font.pixelSize: Utils.kFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: glSet.visible
                text: "OpenGL\n(" + qsTr("Restart to apply") + ")"
            }
            Menu {
                id: glMenu
                width: parent.width/3
                height: parent.height
                itemWidth: width
                model: ListModel {
                    id: glModel
                    ListElement { name: "Auto" }
                    ListElement { name: "Desktop" }
                    ListElement { name: "OpenGLES" }
                    ListElement { name: "Software" }
                }
                onClicked: {
                    var gl = glModel.get(index).name
                    PlayerConfig.openGLType = gl
                    if (gl === "OpenGLES") {
                        angle.sourceComponent = angleMenuComponent
                    } else {
                        angle.sourceComponent = undefined
                    }
                }
                function updateUi() {
                    currentIndex = -1
                    for (var i = 0; i < glModel.count; ++i) {
                        console.log("gl: " + glModel.get(i).name)
                        if (PlayerConfig.openGLType === i) {
                            currentIndex = i
                            break
                        }
                    }
                }
                Component.onCompleted: updateUi()
            }
            Loader {
                id: angle
                width: parent.width/3
                height: parent.height
            }
            Component {
                id: angleMenuComponent
                Menu {
                    anchors.fill: parent
                    itemWidth: width
                    model: ListModel {
                        id: angleModel
                        ListElement { name: "Auto"; detail: "D3D9 > D3D11" }
                        ListElement { name: "D3D9" }
                        ListElement { name: "D3D11" }
                        ListElement { name: "WARP" }
                    }
                    onClicked:  PlayerConfig.ANGLEPlatform = angleModel.get(index).name
                    function updateUi() {
                        currentIndex = -1
                        if (Qt.platform.os !== "windows" && Qt.platform.os !== "wince")
                            return
                        for (var i = 0; i < angleModel.count; ++i) {
                            console.log("d3d: " + angleModel.get(i).name)
                            if (PlayerConfig.ANGLEPlatform === angleModel.get(i).name) {
                                currentIndex = i
                                break
                            }
                        }
                    }
                    Component.onCompleted: updateUi()
                }
            }
        }
        Row {
            width: parent.width
            height: 3*Utils.kItemHeight
            spacing: Utils.kSpacing
            Text {
                font.pixelSize: Utils.kFontSize
                color: "white"
                width: parent.width/3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAnywhere
                text: "Log level\n" + qsTr("Developer only")
            }
            Menu {
                id: logMenu
                width: parent.width/2
                height: parent.height
                itemWidth: width
                model: ListModel {
                    id: logModel
                    ListElement { name: "off" }
                    ListElement { name: "warning" }
                    ListElement { name: "debug" }
                    ListElement { name: "all" }
                }
                onClicked: PlayerConfig.logLevel = logModel.get(index).name
                function updateUi() {
                    currentIndex = -1
                    for (var i = 0; i < logModel.count; ++i) {
                        if (PlayerConfig.logLevel === logModel.get(i).name) {
                            currentIndex = i
                            break
                        }
                    }
                }
                Component.onCompleted: updateUi()
            }
        }
        Row {
            width: parent.width
            height: 2*Utils.kItemHeight
            spacing: Utils.kSpacing
            Text {
                font.pixelSize: Utils.kFontSize
                color: "white"
                width: parent.width/3
                height: parent.height
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAnywhere
                text: qsTr("Language") + "\n" + qsTr("Restart to apply")
            }
            Menu {
                id: langMenu
                width: parent.width/2
                height: parent.height
                itemWidth: width
                model: ListModel {
                    id: langModel
                    ListElement { name: "system" }
                    ListElement { name: "en_US" }
                    ListElement { name: "zh_CN" }
                }
                onClicked: PlayerConfig.language = langModel.get(index).name
                function updateUi() {
                    currentIndex = -1
                    for (var i = 0; i < langModel.count; ++i) {
                        if (PlayerConfig.language === langModel.get(i).name) {
                            currentIndex = i
                            break
                        }
                    }
                }
                Component.onCompleted: updateUi()
            }
        }
    }
    Component.onCompleted: {
        if (Qt.platform.os !== "windows" && Qt.platform.os !== "wince")
            return
        if (PlayerConfig.openGLType === 2) {
            angle.sourceComponent = angleMenuComponent
        }
    }
    Connections {
        target: PlayerConfig
        onLogLevelChanged: logMenu.updateUi()
        onLanguageChanged: langMenu.updateUi()
        onOpenGLTypeChanged: {
            if (glSet.visible)
                glMenu.updateUi()
        }
    }
    }
}
