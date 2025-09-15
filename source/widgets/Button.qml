import QtQuick

Item {
    id: root

    property alias text: label.text
    signal clicked

    width: 116
    height: 26

    Rectangle {
        id: background
        anchors.fill: parent
        color: "lightsteelblue"
        border.color: "slategrey"
    }

    Text {
        id: label
        anchors.centerIn: parent
        text: "Start"
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.clicked();
        }
    }
}
