import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../.."
import ".."

/**
 * Mesh Query page — activates pick mode and displays selected mesh entities.
 * On open: activates pick mode with the chosen mask.
 * On close: deactivates pick mode and clears selection.
 */
FunctionPageBase {
    id: root

    pageTitle: qsTr("Mesh Query")
    pageIcon: "query"
    actionId: "queryMesh"

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        sceneCommand("set_pick_mode", {
            pickMask: typeSelector.mask,
            enabled: true
        });
        sceneCommand("set_labels_visible", { visible: true });
        sceneCommand("set_auto_label", { enabled: true });
    }

    function close() {
        sceneCommand("set_pick_mode", { enabled: false });
        sceneCommand("clear_selection", {});
        sceneCommand("set_labels_visible", { visible: false });
        sceneCommand("set_auto_label", { enabled: false });
        sceneCommand("clear_labels", {});
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function execute() {
        // No-op: this page doesn't submit a request.
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

    /** @brief Map EntityType integer (C++ enum) to command protocol string. */
    function entityTypeTag(typeInt) {
        var map = {
            10: "MeshNode",
            11: "MeshEdge",
            12: "MeshElement"
        };
        return map[typeInt] ?? "MeshElement";
    }

    // ── Entity type selector ───────────────────────────────────────────
    Text {
        text: qsTr("Entity Filter")
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    EntityTypeSelector {
        id: typeSelector

        width: parent.width
        theme: root.theme
        mask: 448
        exclusiveMasks: []
        typeModel: [
            { label: qsTr("Node"), icon: "entityVertex", mask: typeSelector.maskMeshNode },
            { label: qsTr("Edge"), icon: "entityEdge", mask: typeSelector.maskMeshEdge },
            { label: qsTr("Element"), icon: "entityFace", mask: typeSelector.maskMeshElement }
        ]

        onMaskChanged: {
            sceneCommand("set_pick_mode", { pickMask: typeSelector.mask });
        }
    }

    // ── Auto-label toggle ──────────────────────────────────────────────
    RowLayout {
        width: parent.width
        spacing: 8

        Text {
            text: qsTr("Show Labels")
            color: root.theme.textSecondary
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        Switch {
            id: labelToggle

            checked: SelectionService.autoLabel
            onToggled: {
                sceneCommand("set_labels_visible", { visible: checked });
                sceneCommand("set_auto_label", { enabled: checked });
            }
        }
    }

    // ── Pick mode indicator with pulsing dot ───────────────────────────
    Rectangle {
        width: parent.width
        height: 24
        radius: root.theme.radiusSmall
        color: SelectionService.pickEnabled
            ? root.theme.tint(root.theme.accentA, root.theme.darkMode ? 0.14 : 0.08)
            : "transparent"
        visible: SelectionService.pickEnabled

        Behavior on color {
            ColorAnimation { duration: root.theme.animNormal }
        }

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
                text: qsTr("Click to select mesh entities · Right-click to deselect")
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

    // ── Selected entity chips (scrollable) ─────────────────────────────
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
                        sceneCommand("deselect", {
                            entities: [{ shapeId: sid, type: root.entityTypeTag(etype), localId: lid }]
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
            text: SelectionService.pickEnabled
                ? qsTr("Click mesh entities in the viewport to select them.\nRight-click to remove from selection.")
                : qsTr("No mesh entities selected.\nActivate pick mode to begin.")
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
            onClicked: sceneCommand("clear_selection", {})
        }
    }
}
