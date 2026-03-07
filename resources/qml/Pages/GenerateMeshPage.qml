/**
 * @file GenerateMeshPage.qml
 * @brief Function page for mesh generation
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
    executeButtonText: qsTr("Generate")

    width: 420

    readonly property string backendActionName: "generate_mesh"
    readonly property var algorithm2DModel: [
        {
            "text": qsTr("Frontal"),
            "value": "frontal"
        },
        {
            "text": qsTr("Delaunay"),
            "value": "delaunay"
        },
        {
            "text": qsTr("MeshAdapt"),
            "value": "meshadapt"
        },
        {
            "text": qsTr("BAMG"),
            "value": "bamg"
        },
        {
            "text": qsTr("Automatic"),
            "value": "automatic"
        },
        {
            "text": qsTr("Frontal Quad"),
            "value": "frontal_quad"
        }
    ]
    readonly property var algorithm3DModel: [
        {
            "text": qsTr("Delaunay"),
            "value": "delaunay"
        },
        {
            "text": qsTr("Frontal"),
            "value": "frontal"
        },
        {
            "text": qsTr("MMG3D"),
            "value": "mmg3d"
        },
        {
            "text": qsTr("R-tree"),
            "value": "rtree"
        },
        {
            "text": qsTr("HXT"),
            "value": "hxt"
        }
    ]

    property var targetEntities: []
    property real elementSize: 1.0
    property real elementSizeMin: 1.0
    property real elementSizeMax: 2.0
    property int meshDimension: 2
    property string elementType: "triangle"
    property string algorithm2D: "frontal"
    property string algorithm3D: "delaunay"
    property int elementOrder: 1
    property bool optimizeMesh: true
    property var meshEntities: []
    property var meshSummary: ({})

    function getParameters() {
        return {
            "action": backendActionName,
            "entities": targetEntities,
            "elementSize": elementSize,
            "elementSizeMin": elementSizeMin,
            "elementSizeMax": elementSizeMax,
            "meshDimension": meshDimension,
            "elementType": elementType,
            "algorithm2D": algorithm2D,
            "algorithm3D": algorithm3D,
            "elementOrder": elementOrder,
            "optimizeMesh": optimizeMesh
        };
    }

    function syncSizeRangeFromBase() {
        elementSizeMin = Number(Math.max(0.01, elementSize).toFixed(2));
        elementSizeMax = Number(Math.max(elementSizeMin, elementSize * 2.0).toFixed(2));
    }

    function requestMeshPreview() {
        BackendService.request("RenderService", JSON.stringify({
            "action": "ViewPortControl",
            "view_ctrl": {
                "mesh_display": {
                    "surface": false,
                    "wireframe": true,
                    "points": true
                },
                "fit": true
            }
        }));
    }

    Component.onCompleted: syncSizeRangeFromBase()

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
                    text: qsTr("Generate Mesh")
                    font.pixelSize: 15
                    font.bold: true
                    color: Theme.textPrimary
                }

                Label {
                    text: qsTr("Select source geometry, configure the meshing strategy, and preview the generated mesh immediately in the viewport.")
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
            allowedTypes: face_type | solid_type | part_type
            onSelectedEntitiesUpdated: function (entities) {
                root.targetEntities = entities;
            }
        }

        Rectangle {
            width: parent.width
            height: targetRow.implicitHeight + 14
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            RowLayout {
                id: targetRow
                anchors.fill: parent
                anchors.margins: 7
                spacing: 8

                Label {
                    text: qsTr("Selection")
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textSecondary
                }

                Label {
                    text: root.targetEntities.length > 0 ? qsTr("%1 entities selected").arg(root.targetEntities.length) : qsTr("Select face, solid, or part")
                    font.pixelSize: 11
                    color: root.targetEntities.length > 0 ? Theme.textPrimary : Theme.textDisabled
                    Layout.fillWidth: true
                }
            }
        }

        Label {
            text: qsTr("Meshing Strategy")
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
                        text: qsTr("Dimension")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    ComboBox {
                        width: parent.width
                        model: [qsTr("2D Surface"), qsTr("3D Volume")]
                        currentIndex: root.meshDimension === 3 ? 1 : 0
                        onActivated: root.meshDimension = currentIndex === 1 ? 3 : 2
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: qsTr("Element Type")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    ComboBox {
                        width: parent.width
                        model: [qsTr("Triangle"), qsTr("Quad"), qsTr("Auto")]
                        currentIndex: root.elementType === "quad" ? 1 : root.elementType === "auto" ? 2 : 0
                        onActivated: root.elementType = currentIndex === 1 ? "quad" : currentIndex === 2 ? "auto" : "triangle"
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: qsTr("2D Algorithm")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    ComboBox {
                        width: parent.width
                        model: root.algorithm2DModel
                        textRole: "text"
                        Component.onCompleted: {
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].value === root.algorithm2D) {
                                    currentIndex = i;
                                    break;
                                }
                            }
                        }
                        onActivated: root.algorithm2D = model[currentIndex].value
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: qsTr("3D Algorithm")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    ComboBox {
                        width: parent.width
                        enabled: root.meshDimension === 3
                        model: root.algorithm3DModel
                        textRole: "text"
                        Component.onCompleted: {
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].value === root.algorithm3D) {
                                    currentIndex = i;
                                    break;
                                }
                            }
                        }
                        onActivated: root.algorithm3D = model[currentIndex].value
                    }
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: qsTr("Element Order")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    ComboBox {
                        width: parent.width
                        model: [qsTr("Linear (1)"), qsTr("Quadratic (2)")]
                        currentIndex: root.elementOrder === 2 ? 1 : 0
                        onActivated: root.elementOrder = currentIndex === 1 ? 2 : 1
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                    height: optimizeRow.implicitHeight + 12
                    radius: 6
                    color: Theme.surface

                    RowLayout {
                        id: optimizeRow
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 8

                        CheckBox {
                            checked: root.optimizeMesh
                            onCheckedChanged: root.optimizeMesh = checked
                        }

                        Label {
                            text: qsTr("Optimize mesh after generation")
                            font.pixelSize: 11
                            color: Theme.textPrimary
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        Label {
            text: qsTr("Size Control")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
        }

        Rectangle {
            width: parent.width
            height: sizeColumn.implicitHeight + 20
            radius: 8
            color: Theme.surfaceAlt
            border.width: 1
            border.color: Theme.border

            Column {
                id: sizeColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                RowLayout {
                    width: parent.width
                    Label {
                        text: qsTr("Base Size")
                        font.pixelSize: 11
                        color: Theme.textSecondary
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Label {
                        text: root.elementSize.toFixed(2)
                        font.pixelSize: 13
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                Slider {
                    width: parent.width
                    from: 0.05
                    to: 10.0
                    value: root.elementSize
                    stepSize: 0.05
                    onMoved: {
                        root.elementSize = value;
                        root.syncSizeRangeFromBase();
                    }
                }

                GridLayout {
                    width: parent.width
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 8

                    Column {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: qsTr("Min Size")
                            font.pixelSize: 11
                            color: Theme.textSecondary
                        }
                        SpinBox {
                            width: parent.width
                            from: 1
                            to: 1000
                            value: Math.round(root.elementSizeMin * 100)
                            stepSize: 5
                            editable: true
                            textFromValue: function (v) {
                                return (v / 100.0).toFixed(2);
                            }
                            valueFromText: function (text) {
                                return Math.round(Number(text) * 100);
                            }
                            onValueModified: {
                                root.elementSizeMin = value / 100.0;
                                if (root.elementSizeMax < root.elementSizeMin)
                                    root.elementSizeMax = root.elementSizeMin;
                            }
                        }
                    }

                    Column {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: qsTr("Max Size")
                            font.pixelSize: 11
                            color: Theme.textSecondary
                        }
                        SpinBox {
                            width: parent.width
                            from: 1
                            to: 2000
                            value: Math.round(root.elementSizeMax * 100)
                            stepSize: 5
                            editable: true
                            textFromValue: function (v) {
                                return (v / 100.0).toFixed(2);
                            }
                            valueFromText: function (text) {
                                return Math.round(Number(text) * 100);
                            }
                            onValueModified: root.elementSizeMax = Math.max(root.elementSizeMin, value / 100.0)
                        }
                    }
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
                    text: Object.keys(root.meshSummary).length === 0 ? qsTr("No mesh generated yet") : qsTr("Generated %1 nodes / %2 elements").arg(root.meshSummary.generatedNodeCount || 0).arg(root.meshSummary.generatedElementCount || 0)
                    font.pixelSize: 11
                    color: Object.keys(root.meshSummary).length === 0 ? Theme.textDisabled : Theme.textPrimary
                }

                Label {
                    visible: Object.keys(root.meshSummary).length > 0
                    text: qsTr("Document total: %1 nodes / %2 elements").arg(root.meshSummary.nodeCount || 0).arg(root.meshSummary.elementCount || 0)
                    font.pixelSize: 11
                    color: Theme.textSecondary
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
                if (obj.success) {
                    console.log("Mesh generation succeeded:", obj);
                    // root.requestMeshPreview();
                }
            } catch (e) {
                root.meshSummary = ({});
                root.meshEntities = [];
            }
        }
        function onOperationFailed(moduleName, actionName, error) {
            if (moduleName !== root.serviceName || actionName !== root.backendActionName)
                return;
            root.meshSummary = {
                "error": error || qsTr("Mesh generation failed")
            };
            root.meshEntities = [];
        }
    }
}
