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

    // Listen for BackendService operation results
    Connections {
        target: BackendService
        function onOperationFinished(moduleName, actionName, result) {
            console.log("[DocumentSideBar] Operation finished:", moduleName, actionName);
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
