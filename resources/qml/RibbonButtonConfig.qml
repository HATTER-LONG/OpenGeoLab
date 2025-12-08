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
    // GEOMETRY TAB
    // ========================================================================
    readonly property var geometryTab: [
        {
            title: "Create",
            buttons: [
                {
                    id: "addPoint",
                    iconSource: iconBasePath + "point.svg",
                    text: "Point"
                },
                {
                    id: "pointReplace",
                    iconSource: iconBasePath + "point_replace.svg",
                    text: "Point\nReplace"
                },
                {
                    type: "separator"
                },
                {
                    id: "addPlane",
                    iconSource: iconBasePath + "plane.svg",
                    text: "Plane"
                },
                {
                    id: "addLine",
                    iconSource: iconBasePath + "line.svg",
                    text: "Line"
                },
                {
                    id: "addBox",
                    iconSource: iconBasePath + "box.svg",
                    text: "Box"
                }
            ]
        },
        {
            title: "Modify",
            buttons: [
                {
                    id: "toggleRelease",
                    iconSource: iconBasePath + "release.svg",
                    text: "Release"
                },
                {
                    id: "toggle",
                    iconSource: iconBasePath + "toggle.svg",
                    text: "Toggle"
                },
                {
                    id: "toggleStitch",
                    iconSource: iconBasePath + "stitch.svg",
                    text: "Stitch"
                },
                {
                    id: "tangentExtend",
                    iconSource: iconBasePath + "tangent_extend.svg",
                    text: "Tangent\nExtend"
                },
                {
                    id: "projectGeometry",
                    iconSource: iconBasePath + "project.svg",
                    text: "Project"
                }
            ]
        },
        {
            title: "Edit",
            buttons: [
                {
                    id: "trim",
                    iconSource: iconBasePath + "trim.svg",
                    text: "Trim"
                },
                {
                    id: "offset",
                    iconSource: iconBasePath + "offset.svg",
                    text: "Offset"
                },
                {
                    id: "fill",
                    iconSource: iconBasePath + "fill.svg",
                    text: "Fill"
                },
                {
                    id: "surfaceExtend",
                    iconSource: iconBasePath + "surface_extend.svg",
                    text: "Surface\nExtend"
                },
                {
                    id: "surfaceMerge",
                    iconSource: iconBasePath + "surface_merge.svg",
                    text: "Surface\nMerge"
                },
                {
                    id: "suppress",
                    iconSource: iconBasePath + "suppress.svg",
                    text: "Suppress"
                },
                {
                    id: "split",
                    iconSource: iconBasePath + "split.svg",
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
                    iconSource: iconBasePath + "generate_mesh.svg",
                    text: "Generate\nMesh"
                },
                {
                    id: "refineMesh",
                    iconSource: iconBasePath + "refine_mesh.svg",
                    text: "Refine"
                },
                {
                    id: "simplifyMesh",
                    iconSource: iconBasePath + "simplify_mesh.svg",
                    text: "Simplify"
                },
                {
                    id: "smoothMesh",
                    iconSource: iconBasePath + "smooth_mesh.svg",
                    text: "Smooth"
                }
            ]
        },
        {
            title: "Quality",
            buttons: [
                {
                    id: "checkMesh",
                    iconSource: iconBasePath + "check_mesh.svg",
                    text: "Check"
                },
                {
                    id: "repairMesh",
                    iconSource: iconBasePath + "repair_mesh.svg",
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
                    iconSource: iconBasePath + "rotate_view.svg",
                    text: "Rotate"
                },
                {
                    id: "panView",
                    iconSource: iconBasePath + "pan_view.svg",
                    text: "Pan"
                },
                {
                    id: "zoomView",
                    iconSource: iconBasePath + "zoom_view.svg",
                    text: "Zoom"
                },
                {
                    id: "fitAll",
                    iconSource: iconBasePath + "fit_all.svg",
                    text: "Fit All"
                }
            ]
        },
        {
            title: "Selection",
            buttons: [
                {
                    id: "pick",
                    iconSource: iconBasePath + "pick.svg",
                    text: "Pick"
                },
                {
                    id: "boxSelect",
                    iconSource: iconBasePath + "box_select.svg",
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
                    iconSource: iconBasePath + "options.svg",
                    text: "Options"
                },
                {
                    id: "theme",
                    iconSource: iconBasePath + "theme.svg",
                    text: "Theme"
                },
                {
                    id: "help",
                    iconSource: iconBasePath + "help.svg",
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
