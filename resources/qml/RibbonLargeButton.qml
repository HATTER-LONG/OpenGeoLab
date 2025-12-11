pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @brief A large button component for Ribbon toolbar
 *
 * Displays an SVG icon on top and label text below
 */
Rectangle {
    id: ribbonButton

    property string iconSource: ""
    property alias text: labelText.text

    signal clicked

    // Fixed size for consistent layout
    width: 48
    height: 60

    // Layout hints (for when used in RowLayout)
    Layout.preferredWidth: 48
    Layout.preferredHeight: 60

    // Dark theme colors
    property color hoverColor: "#3a3f4b"
    property color pressedColor: "#4a5568"
    property color textColor: "#e1e1e1"

    color: buttonMouseArea.containsMouse ? (buttonMouseArea.pressed ? pressedColor : hoverColor) : "transparent"
    radius: 3
    border.width: buttonMouseArea.containsMouse ? 1 : 0
    border.color: hoverColor

    Column {
        anchors.centerIn: parent
        spacing: 1

        // Icon area
        Rectangle {
            width: 28
            height: 28
            anchors.horizontalCenter: parent.horizontalCenter
            color: "transparent"

            Image {
                anchors.centerIn: parent
                width: 22
                height: 22
                source: ribbonButton.iconSource
                fillMode: Image.PreserveAspectFit
            }
        }

        // Label
        Text {
            id: labelText
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 10
            color: ribbonButton.textColor
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 0.85
        }
    }

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: ribbonButton.clicked()
    }
}
