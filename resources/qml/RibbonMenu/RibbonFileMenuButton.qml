pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Button {
    id: control

    // ===== API =====
    property url iconSource
    property int iconSize: 18

    implicitHeight: 36
    Layout.fillWidth: true
    padding: 8
    font.pixelSize: 13

    hoverEnabled: true

    // ===== Background =====
    background: Rectangle {
        radius: 6
        color: {
            if (control.down)
                return Theme.clicked;
            if (control.hovered)
                return Theme.hovered;
            return Theme.palette.button;
        }
        border.color: Theme.border
        border.width: 1
    }

    // ===== Content =====
    contentItem: RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        ThemedIcon {
            source: control.iconSource
            color: control.hovered ? control.palette.highlight : control.palette.buttonText
        }

        Label {
            text: control.text
            font: control.font
            color: control.palette.buttonText
            verticalAlignment: Text.AlignVCenter
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
