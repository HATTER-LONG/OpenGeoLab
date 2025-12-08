pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

/**
 * @brief Ribbon-style toolbar component similar to Microsoft Office
 *
 * Features:
 * - Tab-based navigation (File, Geometry, Mesh, Interaction, General)
 * - Tool buttons with icons and text labels
 * - Grouped tool sections with separators
 */
Rectangle {
    id: ribbonToolBar

    // Signals for tool actions
    signal openFile
    signal saveFile
    signal importModel
    signal exportModel

    signal addPoint
    signal addPlane
    signal addLine
    signal addBox

    signal toggleRelease
    signal toggleStitch
    signal tangentExtend
    signal projectGeometry

    property int currentTabIndex: 1  // Default to Geometry tab
    property color accentColor: "#0078D4"  // Microsoft blue
    property color hoverColor: "#E5F1FB"
    property color selectedColor: "#CCE4F7"
    property color borderColor: "#D1D1D1"
    property color tabBackgroundColor: "#F3F3F3"
    property color contentBackgroundColor: "#FCFCFC"

    height: 130
    color: tabBackgroundColor

    // Tab bar at the top
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

            // File tab (special styling - like Office)
            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 24
                color: ribbonToolBar.currentTabIndex === 0 ? ribbonToolBar.accentColor : (fileTabArea.containsMouse ? ribbonToolBar.accentColor : "transparent")
                radius: 2

                Text {
                    anchors.centerIn: parent
                    text: "File"
                    color: ribbonToolBar.currentTabIndex === 0 || fileTabArea.containsMouse ? "white" : "#333333"
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: fileTabArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: ribbonToolBar.currentTabIndex = 0
                }
            }

            // Regular tabs
            Repeater {
                model: ["Geometry", "Mesh", "Interaction", "General"]

                Rectangle {
                    id: tabDelegate
                    required property int index
                    required property string modelData

                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 24
                    color: ribbonToolBar.currentTabIndex === (index + 1) ? ribbonToolBar.contentBackgroundColor : (tabMouseArea.containsMouse ? ribbonToolBar.hoverColor : "transparent")
                    border.width: ribbonToolBar.currentTabIndex === (index + 1) ? 1 : 0
                    border.color: ribbonToolBar.borderColor

                    // Hide bottom border when selected
                    Rectangle {
                        visible: ribbonToolBar.currentTabIndex === (tabDelegate.index + 1)
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
                        onClicked: ribbonToolBar.currentTabIndex = tabDelegate.index + 1
                    }
                }
            }
        }
    }

    // Content area
    Rectangle {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom
        color: ribbonToolBar.contentBackgroundColor
        border.width: 1
        border.color: ribbonToolBar.borderColor

        // File tab content
        Row {
            visible: ribbonToolBar.currentTabIndex === 0
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            // File operations group
            RibbonGroup {
                title: ""
                height: parent.height

                RibbonLargeButton {
                    iconSource: "qrc:/icons/new.png"
                    iconText: "📄"
                    text: "New"
                    onClicked: console.log("New file")
                }

                RibbonLargeButton {
                    iconSource: "qrc:/icons/open.png"
                    iconText: "📂"
                    text: "Open"
                    onClicked: ribbonToolBar.openFile()
                }

                RibbonLargeButton {
                    iconSource: "qrc:/icons/import.png"
                    iconText: "📥"
                    text: "Import"
                    onClicked: ribbonToolBar.importModel()
                }

                RibbonLargeButton {
                    iconSource: "qrc:/icons/save.png"
                    iconText: "💾"
                    text: "Save"
                    onClicked: ribbonToolBar.saveFile()
                }

                RibbonLargeButton {
                    iconSource: "qrc:/icons/export.png"
                    iconText: "📤"
                    text: "Export"
                    onClicked: ribbonToolBar.exportModel()
                }
            }
        }

        // Geometry tab content
        Row {
            visible: ribbonToolBar.currentTabIndex === 1
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            // Create group
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
                    onClicked: console.log("Point Replace")
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

            // Modify group
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
                    onClicked: console.log("Toggle")
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

            // Edit group
            RibbonGroup {
                title: "Edit"
                height: parent.height

                RibbonLargeButton {
                    iconText: "✂"
                    text: "Trim"
                    onClicked: console.log("Trim")
                }

                RibbonLargeButton {
                    iconText: "⊖"
                    text: "Offset"
                    onClicked: console.log("Offset")
                }

                RibbonLargeButton {
                    iconText: "◉"
                    text: "Fill"
                    onClicked: console.log("Fill")
                }
            }
        }

        // Mesh tab content
        Row {
            visible: ribbonToolBar.currentTabIndex === 2
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "Mesh Operations"
                height: parent.height

                RibbonLargeButton {
                    iconText: "◇"
                    text: "Generate\nMesh"
                    onClicked: console.log("Generate Mesh")
                }

                RibbonLargeButton {
                    iconText: "△"
                    text: "Refine"
                    onClicked: console.log("Refine Mesh")
                }

                RibbonLargeButton {
                    iconText: "▽"
                    text: "Simplify"
                    onClicked: console.log("Simplify Mesh")
                }

                RibbonLargeButton {
                    iconText: "⬡"
                    text: "Smooth"
                    onClicked: console.log("Smooth Mesh")
                }
            }
        }

        // Interaction tab content
        Row {
            visible: ribbonToolBar.currentTabIndex === 3
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "View"
                height: parent.height

                RibbonLargeButton {
                    iconText: "⟳"
                    text: "Rotate"
                    onClicked: console.log("Rotate View")
                }

                RibbonLargeButton {
                    iconText: "⤡"
                    text: "Pan"
                    onClicked: console.log("Pan View")
                }

                RibbonLargeButton {
                    iconText: "🔍"
                    text: "Zoom"
                    onClicked: console.log("Zoom View")
                }

                RibbonLargeButton {
                    iconText: "⬚"
                    text: "Fit All"
                    onClicked: console.log("Fit All")
                }
            }

            RibbonGroup {
                title: "Selection"
                height: parent.height

                RibbonLargeButton {
                    iconText: "☝"
                    text: "Pick"
                    onClicked: console.log("Pick")
                }

                RibbonLargeButton {
                    iconText: "▢"
                    text: "Box\nSelect"
                    onClicked: console.log("Box Select")
                }
            }
        }

        // General tab content
        Row {
            visible: ribbonToolBar.currentTabIndex === 4
            anchors.fill: parent
            anchors.margins: 5
            spacing: 2

            RibbonGroup {
                title: "Settings"
                height: parent.height

                RibbonLargeButton {
                    iconText: "⚙"
                    text: "Options"
                    onClicked: console.log("Options")
                }

                RibbonLargeButton {
                    iconText: "🎨"
                    text: "Theme"
                    onClicked: console.log("Theme")
                }

                RibbonLargeButton {
                    iconText: "❓"
                    text: "Help"
                    onClicked: console.log("Help")
                }
            }
        }
    }
}
