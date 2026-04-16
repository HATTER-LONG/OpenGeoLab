pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../.."
import ".."

/**
 * Mesh Split page — select mesh edges or nodes, choose a split mode,
 * and execute the split_mesh action to subdivide mesh elements.
 */
FunctionPageBase {
    id: root

    pageTitle: qsTr("Split Mesh")
    pageIcon: "mesh"
    actionId: "splitMesh"

    /** @brief Current selection type: "edge" or "node" */
    property string selectionType: "edge"

    /** @brief Current split mode string for the action request */
    property string splitMode: "auto"

    function defaultSplitMode(type) {
        return type === "node" ? "tria_three" : "auto";
    }

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        selectionType = "edge";
        splitMode = defaultSplitMode(selectionType);
        sceneCommand("set_pick_mode", {
            pickMask: typeSelector.maskMeshEdge,
            enabled: true
        });
    }

    function close() {
        sceneCommand("set_pick_mode", { enabled: false });
        sceneCommand("clear_selection", {});
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function execute() {
        if (RequestService.busy) return;
        if (SelectionService.selections.length === 0) return;

        var selections = [];
        for (var i = 0; i < SelectionService.selections.length; ++i) {
            var sel = SelectionService.selections[i];
            selections.push({
                "type": root.selectionType,
                "localId": sel.localId
            });
        }

        var request = {
            "module": "mesh",
            "action": "split_mesh",
            "param": {
                "shapeId": SelectionService.selections[0].shapeId,
                "selections": selections,
                "mode": root.splitMode
            }
        };

        RequestService.submitAsync(JSON.stringify(request));
        sceneCommand("clear_selection", {});
    }

    /** @brief Send a scene command via the command protocol. */
    function sceneCommand(action, param) {
        RequestService.submitAsync(JSON.stringify({
            module: "scene",
            action: action,
            param: param ?? {},
            mute: true
        }));
    }

    // ── Selection Type Toggle ──────────────────────────────────────────
    Text {
        text: qsTr("Selection Type")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    RowLayout {
        width: parent.width
        spacing: 4

        Repeater {
            model: [
                { label: qsTr("Edge"), value: "edge", pickMask: typeSelector.maskMeshEdge },
                { label: qsTr("Node"), value: "node", pickMask: typeSelector.maskMeshNode }
            ]

            delegate: AbstractButton {
                id: selTypeBtn

                required property var modelData
                required property int index

                readonly property bool selected: root.selectionType === modelData.value

                Layout.fillWidth: true
                Layout.preferredHeight: 36
                hoverEnabled: true

                background: Rectangle {
                    radius: root.theme.radiusSmall
                    color: selTypeBtn.selected
                        ? root.theme.tint(root.theme.accentA,
                            root.theme.darkMode ? 0.24 : 0.14)
                        : selTypeBtn.pressed
                            ? root.theme.surfaceStrong
                            : selTypeBtn.hovered
                                ? root.theme.surfaceMuted
                                : root.theme.surface
                    border.width: selTypeBtn.selected ? 1.5 : 1
                    border.color: selTypeBtn.selected
                        ? root.theme.accentA
                        : selTypeBtn.hovered
                            ? root.theme.tint(root.theme.accentA,
                                root.theme.darkMode ? 0.4 : 0.3)
                            : root.theme.borderSubtle

                    Behavior on color {
                        ColorAnimation { duration: root.theme.animFast }
                    }
                }

                contentItem: Text {
                    text: selTypeBtn.modelData.label
                    font.pixelSize: 12
                    font.bold: selTypeBtn.selected
                    color: selTypeBtn.selected
                        ? root.theme.accentA
                        : root.theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.selectionType = modelData.value;
                    root.sceneCommand("clear_selection", {});
                    root.sceneCommand("set_pick_mode", {
                        pickMask: modelData.pickMask,
                        enabled: true
                    });
                    root.splitMode = root.defaultSplitMode(modelData.value);
                }
            }
        }
    }

    // ── Split Mode Selector ────────────────────────────────────────────
    Text {
        text: qsTr("Split Mode")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    Flow {
        width: parent.width
        spacing: 4
        visible: root.selectionType === "edge"

        Repeater {
            model: [
                { label: qsTr("Auto"), value: "auto" },
                { label: qsTr("4△"), value: "tria_four" },
                { label: qsTr("3□"), value: "quad_three" },
                { label: qsTr("3□+1△"), value: "tria_one_quad_three" },
                { label: qsTr("2□+1△"), value: "tria_one_quad_two" },
                { label: qsTr("2□+3△"), value: "tria_three_quad_two" }
            ]

            delegate: AbstractButton {
                id: edgeModeBtn

                required property var modelData
                required property int index

                readonly property bool selected: root.splitMode === modelData.value

                width: 64
                height: 32
                hoverEnabled: true

                ToolTip.visible: hovered
                ToolTip.text: modelData.value
                ToolTip.delay: 500

                background: Rectangle {
                    radius: root.theme.radiusSmall
                    color: edgeModeBtn.selected
                        ? root.theme.tint(root.theme.accentB,
                            root.theme.darkMode ? 0.24 : 0.14)
                        : edgeModeBtn.pressed
                            ? root.theme.surfaceStrong
                            : edgeModeBtn.hovered
                                ? root.theme.surfaceMuted
                                : root.theme.surface
                    border.width: edgeModeBtn.selected ? 1.5 : 1
                    border.color: edgeModeBtn.selected
                        ? root.theme.accentB
                        : edgeModeBtn.hovered
                            ? root.theme.tint(root.theme.accentB,
                                root.theme.darkMode ? 0.4 : 0.3)
                            : root.theme.borderSubtle

                    Behavior on color {
                        ColorAnimation { duration: root.theme.animFast }
                    }
                }

                contentItem: Text {
                    text: edgeModeBtn.modelData.label
                    font.pixelSize: 11
                    font.bold: edgeModeBtn.selected
                    color: edgeModeBtn.selected
                        ? root.theme.accentB
                        : root.theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.splitMode = modelData.value;
                }
            }
        }
    }

    Flow {
        width: parent.width
        spacing: 4
        visible: root.selectionType === "node"

        Repeater {
            model: [
                { label: qsTr("3△"), value: "tria_three" }
            ]

            delegate: AbstractButton {
                id: nodeModeBtn

                required property var modelData
                required property int index

                readonly property bool selected: root.splitMode === modelData.value

                width: 64
                height: 32
                hoverEnabled: true

                background: Rectangle {
                    radius: root.theme.radiusSmall
                    color: nodeModeBtn.selected
                        ? root.theme.tint(root.theme.accentB,
                            root.theme.darkMode ? 0.24 : 0.14)
                        : nodeModeBtn.pressed
                            ? root.theme.surfaceStrong
                            : nodeModeBtn.hovered
                                ? root.theme.surfaceMuted
                                : root.theme.surface
                    border.width: nodeModeBtn.selected ? 1.5 : 1
                    border.color: nodeModeBtn.selected
                        ? root.theme.accentB
                        : nodeModeBtn.hovered
                            ? root.theme.tint(root.theme.accentB,
                                root.theme.darkMode ? 0.4 : 0.3)
                            : root.theme.borderSubtle

                    Behavior on color {
                        ColorAnimation { duration: root.theme.animFast }
                    }
                }

                contentItem: Text {
                    text: nodeModeBtn.modelData.label
                    font.pixelSize: 11
                    font.bold: nodeModeBtn.selected
                    color: nodeModeBtn.selected
                        ? root.theme.accentB
                        : root.theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    root.splitMode = modelData.value;
                }
            }
        }
    }

    // ── Hidden EntityTypeSelector for mask constants ────────────────────
    EntityTypeSelector {
        id: typeSelector
        visible: false
        theme: root.theme
        mask: 0
    }

    // ── Pick mode indicator ────────────────────────────────────────────
    Rectangle {
        width: parent.width
        height: 24
        radius: root.theme.radiusSmall
        color: SelectionService.pickEnabled
            ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.14 : 0.08)
            : "transparent"
        visible: SelectionService.pickEnabled

        Row {
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                id: pulsingDot

                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: root.theme.accentA

                SequentialAnimation on opacity {
                    running: SelectionService.pickEnabled
                    loops: Animation.Infinite

                    NumberAnimation {
                        from: 1.0
                        to: 0.3
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }

                    NumberAnimation {
                        from: 0.3
                        to: 1.0
                        duration: 800
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            Text {
                text: root.selectionType === "edge"
                    ? qsTr("Click mesh edges in the viewport · Right-click to deselect")
                    : qsTr("Click mesh nodes in the viewport · Right-click to deselect")
                color: root.theme.accentA
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // ── Selection count ────────────────────────────────────────────────
    Text {
        text: qsTr("Selected: %1").arg(SelectionService.selections.length)
        color: root.theme.textSecondary
        font.pixelSize: 12
        visible: SelectionService.selections.length > 0
    }

    // ── Selected entity chips ──────────────────────────────────────────
    Flickable {
        id: chipFlickable

        width: parent.width
        height: Math.min(chipFlow.implicitHeight, 100)
        contentHeight: chipFlow.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        visible: SelectionService.selections.length > 0

        Flow {
            id: chipFlow

            width: chipFlickable.width
            spacing: 6

            Repeater {
                model: SelectionService.selections

                EntityChip {
                    required property var modelData

                    theme: root.theme
                    shapeId: modelData.shapeId
                    entityType: modelData.entityType
                    localId: modelData.localId

                    onRemoveRequested: function(sid, etype, lid) {
                        var typeTag = root.selectionType === "edge" ? "MeshEdge" : "MeshNode";
                        root.sceneCommand("deselect", {
                            entities: [{ shapeId: sid, type: typeTag, localId: lid }]
                        });
                    }
                }
            }
        }
    }

    // ── Empty state ────────────────────────────────────────────────────
    Rectangle {
        width: parent.width
        height: 48
        radius: root.theme.radiusSmall
        color: root.theme.surfaceMuted
        visible: SelectionService.selections.length === 0

        Text {
            anchors.centerIn: parent
            text: root.selectionType === "edge"
                ? qsTr("Select mesh edges in the viewport to split.\nRight-click to remove from selection.")
                : qsTr("Select 3 mesh nodes of a triangle to split.\nRight-click to remove from selection.")
            color: root.theme.textTertiary
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.3
        }
    }

    // ── Clear All button ───────────────────────────────────────────────
    Rectangle {
        id: clearAllButton

        width: parent.width
        height: 32
        radius: root.theme.radiusSmall
        visible: SelectionService.selections.length > 0
        color: clearMouse.pressed
            ? root.theme.surfaceStrong
            : (clearMouse.containsMouse
                ? root.theme.tint(root.theme.surfaceMuted, root.theme.darkMode ? 0.96 : 0.9)
                : root.theme.surfaceMuted)
        border.width: 1
        border.color: clearMouse.containsMouse
            ? root.theme.tint(root.theme.danger, root.theme.darkMode ? 0.56 : 0.36)
            : root.theme.borderSubtle

        Behavior on color {
            ColorAnimation { duration: root.theme.animFast }
        }

        Text {
            anchors.centerIn: parent
            text: qsTr("Clear All")
            color: clearMouse.containsMouse ? root.theme.danger : root.theme.textPrimary
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            id: clearMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.sceneCommand("clear_selection", {})
        }
    }
}
