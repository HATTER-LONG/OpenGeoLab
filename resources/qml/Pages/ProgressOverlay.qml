/**
 * @file ProgressOverlay.qml
 * @brief Progress overlay component for backend operations
 */

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL

/**
 * @brief Progress overlay component for displaying backend operation status
 *
 * Shows progress bar, message, and cancel button. Supports fade-in/out animations,
 * error state (red, 10s timeout) and success state (green, normal timeout).
 */
Item {
    id: root

    /// Tracks BackendService.busy state
    property bool busy: OGL.BackendService.busy
    /// Controls overlay visibility (separate from busy for delayed hide)
    property bool shown: false
    /// Current message text with fallback
    readonly property string messageText: OGL.BackendService.message.length > 0 ? OGL.BackendService.message : qsTr("Working...")
    /// True when in delayed-close phase (waiting to fade out)
    readonly property bool isInDelayedClose: hideSequence.running && !root.busy

    /// Operation result state: "normal", "success", "error"
    property string resultState: "normal"
    /// Hide delay in ms: 10000 for error, 4000 for success/normal
    readonly property int hideDelay: resultState === "error" ? 10000 : 4000

    /// Message color based on result state
    readonly property color messageColor: {
        if (resultState === "error")
            return Theme.danger;
        if (resultState === "success")
            return Theme.success;
        return Theme.palette.text;
    }

    width: 320
    height: visible ? content.implicitHeight + 20 : 0

    visible: shown || hideSequence.running || showSequence.running
    opacity: 0.0

    /// Fade-in animation when showing
    NumberAnimation {
        id: showSequence
        target: root
        property: "opacity"
        from: 0.0
        to: 1.0
        duration: 250
        easing.type: Easing.OutQuad
    }

    /// Delayed fade-out animation sequence
    SequentialAnimation {
        id: hideSequence
        running: false

        PauseAnimation {
            id: hideDelayPause
            duration: root.hideDelay
        }
        NumberAnimation {
            target: root
            property: "opacity"
            to: 0.0
            duration: 250
            easing.type: Easing.InOutQuad
        }
        ScriptAction {
            script: {
                if (!root.busy) {
                    root.shown = false;
                    root.resultState = "normal";
                }
            }
        }
    }

    /**
     * @brief Immediately closes the overlay, skipping any delay
     */
    function closeImmediately() {
        hideSequence.stop();
        showSequence.stop();
        root.opacity = 0.0;
        root.shown = false;
        root.resultState = "normal";
    }

    Connections {
        target: root
        function onBusyChanged() {
            if (root.busy) {
                hideSequence.stop();
                root.shown = true;
                root.resultState = "normal";
                showSequence.restart();
            } else {
                if (root.shown) {
                    hideSequence.restart();
                }
            }
        }
    }

    /// Listen to BackendService operation events for success/error states
    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName, result) {
            root.resultState = "success";
        }

        function onOperationFailed(moduleName, error) {
            root.resultState = "error";
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 8
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: messageLabel
                Layout.fillWidth: true
                text: root.messageText
                color: root.messageColor
                elide: Text.ElideRight
                font.weight: Font.Medium
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                maximumLineCount: 2
            }

            // Cancel / Close button
            Button {
                id: cancelButton
                implicitWidth: 24
                implicitHeight: 24
                flat: true
                hoverEnabled: true
                text: "✕"
                onClicked: {
                    if (root.isInDelayedClose) {
                        root.closeImmediately();
                    } else {
                        OGL.BackendService.cancel();
                    }
                }
                Layout.alignment: Qt.AlignTop

                contentItem: Text {
                    text: cancelButton.text
                    color: Theme.palette.buttonText
                    font.pixelSize: 12
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    radius: 6
                    color: cancelButton.down ? Theme.clicked : cancelButton.hovered ? Theme.hovered : "transparent"
                    border.width: (cancelButton.hovered || cancelButton.down) ? 1 : 0
                    border.color: Theme.border
                }

                ToolTip.visible: hovered
                ToolTip.text: root.isInDelayedClose ? qsTr("Close notification") : qsTr("Cancel operation")
            }
        }
        RowLayout {

            Layout.fillWidth: true
            spacing: 8
            ProgressBar {
                id: bar
                Layout.fillWidth: true
                from: 0
                to: 1
                value: OGL.BackendService.progress >= 0 ? OGL.BackendService.progress : 0
                indeterminate: OGL.BackendService.progress < 0
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.progress >= 0 ? qsTr("%1%").arg(Math.round(OGL.BackendService.progress * 100)) : qsTr("Processing...")
                color: Theme.palette.placeholderText
                horizontalAlignment: Text.AlignRight
                font.pixelSize: 11
            }
        }
    }
}
