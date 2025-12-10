pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

/**
 * @brief Manager for operation panels triggered by Ribbon toolbar actions
 *
 * This component manages the display of operation panels in a consistent manner.
 * Place this in your main window and connect it to your RibbonToolBar signals.
 *
 * Usage pattern:
 * 1. RibbonToolBar emits signal (e.g., toggleRelease)
 * 2. Main.qml connects signal to panelManager.showPanel("release", ...)
 * 3. OperationPanelManager shows the panel
 * 4. User interacts, panel emits applyClicked/cancelClicked
 * 5. Main.qml handles the actual operation
 */
Item {
    id: panelManager

    // Signals for panel actions - connect these in your main application
    signal panelApplied(string panelId, var selectionData)
    signal panelCancelled(string panelId)
    signal selectionRequested(string panelId)

    // Current active panel info
    property string activePanelId: ""
    property bool hasPanelOpen: activePanelId !== ""

    // Panel configurations
    readonly property var panelConfigs: ({
            // Geometry operations
            "release": {
                title: "Extrude",
                hint: "Select the geometry to extrude",
                showWorkflow: true
            },
            "toggle": {
                title: "Revolve",
                hint: "Select geometry to revolve",
                showWorkflow: true
            },
            "stitch": {
                title: "Boolean",
                hint: "Select geometries for boolean operation",
                showWorkflow: true
            },
            "tangentExtend": {
                title: "Tangent Extend",
                hint: "Select edge to extend",
                showWorkflow: true
            },
            "project": {
                title: "Project",
                hint: "Select geometry to project",
                showWorkflow: true
            },
            "trim": {
                title: "Trim",
                hint: "Select curves to trim",
                showWorkflow: true
            },
            "offset": {
                title: "Offset",
                hint: "Select geometry to offset",
                showWorkflow: true
            },
            "fill": {
                title: "Fill",
                hint: "Select boundary to fill",
                showWorkflow: true
            },
            "surfaceExtend": {
                title: "Surface Extend",
                hint: "Select surface edge to extend",
                showWorkflow: true
            },
            "surfaceMerge": {
                title: "Surface Merge",
                hint: "Select surfaces to merge",
                showWorkflow: true
            },
            "suppress": {
                title: "Suppress",
                hint: "Select features to suppress",
                showWorkflow: true
            },
            "split": {
                title: "Split",
                hint: "Select geometry to split",
                showWorkflow: true
            },
            // Mesh operations
            "generateMesh": {
                title: "Auto Mesh",
                hint: "Select geometry to generate mesh",
                showWorkflow: true
            },
            "refineMesh": {
                title: "Refine Mesh",
                hint: "Select mesh regions to refine",
                showWorkflow: true
            },
            "checkMesh": {
                title: "Check Mesh Quality",
                hint: "Select mesh to check quality",
                showWorkflow: false
            },
            "repairMesh": {
                title: "Repair Mesh",
                hint: "Select mesh to repair",
                showWorkflow: true
            },
            // AI operations
            "aiSuggest": {
                title: "Smart Suggest",
                hint: "Select geometry for AI modeling suggestions",
                showWorkflow: true
            },
            "aiOptimize": {
                title: "Auto Optimize",
                hint: "Select mesh for AI optimization",
                showWorkflow: true
            },
            "aiExplore": {
                title: "Design Explore",
                hint: "Select design space for AI exploration",
                showWorkflow: true
            },
            "aiChat": {
                title: "AI Assistant",
                hint: "Ask AI assistant for help",
                showWorkflow: false
            }
        })

    // Position offset properties for panel placement
    // These should be set by the parent to position the panel correctly
    property real panelLeftOffset: 210  // Default: after 200px control panel + margin
    property real panelTopOffset: 10    // Default: small margin from top

    // The actual panel component
    OperationPanel {
        id: operationPanel
        visible: panelManager.hasPanelOpen
        x: panelManager.panelLeftOffset
        y: panelManager.panelTopOffset

        onApplyClicked: {
            panelManager.panelApplied(panelManager.activePanelId, {
                selectedCount: operationPanel.selectedCount
            });
            panelManager.closePanel();
        }

        onCancelClicked: {
            panelManager.panelCancelled(panelManager.activePanelId);
            panelManager.closePanel();
        }

        onSelectionRequested: {
            panelManager.selectionRequested(panelManager.activePanelId);
        }
    }

    // Show a panel by ID
    function showPanel(panelId: string): void {
        const config = panelConfigs[panelId];
        if (config) {
            operationPanel.title = config.title;
            operationPanel.selectionHint = config.hint;
            operationPanel.showWorkflowSection = config.showWorkflow;
            operationPanel.reset();
            activePanelId = panelId;
        } else {
            console.warn("Unknown panel ID:", panelId);
        }
    }

    // Close the current panel
    function closePanel(): void {
        activePanelId = "";
        operationPanel.reset();
    }

    // Update selection count for the current panel
    function updateSelection(count: int): void {
        if (hasPanelOpen) {
            operationPanel.setSelection(count);
        }
    }

    // Toggle panel - if same panel is open, close it; otherwise open the new one
    function togglePanel(panelId: string): void {
        if (activePanelId === panelId) {
            closePanel();
        } else {
            showPanel(panelId);
        }
    }
}
