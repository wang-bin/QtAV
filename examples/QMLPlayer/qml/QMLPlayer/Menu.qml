import QtQuick 2.0

Item {
    id: root
    property alias model: listView.model
    property int itemWidth: 80
    property int itemHeight: 30
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
            text: name
            anchors.horizontalCenter: parent.horizontalCenter
            width: itemWidth
            height: itemHeight
            onClicked: {
                if (d.selectedItem == delegateItem)
                    return
                if (d.selectedItem)
                    d.selectedItem.state = "baseState"
                d.selectedItem = delegateItem
                root.clicked(index, delegateItem.x, delegateItem.y)
            }
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        focus: true

        delegate: itemDelegate
    }
}
