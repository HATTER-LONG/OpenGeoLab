/**
 * @file AddTorusPage.qml
 * @brief Function page for creating a torus (ring shape)
 *
 * Allows user to input center position, major radius (ring radius),
 * and minor radius (tube radius) to create a torus geometry.
 * The torus axis is along the Z direction.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Torus")
    pageIcon: "qrc:/opengeolab/resources/icons/torus.svg"
    serviceName: "GeometryService"
    actionId: "addTorus"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Center coordinates
    property real centerX: 0.0
    property real centerY: 0.0
    property real centerZ: 0.0

    /// Major radius (distance from center to tube center)
    property real majorRadius: 10.0

    /// Minor radius (tube radius)
    property real minorRadius: 3.0

    /// Torus name
    property string torusName: ""

    function getParameters() {
        return {
            "action": "create",
            "type": "torus",
            "name": torusName || "Torus_" + Date.now(),
            "x": centerX,
            "y": centerY,
            "z": centerZ,
            "majorRadius": majorRadius,
            "minorRadius": minorRadius
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        ParamField {
            label: qsTr("Torus Name")
            placeholder: qsTr("Auto-generated if empty")
            value: root.torusName
            onValueEdited: newValue => root.torusName = newValue
        }

        CoordinateField {
            label: qsTr("Center Point")
            coordX: root.centerX
            coordY: root.centerY
            coordZ: root.centerZ
            onCoordinateChanged: (x, y, z) => {
                root.centerX = x;
                root.centerY = y;
                root.centerZ = z;
            }
        }

        // Radii section
        Label {
            text: qsTr("Radii")
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        RowLayout {
            width: parent.width
            spacing: 6

            DimensionInput {
                Layout.fillWidth: true
                label: "R1"
                tooltipText: qsTr("Major radius (ring)")
                value: root.majorRadius
                accentColor: "#E53935"
                onValueEdited: newVal => root.majorRadius = newVal
            }

            DimensionInput {
                Layout.fillWidth: true
                label: "R2"
                tooltipText: qsTr("Minor radius (tube)")
                value: root.minorRadius
                accentColor: "#43A047"
                onValueEdited: newVal => root.minorRadius = newVal
            }
        }

        // Validation warning
        Rectangle {
            visible: root.minorRadius >= root.majorRadius
            width: parent.width
            height: warnText.implicitHeight + 12
            radius: 4
            color: Qt.rgba(1, 0.6, 0, 0.15)
            border.width: 1
            border.color: Qt.rgba(1, 0.6, 0, 0.4)

            Label {
                id: warnText
                anchors.fill: parent
                anchors.margins: 6
                text: qsTr("⚠️ Minor radius should be less than major radius for a valid torus.")
                font.pixelSize: 11
                color: "#FF9800"
                wrapMode: Text.WordWrap
            }
        }

        // Info section
        Rectangle {
            width: parent.width
            height: infoColumn.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt

            Column {
                id: infoColumn
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4

                RowLayout {
                    width: parent.width
                    spacing: 8

                    Label {
                        text: qsTr("Volume:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    Label {
                        // V = 2 * π² * R * r²
                        property real volume: 2 * Math.PI * Math.PI * root.majorRadius * root.minorRadius * root.minorRadius
                        text: volume.toFixed(3)
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8

                    Label {
                        text: qsTr("Surface Area:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    Label {
                        // A = 4 * π² * R * r
                        property real area: 4 * Math.PI * Math.PI * root.majorRadius * root.minorRadius
                        text: area.toFixed(3)
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                RowLayout {
                    width: parent.width
                    spacing: 8

                    Label {
                        text: qsTr("Outer Diameter:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    Label {
                        text: ((root.majorRadius + root.minorRadius) * 2).toFixed(3)
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }
            }
        }
    }
}
