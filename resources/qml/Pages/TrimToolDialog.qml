pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenGeoLab 1.0 as OGL
import "." as Pages

/**
 * @file TrimToolDialog.qml
 * @brief Non-modal tool dialog for trimming geometry
 *
 * Allows users to trim geometry by selecting a target and a trim tool.
 * Integrates with the viewport for geometry selection.
 */
Pages.ToolDialog {
    id: root

    title: qsTr("Trim Geometry")
    okButtonText: qsTr("Apply")
    preferredContentHeight: 380

    okEnabled: !OGL.BackendService.busy && trimTargetSelector.selectedId > 0

    onAccepted: {
        const params = {
            targetId: trimTargetSelector.selectedId,
            targetType: trimTargetSelector.selectionType,
            toolId: trimToolSelector.selectedId,
            toolType: trimToolSelector.selectionType,
            mode: modeCombo.currentText,
            keepOriginal: keepOriginalCheck.checked
        };
        OGL.BackendService.request("Trim", params);
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("Select geometry to trim and configure options:")
            color: Theme.textSecondaryColor
            wrapMode: Text.Wrap
            font.pixelSize: 12
        }

        // Geometry selector for the element to trim
        Pages.GeometrySelector {
            id: trimTargetSelector
            Layout.fillWidth: true
            label: qsTr("Geometry to Trim:")
            selectionType: 3 // Face by default
            placeholder: qsTr("Click to select target...")

            onSelectionChanged: (id, type) => {
                if (root.viewport) {
                    root.viewport.selectionMode = isSelecting ? type : 0;
                }
            }

            onIsSelectingChanged: {
                if (root.viewport) {
                    root.viewport.selectionMode = isSelecting ? selectionType : 0;
                }
            }
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderColor
        }

        // Geometry selector for the trim tool (plane/surface)
        Pages.GeometrySelector {
            id: trimToolSelector
            Layout.fillWidth: true
            label: qsTr("Trim Tool (Plane/Surface):")
            selectionType: 3 // Face by default
            placeholder: qsTr("Click to select tool...")
            visible: modeCombo.currentIndex !== 0

            onSelectionChanged: (id, type) => {
                if (root.viewport && isSelecting) {
                    root.viewport.selectionMode = type;
                }
            }

            onIsSelectingChanged: {
                if (root.viewport) {
                    root.viewport.selectionMode = isSelecting ? selectionType : 0;
                }
            }
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderColor
            visible: modeCombo.currentIndex !== 0
        }

        // Trim options
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 8

            Label {
                text: qsTr("Trim Mode:")
                color: Theme.textPrimaryColor
                font.pixelSize: 13
            }

            ComboBox {
                id: modeCombo
                Layout.fillWidth: true
                model: [qsTr("Auto"), qsTr("By Plane"), qsTr("By Surface")]
                enabled: !OGL.BackendService.busy

                contentItem: Text {
                    text: modeCombo.displayText
                    color: Theme.textPrimaryColor
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    leftPadding: 10
                }

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 32
                    color: modeCombo.enabled ? Theme.surfaceColor : Theme.surfaceAltColor
                    border.width: 1
                    border.color: modeCombo.pressed ? Theme.accentColor : Theme.borderColor
                    radius: 4
                }
            }
        }

        CheckBox {
            id: keepOriginalCheck
            text: qsTr("Keep original geometry")
            checked: false
            enabled: !OGL.BackendService.busy
        }

        // Status area
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: OGL.BackendService.busy

            BusyIndicator {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                running: true
            }

            Label {
                Layout.fillWidth: true
                text: OGL.BackendService.message
                color: Theme.textSecondaryColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    // Handle viewport selection
    Connections {
        target: root.viewport

        function onSelectionChanged(id: int, type: int): void {
            if (trimTargetSelector.isSelecting) {
                trimTargetSelector.setSelection(id, root.getSelectionInfo(id, type));
            } else if (trimToolSelector.isSelecting) {
                trimToolSelector.setSelection(id, root.getSelectionInfo(id, type));
            }
        }
    }

    // Handle operation completion
    Connections {
        target: OGL.BackendService

        function onOperationFinished(moduleName: string, _result: var): void {
            if (moduleName === "Trim") {
                trimTargetSelector.clearSelection();
                trimToolSelector.clearSelection();
            }
        }
    }

    function getSelectionInfo(id: int, type: int): string {
        switch (type) {
        case 1:
            return qsTr("Vertex %1").arg(id);
        case 2:
            return qsTr("Edge %1").arg(id);
        case 3:
            return qsTr("Face %1").arg(id);
        case 4:
            return qsTr("Part %1").arg(id);
        default:
            return qsTr("Element %1").arg(id);
        }
    }

    onIsVisibleChanged: {
        if (!isVisible) {
            trimTargetSelector.stopSelection();
            trimToolSelector.stopSelection();
            if (root.viewport) {
                root.viewport.selectionMode = 0;
            }
        }
    }
}
