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
        SelectionService.setLabelsVisible(false);
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
        mask: 11

        onMaskChanged: {
            SelectionService.pickMask = typeSelector.mask;
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
            onCheckedChanged: SelectionService.autoLabel = checked
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
                text: qsTr("Click to select · Right-click to deselect")
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
                        SelectionService.removeSelection(sid, etype, lid);
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
                ? qsTr("Click entities in the viewport to select them.\nRight-click to remove from selection.")
                : qsTr("No entities selected.\nActivate pick mode to begin.")
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
            onClicked: SelectionService.clearSelection()
        }
    }
}
