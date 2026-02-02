pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: sidebar

    /// Whether the sidebar is expanded
    property bool expanded: true

    /// Collapsed width
    readonly property int collapsedWidth: 32

    /// Expanded width
    readonly property int expandedWidth: 240

    width: sidebar.expanded ? sidebar.expandedWidth : sidebar.collapsedWidth
    color: Theme.surfaceAlt

    Behavior on width {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }
    // Toggle button
    Rectangle {
        id: toggleButton
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        width: 24
        height: 24
        radius: 4
        color: toggleArea.containsMouse ? Theme.hovered : "transparent"

        ThemedIcon {
            anchors.centerIn: parent
            source: sidebar.expanded ? "qrc:/opengeolab/resources/icons/collapse.svg" : "qrc:/opengeolab/resources/icons/expand.svg"
            size: 16
        }

        MouseArea {
            id: toggleArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: sidebar.expanded = !sidebar.expanded
        }

        ToolTip.visible: toggleArea.containsMouse
        ToolTip.text: sidebar.expanded ? qsTr("Collapse sidebar") : qsTr("Expand sidebar")
        ToolTip.delay: 500
    }

    // Header
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: toggleButton.left
        height: 32
        color: "transparent"
        visible: sidebar.expanded

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Document")
            font.pixelSize: 13
            font.bold: true
            color: Theme.textPrimary
        }
    }

    // Separator
    Rectangle {
        id: separator
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
        visible: sidebar.expanded
    }
}
