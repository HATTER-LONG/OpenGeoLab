/**
 * @file MeshQueryPage.qml
 * @brief Mesh query page for inspecting mesh node and element properties
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Mesh Query")
    pageIcon: "qrc:/opengeolab/resources/icons/query.svg"
    serviceName: "MeshService"
    actionId: "meshQuery"

    executeButtonText: qsTr("Query")

    width: 380

    /// Query results model returned by backend action `query_mesh_entity_info`
    property var queryResults: []
    /// Last query error message (if any)
    property string queryError: ""

    function getParameters() {
        return {
            action: "query_mesh_entity_info",
            entities: picker.currentSelectedEntities()
        };
    }

    Connections {
        target: BackendService
        enabled: root.visible
        function onOperationFinished(moduleName, actionName, result) {
            if (moduleName !== "MeshService")
                return;
            if (actionName !== "query_mesh_entity_info")
                return;
            try {
                const data = JSON.parse(result);
                root.queryResults = data.entities || [];
                root.queryError = "";
            } catch (e) {
                root.queryResults = [];
                root.queryError = "Failed to parse query result: " + e;
            }
        }
        function onOperationFailed(moduleName, actionName, error) {
            if (moduleName !== "MeshService")
                return;
            if (actionName !== "query_mesh_entity_info")
                return;
            root.queryResults = [];
            root.queryError = error || qsTr("Query failed");
        }
    }

    Column {
        width: parent.width
        spacing: 12

        // Entity Selector - mesh node and element types only
        Selector {
            id: picker
            width: parent.width
            visible: true

            // Allow picking: mesh nodes and mesh elements
            allowedTypes: mesh_node_type | mesh_element_type
        }

        // Result header
        Label {
            text: qsTr("Query Result")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
            visible: true
        }

        Rectangle {
            visible: root.queryError.length > 0
            width: parent.width
            height: errorText.implicitHeight + 12
            radius: 4
            color: Theme.surfaceAlt

            Label {
                id: errorText
                anchors.fill: parent
                anchors.margins: 6
                text: root.queryError
                font.pixelSize: 10
                color: Theme.danger
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            width: parent.width
            height: 200
            radius: 4
            color: Theme.surfaceAlt
            clip: true

            ListView {
                id: resultList
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                spacing: 6
                model: root.queryResults

                delegate: Rectangle {
                    id: row
                    required property var modelData
                    width: resultList.width
                    radius: 4
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.border
                    implicitHeight: contentColumn.implicitHeight + 12
                    Column {
                        id: contentColumn
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        Label {
                            text: {
                                if (row.modelData.type === "MeshNode") {
                                    return "MeshNode: " + (row.modelData.nodeId || "");
                                } else if (row.modelData.type === "MeshElement") {
                                    return row.modelData.elementType + ": " + (row.modelData.elementUID || "");
                                }
                                return row.modelData.type || "";
                            }
                            font.pixelSize: 11
                            font.bold: true
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }

                        ScrollView {
                            width: parent.width
                            height: 150
                            clip: true

                            TextArea {
                                readOnly: true
                                wrapMode: TextArea.Wrap
                                text: JSON.stringify(row.modelData, null, 2)
                                font.pixelSize: 9
                                font.family: "Consolas, monospace"
                                color: Theme.textSecondary
                                background: null
                            }
                        }
                    }
                }

                footer: Item {
                    width: resultList.width
                    height: (root.queryResults.length === 0 && root.queryError.length === 0) ? emptyHint.implicitHeight + 12 : 0
                    visible: root.queryResults.length === 0 && root.queryError.length === 0

                    Label {
                        id: emptyHint
                        anchors.centerIn: parent
                        text: qsTr("No results. Select mesh entities and click Query.")
                        font.pixelSize: 10
                        color: Theme.textSecondary
                    }
                }
            }
        }
    }
}
