/**
 * @file GeoQueryPage.qml
 * @brief Function page for querying geometry entity information
 *
 * Allows user to select entities from the viewport using the Selector control
 * and displays detailed information about the selected entities through
 * the query_entity backend action.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../util"
import OpenGeoLab 1.0

FunctionPageBase {
    id: root

    pageTitle: qsTr("Query")
    pageIcon: "qrc:/opengeolab/resources/icons/query.svg"
    serviceName: "GeometryService"
    actionId: "query_entity"

    width: 360

    // ===============================
    // Parameters
    // ===============================

    /// Currently selected entities for query
    property var selectedEntities: []

    function getParameters() {
        // Build entity_ids array from selected entities
        let entityIds = [];
        for (let i = 0; i < selectedEntities.length; i++) {
            entityIds.push(selectedEntities[i].uid);
        }

        return {
            "action": actionId,
            "entity_ids": entityIds
        };
    }

    // ===============================
    // UI Content
    // ===============================

    Column {
        width: parent.width
        spacing: 12

        // Entity Selector
        Selector {
            id: picker
            width: parent.width

            onSelectionChanged: entities => {
                root.selectedEntities = entities;
            }
        }

        // Divider
        Rectangle {
            width: parent.width
            height: 1
            color: Theme.border
            visible: root.selectedEntities.length > 0
        }

        // Query Results Section
        Label {
            visible: queryResults.count > 0
            text: qsTr("Query Results")
            font.pixelSize: 11
            font.bold: true
            color: Theme.textSecondary
        }

        // Results list
        ListView {
            id: queryResults
            width: parent.width
            height: Math.min(contentHeight, 300)
            clip: true
            spacing: 8

            model: ListModel {
                id: resultsModel
            }

            delegate: Rectangle {
                required property var model
                required property int index

                width: queryResults.width
                height: resultColumn.implicitHeight + 16
                radius: 6
                color: Theme.surfaceAlt
                border.width: 1
                border.color: Theme.border

                Column {
                    id: resultColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    // Header with type and ID
                    RowLayout {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: typeLabel.implicitWidth + 12
                            Layout.preferredHeight: 20
                            radius: 3
                            color: Theme.accent

                            Label {
                                id: typeLabel
                                anchors.centerIn: parent
                                text: model.entityType || "Unknown"
                                font.pixelSize: 10
                                font.bold: true
                                color: Theme.white
                            }
                        }

                        Label {
                            text: "ID: " + (model.entityId || "N/A")
                            font.pixelSize: 11
                            font.family: "Consolas, monospace"
                            color: Theme.textPrimary
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "UID: " + (model.entityUid || "N/A")
                            font.pixelSize: 10
                            font.family: "Consolas, monospace"
                            color: Theme.textSecondary
                        }
                    }

                    // Entity name
                    Label {
                        visible: model.entityName && model.entityName.length > 0
                        text: model.entityName || ""
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        width: parent.width
                    }

                    // Owner info
                    RowLayout {
                        visible: model.ownerPartName && model.ownerPartName.length > 0
                        width: parent.width
                        spacing: 4

                        Label {
                            text: qsTr("Part:")
                            font.pixelSize: 10
                            color: Theme.textSecondary
                        }

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 2
                            color: model.partColor || Theme.accent
                        }

                        Label {
                            text: model.ownerPartName || ""
                            font.pixelSize: 10
                            color: Theme.textPrimary
                        }
                    }

                    // Bounding box info
                    GridLayout {
                        visible: model.hasBBox
                        width: parent.width
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 2

                        Label {
                            text: qsTr("Center:")
                            font.pixelSize: 10
                            color: Theme.textSecondary
                        }

                        Label {
                            text: model.center || ""
                            font.pixelSize: 10
                            font.family: "Consolas, monospace"
                            color: Theme.textPrimary
                        }

                        Label {
                            text: qsTr("Size:")
                            font.pixelSize: 10
                            color: Theme.textSecondary
                        }

                        Label {
                            text: model.size || ""
                            font.pixelSize: 10
                            font.family: "Consolas, monospace"
                            color: Theme.textPrimary
                        }
                    }
                }
            }

            // Empty state
            Label {
                visible: queryResults.count === 0 && root.selectedEntities.length > 0
                anchors.centerIn: parent
                text: qsTr("Click 'Execute' to query selected entities")
                font.pixelSize: 11
                color: Theme.textSecondary
            }
        }

        // Hint when nothing selected
        Rectangle {
            visible: root.selectedEntities.length === 0 && queryResults.count === 0
            width: parent.width
            height: hintText.implicitHeight + 16
            radius: 4
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1)
            border.width: 1
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.3)

            Label {
                id: hintText
                anchors.fill: parent
                anchors.margins: 8
                text: qsTr("💡 Select entities using the buttons above, then click Execute to view their properties.")
                font.pixelSize: 11
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }
    }

    // ===============================
    // Backend Connection
    // ===============================

    Connections {
        target: BackendService
        enabled: root.visible

        function onOperationFinished(moduleName, actionName, result) {
            if (actionName !== root.actionId)
                return;

            console.log("[GeoQueryPage] Query completed");

            try {
                let response = JSON.parse(result);
                resultsModel.clear();

                if (response.success) {
                    // Handle batch query
                    if (response.entities && Array.isArray(response.entities)) {
                        for (let entity of response.entities) {
                            addEntityToResults(entity);
                        }
                    } else
                    // Handle single entity query
                    if (response.entity) {
                        addEntityToResults(response.entity);
                    }
                }
            } catch (e) {
                console.error("[GeoQueryPage] Failed to parse result:", e);
            }
        }

        function onOperationFailed(moduleName, actionName, error) {
            if (actionName !== root.actionId)
                return;
            console.warn("[GeoQueryPage] Query failed:", error);
        }
    }

    /// Helper function to add entity info to results model
    function addEntityToResults(entity) {
        let centerStr = "";
        let sizeStr = "";
        let hasBBox = false;

        if (entity.center && entity.center.length === 3) {
            centerStr = "(" + entity.center[0].toFixed(2) + ", " + entity.center[1].toFixed(2) + ", " + entity.center[2].toFixed(2) + ")";
            hasBBox = true;
        }

        if (entity.size && entity.size.length === 3) {
            sizeStr = entity.size[0].toFixed(2) + " × " + entity.size[1].toFixed(2) + " × " + entity.size[2].toFixed(2);
        }

        resultsModel.append({
            entityId: entity.id || 0,
            entityUid: entity.uid || 0,
            entityType: entity.type || "Unknown",
            entityName: entity.name || "",
            ownerPartId: entity.owning_part_id || 0,
            ownerPartName: entity.owning_part_name || "",
            partColor: entity.part_color || "",
            center: centerStr,
            size: sizeStr,
            hasBBox: hasBBox
        });
    }
}
