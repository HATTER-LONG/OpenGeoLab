/**
 * @file GenerateMeshPage.qml
 * @brief Function page for mesh generation
 *
 * Allows user to configure mesh generation parameters including
 * element type (triangle/quad), mesh dimension (2D/3D), and element size.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Generate Mesh")
    pageIcon: "qrc:/opengeolab/resources/icons/mesh.svg"
    serviceName: "MeshService"
    actionId: "generateMesh"

    width: 340

    // =========================================================
    // Parameters
    // =========================================================

    /// Target geometry entity handles: [{uid, type}, ...]
    property var targetEntities: []
    /// Global mesh element size
    property real elementSize: 1.0
    /// Mesh dimension: 2 = surface, 3 = volume
    property int meshDimension: 2
    /// Element type: "triangle", "quad", "auto"
    property string elementType: "triangle"

    /// Result list from backend
    property var meshEntities: []

    function getParameters() {
        return {
            "action": "generate_mesh",
            "entities": targetEntities,
            "elementSize": elementSize,
            "meshDimension": meshDimension,
            "elementType": elementType
        };
    }

    // =========================================================
    // Content
    // =========================================================

    Column {
        width: parent.width
        spacing: 12

        Selector {
            id: selector
            width: parent.width
            allowedTypes: face_type | solid_type | part_type
            onSelectedEntitiesUpdated: function (entities) {
                root.targetEntities = entities;
            }
        }

        // Target selection info
        Rectangle {
            width: parent.width
            height: targetRow.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            RowLayout {
                id: targetRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Label {
                    text: qsTr("Targets")
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textSecondary
                }

                Label {
                    text: root.targetEntities.length > 0 ? qsTr("%1 entities selected").arg(root.targetEntities.length) : qsTr("Select face/solid/part")
                    font.pixelSize: 11
                    color: root.targetEntities.length > 0 ? Theme.textPrimary : Theme.textDisabled
                    Layout.fillWidth: true
                }
            }
        }

        // Mesh dimension selector
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Mesh Dimension")
                font.pixelSize: 11
                font.bold: true
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 6

                BaseButton {
                    Layout.fillWidth: true
                    text: qsTr("2D Surface")
                    highlighted: root.meshDimension === 2
                    onClicked: root.meshDimension = 2
                }

                BaseButton {
                    Layout.fillWidth: true
                    text: qsTr("3D Volume")
                    highlighted: root.meshDimension === 3
                    onClicked: root.meshDimension = 3
                }
            }
        }

        // Element type selector
        Column {
            width: parent.width
            spacing: 4

            Label {
                text: qsTr("Element Type")
                font.pixelSize: 11
                font.bold: true
                color: Theme.textSecondary
            }

            RowLayout {
                width: parent.width
                spacing: 6

                BaseButton {
                    Layout.fillWidth: true
                    text: qsTr("Triangle")
                    highlighted: root.elementType === "triangle"
                    onClicked: root.elementType = "triangle"
                }

                BaseButton {
                    Layout.fillWidth: true
                    text: qsTr("Quad")
                    highlighted: root.elementType === "quad"
                    onClicked: root.elementType = "quad"
                }

                BaseButton {
                    Layout.fillWidth: true
                    text: qsTr("Auto")
                    highlighted: root.elementType === "auto"
                    onClicked: root.elementType = "auto"
                }
            }
        }

        // Element size
        Column {
            width: parent.width
            spacing: 4

            RowLayout {
                width: parent.width

                Label {
                    text: qsTr("Element Size")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: root.elementSize.toFixed(2)
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                }
            }

            Slider {
                id: sizeSlider
                width: parent.width
                from: 0.1
                to: 10.0
                value: root.elementSize
                stepSize: 0.1

                onValueChanged: root.elementSize = value

                background: Rectangle {
                    x: sizeSlider.leftPadding
                    y: sizeSlider.topPadding + sizeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: sizeSlider.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: Theme.border

                    Rectangle {
                        width: sizeSlider.visualPosition * parent.width
                        height: parent.height
                        color: Theme.accent
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: sizeSlider.leftPadding + sizeSlider.visualPosition * (sizeSlider.availableWidth - width)
                    y: sizeSlider.topPadding + sizeSlider.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: sizeSlider.pressed ? Qt.darker(Theme.accent, 1.2) : Theme.accent
                    border.width: 2
                    border.color: Theme.white
                }
            }
        }

        // Result list
        Column {
            width: parent.width
            spacing: 6

            Label {
                text: qsTr("Mesh Results")
                font.pixelSize: 11
                font.bold: true
                color: Theme.textSecondary
            }

            Rectangle {
                width: parent.width
                height: Math.min(160, Math.max(44, resultList.contentHeight + 8))
                radius: 4
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.border

                ListView {
                    id: resultList
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    model: root.meshEntities

                    delegate: Rectangle {
                        id: meshItem
                        width: ListView.view.width
                        height: 36
                        radius: 4
                        color: Theme.surface
                        required property var modelData
                        required property int index
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8

                            Label {
                                text: meshItem.modelData.type
                                font.pixelSize: 11
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            Label {
                                text: qsTr("id=%1").arg(meshItem.modelData.id)
                                font.pixelSize: 11
                                color: Theme.textSecondary
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Label {
                                text: qsTr("nodes=%1, elems=%2").arg(meshItem.modelData.stats.node_count).arg(meshItem.modelData.stats.element_count)
                                font.pixelSize: 11
                                color: Theme.textSecondary
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: root.meshEntities.length === 0
                        text: qsTr("No mesh yet")
                        font.pixelSize: 11
                        color: Theme.textDisabled
                    }
                }
            }
        }
    }

    Connections {
        target: BackendService
        function onOperationFinished(module_name, action_name, result) {
            if (module_name !== root.serviceName || action_name !== root.actionId)
                return;
            try {
                const obj = JSON.parse(result);
                root.meshEntities = obj.mesh_entities ? obj.mesh_entities : [];
            } catch (e) {
                root.meshEntities = [];
            }
        }
        function onOperationFailed(module_name, action_name, error) {
            if (module_name !== root.serviceName || action_name !== root.actionId)
                return;
            root.meshEntities = [];
        }
    }
}
