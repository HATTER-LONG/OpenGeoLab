pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

/**
 * @brief A large button component for Ribbon toolbar
 *
 * Displays an icon (or emoji text) on top and label text below
 */
Rectangle {
    id: ribbonButton

    property string iconSource: ""
    property string iconText: "?"
    property alias text: labelText.text

    signal clicked

    Layout.preferredWidth: 50
    Layout.preferredHeight: 70
    Layout.fillHeight: true

    color: buttonMouseArea.containsMouse ? (buttonMouseArea.pressed ? "#CCE4F7" : "#E5F1FB") : "transparent"
    radius: 3
    border.width: buttonMouseArea.containsMouse ? 1 : 0
    border.color: "#CCE4F7"

    Column {
        anchors.centerIn: parent
        spacing: 2

        // Icon area
        Rectangle {
            width: 32
            height: 32
            anchors.horizontalCenter: parent.horizontalCenter
            color: "transparent"

            // Use image if iconSource is provided, otherwise use text
            Image {
                anchors.centerIn: parent
                width: 24
                height: 24
                source: ribbonButton.iconSource
                visible: ribbonButton.iconSource !== ""
                fillMode: Image.PreserveAspectFit
            }

            Text {
                anchors.centerIn: parent
                text: ribbonButton.iconText
                font.pixelSize: 20
                color: "#444444"
                visible: ribbonButton.iconSource === ""
            }
        }

        // Label
        Text {
            id: labelText
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 11
            color: "#333333"
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 0.9
        }
    }

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: ribbonButton.clicked()
    }
}
