import QtQuick
import QtQuick.Layouts
import OpenGeoLab.Services 1.0
import "../.."
import ".."

/**
 * Geometry Query page — activates pick mode and displays selected entities.
 * On open: activates pick mode with the chosen mask.
 * On close: deactivates pick mode and clears selection.
 */
FunctionPageBase {
    id: root

    pageTitle: qsTr("Geometry Query")
    pageIcon: "query"
    actionId: "queryGeometry"

    function open(payload) {
        root.x = 292;
        root.y = 0;
        root.pageVisible = true;
        root.forceActiveFocus();
        SelectionService.activatePickMode(typeSelector.mask);
    }

    function close() {
        SelectionService.deactivatePickMode();
        SelectionService.clearSelection();
        root.pageVisible = false;
        if (MainPages.currentOpenPage === root.actionId) {
            MainPages.currentOpenPage = "";
        }
    }

    function execute() {
        // No-op: this page doesn't submit a request.
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
        mask: 7

        onMaskChanged: {
            SelectionService.setPickMask(typeSelector.mask);
        }
    }

    // ── Pick status indicator ──────────────────────────────────────────
    Rectangle {
        width: parent.width
        height: 28
        radius: root.theme.radiusSmall
        color: SelectionService.pickEnabled
            ? root.theme.tint(root.theme.success, root.theme.darkMode ? 0.18 : 0.10)
            : root.theme.surfaceMuted

        Text {
            anchors.centerIn: parent
            text: SelectionService.pickEnabled
                ? qsTr("Pick mode active — click to select")
                : qsTr("Pick mode inactive")
            color: SelectionService.pickEnabled
                ? root.theme.success
                : root.theme.textSecondary
            font.pixelSize: 11
        }
    }

    // ── Selection count ────────────────────────────────────────────────
    Text {
        text: qsTr("Selected: %1").arg(SelectionService.selections.length)
        color: root.theme.textSecondary
        font.pixelSize: 12
    }

    // ── Selected entity chips ──────────────────────────────────────────
    Flow {
        id: chipFlow

        width: parent.width
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
                    SelectionService.removeSelection(sid, etype, lid);
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
            text: qsTr("No entities selected.\nLeft-click to add, right-click to remove.")
            color: root.theme.textSecondary
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
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
            onClicked: SelectionService.clearSelection()
        }
    }
}
