import QtQuick
import QtQuick.Layouts
import "../.."

/**
 * ShapeSelector — dropdown that fetches shapes from GeometryModule and
 * lets the user pick one. Emits shapeSelected(shapeId) on change.
 *
 * NOTE: RequestService and ModuleDataNotifier are app-level singletons.
 * When they are unavailable (e.g. viewport not yet wired), the selector
 * degrades to a static "No shapes available" state.
 */
Item {
    id: root

    required property var theme
    property int selectedShapeId: -1
    property string label: qsTr("Target Shape")

    signal shapeSelected(int shapeId)

    property var shapeList: []
    property int _comboIndex: -1

    implicitHeight: col.implicitHeight

    function refresh() {
        if (typeof RequestService === "undefined") return
        RequestService.submitAsync(JSON.stringify({
            module: "geometry",
            action: "list_shapes",
            param: {},
            mute: true
        }))
    }

    Connections {
        target: typeof RequestService !== "undefined" ? RequestService : null
        function onResponseReady(responseJson, muted) {
            if (!muted) return
            try {
                const resp = JSON.parse(responseJson)
                if (resp.action === "list_shapes" && resp.ok) {
                    root.shapeList = resp.shapes || []
                    if (root.selectedShapeId > 0) {
                        for (let i = 0; i < root.shapeList.length; ++i) {
                            if (root.shapeList[i].shapeId === root.selectedShapeId) {
                                root._comboIndex = i
                                return
                            }
                        }
                    }
                    if (root.shapeList.length > 0 && root.selectedShapeId <= 0) {
                        root._comboIndex = 0
                        root.selectedShapeId = root.shapeList[0].shapeId
                        root.shapeSelected(root.selectedShapeId)
                    }
                }
            } catch (e) { /* ignore */ }
        }
    }

    Connections {
        target: typeof ModuleDataNotifier !== "undefined" ? ModuleDataNotifier : null
        function onGeometryDataChanged() {
            root.refresh()
        }
    }

    Component.onCompleted: root.refresh()

    ColumnLayout {
        id: col
        width: parent.width
        spacing: 4

        Text {
            text: root.label
            color: root.theme.textSecondary
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            height: 28
            radius: root.theme.radiusSmall
            color: root.theme.surface
            border.color: selectorArea.containsMouse ? root.theme.accentB : root.theme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: {
                        if (root.shapeList.length === 0)
                            return qsTr("No shapes available")
                        if (root._comboIndex >= 0 && root._comboIndex < root.shapeList.length) {
                            const s = root.shapeList[root._comboIndex]
                            return s.name + " (#" + s.shapeId + ")"
                        }
                        return qsTr("Select a shape...")
                    }
                    color: root.shapeList.length === 0
                           ? root.theme.textTertiary
                           : root.theme.textPrimary
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    text: "\u25BE"
                    color: root.theme.textSecondary
                    font.pixelSize: 10
                }
            }

            MouseArea {
                id: selectorArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (root.shapeList.length > 0) {
                        dropDown.visible = !dropDown.visible
                    }
                }
            }
        }

        // Dropdown list
        Rectangle {
            id: dropDown
            Layout.fillWidth: true
            visible: false
            height: Math.min(listView.contentHeight + 4, 160)
            radius: root.theme.radiusSmall
            color: root.theme.surface
            border.color: root.theme.borderSubtle
            border.width: 1
            z: 100
            clip: true

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 2
                model: root.shapeList
                delegate: Rectangle {
                    width: listView.width
                    height: 26
                    radius: 2
                    color: delegateArea.containsMouse
                           ? root.theme.surfaceMuted
                           : (index === root._comboIndex ? root.theme.surfaceMuted : "transparent")

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        text: modelData.name + " (#" + modelData.shapeId + ")"
                        color: root.theme.textPrimary
                        font.pixelSize: 12
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    MouseArea {
                        id: delegateArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root._comboIndex = index
                            root.selectedShapeId = modelData.shapeId
                            root.shapeSelected(modelData.shapeId)
                            dropDown.visible = false
                        }
                    }
                }
            }
        }
    }
}
