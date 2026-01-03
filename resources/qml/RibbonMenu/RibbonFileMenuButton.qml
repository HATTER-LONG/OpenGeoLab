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
            if (!control.enabled)
                return control.palette.button;
            if (control.down)
                return control.palette.highlight;
            if (control.hovered)
                return control.palette.midlight;
            return "transparent";
        }
        border.width: 1
        border.color: Theme.border
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
        // Image {
        //     source: control.iconSource
        //     width: control.iconSize
        //     height: control.iconSize
        //     fillMode: Image.PreserveAspectFit
        //     Layout.alignment: Qt.AlignVCenter
        //     opacity: control.enabled ? 1.0 : 0.5
        // }

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
