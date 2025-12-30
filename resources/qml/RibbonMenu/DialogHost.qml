pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

/**
 * @brief Simplified dialog host for displaying modal popups.
 *
 * Provides a centered popup container with:
 * - Modal overlay with dim background
 * - Auto-sizing based on content
 * - Escape key to close
 * - Animation effects
 */
Item {
    id: host
    anchors.fill: parent

    // Active dialog instance.
    property var _activeDialog: null

    /**
     * @brief Opens a dialog from a Component.
     * @param component QML Component to instantiate.
     * @param properties Initial property values for the dialog.
     */
    function openDialog(component, properties): void {
        if (!component || !component.createObject) {
            console.error("[DialogHost] Invalid component");
            return;
        }

        close();

        _activeDialog = component.createObject(dialogContainer, properties || ({}));
        if (!_activeDialog) {
            console.error("[DialogHost] Failed to create dialog");
            return;
        }

        popup.open();
    }

    /**
     * @brief Closes the active dialog.
     */
    function close(): void {
        popup.close();
    }

    function _cleanup(): void {
        if (_activeDialog) {
            if (_activeDialog.destroy)
                _activeDialog.destroy();
            _activeDialog = null;
        }
    }

    // Modal popup with dim overlay.
    Popup {
        id: popup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape

        anchors.centerIn: parent
        padding: 0

        // Auto-size based on content.
        width: host._activeDialog ? host._activeDialog.implicitWidth : 480
        height: host._activeDialog ? host._activeDialog.implicitHeight : 300

        background: Item {}

        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 150
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "scale"
                    from: 0.9
                    to: 1
                    duration: 150
                    easing.type: Easing.OutCubic
                }
            }
        }

        exit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 100
                    easing.type: Easing.InCubic
                }
            }
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(0, 0, 0, 0.5)

            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }
        }

        onClosed: host._cleanup()

        contentItem: Item {
            id: dialogContainer
            anchors.fill: parent

            Connections {
                target: host._activeDialog
                ignoreUnknownSignals: true

                function onCloseRequested() {
                    popup.close();
                }
            }
        }
    }
}
