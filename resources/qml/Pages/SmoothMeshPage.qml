/**
 * @file SmoothMeshPage.qml
 * @brief Function page for mesh smoothing.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Smooth Mesh")
    pageIcon: "qrc:/opengeolab/resources/icons/smooth_mesh.svg"
    serviceName: "MeshService"
    actionId: "smoothMesh"
    executeButtonText: qsTr("Smooth")

    width: 420

    readonly property string backendActionName: "smooth_mesh"

    property var targetEntities: []
    property int iterations: 6
    property real factor: 0.35
    property string method: "laplacian"
    property bool preserveBoundaries: true
    property var meshEntities: []
    property var meshSummary: ({})

    function getParameters() {
        return {
            "action": backendActionName,
            "entities": targetEntities,
            "iterations": iterations,
            "factor": factor,
            "method": method,
            "preserveBoundaries": preserveBoundaries
        };
    }

    function requestMeshPreview() {
        BackendService.request("RenderService", JSON.stringify({
            "action": "ViewPortControl",
            "view_ctrl": {
                "mesh_display": {
                    "surface": true,
                    "wireframe": true,
                    "points": false
                },
                "fit": true
            }
        }));
    }

    Column {
        width: parent.width
        spacing: 14

        Rectangle {
            width: parent.width
            height: heroColumn.implicitHeight + 20
            radius: 10
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Column {
                id: heroColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Label {
                    text: qsTr("Smooth Mesh Region")
                    font.pixelSize: 15
                    font.bold: true
                    color: Theme.textPrimary
                }

                Label {
                    text: qsTr("Select nodes, lines, elements, or a Part, then relax the mesh with a focused parameter set that fits smaller side panels cleanly.")
                    font.pixelSize: 11
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            text: qsTr("Targets")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
        }

        Selector {
            width: parent.width
            allowedTypes: mesh_node_type | mesh_line_type | mesh_element_type | part_type
            onSelectedEntitiesUpdated: function (entities) {
                root.targetEntities = entities;
            }
        }

        Rectangle {
            width: parent.width
            height: targetInfo.implicitHeight + 14
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Column {
                id: targetInfo
                anchors.fill: parent
                anchors.margins: 7
                spacing: 4

                Label {
                    text: root.targetEntities.length > 0 ? qsTr("%1 targets selected for smoothing").arg(root.targetEntities.length) : qsTr("Select mesh nodes, lines, elements, or a Part")
                    font.pixelSize: 11
                    color: root.targetEntities.length > 0 ? Theme.textPrimary : Theme.textDisabled
                }

                Label {
                    text: qsTr("Node picks smooth a local neighborhood, while line, element, and Part picks expand to their connected region automatically.")
                    font.pixelSize: 10
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            text: qsTr("Smoothing Strategy")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
        }

        Rectangle {
            width: parent.width
            height: strategyGrid.implicitHeight + 20
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            GridLayout {
                id: strategyGrid
                anchors.fill: parent
                anchors.margins: 10
                columns: 2
                rowSpacing: 10
                columnSpacing: 10

                Column {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: qsTr("Method")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    ComboBox {
                        width: parent.width
                        model: [qsTr("Laplacian"), qsTr("Taubin")]
                        currentIndex: root.method === "taubin" ? 1 : 0
                        onActivated: root.method = currentIndex === 1 ? "taubin" : "laplacian"
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: qsTr("Iterations")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    SpinBox {
                        width: parent.width
                        from: 1
                        to: 100
                        value: root.iterations
                        editable: true
                        onValueModified: root.iterations = value
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: qsTr("Smoothing Factor")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }

                    Slider {
                        width: parent.width
                        from: 0.05
                        to: 1.0
                        stepSize: 0.05
                        value: root.factor
                        onMoved: root.factor = Number(value.toFixed(2))
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                    height: factorInfo.implicitHeight + 12
                    radius: 6
                    color: Theme.surface

                    RowLayout {
                        id: factorInfo
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        Label {
                            text: qsTr("Current factor")
                            font.pixelSize: 11
                            color: Theme.textSecondary
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: root.factor.toFixed(2)
                            font.pixelSize: 12
                            font.bold: true
                            color: Theme.textPrimary
                        }
                    }
                }
            }
        }

        Label {
            text: qsTr("Boundary Control")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
        }

        Rectangle {
            width: parent.width
            height: boundaryColumn.implicitHeight + 20
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Column {
                id: boundaryColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                RowLayout {
                    width: parent.width
                    spacing: 8

                    CheckBox {
                        checked: root.preserveBoundaries
                        onCheckedChanged: root.preserveBoundaries = checked
                    }

                    Label {
                        text: qsTr("Preserve open boundaries and sharp feature borders")
                        font.pixelSize: 11
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }

                Label {
                    text: root.method === "taubin" ? qsTr("Taubin smoothing alternates positive and negative Laplacian passes to reduce volume shrinkage.") : qsTr("Laplacian smoothing moves each unlocked node toward the average of its adjacent nodes.")
                    font.pixelSize: 10
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            text: qsTr("Last Result")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
        }

        Rectangle {
            width: parent.width
            height: resultColumn.implicitHeight + 12
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Column {
                id: resultColumn
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                Label {
                    visible: !!root.meshSummary.error
                    text: root.meshSummary.error || ""
                    font.pixelSize: 11
                    color: Theme.danger
                    wrapMode: Text.WordWrap
                }

                Label {
                    text: Object.keys(root.meshSummary).length === 0 ? qsTr("No smoothing result yet") : qsTr("Smoothed %1 nodes").arg(root.meshSummary.smoothedNodeCount || 0)
                    font.pixelSize: 11
                    color: Object.keys(root.meshSummary).length === 0 ? Theme.textDisabled : Theme.textPrimary
                }

                Label {
                    visible: Object.keys(root.meshSummary).length > 0
                    text: qsTr("Targets: %1 nodes, locked boundary nodes: %2, max displacement: %3").arg(root.meshSummary.targetNodeCount || 0).arg(root.meshSummary.boundaryNodeCount || 0).arg(Number(root.meshSummary.maxDisplacement || 0).toFixed(4))
                    font.pixelSize: 11
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                }

                Label {
                    visible: Object.keys(root.meshSummary).length > 0
                    text: qsTr("Method: %1, iterations: %2, document total: %3 nodes / %4 elements").arg(root.meshSummary.method || "-").arg(root.meshSummary.iterations || 0).arg(root.meshSummary.nodeCount || 0).arg(root.meshSummary.elementCount || 0)
                    font.pixelSize: 11
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: root.meshEntities
                    delegate: RowLayout {
                        id: meshEntityRow
                        required property var modelData
                        width: parent.width
                        spacing: 8

                        Label {
                            text: meshEntityRow.modelData.type
                            font.pixelSize: 11
                            font.bold: true
                            color: Theme.textPrimary
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("count=%1").arg(meshEntityRow.modelData.count)
                            font.pixelSize: 11
                            color: Theme.textSecondary
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: BackendService
        enabled: root.visible

        function onOperationFinished(moduleName, actionName, result) {
            if (moduleName !== root.serviceName || actionName !== root.backendActionName)
                return;
            try {
                const obj = JSON.parse(result);
                root.meshSummary = obj;
                root.meshEntities = obj.mesh_entities || [];
                if (obj.success)
                    root.requestMeshPreview();
            } catch (e) {
                root.meshSummary = ({});
                root.meshEntities = [];
            }
        }

        function onOperationFailed(moduleName, actionName, error) {
            if (moduleName !== root.serviceName || actionName !== root.backendActionName)
                return;
            root.meshSummary = {
                "error": error || qsTr("Mesh smoothing failed")
            };
            root.meshEntities = [];
        }
    }
}
