/**
 * @file QueryPage.qml
 * @brief Query page for interactive entity information lookup
 *
 * Provides a floating panel where users can:
 * - Select entities via the viewport
 * - View detailed entity information
 * - Navigate entity hierarchy
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import ".."
import "../util" as Util

FunctionPageBase {
    id: root

    pageTitle: qsTr("Query Entity")
    pageIcon: "qrc:/opengeolab/resources/icons/query.svg"
    serviceName: "" // We don't send a request on execute
    actionId: "queryEntity"

    // Override default size
    width: 360
    height: 500

    // =========================================================
    // Properties
    // =========================================================

    /// Currently selected entity IDs for query
    property var selectedEntityIds: []

    /// Query results from backend
    property var queryResults: []

    /// Current selection mode
    property int currentSelectionMode: 3 // Face

    /// Cached viewport reference
    property var viewport: null

    // =========================================================
    // Selection Mode Options
    // =========================================================

    readonly property var selectionModes: [
        {
            name: qsTr("Vertex"),
            mode: 1,
            icon: "point.svg"
        },
        {
            name: qsTr("Edge"),
            mode: 2,
            icon: "line.svg"
        },
        {
            name: qsTr("Face"),
            mode: 3,
            icon: "face.svg"
        },
        {
            name: qsTr("Solid"),
            mode: 4,
            icon: "box.svg"
        },
        {
            name: qsTr("Part"),
            mode: 7,
            icon: "part.svg"
        },
        {
            name: qsTr("All"),
            mode: 8,
            icon: "all.svg"
        }
    ]

    // =========================================================
    // Lifecycle
    // =========================================================

    function open(payload) {
        // Clear previous state
        selectedEntityIds = [];
        queryResults = [];

        // Position the page
        _lastSidebarWidth = _getSidebarWidth();
        x = _clampX(_minXRightOfSidebar());

        pageVisible = true;
        root.forceActiveFocus();

        // Enable picking mode on viewport
        viewport = findViewport();
        if (viewport) {
            viewport.selectionMode = currentSelectionMode;
            viewport.pickingEnabled = true;
        }
    }

    function close() {
        pageVisible = false;

        // Disable picking mode
        viewport = findViewport();
        if (viewport) {
            viewport.pickingEnabled = false;
            viewport.selectionMode = 0;
        }

        // Viewport will clear preview/selection highlights when pickingEnabled is turned off.

        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function findViewport() {
        if (MainPages.mainWindow) {
            return MainPages.mainWindow.contentItem.children[0].children[1]; // GLViewport
        }
        return null;
    }

    // =========================================================
    // Query Execution
    // =========================================================

    function queryEntities(entityIds) {
        if (!entityIds || entityIds.length === 0) {
            queryResults = [];
            return;
        }

        BackendService.request("GeometryService", JSON.stringify({
            action: "query_entity",
            entity_ids: entityIds,
            _meta: {
                silent: true
            }
        }));
    }

    // =========================================================
    // Backend Response Handling
    // =========================================================

    Connections {
        target: BackendService
        function onOperationFinished(module, action, result) {
            if (module === "GeometryService" && action === "query_entity") {
                try {
                    let response = JSON.parse(result);
                    if (response.success && response.entities) {
                        root.queryResults = response.entities;
                    }
                } catch (e) {
                    console.error("[QueryPage] Failed to parse query result:", e);
                }
            }
        }
    }

    // =========================================================
    // Content
    // =========================================================

    content: [
        // Selection Mode Selector
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: qsTr("Selection Mode")
                font.pixelSize: 12
                font.bold: true
                color: Theme.textPrimary
            }

            Flow {
                Layout.fillWidth: true
                spacing: 4

                Repeater {
                    model: root.selectionModes

                    Util.SelectableButton {
                        required property var modelData
                        required property int index

                        text: modelData.name
                        selected: root.currentSelectionMode === modelData.mode
                        width: 54
                        height: 28

                        onClicked: {
                            root.currentSelectionMode = modelData.mode;
                            let viewport = root.findViewport();
                            if (viewport) {
                                viewport.selectionMode = modelData.mode;
                            }
                        }
                    }
                }
            }
        },

        // Selection List
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 12
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Selected Entities")
                    font.pixelSize: 12
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                }

                Label {
                    text: root.selectedEntityIds.length > 0 ? root.selectedEntityIds.length.toString() : ""
                    font.pixelSize: 11
                    color: Theme.textSecondary
                    visible: root.selectedEntityIds.length > 0
                }

                Util.BaseButton {
                    text: qsTr("Clear")
                    implicitWidth: 48
                    implicitHeight: 24
                    visible: root.selectedEntityIds.length > 0

                    onClicked: {
                        if (root.viewport) {
                            root.viewport.clearPickedSelection();
                        }
                    }
                }
            }

            // Entity list scroll view
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(120, entityListColumn.implicitHeight + 8)
                clip: true

                Rectangle {
                    width: parent.width
                    height: entityListColumn.implicitHeight + 8
                    color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.2) : Qt.rgba(0, 0, 0, 0.05)
                    radius: 4

                    Column {
                        id: entityListColumn
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 2

                        Repeater {
                            model: root.selectedEntityIds

                            Rectangle {
                                required property var modelData
                                required property int index

                                width: parent.width
                                height: 24
                                color: entityItemArea.containsMouse ? Theme.hoverBackground : "transparent"
                                radius: 2

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 4
                                    spacing: 8

                                    Label {
                                        text: "ID: " + modelData
                                        font.pixelSize: 11
                                        font.family: "Consolas"
                                        color: Theme.textPrimary
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: "×"
                                        font.pixelSize: 14
                                        color: Theme.textSecondary
                                        visible: entityItemArea.containsMouse

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                if (root.viewport) {
                                                    root.viewport.deselectEntity(modelData);
                                                }
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    id: entityItemArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    propagateComposedEvents: true
                                }
                            }
                        }

                        // Empty state
                        Label {
                            visible: root.selectedEntityIds.length === 0
                            text: qsTr("Click on geometry to select")
                            font.pixelSize: 11
                            font.italic: true
                            color: Theme.textSecondary
                            padding: 8
                        }
                    }
                }
            }
        },

        // Query Results
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 12
            spacing: 8
            visible: root.queryResults.length > 0

            Label {
                text: qsTr("Entity Information")
                font.pixelSize: 12
                font.bold: true
                color: Theme.textPrimary
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(200, resultsColumn.implicitHeight + 8)
                clip: true

                Column {
                    id: resultsColumn
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: root.queryResults

                        Rectangle {
                            required property var modelData
                            required property int index

                            width: parent.width
                            color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.2) : Qt.rgba(0, 0, 0, 0.05)
                            radius: 4
                            height: entityInfoColumn.implicitHeight + 16

                            Column {
                                id: entityInfoColumn
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                // Header
                                RowLayout {
                                    width: parent.width

                                    Label {
                                        text: modelData.type || "Unknown"
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: Theme.accent
                                    }

                                    Label {
                                        text: "#" + (modelData.id || 0)
                                        font.pixelSize: 11
                                        font.family: "Consolas"
                                        color: Theme.textSecondary
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }

                                    Rectangle {
                                        visible: modelData.part_color !== undefined
                                        width: 12
                                        height: 12
                                        radius: 2
                                        color: modelData.part_color || "transparent"
                                        border.width: 1
                                        border.color: Theme.border
                                    }
                                }

                                // Name
                                Label {
                                    visible: modelData.name && modelData.name.length > 0
                                    text: modelData.name
                                    font.pixelSize: 11
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                // Owning Part
                                Label {
                                    visible: modelData.owning_part_name !== undefined
                                    text: qsTr("Part: ") + (modelData.owning_part_name || "None")
                                    font.pixelSize: 10
                                    color: Theme.textSecondary
                                }

                                // Bounding Box
                                Label {
                                    visible: modelData.size !== undefined
                                    text: qsTr("Size: ") + formatSize(modelData.size)
                                    font.pixelSize: 10
                                    color: Theme.textSecondary

                                    function formatSize(size) {
                                        if (!size || size.length < 3)
                                            return "N/A";
                                        return size[0].toFixed(2) + " × " + size[1].toFixed(2) + " × " + size[2].toFixed(2);
                                    }
                                }

                                // Center
                                Label {
                                    visible: modelData.center !== undefined
                                    text: qsTr("Center: ") + formatPoint(modelData.center)
                                    font.pixelSize: 10
                                    color: Theme.textSecondary

                                    function formatPoint(pt) {
                                        if (!pt || pt.length < 3)
                                            return "N/A";
                                        return "(" + pt[0].toFixed(2) + ", " + pt[1].toFixed(2) + ", " + pt[2].toFixed(2) + ")";
                                    }
                                }

                                // Hierarchy info
                                RowLayout {
                                    width: parent.width
                                    spacing: 16

                                    Label {
                                        visible: modelData.parent_ids && modelData.parent_ids.length > 0
                                        text: qsTr("Parents: ") + (modelData.parent_ids ? modelData.parent_ids.length : 0)
                                        font.pixelSize: 10
                                        color: Theme.textSecondary
                                    }

                                    Label {
                                        visible: modelData.child_ids && modelData.child_ids.length > 0
                                        text: qsTr("Children: ") + (modelData.child_ids ? modelData.child_ids.length : 0)
                                        font.pixelSize: 10
                                        color: Theme.textSecondary
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    ]

    // =========================================================
    // Global Mouse Tracking for Viewport Interaction
    // =========================================================

    // Selection is now driven by GLViewport signals in picking mode.

    Connections {
        target: root.viewport

        function onSelectionChanged(entity_ids) {
            root.selectedEntityIds = entity_ids
            root.queryEntities(entity_ids)
        }
    }
}
