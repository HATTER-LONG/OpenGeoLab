/**
 * @file EntitySelector.qml
 * @brief Reusable entity selection component for geometry picking
 *
 * This component does NOT monitor mouse movement in QML.
 * When active, it enables GLViewport picking mode. The viewport performs
 * hover picking and selection internally, updates highlight automatically,
 * and pushes results back via Qt signals.
 */
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0
import ".."

Item {
    id: root

    anchors.fill: viewport ? viewport : undefined

    // =========================================================
    // Public API
    // =========================================================

    /// Current selection mode (0=None, 1=Vertex, 2=Edge, 3=Face, 4=Solid, 7=Part, 8=Multi)
    property int selectionMode: 3

    /// List of currently selected entity IDs (driven by GLViewport)
    property var selectedEntities: []

    /// Whether to enable preview highlighting on hover (handled by GLViewport)
    property bool enablePreview: true

    /// Whether multi-selection is enabled (handled by GLViewport via Ctrl)
    property bool enableMultiSelect: true

    /// Maximum number of selections (0 = unlimited). Currently enforced by GLViewport.
    property int maxSelections: 0

    /// Reference to the GLViewport for picking
    property var viewport: null

    /// Whether picking interaction is active
    property bool active: true

    /// Signal emitted when selection changes
    signal selectionChanged(var entityIds)

    /// Signal emitted when an entity is hovered
    signal entityHovered(int entityId)

    /// Signal emitted when hover ends
    signal hoverEnded

    // =========================================================
    // Internal State
    // =========================================================

    /// Currently previewed entity ID (driven by GLViewport)
    property int previewEntityId: 0

    // =========================================================
    // Methods
    // =========================================================

    /**
     * @brief Clear all selections
     */
    function clearSelection() {
        if (viewport) {
            viewport.clearPickedSelection();
        }
    }

    /**
     * @brief Remove an entity from selection
     * @param entityId Entity ID to remove
     */
    function removeFromSelection(entityId) {
        if (viewport) {
            viewport.deselectEntity(entityId);
        }
    }

    // =========================================================
    // Viewport wiring
    // =========================================================

    function applyViewportState() {
        if (!viewport)
            return;
        viewport.pickingEnabled = active;
        viewport.selectionMode = active ? selectionMode : 0;
    }

    onActiveChanged: applyViewportState()
    onViewportChanged: applyViewportState()

    onSelectionModeChanged: {
        if (viewport && active) {
            viewport.selectionMode = selectionMode;
        }
    }

    Connections {
        target: root.viewport

        function onHoveredEntityChanged(entity_id) {
            root.previewEntityId = entity_id;
            if (entity_id && entity_id > 0) {
                root.entityHovered(entity_id);
            } else {
                root.hoverEnded();
            }
        }

        function onSelectionChanged(entity_ids) {
            root.selectedEntities = entity_ids;
            root.selectionChanged(entity_ids);
        }
    }

    // Box selection visualization (driven by GLViewport properties)
    Rectangle {
        visible: viewport && viewport.boxSelecting
        x: viewport ? viewport.boxSelectionRect.x : 0
        y: viewport ? viewport.boxSelectionRect.y : 0
        width: viewport ? viewport.boxSelectionRect.width : 0
        height: viewport ? viewport.boxSelectionRect.height : 0
        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.1)
        border.color: Theme.accent
        border.width: 1
        radius: 2
    }

}

