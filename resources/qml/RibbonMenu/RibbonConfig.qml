/**
 * @file RibbonConfig.qml
 * @brief Configuration data for the ribbon toolbar
 *
 * Defines tab structure, groups, and action items for the ribbon menu.
 * Serves as the single source of truth for ribbon UI configuration.
 */
pragma ComponentBehavior: Bound
import QtQuick

QtObject {
    id: ribbonConfig

    /// Base path for ribbon icon resources
    readonly property string iconBasePath: "qrc:/opengeolab/resources/icons/"

    /// Tab definitions with groups and action items
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
                                id: "addBox",
                                text: qsTr("Box"),
                                iconSource: ribbonConfig.iconBasePath + "box.svg",
                                tooltip: qsTr("Create Box")
                            },
                            {
                                type: "button",
                                id: "addCylinder",
                                text: qsTr("Cylinder"),
                                iconSource: ribbonConfig.iconBasePath + "cylinder.svg",
                                tooltip: qsTr("Create Cylinder")
                            },
                            {
                                type: "button",
                                id: "addSphere",
                                text: qsTr("Sphere"),
                                iconSource: ribbonConfig.iconBasePath + "sphere.svg",
                                tooltip: qsTr("Create Sphere")
                            },
                            {
                                type: "button",
                                id: "addTorus",
                                text: qsTr("Torus"),
                                iconSource: ribbonConfig.iconBasePath + "torus.svg",
                                tooltip: qsTr("Create Torus")
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
                                iconSource: ribbonConfig.iconBasePath + "trim.svg",
                                tooltip: qsTr("Trim Geometry")
                            },
                            {
                                type: "button",
                                id: "offset",
                                text: qsTr("Offset"),
                                iconSource: ribbonConfig.iconBasePath + "offset.svg",
                                tooltip: qsTr("Offset Geometry")
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
                                iconSource: iconBasePath + "smooth_mesh.svg",
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
                                iconSource: iconBasePath + "ai_suggestion.svg",
                                tooltip: qsTr("AI Suggest")
                            },
                            {
                                type: "button",
                                id: "aiChat",
                                text: qsTr("Chat"),
                                iconSource: iconBasePath + "ai_chat.svg",
                                tooltip: qsTr("AI Chat")
                            }
                        ]
                    }
                ]
            }
        ])
}
