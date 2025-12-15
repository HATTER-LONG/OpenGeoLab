pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import OpenGeoLab

/**
 * @file Main.qml
 * @brief Main application window for OpenGeoLab
 *
 * This is the root QML component that sets up the main window layout including:
 * - Ribbon toolbar at the top
 * - Model tree panel on the left
 * - 3D geometry viewport in the center
 * - View control toolbar at the bottom right
 */
Window {
    id: root
    visible: true
    width: 1200
    height: 800
    title: "OpenGeoLab - 3D Geometry Renderer"

    // ========================================================================
    // Dark Theme Color Constants
    // ========================================================================
    readonly property color textColor: "#e1e1e1"
    readonly property color borderColor: "#3a3f4b"
    readonly property color renderBackground: "#2d3238"

    Component.onCompleted: {
        ModelImporter.setTargetRenderer(geometryRenderer);
        GeometryCreator.setTargetRenderer(geometryRenderer);
    }

    // Create Box Dialog
    CreateBoxDialog {
        id: createBoxDialog
        anchors.centerIn: parent

        onBoxCreated: function (width, height, depth) {
            console.log("Creating box:", width, "x", height, "x", depth);
            GeometryCreator.createBox(width, height, depth);
        }
    }

    Connections {
        target: ModelImporter
        function onModelLoaded(filename) {
            statusText.text = "Loaded: " + filename;
            statusText.color = "lightgreen";
        }
        function onModelLoadFailed(error) {
            statusText.text = "Error: " + error;
            statusText.color = "red";
        }
    }

    Connections {
        target: GeometryCreator
        function onGeometryCreated(name) {
            statusText.text = "Created: " + name;
            statusText.color = "lightgreen";
        }
        function onGeometryCreationFailed(error) {
            statusText.text = "Error: " + error;
            statusText.color = "red";
        }
    }

    // File dialog for model import
    FileDialog {
        id: fileDialog
        title: "Import Model"
        nameFilters: ["STEP files (*.stp *.step)", "BREP files (*.brep *.brp)", "All files (*)"]
        onAccepted: {
            statusText.text = "Loading model...";
            statusText.color = "yellow";
            ModelImporter.importModel(selectedFile);
        }
    }

    // ========================================================================
    // Operation Panel Manager - handles tool operation panels
    // ========================================================================
    OperationPanelManager {
        id: panelManager
        anchors.fill: parent
        z: 1000

        onPanelApplied: function (panelId, selectionData) {
            console.log("Panel applied:", panelId, "with", selectionData.selectedCount, "selections");
            switch (panelId) {
            case "release":
                console.log("Executing Release operation...");
                break;
            case "toggle":
                console.log("Executing Toggle operation...");
                break;
            }
        }

        onPanelCancelled: function (panelId) {
            console.log("Panel cancelled:", panelId);
        }

        onSelectionRequested: function (panelId) {
            console.log("Selection requested for:", panelId);
            Qt.callLater(function () {
                panelManager.updateSelection(3);
            });
        }
    }

    // ========================================================================
    // Ribbon Toolbar
    // ========================================================================
    RibbonToolBar {
        id: ribbonToolBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        // File operations
        onNewFile: {
            console.log("New file - TODO");
            modelTreePanel.resetToDefault();
            statusText.text = "New model created";
            statusText.color = "lightgreen";
        }
        onImportModel: fileDialog.open()
        onExportModel: console.log("Export model - TODO")
        onShowOptions: console.log("Show options - TODO")

        // Geometry operations - show operation panels
        onToggleRelease: panelManager.togglePanel("release")
        onToggle: panelManager.togglePanel("toggle")
        onToggleStitch: panelManager.togglePanel("stitch")
        onTangentExtend: panelManager.togglePanel("tangentExtend")
        onProjectGeometry: panelManager.togglePanel("project")
        onTrim: panelManager.togglePanel("trim")
        onOffset: panelManager.togglePanel("offset")
        onFill: panelManager.togglePanel("fill")
        onSurfaceExtend: panelManager.togglePanel("surfaceExtend")
        onSurfaceMerge: panelManager.togglePanel("surfaceMerge")
        onSuppress: panelManager.togglePanel("suppress")
        onSplit: panelManager.togglePanel("split")

        // Geometry creation
        onAddBox: createBoxDialog.open()
        onAddPoint: console.log("Add point - TODO")
        onAddPlane: console.log("Add plane - TODO")
        onAddLine: console.log("Add line - TODO")

        // Mesh operations
        onGenerateMesh: panelManager.togglePanel("generateMesh")
        onRefineMesh: panelManager.togglePanel("refineMesh")
        onCheckMesh: panelManager.togglePanel("checkMesh")
        onRepairMesh: panelManager.togglePanel("repairMesh")

        // AI operations
        onAiSuggest: panelManager.togglePanel("aiSuggest")
        onAiOptimize: panelManager.togglePanel("aiOptimize")
        onAiExplore: panelManager.togglePanel("aiExplore")
        onAiChat: panelManager.togglePanel("aiChat")
    }

    // Status text overlay
    Text {
        id: statusText
        color: "white"
        font.pixelSize: 14
        font.bold: true
        text: "Ready - Import a BREP or STEP model to begin"
        anchors.top: ribbonToolBar.bottom
        anchors.left: modelTreePanel.right
        anchors.margins: 15
        z: 100

        Rectangle {
            anchors.fill: parent
            anchors.margins: -5
            color: Qt.rgba(0, 0, 0, 0.7)
            radius: 5
            z: -1
        }
    }

    // 3D Geometry renderer
    Geometry3D {
        id: geometryRenderer
        anchors.left: modelTreePanel.right
        anchors.right: parent.right
        anchors.top: ribbonToolBar.bottom
        anchors.bottom: parent.bottom

        color: Qt.rgba(0, 0, 0, 0)
        backgroundColor: root.renderBackground
    }

    // View Control Toolbar
    ViewControlToolbar {
        id: viewControlToolbar
        targetRenderer: geometryRenderer
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 20
        anchors.bottomMargin: 60
        z: 100
    }

    // Left Model Tree Panel
    ModelTreeView {
        id: modelTreePanel
        width: 220
        anchors.left: parent.left
        anchors.top: ribbonToolBar.bottom
        anchors.bottom: parent.bottom

        onItemSelected: function (index, name, type) {
            console.log("Selected:", name, "Type:", type);
            statusText.text = "Selected: " + name;
            statusText.color = "lightblue";
        }

        onItemDoubleClicked: function (index, name, type) {
            console.log("Double clicked:", name, "Type:", type);
        }
    }

    // Information overlay
    Rectangle {
        color: Qt.rgba(0.12, 0.13, 0.16, 0.85)
        radius: 10
        border.width: 1
        border.color: root.borderColor
        anchors.fill: label
        anchors.margins: -10
    }

    Text {
        id: label
        color: root.textColor
        wrapMode: Text.WordWrap
        text: qsTr("OpenGeoLab - CAE Software. Use Ribbon toolbar for geometry modeling, mesh generation and AI assistant.\nCtrl+drag to rotate (trackball), Shift+drag or middle button to pan, scroll wheel to zoom.")
        anchors.right: parent.right
        anchors.left: modelTreePanel.right
        anchors.leftMargin: 20
        anchors.bottom: parent.bottom
        anchors.margins: 20
    }
}
