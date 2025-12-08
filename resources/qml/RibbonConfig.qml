pragma ComponentBehavior: Bound
import QtQuick

/**
 * @brief Configuration for Ribbon toolbar tabs and groups
 *
 * This component provides the configuration for all tabs and their content.
 * Modify this file to add/remove/reorder toolbar buttons.
 */
QtObject {
    id: config

    // Tab definitions (excluding File which is a menu)
    readonly property var tabs: [
        {
            name: "Geometry",
            index: 0
        },
        {
            name: "Mesh",
            index: 1
        },
        {
            name: "Interaction",
            index: 2
        },
        {
            name: "General",
            index: 3
        }
    ]

    // Default selected tab index
    readonly property int defaultTabIndex: 0  // Geometry tab

    // ============================================
    // GEOMETRY TAB CONFIGURATION
    // ============================================
    readonly property var geometryGroups: [
        {
            title: "Create",
            buttons: [
                {
                    iconText: "•",
                    text: "Point",
                    action: "addPoint"
                },
                {
                    iconText: "⊕",
                    text: "Point\nReplace",
                    action: "pointReplace"
                },
                {
                    type: "separator"
                },
                {
                    iconText: "▭",
                    text: "Plane",
                    action: "addPlane"
                },
                {
                    iconText: "╱",
                    text: "Line",
                    action: "addLine"
                },
                {
                    iconText: "☐",
                    text: "Box",
                    action: "addBox"
                }
            ]
        },
        {
            title: "Modify",
            buttons: [
                {
                    iconText: "⇥",
                    text: "Release",
                    action: "toggleRelease"
                },
                {
                    iconText: "⊞",
                    text: "Toggle",
                    action: "toggle"
                },
                {
                    iconText: "⊟",
                    text: "Stitch",
                    action: "toggleStitch"
                },
                {
                    iconText: "↗",
                    text: "Tangent\nExtend",
                    action: "tangentExtend"
                },
                {
                    iconText: "⊡",
                    text: "Project",
                    action: "projectGeometry"
                }
            ]
        },
        {
            title: "Edit",
            buttons: [
                {
                    iconText: "✂",
                    text: "Trim",
                    action: "trim"
                },
                {
                    iconText: "⊖",
                    text: "Offset",
                    action: "offset"
                },
                {
                    iconText: "◉",
                    text: "Fill",
                    action: "fill"
                },
                {
                    iconText: "↔",
                    text: "Surface\nExtend",
                    action: "surfaceExtend"
                },
                {
                    iconText: "⊘",
                    text: "Suppress",
                    action: "suppress"
                },
                {
                    iconText: "⊕",
                    text: "Surface\nMerge",
                    action: "surfaceMerge"
                },
                {
                    iconText: "✂",
                    text: "Split",
                    action: "split"
                }
            ]
        }
    ]

    // ============================================
    // MESH TAB CONFIGURATION
    // ============================================
    readonly property var meshGroups: [
        {
            title: "Mesh Operations",
            buttons: [
                {
                    iconText: "◇",
                    text: "Generate\nMesh",
                    action: "generateMesh"
                },
                {
                    iconText: "△",
                    text: "Refine",
                    action: "refineMesh"
                },
                {
                    iconText: "▽",
                    text: "Simplify",
                    action: "simplifyMesh"
                },
                {
                    iconText: "⬡",
                    text: "Smooth",
                    action: "smoothMesh"
                }
            ]
        },
        {
            title: "Quality",
            buttons: [
                {
                    iconText: "✓",
                    text: "Check",
                    action: "checkMesh"
                },
                {
                    iconText: "🔧",
                    text: "Repair",
                    action: "repairMesh"
                }
            ]
        }
    ]

    // ============================================
    // INTERACTION TAB CONFIGURATION
    // ============================================
    readonly property var interactionGroups: [
        {
            title: "View",
            buttons: [
                {
                    iconText: "⟳",
                    text: "Rotate",
                    action: "rotateView"
                },
                {
                    iconText: "⤡",
                    text: "Pan",
                    action: "panView"
                },
                {
                    iconText: "🔍",
                    text: "Zoom",
                    action: "zoomView"
                },
                {
                    iconText: "⬚",
                    text: "Fit All",
                    action: "fitAll"
                }
            ]
        },
        {
            title: "Selection",
            buttons: [
                {
                    iconText: "☝",
                    text: "Pick",
                    action: "pick"
                },
                {
                    iconText: "▢",
                    text: "Box\nSelect",
                    action: "boxSelect"
                }
            ]
        }
    ]

    // ============================================
    // GENERAL TAB CONFIGURATION
    // ============================================
    readonly property var generalGroups: [
        {
            title: "Settings",
            buttons: [
                {
                    iconText: "⚙",
                    text: "Options",
                    action: "options"
                },
                {
                    iconText: "🎨",
                    text: "Theme",
                    action: "theme"
                },
                {
                    iconText: "❓",
                    text: "Help",
                    action: "help"
                }
            ]
        }
    ]

    // Get groups for a specific tab index
    function getGroupsForTab(tabIndex: int): var {
        switch (tabIndex) {
        case 0:
            return geometryGroups;
        case 1:
            return meshGroups;
        case 2:
            return interactionGroups;
        case 3:
            return generalGroups;
        default:
            return [];
        }
    }
}
