import QtQuick
import QtQuick.Layouts
import "../.."
import ".."

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Cylinder")
    pageIcon: "cylinder"
    actionId: "addCylinder"

    property string cylinderName: ""
    property real centerX: 0.0
    property real centerY: 0.0
    property real centerZ: 0.0
    property real radius: 5.0
    property real cylHeight: 10.0

    function getParameters() {
        return {
            module: "geometry",
            action: "create_cylinder",
            param: {
                name: root.cylinderName,
                x: root.centerX, y: root.centerY, z: root.centerZ,
                radius: root.radius, height: root.cylHeight
            },
            mute: false
        };
    }

    ParamField {
        width: parent.width
        theme: root.theme
        label: qsTr("Cylinder Name")
        placeholder: qsTr("Auto-generated if empty")
        value: root.cylinderName

        onValueEdited: function(newValue) {
            root.cylinderName = newValue;
        }
    }

    CoordinateField {
        width: parent.width
        theme: root.theme
        label: qsTr("Center Point")
        coordX: root.centerX
        coordY: root.centerY
        coordZ: root.centerZ

        onCoordinateChanged: function(x, y, z) {
            root.centerX = x;
            root.centerY = y;
            root.centerZ = z;
        }
    }

    Text {
        text: qsTr("Dimensions")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 6

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("R")
            value: root.radius
            accentColor: root.theme.axisX
            tooltipText: qsTr("Radius")

            onValueEdited: function(newVal) {
                root.radius = newVal;
            }
        }

        DimensionInput {
            Layout.fillWidth: true
            theme: root.theme
            label: qsTr("H")
            value: root.cylHeight
            accentColor: root.theme.axisZ
            tooltipText: qsTr("Height")

            onValueEdited: function(newVal) {
                root.cylHeight = newVal;
            }
        }
    }

    Rectangle {
        width: parent.width
        height: infoColumn.implicitHeight + 16
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted

        Column {
            id: infoColumn

            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            RowLayout {
                width: parent.width
                spacing: 4

                Text {
                    Layout.preferredWidth: 86
                    text: qsTr("Volume:")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                }

                Text {
                    Layout.fillWidth: true
                    text: (Math.PI * root.radius * root.radius * root.cylHeight).toFixed(3)
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                    horizontalAlignment: Text.AlignLeft
                }
            }

            RowLayout {
                width: parent.width
                spacing: 4

                Text {
                    Layout.preferredWidth: 86
                    text: qsTr("Surface Area:")
                    color: root.theme.textSecondary
                    font.pixelSize: 11
                }

                Text {
                    Layout.fillWidth: true
                    text: (2 * Math.PI * root.radius * (root.radius + root.cylHeight)).toFixed(3)
                    color: root.theme.textPrimary
                    font.pixelSize: 11
                    font.bold: true
                }
            }
        }
    }
}
