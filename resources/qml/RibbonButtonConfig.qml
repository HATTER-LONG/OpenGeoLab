pragma ComponentBehavior: Bound
import QtQuick

/**
 * @file RibbonButtonConfig.qml
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
 * - iconSource: Path to SVG icon resource (e.g., iconBasePath + "icon.svg")
 * - text: Button label (use \n for multi-line)
 * - tooltip: Optional tooltip text
 * - type: "button" (default) or "separator"
 */
QtObject {
    id: buttonConfig

    // Icon base path - matches RESOURCE_PREFIX in CMakeLists.txt
    readonly property string iconBasePath: "qrc:/scenegraph/opengeolab/resources/icons/"

    // ========================================================================
    // GEOMETRY TAB - Geometry Modeling Tools
    // ========================================================================
    readonly property var geometryTab: [
        {
            title: "Create",
            buttons: [
                {
                    id: "addPoint",
                    iconSource: iconBasePath + "point.svg",
                    text: "Point",
                    tooltip: "Create a point"
                },
                {
                    id: "addLine",
                    iconSource: iconBasePath + "line.svg",
                    text: "Line",
                    tooltip: "Create a line"
                },
                {
                    id: "addPlane",
                    iconSource: iconBasePath + "plane.svg",
                    text: "Plane",
                    tooltip: "Create a plane"
                },
                {
                    id: "addBox",
                    iconSource: iconBasePath + "box.svg",
                    text: "Box",
                    tooltip: "Create a box"
                }
            ]
        },
        {
            title: "Transform",
            buttons: [
                {
                    id: "toggleRelease",
                    iconSource: iconBasePath + "release.svg",
                    text: "Extrude",
                    tooltip: "Extrude geometry"
                },
                {
                    id: "toggle",
                    iconSource: iconBasePath + "toggle.svg",
                    text: "Revolve",
                    tooltip: "Revolve geometry"
                },
                {
                    id: "toggleStitch",
                    iconSource: iconBasePath + "stitch.svg",
                    text: "Boolean",
                    tooltip: "Boolean operations"
                }
            ]
        },
        {
            title: "Edit",
            buttons: [
                {
                    id: "trim",
                    iconSource: iconBasePath + "trim.svg",
                    text: "Trim",
                    tooltip: "Trim geometry"
                },
                {
                    id: "offset",
                    iconSource: iconBasePath + "offset.svg",
                    text: "Offset",
                    tooltip: "Offset geometry"
                },
                {
                    id: "fill",
                    iconSource: iconBasePath + "fill.svg",
                    text: "Fill",
                    tooltip: "Fill region"
                },
                {
                    id: "split",
                    iconSource: iconBasePath + "split.svg",
                    text: "Split",
                    tooltip: "Split geometry"
                }
            ]
        }
    ]

    // ========================================================================
    // MESH TAB - Meshing Tools
    // ========================================================================
    readonly property var meshTab: [
        {
            title: "Mesh Generation",
            buttons: [
                {
                    id: "generateMesh",
                    iconSource: iconBasePath + "generate_mesh.svg",
                    text: "Auto\nMesh",
                    tooltip: "Automatic mesh generation"
                },
                {
                    id: "refineMesh",
                    iconSource: iconBasePath + "refine_mesh.svg",
                    text: "Refine",
                    tooltip: "Refine mesh"
                },
                {
                    id: "simplifyMesh",
                    iconSource: iconBasePath + "simplify_mesh.svg",
                    text: "Coarsen",
                    tooltip: "Coarsen mesh"
                }
            ]
        },
        {
            title: "Mesh Type",
            buttons: [
                {
                    id: "smoothMesh",
                    iconSource: iconBasePath + "smooth_mesh.svg",
                    text: "Triangle",
                    tooltip: "Triangle mesh"
                },
                {
                    id: "checkMesh",
                    iconSource: iconBasePath + "check_mesh.svg",
                    text: "Quad",
                    tooltip: "Quadrilateral mesh"
                },
                {
                    id: "repairMesh",
                    iconSource: iconBasePath + "repair_mesh.svg",
                    text: "Tetra",
                    tooltip: "Tetrahedral mesh"
                }
            ]
        },
        {
            title: "Quality",
            buttons: [
                {
                    id: "checkMesh",
                    iconSource: iconBasePath + "check_mesh.svg",
                    text: "Check",
                    tooltip: "Check mesh quality"
                },
                {
                    id: "repairMesh",
                    iconSource: iconBasePath + "repair_mesh.svg",
                    text: "Repair",
                    tooltip: "Repair mesh"
                }
            ]
        }
    ]

    // ========================================================================
    // AI TAB - AI-Assisted Design Tools
    // ========================================================================
    readonly property var aiTab: [
        {
            title: "AI Assist",
            buttons: [
                {
                    id: "aiSuggest",
                    iconSource: iconBasePath + "ai_suggest.svg",
                    text: "Smart\nSuggest",
                    tooltip: "Smart geometry modeling suggestions"
                },
                {
                    id: "aiOptimize",
                    iconSource: iconBasePath + "ai_optimize.svg",
                    text: "Auto\nOptimize",
                    tooltip: "Automatic mesh optimization"
                },
                {
                    id: "aiExplore",
                    iconSource: iconBasePath + "ai_explore.svg",
                    text: "Design\nExplore",
                    tooltip: "Design space exploration"
                }
            ]
        },
        {
            title: "AI Chat",
            buttons: [
                {
                    id: "aiChat",
                    iconSource: iconBasePath + "ai_chat.svg",
                    text: "AI\nAssistant",
                    tooltip: "AI design assistant chat"
                }
            ]
        },
        {
            title: "Settings",
            buttons: [
                {
                    id: "options",
                    iconSource: iconBasePath + "options.svg",
                    text: "Options",
                    tooltip: "Application settings"
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
            return aiTab;
        default:
            return [];
        }
    }

    // Get all button IDs (useful for signal generation)
    function getAllButtonIds(): var {
        let ids = [];
        let tabs = [geometryTab, meshTab, aiTab];
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
