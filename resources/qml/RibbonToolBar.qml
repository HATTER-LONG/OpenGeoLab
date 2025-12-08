pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

/**
 * @brief Ribbon-style toolbar component similar to Microsoft Office
 *
 * Features:
 * - Tab-based navigation (Geometry, Mesh, Interaction, General)
 * - File button opens a popup menu (Office backstage style)
 * - Configuration-driven button generation (see RibbonButtonConfig.qml)
 *
 * To add new buttons:
 * 1. Edit RibbonButtonConfig.qml to add button definition
 * 2. Add signal below if needed
 * 3. Connect signal in Main.qml
 */
Rectangle {
    id: ribbonToolBar

    // ========================================================================
    // SIGNALS - Auto-dispatched based on button ID from config
    // ========================================================================

    // File operations (from FileMenu)
    signal newFile
    signal openFile
    signal saveFile
    signal saveAsFile
    signal importModel
    signal exportModel
    signal replayFile
    signal showOptions
    signal exitApp

    // Geometry operations
    signal addPoint
    signal addPlane
    signal addLine
    signal addBox
    signal pointReplace
    signal toggleRelease
    signal toggle
    signal toggleStitch
    signal tangentExtend
    signal projectGeometry
    signal trim
    signal offset
    signal fill
    signal surfaceExtend
    signal suppress
    signal surfaceMerge
    signal split

    // Mesh operations
    signal generateMesh
    signal refineMesh
    signal simplifyMesh
    signal smoothMesh
    signal checkMesh
    signal repairMesh

    // Interaction operations
    signal rotateView
    signal panView
    signal zoomView
    signal fitAll
    signal pick
    signal boxSelect

    // General operations
    signal options
    signal theme
    signal help

    // ========================================================================
    // PROPERTIES
    // ========================================================================

    property int currentTabIndex: 0  // Default to Geometry tab
    property color accentColor: "#0078D4"
    property color hoverColor: "#E5F1FB"
    property color selectedColor: "#CCE4F7"
    property color borderColor: "#D1D1D1"
    property color tabBackgroundColor: "#F3F3F3"
    property color contentBackgroundColor: "#FCFCFC"

    readonly property var tabNames: ["Geometry", "Mesh", "Interaction", "General"]

    height: 130
    color: tabBackgroundColor

    // Button configuration instance
    RibbonButtonConfig {
        id: buttonConfig
    }

    // ========================================================================
    // FILE MENU POPUP
    // ========================================================================
    RibbonFileMenu {
        id: fileMenu
        x: 0
        y: tabBar.height

        onNewFile: ribbonToolBar.newFile()
        onOpenFile: ribbonToolBar.openFile()
        onSaveFile: ribbonToolBar.saveFile()
        onSaveAsFile: ribbonToolBar.saveAsFile()
        onImportModel: ribbonToolBar.importModel()
        onExportModel: ribbonToolBar.exportModel()
        onReplayFile: ribbonToolBar.replayFile()
        onShowOptions: ribbonToolBar.showOptions()
        onExitApp: ribbonToolBar.exitApp()
    }

    // ========================================================================
    // TAB BAR
    // ========================================================================
    Rectangle {
        id: tabBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 28
        color: "white"

        RowLayout {
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            // File button (opens menu)
            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 24
                color: fileMenu.visible ? ribbonToolBar.accentColor : (fileButtonArea.containsMouse ? ribbonToolBar.accentColor : "transparent")
                radius: 2

                Text {
                    anchors.centerIn: parent
                    text: "File"
                    color: fileMenu.visible || fileButtonArea.containsMouse ? "white" : "#333333"
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: fileButtonArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: fileMenu.open()
                }
            }

            // Tab buttons
            Repeater {
                model: ribbonToolBar.tabNames

                Rectangle {
                    id: tabDelegate
                    required property int index
                    required property string modelData

                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 24
                    color: ribbonToolBar.currentTabIndex === index ? ribbonToolBar.contentBackgroundColor : (tabMouseArea.containsMouse ? ribbonToolBar.hoverColor : "transparent")
                    border.width: ribbonToolBar.currentTabIndex === index ? 1 : 0
                    border.color: ribbonToolBar.borderColor

                    Rectangle {
                        visible: ribbonToolBar.currentTabIndex === tabDelegate.index
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 1
                        anchors.rightMargin: 1
                        height: 2
                        color: ribbonToolBar.contentBackgroundColor
                    }

                    Text {
                        anchors.centerIn: parent
                        text: tabDelegate.modelData
                        color: "#333333"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: tabMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: ribbonToolBar.currentTabIndex = tabDelegate.index
                    }
                }
            }
        }
    }

    // ========================================================================
    // CONTENT AREA - Dynamic Tab Content
    // ========================================================================
    Rectangle {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom
        color: ribbonToolBar.contentBackgroundColor
        border.width: 1
        border.color: ribbonToolBar.borderColor

        // Geometry Tab
        RibbonTabContent {
            visible: ribbonToolBar.currentTabIndex === 0
            anchors.fill: parent
            groups: buttonConfig.geometryTab
            onButtonClicked: actionId => ribbonToolBar.dispatchAction(actionId)
        }

        // Mesh Tab
        RibbonTabContent {
            visible: ribbonToolBar.currentTabIndex === 1
            anchors.fill: parent
            groups: buttonConfig.meshTab
            onButtonClicked: actionId => ribbonToolBar.dispatchAction(actionId)
        }

        // Interaction Tab
        RibbonTabContent {
            visible: ribbonToolBar.currentTabIndex === 2
            anchors.fill: parent
            groups: buttonConfig.interactionTab
            onButtonClicked: actionId => ribbonToolBar.dispatchAction(actionId)
        }

        // General Tab
        RibbonTabContent {
            visible: ribbonToolBar.currentTabIndex === 3
            anchors.fill: parent
            groups: buttonConfig.generalTab
            onButtonClicked: actionId => ribbonToolBar.dispatchAction(actionId)
        }
    }

    // ========================================================================
    // ACTION DISPATCHER
    // ========================================================================

    /**
     * Dispatch action by ID to the corresponding signal
     * This maps button IDs from config to actual signals
     */
    function dispatchAction(actionId: string): void {
        switch (actionId) {
        // Geometry actions
        case "addPoint":
            addPoint();
            break;
        case "addPlane":
            addPlane();
            break;
        case "addLine":
            addLine();
            break;
        case "addBox":
            addBox();
            break;
        case "pointReplace":
            pointReplace();
            break;
        case "toggleRelease":
            toggleRelease();
            break;
        case "toggle":
            toggle();
            break;
        case "toggleStitch":
            toggleStitch();
            break;
        case "tangentExtend":
            tangentExtend();
            break;
        case "projectGeometry":
            projectGeometry();
            break;
        case "trim":
            trim();
            break;
        case "offset":
            offset();
            break;
        case "fill":
            fill();
            break;
        case "surfaceExtend":
            surfaceExtend();
            break;
        case "suppress":
            suppress();
            break;
        case "surfaceMerge":
            surfaceMerge();
            break;
        case "split":
            split();
            break;

        // Mesh actions
        case "generateMesh":
            generateMesh();
            break;
        case "refineMesh":
            refineMesh();
            break;
        case "simplifyMesh":
            simplifyMesh();
            break;
        case "smoothMesh":
            smoothMesh();
            break;
        case "checkMesh":
            checkMesh();
            break;
        case "repairMesh":
            repairMesh();
            break;

        // Interaction actions
        case "rotateView":
            rotateView();
            break;
        case "panView":
            panView();
            break;
        case "zoomView":
            zoomView();
            break;
        case "fitAll":
            fitAll();
            break;
        case "pick":
            pick();
            break;
        case "boxSelect":
            boxSelect();
            break;

        // General actions
        case "options":
            options();
            break;
        case "theme":
            theme();
            break;
        case "help":
            help();
            break;
        default:
            console.warn("Unknown action:", actionId);
        }
    }

    // ========================================================================
    // PUBLIC API
    // ========================================================================

    function setRecentFiles(files: list<string>): void {
        fileMenu.setRecentFiles(files);
    }
}
