pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: ribbonConfig

    readonly property string iconBasePath: "qrc:/scenegraph/opengeolab/resources/icons/"

    readonly property var tabs: ([
            {
                id: "geometry",
                title: qsTr("Geometry"),
                groups: [
                    {
                        title: qsTr("Create"),
                        items: [
                            {
                                type: "button",
                                id: "addPoint",
                                text: qsTr("Point"),
                                iconSource: iconBasePath + "point.svg",
                                tooltip: qsTr("Add Point")
                            },
                            {
                                type: "button",
                                id: "addLine",
                                text: qsTr("Line"),
                                iconSource: iconBasePath + "line.svg",
                                tooltip: qsTr("Add Line")
                            },
                            {
                                type: "button",
                                id: "addBox",
                                text: qsTr("Box"),
                                iconSource: iconBasePath + "box.svg",
                                tooltip: qsTr("Add Box")
                            }
                        ]
                    },
                    {
                        title: qsTr("Modify"),
                        items: [
                            {
                                type: "button",
                                id: "trim",
                                text: qsTr("Trim"),
                                iconSource: iconBasePath + "trim.svg",
                                tooltip: qsTr("Trim")
                            },
                            {
                                type: "button",
                                id: "offset",
                                text: qsTr("Offset"),
                                iconSource: iconBasePath + "offset.svg",
                                tooltip: qsTr("Offset")
                            }
                        ]
                    }
                ]
            },
            {
                id: "mesh",
                title: qsTr("Mesh"),
                groups: [
                    {
                        title: qsTr("Mesh"),
                        items: [
                            {
                                type: "button",
                                id: "generateMesh",
                                text: qsTr("Generate"),
                                iconSource: iconBasePath + "mesh.svg",
                                tooltip: qsTr("Generate Mesh")
                            },
                            {
                                type: "button",
                                id: "smoothMesh",
                                text: qsTr("Smooth"),
                                iconSource: iconBasePath + "smooth.svg",
                                tooltip: qsTr("Smooth Mesh")
                            }
                        ]
                    }
                ]
            },
            {
                id: "ai",
                title: qsTr("AI"),
                groups: [
                    {
                        title: qsTr("Assist"),
                        items: [
                            {
                                type: "button",
                                id: "aiSuggest",
                                text: qsTr("Suggest"),
                                iconSource: iconBasePath + "ai.svg",
                                tooltip: qsTr("AI Suggest")
                            },
                            {
                                type: "button",
                                id: "aiChat",
                                text: qsTr("Chat"),
                                iconSource: iconBasePath + "chat.svg",
                                tooltip: qsTr("AI Chat")
                            }
                        ]
                    }
                ]
            }
        ])

    function validate(): bool {
        const seen = {};
        let ok = true;

        for (let t = 0; t < tabs.length; t++) {
            const tab = tabs[t];
            if (!tab || !tab.groups) {
                console.error("[RibbonConfig] Invalid tab at index:", t);
                ok = false;
                continue;
            }

            for (let g = 0; g < tab.groups.length; g++) {
                const group = tab.groups[g];
                const items = group.items || [];
                for (let i = 0; i < items.length; i++) {
                    const it = items[i];
                    if (!it)
                        continue;

                    const type = it.type || "button";
                    if (type !== "button" && type !== "separator") {
                        console.warn("[RibbonConfig] Unknown item type:", type, "tab:", tab.id, "group:", group.title);
                    }

                    if (type === "button") {
                        if (!it.id || String(it.id).trim() === "") {
                            console.error("[RibbonConfig] Button missing id:", it, "tab:", tab.id, "group:", group.title);
                            ok = false;
                            continue;
                        }
                        if (seen[it.id]) {
                            console.error("[RibbonConfig] Duplicate action id:", it.id, "previous:", seen[it.id], "current:", {
                                tab: tab.id,
                                group: group.title
                            });
                            ok = false;
                        } else {
                            seen[it.id] = {
                                tab: tab.id,
                                group: group.title
                            };
                        }
                    }
                }
            }
        }
    }
}
