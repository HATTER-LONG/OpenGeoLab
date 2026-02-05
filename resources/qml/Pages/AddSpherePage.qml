/**
 * @file AddSpherePage.qml
 * @brief Function page for creating a sphere
 *
 * Allows user to input center coordinates and radius to create
 * a sphere geometry.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Sphere")
    pageIcon: "qrc:/opengeolab/resources/icons/sphere.svg"
    serviceName: "GeometryService"
    actionId: "addSphere"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Center coordinates
    property real centerX: 0.0
    property real centerY: 0.0
    property real centerZ: 0.0

    /// Sphere radius
    property real radius: 5.0

    /// Sphere name
    property string sphereName: ""

    function getParameters() {
        return {
            "action": "create",
            "type": "sphere",
            "name": sphereName || "Sphere_" + Date.now(),
            "x": centerX,
            "y": centerY,
            "z": centerZ,
            "radius": radius
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        ParamField {
            label: qsTr("Sphere Name")
            placeholder: qsTr("Auto-generated if empty")
            value: root.sphereName
            onValueEdited: newValue => root.sphereName = newValue
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

        // Radius section
        Label {
            text: qsTr("Radius")
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        DimensionInput {
            width: parent.width
            label: "R"
            value: root.radius
            accentColor: "#E53935"
            onValueEdited: newVal => root.radius = newVal
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
                        property real volume: (4.0 / 3.0) * Math.PI * Math.pow(root.radius, 3)
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
                        property real area: 4 * Math.PI * root.radius * root.radius
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
                        text: qsTr("Diameter:")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    Label {
                        text: (root.radius * 2).toFixed(3)
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }
            }
        }
    }
}
