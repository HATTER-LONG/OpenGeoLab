/**
 * @file AddCylinderPage.qml
 * @brief Function page for creating a cylinder
 *
 * Allows user to input center position, radius, and height to create
 * a cylinder geometry. The cylinder axis is along the Z direction.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Cylinder")
    pageIcon: "qrc:/opengeolab/resources/icons/cylinder.svg"
    serviceName: "GeometryService"
    actionId: "addCylinder"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Center coordinates (base center)
    property real centerX: 0.0
    property real centerY: 0.0
    property real centerZ: 0.0

    /// Cylinder radius
    property real radius: 5.0

    /// Cylinder height
    property real cylinder_height: 10.0

    /// Cylinder name
    property string cylinderName: ""

    function getParameters() {
        return {
            "action": "create",
            "type": "cylinder",
            "name": cylinderName || "Cylinder_" + Date.now(),
            "x": centerX,
            "y": centerY,
            "z": centerZ,
            "radius": radius,
            "height": cylinder_height
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        ParamField {
            label: qsTr("Cylinder Name")
            placeholder: qsTr("Auto-generated if empty")
            value: root.cylinderName
            onValueEdited: newValue => root.cylinderName = newValue
        }

        CoordinateField {
            label: qsTr("Base Center")
            coordX: root.centerX
            coordY: root.centerY
            coordZ: root.centerZ
            onCoordinateChanged: (x, y, z) => {
                root.centerX = x;
                root.centerY = y;
                root.centerZ = z;
            }
        }

        // Dimensions section
        Label {
            text: qsTr("Dimensions")
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        RowLayout {
            width: parent.width
            spacing: 6

            DimensionInput {
                Layout.fillWidth: true
                label: "R"
                value: root.radius
                accentColor: "#E53935"
                onValueEdited: newVal => root.radius = newVal
            }

            DimensionInput {
                Layout.fillWidth: true
                label: "H"
                value: root.cylinder_height
                accentColor: "#1E88E5"
                onValueEdited: newVal => root.cylinder_height = newVal
            }
        }

        // Volume info
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
                        property real volume: Math.PI * root.radius * root.radius * root.cylinder_height
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
                        property real area: 2 * Math.PI * root.radius * (root.radius + root.cylinder_height)
                        text: area.toFixed(3)
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }
            }
        }
    }

    // =========================================================
    // Internal dimension input component
    // =========================================================
    component DimensionInput: Item {
        id: dimItem

        property string label: ""
        property real value: 0.0
        property int decimals: 3
        property color accentColor: Theme.accent

        signal valueEdited(real newVal)

        implicitHeight: 28

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: Theme.surface
            border.width: dimField.activeFocus ? 2 : 1
            border.color: dimField.activeFocus ? dimItem.accentColor : Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 2

                Rectangle {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    radius: 3
                    color: Qt.rgba(dimItem.accentColor.r, dimItem.accentColor.g, dimItem.accentColor.b, 0.2)

                    Label {
                        anchors.centerIn: parent
                        text: dimItem.label
                        font.pixelSize: 10
                        font.bold: true
                        color: dimItem.accentColor
                    }
                }

                TextField {
                    id: dimField
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    text: dimItem.value.toFixed(dimItem.decimals)
                    selectByMouse: true

                    font.pixelSize: 11
                    color: Theme.textPrimary

                    background: Item {}

                    validator: DoubleValidator {
                        bottom: 0.001
                        decimals: dimItem.decimals
                        notation: DoubleValidator.StandardNotation
                    }

                    onEditingFinished: {
                        const newVal = Math.max(0.001, parseFloat(text) || 0);
                        if (newVal !== dimItem.value) {
                            dimItem.valueEdited(newVal);
                        }
                    }
                }
            }
        }
    }
}
