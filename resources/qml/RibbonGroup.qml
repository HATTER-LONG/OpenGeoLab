pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @file RibbonGroup.qml
 * @brief A group container component for Ribbon toolbar
 *
 * Displays a titled group of buttons with a title label at the bottom.
 */
Rectangle {
    id: ribbonGroup

    property string title: "Group"

    // Dark theme colors (fixed)
    readonly property color titleColor: "#b8b8b8"
    readonly property color separatorColor: "#3a3f4b"

    implicitWidth: contentRow.implicitWidth + 16
    Layout.fillHeight: true
    color: "transparent"

    // Group content area - use Item with Row for dynamic content
    default property alias content: contentRow.data

    Item {
        id: contentContainer
        anchors.top: parent.top
        anchors.bottom: titleBar.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 5
        anchors.bottomMargin: 0
        width: contentRow.width

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 4
        }
    }

    // Title bar at bottom
    Rectangle {
        id: titleBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 16
        color: "transparent"

        Text {
            anchors.centerIn: parent
            text: ribbonGroup.title
            font.pixelSize: 10
            color: ribbonGroup.titleColor
        }
    }

    // Right separator
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 5
        anchors.bottomMargin: 5
        width: 1
        color: ribbonGroup.separatorColor
    }
}
