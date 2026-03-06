/**
 * @file DocumentSideBar.qml
 * @brief Document sidebar component for displaying part list
 *
 * Provides a collapsible sidebar that shows all parts in the current document
 * with their names, colors, and entity counts. Communicates with BackendService
 * to fetch and display part information.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0

Rectangle {
    id: sidebar

    /// Whether the sidebar is expanded
    property bool expanded: true

    /// Collapsed width
    readonly property int collapsedWidth: 32

    /// Expanded width
    readonly property int expandedWidth: 240

    /// Part list data model
    property var partListModel: []

    /// Whether data is being loaded
    property bool isLoading: false

    readonly property bool hasParts: sidebar.partListModel.length > 0
    readonly property bool allGeometryVisible: sidebar.hasParts && sidebar.partListModel.every(part => part.geometry_visible !== false)
    readonly property bool allMeshVisible: sidebar.hasParts && sidebar.partListModel.every(part => part.mesh_visible !== false)

    width: sidebar.expanded ? sidebar.expandedWidth : sidebar.collapsedWidth
    color: Theme.surfaceAlt

    Behavior on width {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    /**
     * @brief Refresh the part list from BackendService
     */
    function refreshPartList() {
        sidebar.isLoading = true;
        BackendService.request("GeometryService", JSON.stringify({
            action: "get_part_list",
            _meta: {
                silent: true,
                defer_if_busy: true
            }
        }));
    }

    function applyVisibilityPatch(partUids, patch) {
        const uidSet = new Set(partUids);
        const updated = sidebar.partListModel.slice();
        let changed = false;

        for (let i = 0; i < updated.length; ++i) {
            const part = updated[i];
            if (!uidSet.has(part.uid))
                continue;

            updated[i] = Object.assign({}, part, patch);
            changed = true;
        }

        if (changed)
            sidebar.partListModel = updated;
    }

    function requestPartVisibility(partUids, patch) {
        const uidArray = Array.isArray(partUids) ? partUids.slice() : [partUids];
        sidebar.applyVisibilityPatch(uidArray, patch);

        const request = {
            action: "PartVisibilityControl",
            part_visibility: Object.assign({
                part_uids: uidArray
            }, patch),
            _meta: {
                silent: true
            }
        };
        BackendService.request("RenderService", JSON.stringify(request));
    }

    function updatePartVisibility(partUids, geometryVisible, meshVisible) {
        const patch = {};
        if (geometryVisible !== undefined)
            patch.geometry_visible = geometryVisible;
        if (meshVisible !== undefined)
            patch.mesh_visible = meshVisible;
        sidebar.applyVisibilityPatch(partUids, patch);
    }

    function allPartUids() {
        return sidebar.partListModel.map(part => part.uid);
    }

    function toggleAllGeometry() {
        if (!sidebar.hasParts)
            return;
        sidebar.requestPartVisibility(sidebar.allPartUids(), {
            geometry_visible: !sidebar.allGeometryVisible
        });
    }

    function toggleAllMesh() {
        if (!sidebar.hasParts)
            return;
        sidebar.requestPartVisibility(sidebar.allPartUids(), {
            mesh_visible: !sidebar.allMeshVisible
        });
    }

    // Listen for BackendService operation results
    Connections {
        target: BackendService
        function onOperationFinished(moduleName, actionName, result) {
            console.log("[DocumentSideBar] Operation finished:", moduleName, actionName);
            if (moduleName === "RenderService" && actionName === "PartVisibilityControl") {
                try {
                    const data = JSON.parse(result);
                    if (data.success === false) {
                        console.warn("[DocumentSideBar] Visibility request failed:", data.error);
                        sidebar.refreshPartList();
                        return;
                    }

                    const partUids = data.part_uids || (data.part_uid !== undefined ? [data.part_uid] : []);
                    sidebar.updatePartVisibility(partUids, data.geometry_visible, data.mesh_visible);
                } catch (e) {
                    console.warn("[DocumentSideBar] Failed to parse visibility result:", e);
                    sidebar.refreshPartList();
                }
                return;
            }

            if (moduleName !== "GeometryService")
                return;
            if (actionName !== "get_part_list")
                return;

            try {
                const data = JSON.parse(result);
                console.log("[DocumentSideBar] Received part list:", data.parts);
                sidebar.partListModel = data.parts || [];
                sidebar.isLoading = false;
            } catch (e) {
                console.warn("[DocumentSideBar] Failed to parse result:", e);
                sidebar.isLoading = false;
            }
        }

        function onOperationFailed(moduleName, actionName, error) {
            if (moduleName === "RenderService" && actionName === "PartVisibilityControl") {
                console.warn("[DocumentSideBar] Failed to update part visibility:", error);
                sidebar.refreshPartList();
                return;
            }
            if (moduleName !== "GeometryService")
                return;
            if (actionName !== "get_part_list")
                return;
            sidebar.isLoading = false;
            console.warn("[DocumentSideBar] Failed to get part list:", error);
        }
    }

    // Initial load on component completion
    Component.onCompleted: {}

    // Toggle button
    Rectangle {
        id: toggleButton
        anchors.top: parent.top
        anchors.right: parent.right
        // anchors.margins: 4
        width: 32
        height: 32
        // radius: 4
        color: toggleArea.containsMouse ? Theme.hovered : Theme.surfaceHighLight

        ThemedIcon {
            anchors.centerIn: parent
            source: sidebar.expanded ? "qrc:/opengeolab/resources/icons/collapse.svg" : "qrc:/opengeolab/resources/icons/expand.svg"
            size: 16
        }

        MouseArea {
            id: toggleArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: sidebar.expanded = !sidebar.expanded
        }

        ToolTip.visible: toggleArea.containsMouse
        ToolTip.text: sidebar.expanded ? qsTr("Collapse sidebar") : qsTr("Expand sidebar")
        ToolTip.delay: 500
    }

    // Header
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: toggleButton.left
        height: 32
        color: Theme.surfaceHighLight
        visible: sidebar.expanded

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 8
            spacing: 8

            Label {
                text: qsTr("Document")
                font.pixelSize: 13
                font.bold: true
                color: Theme.textPrimary
                Layout.fillWidth: true
            }

            // // Refresh button
            // Rectangle {
            //     width: 24
            //     height: 24
            //     radius: 4
            //     color: refreshArea.containsMouse ? Theme.hovered : "transparent"

            //     ThemedIcon {
            //         anchors.centerIn: parent
            //         source: "qrc:/opengeolab/resources/icons/refresh.svg"
            //         size: 14
            //         rotation: sidebar.isLoading ? refreshAnimation.angle : 0
            //     }

            //     NumberAnimation on rotation {
            //         id: refreshAnimation
            //         property real angle: 0
            //         running: sidebar.isLoading
            //         from: 0
            //         to: 360
            //         duration: 1000
            //         loops: Animation.Infinite
            //     }

            //     MouseArea {
            //         id: refreshArea
            //         anchors.fill: parent
            //         hoverEnabled: true
            //         onClicked: sidebar.refreshPartList()
            //     }

            //     ToolTip.visible: refreshArea.containsMouse
            //     ToolTip.text: qsTr("Refresh part list")
            //     ToolTip.delay: 500
            // }
        }
    }

    // Separator
    Rectangle {
        id: separator
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
        visible: sidebar.expanded
    }

    // Part count summary
    Rectangle {
        id: summaryBar
        anchors.top: separator.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 24
        color: "transparent"
        visible: sidebar.expanded

        Label {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Parts: %1").arg(sidebar.partListModel.length)
            font.pixelSize: 11
            color: Theme.textSecondary
        }

        RowLayout {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Rectangle {
                width: 22
                height: 22
                radius: 6
                border.width: 1
                border.color: sidebar.allGeometryVisible ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isDark ? 0.9 : 0.55) : Theme.border
                color: geometrySummaryArea.pressed ? (sidebar.allGeometryVisible ? Qt.darker(Theme.surfaceHighLight, 1.08) : Theme.clicked) : (geometrySummaryArea.containsMouse ? Theme.hovered : (sidebar.allGeometryVisible ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isDark ? 0.20 : 0.12) : "transparent"))
                opacity: sidebar.hasParts ? 1.0 : 0.45

                ThemedIcon {
                    anchors.centerIn: parent
                    source: "qrc:/opengeolab/resources/icons/face.svg"
                    size: 13
                    color: sidebar.allGeometryVisible ? Theme.accent : Theme.textSecondary
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 2
                    height: 13
                    radius: 1
                    rotation: 45
                    color: Theme.danger
                    opacity: sidebar.allGeometryVisible ? 0.0 : 0.88
                }

                MouseArea {
                    id: geometrySummaryArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: sidebar.hasParts
                    onClicked: sidebar.toggleAllGeometry()
                }

                ToolTip.visible: geometrySummaryArea.containsMouse
                ToolTip.text: sidebar.allGeometryVisible ? qsTr("Hide all geometry") : qsTr("Show all geometry")
                ToolTip.delay: 400
            }

            Rectangle {
                width: 22
                height: 22
                radius: 6
                border.width: 1
                border.color: sidebar.allMeshVisible ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isDark ? 0.9 : 0.55) : Theme.border
                color: meshSummaryArea.pressed ? (sidebar.allMeshVisible ? Qt.darker(Theme.surfaceHighLight, 1.08) : Theme.clicked) : (meshSummaryArea.containsMouse ? Theme.hovered : (sidebar.allMeshVisible ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isDark ? 0.20 : 0.12) : "transparent"))
                opacity: sidebar.hasParts ? 1.0 : 0.45

                ThemedIcon {
                    anchors.centerIn: parent
                    source: "qrc:/opengeolab/resources/icons/mesh.svg"
                    size: 13
                    color: sidebar.allMeshVisible ? Theme.accent : Theme.textSecondary
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: 2
                    height: 13
                    radius: 1
                    rotation: 45
                    color: Theme.danger
                    opacity: sidebar.allMeshVisible ? 0.0 : 0.88
                }

                MouseArea {
                    id: meshSummaryArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: sidebar.hasParts
                    onClicked: sidebar.toggleAllMesh()
                }

                ToolTip.visible: meshSummaryArea.containsMouse
                ToolTip.text: sidebar.allMeshVisible ? qsTr("Hide all mesh") : qsTr("Show all mesh")
                ToolTip.delay: 400
            }
        }
    }

    // Part list
    ListView {
        id: partListView
        anchors.top: summaryBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 4
        visible: sidebar.expanded
        clip: true

        model: sidebar.partListModel
        spacing: 2

        delegate: PartListItem {
            required property var modelData
            required property int index

            width: partListView.width
            partData: modelData
            partIndex: index
            onToggleGeometryRequested: function (partUid, visible) {
                console.log("Requesting geometry visibility change for part", partUid, "visible:", visible);
                sidebar.requestPartVisibility(partUid, {
                    geometry_visible: visible
                });
            }
            onToggleMeshRequested: function (partUid, visible) {
                console.log("Requesting mesh visibility change for part", partUid, "visible:", visible);
                sidebar.requestPartVisibility(partUid, {
                    mesh_visible: visible
                });
            }
        }

        // Empty state
        Label {
            anchors.centerIn: parent
            visible: sidebar.partListModel.length === 0 && !sidebar.isLoading
            text: qsTr("No parts in document")
            font.pixelSize: 12
            color: Theme.textSecondary
        }

        // Loading indicator
        BusyIndicator {
            anchors.centerIn: parent
            visible: sidebar.isLoading
            running: sidebar.isLoading
            width: 32
            height: 32
        }
    }
}
