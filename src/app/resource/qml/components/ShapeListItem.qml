pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import "../theme"

/// @brief Single shape entry — collapsed header with expandable details.
Item {
    id: root

    required property AppTheme theme
    required property int shapeId
    required property string name
    required property string shapeType
    required property bool hasTessellation
    required property var topology
    required property var boundingBox
    required property bool geoVisible
    required property bool meshVisible
    property string shapeColor: ""

    signal toggleGeoVisibility(int shapeId)
    signal toggleMeshVisibility(int shapeId)

    property bool expanded: false

    implicitWidth: parent ? parent.width : 260
    implicitHeight: col.implicitHeight

    HoverHandler { id: rowHover }

    Rectangle {
        anchors.fill: parent
        radius: root.theme.radiusSmall
        color: root.expanded
               ? root.theme.tint(root.theme.accentPrimary, 0.08)
               : rowHover.hovered
                 ? root.theme.tint(root.theme.textPrimary, 0.06)
                 : "transparent"
        border.color: root.expanded ? root.theme.accentPrimary : "transparent"
        border.width: 1

        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Column {
        id: col
        width: parent.width
        padding: 6

        // --- Clickable header row ---
        Item {
            width: parent.width - 12
            height: headerRow.implicitHeight

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.expanded = !root.expanded
            }

            RowLayout {
                id: headerRow
                anchors.fill: parent
                spacing: 6

                // Color block
                Rectangle {
                    width: 4
                    height: 16
                    radius: 2
                    color: root.shapeColor.length > 0 ? root.shapeColor : "#808080"
                }

                // ID
                Text {
                    text: "#" + root.shapeId
                    font.pixelSize: 11
                    color: root.theme.textTertiary
                }

                // Name
                Text {
                    Layout.fillWidth: true
                    text: root.name
                    font.pixelSize: 13
                    color: root.theme.textPrimary
                    elide: Text.ElideRight
                }

                // Geo visibility toggle
                Item {
                    id: geoBtn
                    width: 22; height: 22

                    HoverHandler { id: geoBtnHover }

                    Rectangle {
                        anchors.fill: parent
                        radius: root.theme.radiusSmall
                        color: geoBtnHover.hovered
                               ? root.theme.tint(root.theme.textPrimary, 0.1)
                               : "transparent"
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }

                    AppIcon {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        theme: root.theme
                        iconKind: "cubeOutline"
                        primaryColor: root.geoVisible ? root.theme.accentPrimary
                                                      : root.theme.textTertiary
                        opacity: root.geoVisible ? 1.0 : 0.4
                        scale: geoBtnHover.hovered ? 1.1 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100 } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.toggleGeoVisibility(root.shapeId)
                    }
                }

                // Mesh visibility toggle
                Item {
                    id: meshBtn
                    width: 22; height: 22

                    HoverHandler { id: meshBtnHover }

                    Rectangle {
                        anchors.fill: parent
                        radius: root.theme.radiusSmall
                        color: meshBtnHover.hovered
                               ? root.theme.tint(root.theme.textPrimary, 0.1)
                               : "transparent"
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }

                    AppIcon {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        theme: root.theme
                        iconKind: "smoothMesh"
                        primaryColor: root.meshVisible ? root.theme.accentPrimary
                                                       : root.theme.textTertiary
                        opacity: root.meshVisible ? 1.0 : 0.4
                        scale: meshBtnHover.hovered ? 1.1 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100 } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.toggleMeshVisibility(root.shapeId)
                    }
                }

                // Type label
                Text {
                    text: root.shapeType
                    font.pixelSize: 11
                    color: root.theme.textTertiary
                }
            }
        }

        // --- Expanded detail rows ---
        Column {
            id: detailCol
            width: parent.width - 12
            visible: root.expanded
            spacing: 4
            topPadding: 4

            Rectangle {
                width: parent.width
                height: 1
                color: root.theme.borderSubtle
            }

            // Topology row
            Text {
                width: parent.width
                text: {
                    let t = root.topology || {}
                    let parts = []
                    if (t.faces !== undefined)    parts.push(qsTr("%1 Faces").arg(t.faces))
                    if (t.edges !== undefined)    parts.push(qsTr("%1 Edges").arg(t.edges))
                    if (t.vertices !== undefined) parts.push(qsTr("%1 Vertices").arg(t.vertices))
                    return qsTr("Topology") + "    " + parts.join(" \u00B7 ")
                }
                font.pixelSize: 12
                color: root.theme.textSecondary
                wrapMode: Text.WordWrap
            }

            // Bounds row
            Text {
                width: parent.width
                text: {
                    let bb = root.boundingBox || {}
                    let mn = bb.min || [0,0,0]
                    let mx = bb.max || [0,0,0]
                    let dx = (mx[0] - mn[0]).toFixed(1)
                    let dy = (mx[1] - mn[1]).toFixed(1)
                    let dz = (mx[2] - mn[2]).toFixed(1)
                    return qsTr("Bounds") + "    " + dx + " \u00D7 " + dy + " \u00D7 " + dz
                }
                font.pixelSize: 12
                color: root.theme.textSecondary
            }

            // Tessellation status
            Text {
                width: parent.width
                text: root.hasTessellation ? qsTr("Tessellated") + " \u2705"
                                           : qsTr("Not tessellated") + " \u23F3"
                font.pixelSize: 12
                color: root.theme.textSecondary
            }
        }
    }
}
