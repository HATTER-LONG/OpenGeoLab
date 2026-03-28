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
    property var visibilityState: ({})

    implicitWidth: 280

    function fetchShapeList() {
        RequestService.submitAsync(JSON.stringify({
            module: "geometry",
            action: "list_shapes",
            param: {},
            mute: true
        }))
    }

    function toggleGeoVisibility(shapeId) {
        let state = root.visibilityState
        if (!(shapeId in state)) {
            state[shapeId] = { geo: true, mesh: false }
        }
        state[shapeId].geo = !state[shapeId].geo
        root.visibilityState = state
    }

    function toggleMeshVisibility(shapeId) {
        let state = root.visibilityState
        if (!(shapeId in state)) {
            state[shapeId] = { geo: true, mesh: false }
        }
        state[shapeId].mesh = !state[shapeId].mesh
        root.visibilityState = state
    }

    function geoVisible(shapeId) {
        return !(shapeId in root.visibilityState)
               || root.visibilityState[shapeId].geo
    }

    function meshVisible(shapeId) {
        return (shapeId in root.visibilityState)
               && root.visibilityState[shapeId].mesh
    }

    Timer {
        id: refreshTimer
        interval: 100
        repeat: false
        onTriggered: root.fetchShapeList()
    }

    Connections {
        target: ModuleDataNotifier
        function onGeometryDataChanged() {
            refreshTimer.restart()
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
            } catch (e) {
                // Ignore non-JSON or unrelated responses
            }
        }
    }

    Component.onCompleted: fetchShapeList()

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
                geoVisible: root.geoVisible(modelData.shapeId ?? 0)
                meshVisible: root.meshVisible(modelData.shapeId ?? 0)

                onToggleGeoVisibility: (sid) => root.toggleGeoVisibility(sid)
                onToggleMeshVisibility: (sid) => root.toggleMeshVisibility(sid)
            }
        }
    }
}