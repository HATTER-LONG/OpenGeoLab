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
 * - Tool buttons with icons and text labels
 * - Grouped tool sections with separators
 * - Configuration separated for easy customization
 */
Rectangle {
    id: ribbonToolBar

    // ============================================
    // SIGNALS - Connect these in your main application
    // ============================================

    // File operations (emitted from FileMenu)
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

    // ============================================
    // PROPERTIES
    // ============================================

    property int currentTabIndex: 0  // Default to Geometry tab (0-based, File is not a tab)
    property color accentColor: "#0078D4"  // Microsoft blue
    property color hoverColor: "#E5F1FB"
    property color selectedColor: "#CCE4F7"
    property color borderColor: "#D1D1D1"
    property color tabBackgroundColor: "#F3F3F3"
    property color contentBackgroundColor: "#FCFCFC"

    // Tab names (File is not included - it's a menu button)
    readonly property var tabNames: ["Geometry", "Mesh", "Interaction", "General"]

    height: 130
    color: tabBackgroundColor

    // ============================================
    // FILE MENU POPUP
    // ============================================
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

    // ============================================
    // TAB BAR
    // ============================================
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

            // File button (opens menu, not a tab)
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

            // Regular tabs
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

                    // Hide bottom border when selected
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

    // ============================================
    // CONTENT AREA
    // ============================================
    Rectangle {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom
        color: ribbonToolBar.contentBackgroundColor
        border.width: 1
        border.color: ribbonToolBar.borderColor

        // ========== GEOMETRY TAB ==========
        Row {
            visible: ribbonToolBar.currentTabIndex === 0
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "Create"
                height: parent.height

                RibbonLargeButton {
                    iconText: "•"
                    text: "Point"
                    onClicked: ribbonToolBar.addPoint()
                }

                RibbonLargeButton {
                    iconText: "⊕"
                    text: "Point\nReplace"
                    onClicked: ribbonToolBar.pointReplace()
                }

                RibbonGroupSeparator {}

                RibbonLargeButton {
                    iconText: "▭"
                    text: "Plane"
                    onClicked: ribbonToolBar.addPlane()
                }

                RibbonLargeButton {
                    iconText: "╱"
                    text: "Line"
                    onClicked: ribbonToolBar.addLine()
                }

                RibbonLargeButton {
                    iconText: "☐"
                    text: "Box"
                    onClicked: ribbonToolBar.addBox()
                }
            }

            RibbonGroup {
                title: "Modify"
                height: parent.height

                RibbonLargeButton {
                    iconText: "⇥"
                    text: "Release"
                    onClicked: ribbonToolBar.toggleRelease()
                }

                RibbonLargeButton {
                    iconText: "⊞"
                    text: "Toggle"
                    onClicked: ribbonToolBar.toggle()
                }

                RibbonLargeButton {
                    iconText: "⊟"
                    text: "Stitch"
                    onClicked: ribbonToolBar.toggleStitch()
                }

                RibbonLargeButton {
                    iconText: "↗"
                    text: "Tangent\nExtend"
                    onClicked: ribbonToolBar.tangentExtend()
                }

                RibbonLargeButton {
                    iconText: "⊡"
                    text: "Project"
                    onClicked: ribbonToolBar.projectGeometry()
                }
            }

            RibbonGroup {
                title: "Edit"
                height: parent.height

                RibbonLargeButton {
                    iconText: "✂"
                    text: "Trim"
                    onClicked: ribbonToolBar.trim()
                }

                RibbonLargeButton {
                    iconText: "⊖"
                    text: "Offset"
                    onClicked: ribbonToolBar.offset()
                }

                RibbonLargeButton {
                    iconText: "◉"
                    text: "Fill"
                    onClicked: ribbonToolBar.fill()
                }

                RibbonLargeButton {
                    iconText: "↔"
                    text: "Surface\nExtend"
                    onClicked: ribbonToolBar.surfaceExtend()
                }

                RibbonLargeButton {
                    iconText: "⊗"
                    text: "Surface\nMerge"
                    onClicked: ribbonToolBar.surfaceMerge()
                }

                RibbonLargeButton {
                    iconText: "⊘"
                    text: "Suppress"
                    onClicked: ribbonToolBar.suppress()
                }

                RibbonLargeButton {
                    iconText: "⫽"
                    text: "Split"
                    onClicked: ribbonToolBar.split()
                }
            }
        }

        // ========== MESH TAB ==========
        Row {
            visible: ribbonToolBar.currentTabIndex === 1
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "Mesh Operations"
                height: parent.height

                RibbonLargeButton {
                    iconText: "◇"
                    text: "Generate\nMesh"
                    onClicked: ribbonToolBar.generateMesh()
                }

                RibbonLargeButton {
                    iconText: "△"
                    text: "Refine"
                    onClicked: ribbonToolBar.refineMesh()
                }

                RibbonLargeButton {
                    iconText: "▽"
                    text: "Simplify"
                    onClicked: ribbonToolBar.simplifyMesh()
                }

                RibbonLargeButton {
                    iconText: "⬡"
                    text: "Smooth"
                    onClicked: ribbonToolBar.smoothMesh()
                }
            }

            RibbonGroup {
                title: "Quality"
                height: parent.height

                RibbonLargeButton {
                    iconText: "✓"
                    text: "Check"
                    onClicked: ribbonToolBar.checkMesh()
                }

                RibbonLargeButton {
                    iconText: "🔧"
                    text: "Repair"
                    onClicked: ribbonToolBar.repairMesh()
                }
            }
        }

        // ========== INTERACTION TAB ==========
        Row {
            visible: ribbonToolBar.currentTabIndex === 2
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "View"
                height: parent.height

                RibbonLargeButton {
                    iconText: "⟳"
                    text: "Rotate"
                    onClicked: ribbonToolBar.rotateView()
                }

                RibbonLargeButton {
                    iconText: "⤡"
                    text: "Pan"
                    onClicked: ribbonToolBar.panView()
                }

                RibbonLargeButton {
                    iconText: "🔍"
                    text: "Zoom"
                    onClicked: ribbonToolBar.zoomView()
                }

                RibbonLargeButton {
                    iconText: "⬚"
                    text: "Fit All"
                    onClicked: ribbonToolBar.fitAll()
                }
            }

            RibbonGroup {
                title: "Selection"
                height: parent.height

                RibbonLargeButton {
                    iconText: "☝"
                    text: "Pick"
                    onClicked: ribbonToolBar.pick()
                }

                RibbonLargeButton {
                    iconText: "▢"
                    text: "Box\nSelect"
                    onClicked: ribbonToolBar.boxSelect()
                }
            }
        }

        // ========== GENERAL TAB ==========
        Row {
            visible: ribbonToolBar.currentTabIndex === 3
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "Settings"
                height: parent.height

                RibbonLargeButton {
                    iconText: "⚙"
                    text: "Options"
                    onClicked: ribbonToolBar.options()
                }

                RibbonLargeButton {
                    iconText: "🎨"
                    text: "Theme"
                    onClicked: ribbonToolBar.theme()
                }

                RibbonLargeButton {
                    iconText: "❓"
                    text: "Help"
                    onClicked: ribbonToolBar.help()
                }
            }
        }
    }

    // ============================================
    // PUBLIC FUNCTIONS
    // ============================================

    // Update recent files in the file menu
    function setRecentFiles(files: list<string>): void {
        fileMenu.setRecentFiles(files);
    }

    // Handle action by name (for dynamic button configuration)
    function handleAction(actionName: string): void {
        switch (actionName) {
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
            console.log("Unknown action:", actionName);
        }
    }
}
