pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @brief A group container component for Ribbon toolbar
 *
 * Displays a titled group with a bottom label
 */
Rectangle {
    id: ribbonGroup

    property string title: "Group"

    implicitWidth: contentRow.implicitWidth + 16
    Layout.fillHeight: true
    color: "transparent"

    // Group content area
    default property alias content: contentRow.data

    RowLayout {
        id: contentRow
        anchors.top: parent.top
        anchors.bottom: titleBar.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 5
        anchors.bottomMargin: 0
        spacing: 4
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
            color: "#666666"
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
        color: "#D1D1D1"
    }
}
