/**
 * @file ViewControlButton.qml
 * @brief A styled button for view control toolbar
 */
import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root

    property string iconSource: ""
    property string tooltipText: ""
    property bool hasDropdown: false

    implicitWidth: 32
    implicitHeight: 32

    hoverEnabled: true

    background: Rectangle {
        radius: 4
        color: {
            if (root.pressed) {
                return Qt.rgba(0.35, 0.35, 0.4, 1.0);
            } else if (root.hovered) {
                return Qt.rgba(0.25, 0.25, 0.3, 1.0);
            } else {
                return "transparent";
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: 100
            }
        }
    }

    contentItem: Item {
        Image {
            id: iconImage
            anchors.centerIn: parent
            width: 18
            height: 18
            source: root.iconSource
            sourceSize: Qt.size(18, 18)
            fillMode: Image.PreserveAspectFit
            smooth: true
            antialiasing: true
            opacity: root.enabled ? 1.0 : 0.5

            // Fallback when icon is not available
            visible: status === Image.Ready
        }

        // Fallback text when icon is not available
        Text {
            anchors.centerIn: parent
            text: root.hasDropdown ? "▼" : "?"
            color: "white"
            font.pixelSize: 12
            visible: iconImage.status !== Image.Ready
        }

        // Dropdown indicator
        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 2
            text: "▾"
            color: Qt.rgba(0.7, 0.7, 0.7, 1.0)
            font.pixelSize: 8
            visible: root.hasDropdown
        }
    }

    ToolTip.visible: root.hovered && root.tooltipText !== ""
    ToolTip.text: root.tooltipText
    ToolTip.delay: 500
}
