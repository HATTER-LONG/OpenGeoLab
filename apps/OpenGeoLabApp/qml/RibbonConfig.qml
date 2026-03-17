pragma ComponentBehavior: Bound

import QtQml

QtObject {
    readonly property var tabs: [qsTr("Geometry"), qsTr("Mesh"), qsTr("AI")]
    readonly property var groupsModel: [[
            {
                "title": qsTr("Create"),
                "actionKeys": ["addBox", "addCylinder", "addSphere", "addTorus"]
            },
            {
                "title": qsTr("Modify"),
                "actionKeys": ["trim", "offset"]
            },
            {
                "title": qsTr("Inspect"),
                "actionKeys": ["queryGeometry"]
            }
        ], [
            {
                "title": qsTr("Mesh"),
                "actionKeys": ["generateMesh", "smoothMesh"]
            },
            {
                "title": qsTr("Inspect"),
                "actionKeys": ["queryMesh"]
            }
        ], [
            {
                "title": qsTr("Assist"),
                "actionKeys": ["aiSuggest", "aiChat"]
            }
        ]]
}
