pragma ComponentBehavior: Bound

import QtQml

QtObject {
    readonly property var tabs: [qsTr("Geometry"), qsTr("Mesh"), qsTr("AI")]
    readonly property var groupsModel: [[
            {
                "title": qsTr("Create"),
                "actions": [
                    {
                        "key": "addBox",
                        "title": qsTr("Box"),
                        "icon": "box",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    },
                    {
                        "key": "addCylinder",
                        "title": qsTr("Cylinder"),
                        "icon": "cylinder",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    },
                    {
                        "key": "addSphere",
                        "title": qsTr("Sphere"),
                        "icon": "sphere",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    },
                    {
                        "key": "addTorus",
                        "title": qsTr("Torus"),
                        "icon": "torus",
                        "accentOne": "accentA",
                        "accentTwo": "accentB"
                    }
                ]
            },
            {
                "title": qsTr("Modify"),
                "actions": [
                    {
                        "key": "trim",
                        "title": qsTr("Trim"),
                        "icon": "trim",
                        "accentOne": "accentD",
                        "accentTwo": "accentC"
                    },
                    {
                        "key": "offset",
                        "title": qsTr("Offset"),
                        "icon": "offset",
                        "accentOne": "accentD",
                        "accentTwo": "accentD"
                    }
                ]
            },
            {
                "title": qsTr("Inspect"),
                "actions": [
                    {
                        "key": "queryGeometry",
                        "title": qsTr("Query"),
                        "icon": "query",
                        "accentOne": "accentE",
                        "accentTwo": "accentB"
                    }
                ]
            }
        ], [
            {
                "title": qsTr("Mesh"),
                "actions": [
                    {
                        "key": "generateMesh",
                        "title": qsTr("Generate"),
                        "icon": "mesh",
                        "accentOne": "accentB",
                        "accentTwo": "accentA"
                    },
                    {
                        "key": "smoothMesh",
                        "title": qsTr("Smooth"),
                        "icon": "smoothMesh",
                        "accentOne": "accentB",
                        "accentTwo": "accentA"
                    }
                ]
            },
            {
                "title": qsTr("Inspect"),
                "actions": [
                    {
                        "key": "queryMesh",
                        "title": qsTr("Query"),
                        "icon": "query",
                        "accentOne": "accentC",
                        "accentTwo": "accentA"
                    }
                ]
            }
        ], [
            {
                "title": qsTr("Assist"),
                "actions": [
                    {
                        "key": "aiSuggest",
                        "title": qsTr("Suggest"),
                        "icon": "aiSuggest",
                        "accentOne": "accentE",
                        "accentTwo": "accentA"
                    },
                    {
                        "key": "aiChat",
                        "title": qsTr("Chat"),
                        "icon": "aiChat",
                        "accentOne": "accentE",
                        "accentTwo": "accentA"
                    }
                ]
            }
        ]]

    function groupsForTab(tabIndex) {
        if (tabIndex < 0 || tabIndex >= groupsModel.length) {
            return [];
        }

        return groupsModel[tabIndex];
    }
}
