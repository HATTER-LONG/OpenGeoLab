/**
 * @file AddBoxPage.qml
 * @brief Function page for creating a box
 *
 * Allows user to input origin and dimensions to create a box geometry.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Create Box")
    pageIcon: "qrc:/opengeolab/resources/icons/box.svg"
    serviceName: "GeometryService"
    actionId: "addBox"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Origin coordinates
    property real originX: 0.0
    property real originY: 0.0
    property real originZ: 0.0

    /// Box dimensions
    property real dimX: 10.0
    property real dimY: 10.0
    property real dimZ: 10.0

    /// Box name
    property string boxName: ""

    function getParameters(): var {
        return {
            "action": "createBox",
            "name": boxName || "Box_" + Date.now(),
            "origin": {
                "x": originX,
                "y": originY,
                "z": originZ
            },
            "dimensions": {
                "x": dimX,
                "y": dimY,
                "z": dimZ
            }
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        ParamField {
            label: qsTr("Box Name")
            placeholder: qsTr("Auto-generated if empty")
            value: root.boxName
            onValueEdited: newValue => root.boxName = newValue
        }

        CoordinateField {
            label: qsTr("Origin Point")
            coordX: root.originX
            coordY: root.originY
            coordZ: root.originZ
            onCoordinateChanged: (x, y, z) => {
                root.originX = x;
                root.originY = y;
                root.originZ = z;
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
                label: "W"
                value: root.dimX
                accentColor: "#E53935"
                onValueEdited: newVal => root.dimX = newVal
            }

            DimensionInput {
                Layout.fillWidth: true
                label: "H"
                value: root.dimY
                accentColor: "#43A047"
                onValueEdited: newVal => root.dimY = newVal
            }

            DimensionInput {
                Layout.fillWidth: true
                label: "D"
                value: root.dimZ
                accentColor: "#1E88E5"
                onValueEdited: newVal => root.dimZ = newVal
            }
        }

        // Volume info
        Rectangle {
            width: parent.width
            height: volumeRow.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt

            RowLayout {
                id: volumeRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Label {
                    text: qsTr("Volume:")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Label {
                    property real volume: root.dimX * root.dimY * root.dimZ
                    text: volume.toFixed(3)
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
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
