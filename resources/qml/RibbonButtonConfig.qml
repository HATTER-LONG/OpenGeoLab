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
    // GEOMETRY TAB - 几何建模工具
    // ========================================================================
    readonly property var geometryTab: [
        {
            title: "Create",
            buttons: [
                {
                    id: "addPoint",
                    iconSource: iconBasePath + "point.svg",
                    text: "Point",
                    tooltip: "创建点"
                },
                {
                    id: "addLine",
                    iconSource: iconBasePath + "line.svg",
                    text: "Line",
                    tooltip: "创建线"
                },
                {
                    id: "addPlane",
                    iconSource: iconBasePath + "plane.svg",
                    text: "Plane",
                    tooltip: "创建平面"
                },
                {
                    id: "addBox",
                    iconSource: iconBasePath + "box.svg",
                    text: "Box",
                    tooltip: "创建立方体"
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
                    tooltip: "拉伸几何体"
                },
                {
                    id: "toggle",
                    iconSource: iconBasePath + "toggle.svg",
                    text: "Revolve",
                    tooltip: "旋转几何体"
                },
                {
                    id: "toggleStitch",
                    iconSource: iconBasePath + "stitch.svg",
                    text: "Boolean",
                    tooltip: "布尔运算"
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
                    tooltip: "修剪几何体"
                },
                {
                    id: "offset",
                    iconSource: iconBasePath + "offset.svg",
                    text: "Offset",
                    tooltip: "偏移几何体"
                },
                {
                    id: "fill",
                    iconSource: iconBasePath + "fill.svg",
                    text: "Fill",
                    tooltip: "填充区域"
                },
                {
                    id: "split",
                    iconSource: iconBasePath + "split.svg",
                    text: "Split",
                    tooltip: "分割几何体"
                }
            ]
        }
    ]

    // ========================================================================
    // MESH TAB - 网格划分工具
    // ========================================================================
    readonly property var meshTab: [
        {
            title: "Mesh Generation",
            buttons: [
                {
                    id: "generateMesh",
                    iconSource: iconBasePath + "generate_mesh.svg",
                    text: "Auto\nMesh",
                    tooltip: "自动网格划分"
                },
                {
                    id: "refineMesh",
                    iconSource: iconBasePath + "refine_mesh.svg",
                    text: "Refine",
                    tooltip: "网格加密"
                },
                {
                    id: "simplifyMesh",
                    iconSource: iconBasePath + "simplify_mesh.svg",
                    text: "Coarsen",
                    tooltip: "网格粗化"
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
                    tooltip: "三角形网格"
                },
                {
                    id: "checkMesh",
                    iconSource: iconBasePath + "check_mesh.svg",
                    text: "Quad",
                    tooltip: "四边形网格"
                },
                {
                    id: "repairMesh",
                    iconSource: iconBasePath + "repair_mesh.svg",
                    text: "Tetra",
                    tooltip: "四面体网格"
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
                    tooltip: "网格质量检查"
                },
                {
                    id: "repairMesh",
                    iconSource: iconBasePath + "repair_mesh.svg",
                    text: "Repair",
                    tooltip: "网格修复"
                }
            ]
        }
    ]

    // ========================================================================
    // AI TAB - AI 辅助设计工具
    // ========================================================================
    readonly property var aiTab: [
        {
            title: "AI Assist",
            buttons: [
                {
                    id: "aiSuggest",
                    iconSource: iconBasePath + "ai_suggest.svg",
                    text: "Smart\nSuggest",
                    tooltip: "智能几何建模建议"
                },
                {
                    id: "aiOptimize",
                    iconSource: iconBasePath + "ai_optimize.svg",
                    text: "Auto\nOptimize",
                    tooltip: "自动网格优化"
                },
                {
                    id: "aiExplore",
                    iconSource: iconBasePath + "ai_explore.svg",
                    text: "Design\nExplore",
                    tooltip: "设计空间探索"
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
                    tooltip: "AI 设计助手对话"
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
