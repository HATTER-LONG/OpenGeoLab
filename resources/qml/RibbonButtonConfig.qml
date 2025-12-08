pragma ComponentBehavior: Bound
import QtQuick

/**
 * @brief Centralized button configuration for Ribbon toolbar
 *
 * This singleton provides all button definitions for the Ribbon toolbar.
 * To add a new button:
 * 1. Add the button config to the appropriate group in the corresponding tab
 * 2. Add the signal name to RibbonToolBar.qml if it's a new action
 * 3. Connect the signal in Main.qml
 *
 * Button config properties:
 * - id: Unique identifier, used as signal name
 * - icon: Unicode icon character
 * - text: Button label (use \n for multi-line)
 * - tooltip: Optional tooltip text
 * - type: "button" (default) or "separator"
 */
QtObject {
    id: buttonConfig

    // ========================================================================
    // GEOMETRY TAB
    // ========================================================================
    readonly property var geometryTab: [
        {
            title: "Create",
            buttons: [
                {
                    id: "addPoint",
                    icon: "•",
                    text: "Point"
                },
                {
                    id: "pointReplace",
                    icon: "⊕",
                    text: "Point\nReplace"
                },
                {
                    type: "separator"
                },
                {
                    id: "addPlane",
                    icon: "▭",
                    text: "Plane"
                },
                {
                    id: "addLine",
                    icon: "╱",
                    text: "Line"
                },
                {
                    id: "addBox",
                    icon: "☐",
                    text: "Box"
                }
            ]
        },
        {
            title: "Modify",
            buttons: [
                {
                    id: "toggleRelease",
                    icon: "⇥",
                    text: "Release"
                },
                {
                    id: "toggle",
                    icon: "⊞",
                    text: "Toggle"
                },
                {
                    id: "toggleStitch",
                    icon: "⊟",
                    text: "Stitch"
                },
                {
                    id: "tangentExtend",
                    icon: "↗",
                    text: "Tangent\nExtend"
                },
                {
                    id: "projectGeometry",
                    icon: "⊡",
                    text: "Project"
                }
            ]
        },
        {
            title: "Edit",
            buttons: [
                {
                    id: "trim",
                    icon: "✂",
                    text: "Trim"
                },
                {
                    id: "offset",
                    icon: "⊖",
                    text: "Offset"
                },
                {
                    id: "fill",
                    icon: "◉",
                    text: "Fill"
                },
                {
                    id: "surfaceExtend",
                    icon: "↔",
                    text: "Surface\nExtend"
                },
                {
                    id: "surfaceMerge",
                    icon: "⊗",
                    text: "Surface\nMerge"
                },
                {
                    id: "suppress",
                    icon: "⊘",
                    text: "Suppress"
                },
                {
                    id: "split",
                    icon: "⫽",
                    text: "Split"
                }
            ]
        }
    ]

    // ========================================================================
    // MESH TAB
    // ========================================================================
    readonly property var meshTab: [
        {
            title: "Mesh Operations",
            buttons: [
                {
                    id: "generateMesh",
                    icon: "◇",
                    text: "Generate\nMesh"
                },
                {
                    id: "refineMesh",
                    icon: "△",
                    text: "Refine"
                },
                {
                    id: "simplifyMesh",
                    icon: "▽",
                    text: "Simplify"
                },
                {
                    id: "smoothMesh",
                    icon: "⬡",
                    text: "Smooth"
                }
            ]
        },
        {
            title: "Quality",
            buttons: [
                {
                    id: "checkMesh",
                    icon: "✓",
                    text: "Check"
                },
                {
                    id: "repairMesh",
                    icon: "🔧",
                    text: "Repair"
                }
            ]
        }
    ]

    // ========================================================================
    // INTERACTION TAB
    // ========================================================================
    readonly property var interactionTab: [
        {
            title: "View",
            buttons: [
                {
                    id: "rotateView",
                    icon: "⟳",
                    text: "Rotate"
                },
                {
                    id: "panView",
                    icon: "⤡",
                    text: "Pan"
                },
                {
                    id: "zoomView",
                    icon: "🔍",
                    text: "Zoom"
                },
                {
                    id: "fitAll",
                    icon: "⬚",
                    text: "Fit All"
                }
            ]
        },
        {
            title: "Selection",
            buttons: [
                {
                    id: "pick",
                    icon: "☝",
                    text: "Pick"
                },
                {
                    id: "boxSelect",
                    icon: "▢",
                    text: "Box\nSelect"
                }
            ]
        }
    ]

    // ========================================================================
    // GENERAL TAB
    // ========================================================================
    readonly property var generalTab: [
        {
            title: "Settings",
            buttons: [
                {
                    id: "options",
                    icon: "⚙",
                    text: "Options"
                },
                {
                    id: "theme",
                    icon: "🎨",
                    text: "Theme"
                },
                {
                    id: "help",
                    icon: "❓",
                    text: "Help"
                }
            ]
        }
    ]

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    // Get tab configuration by index
    function getTabConfig(tabIndex: int): var {
        switch (tabIndex) {
        case 0:
            return geometryTab;
        case 1:
            return meshTab;
        case 2:
            return interactionTab;
        case 3:
            return generalTab;
        default:
            return [];
        }
    }

    // Get all button IDs (useful for signal generation)
    function getAllButtonIds(): var {
        let ids = [];
        let tabs = [geometryTab, meshTab, interactionTab, generalTab];
        for (let tab of tabs) {
            for (let group of tab) {
                for (let btn of group.buttons) {
                    if (btn.type !== "separator" && btn.id) {
                        ids.push(btn.id);
                    }
                }
            }
        }
        return ids;
    }
}
