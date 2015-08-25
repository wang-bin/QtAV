import QtQuick 2.0
import "utils.js" as Utils

Item {
    id: root
    property alias model: listView.model
    property int currentIndex: -1 //listView.currentIndex //wrong value
    property int itemWidth: Utils.kItemWidth
    property int itemHeight: Utils.kItemHeight
    property alias header: listView.header
    property alias listOrientation: listView.orientation
    property alias spacing: listView.spacing
    property alias contentWidth: listView.contentWidth
    property alias contentHeight: listView.contentHeight
    signal clicked(int index, int x, int y)
    height: model.count * itemHeight
    QtObject {
        id: d
        property Item selectedItem
        property string detail: ""
    }
    Component {
        id: itemDelegate
        DelegateItem {
            id: delegateItem
            text: qsTr(name)
            anchors.horizontalCenter: parent.horizontalCenter
            width: itemWidth
            height: itemHeight
            onClicked: {
                root.clicked(index, delegateItem.x, delegateItem.y)
                if (d.selectedItem == delegateItem)
                    return
                if (d.selectedItem)
                    d.selectedItem.state = "baseState"
                d.selectedItem = delegateItem
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        focus: true
        currentIndex: -1
        delegate: itemDelegate
    }
    onCurrentIndexChanged: {
         if (d.selectedItem)
            d.selectedItem.state = "baseState"
        listView.currentIndex = currentIndex
        d.selectedItem = listView.currentItem
        if (d.selectedItem)
            d.selectedItem.state = "selected"
    }
    Component.onCompleted: {
        listView.currentIndex = currentIndex
        d.selectedItem = listView.currentItem
        if (d.selectedItem)
            d.selectedItem.state = "selected"
    }
}
