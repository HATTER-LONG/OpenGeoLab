pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../theme"
import "../components"

/// @brief Left sidebar — shape explorer with real-time list.
Item {
    id: root

    required property AppTheme theme

    property var shapeList: []
    property var nodeMap: ({})

    implicitWidth: 280

    function fetchShapeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "geometry",
            action: "list_shapes",
            param: {},
            mute: true
        }))
    }

    function fetchNodeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: "list_nodes",
            param: {},
            mute: true
        }))
    }

    function toggleGeoVisibility(shapeId) {
        let newVisible = !root.geoVisible(shapeId)

        // Optimistic local update — prevents stale reads on rapid double-click
        let map = root.nodeMap
        if (!(shapeId in map)) {
            map[shapeId] = { visible: newVisible, meshVisible: false }
        } else {
            map[shapeId].visible = newVisible
        }
        root.nodeMap = map

        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: "set_visibility",
            param: {
                nodes: [{ nodeId: shapeId, visible: newVisible }]
            },
            mute: true
        }))
    }

    function toggleMeshVisibility(shapeId) {
        // Mesh visibility is a local UI concern (no scene graph involvement yet)
        let map = root.nodeMap
        if (!(shapeId in map)) {
            map[shapeId] = { visible: true, meshVisible: false }
        }
        map[shapeId].meshVisible = !map[shapeId].meshVisible
        root.nodeMap = map
    }

    function geoVisible(shapeId) {
        if (shapeId in root.nodeMap && root.nodeMap[shapeId].visible !== undefined) {
            return root.nodeMap[shapeId].visible
        }
        return true
    }

    function meshVisible(shapeId) {
        return (shapeId in root.nodeMap)
               && root.nodeMap[shapeId].meshVisible === true
    }

    Timer {
        id: refreshTimer
        interval: 100
        repeat: false
        onTriggered: root.fetchShapeList()
    }

    Timer {
        id: sceneRefreshTimer
        interval: 100
        repeat: false
        onTriggered: root.fetchNodeList()
    }

    Connections {
        target: ModuleDataNotifier
        function onGeometryDataChanged() {
            refreshTimer.restart()
        }
        function onSceneDataChanged() {
            sceneRefreshTimer.restart()
        }
    }

    Connections {
        target: RequestService
        function onResponseReady(responseJson, muted) {
            try {
                const resp = JSON.parse(responseJson)
                if (resp.action === "list_shapes" && resp.ok) {
                    root.shapeList = resp.shapes || []
                }
                if (resp.action === "list_nodes" && resp.ok) {
                    let map = {}
                    let oldMap = root.nodeMap
                    for (let i = 0; i < resp.nodes.length; ++i) {
                        let n = resp.nodes[i]
                        map[n.nodeId] = {
                            visible: n.visible,
                            meshVisible: (n.nodeId in oldMap)
                                         ? oldMap[n.nodeId].meshVisible === true
                                         : false
                        }
                    }
                    root.nodeMap = map
                }
            } catch (e) {
                // Ignore non-JSON or unrelated responses
            }
        }
    }

    Component.onCompleted: {
        fetchShapeList()
        fetchNodeList()
    }

    SectionCard {
        anchors.fill: parent
        anchors.margins: 0
        theme: root.theme
        title: qsTr("Scene Explorer")
        subtitle: ""

        // Empty state
        Column {
            visible: root.shapeList.length === 0
            width: parent.width
            spacing: 8
            topPadding: 20

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "\uD83D\uDCE6"
                font.pixelSize: 32
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No geometry loaded.")
                font.pixelSize: 13
                color: root.theme.textTertiary
                wrapMode: Text.WordWrap
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Create a shape or import a model to get started.")
                font.pixelSize: 12
                color: root.theme.textTertiary
                wrapMode: Text.WordWrap
            }
        }

        // Shape list
        ListView {
            visible: root.shapeList.length > 0
            width: parent.width
            height: contentHeight
            model: root.shapeList
            clip: true
            spacing: 2
            interactive: false

            delegate: ShapeListItem {
                required property var modelData
                required property int index

                width: ListView.view.width
                theme: root.theme
                shapeId: modelData.shapeId ?? 0
                name: modelData.name ?? ""
                shapeType: modelData.shapeType ?? "Shape"
                hasTessellation: modelData.hasTessellation ?? false
                topology: modelData.topology ?? {}
                boundingBox: modelData.boundingBox ?? {}
                shapeColor: modelData.color ?? ""
                geoVisible: root.geoVisible(modelData.shapeId ?? 0)
                meshVisible: root.meshVisible(modelData.shapeId ?? 0)

                onToggleGeoVisibility: (sid) => root.toggleGeoVisibility(sid)
                onToggleMeshVisibility: (sid) => root.toggleMeshVisibility(sid)
            }
        }
    }
}